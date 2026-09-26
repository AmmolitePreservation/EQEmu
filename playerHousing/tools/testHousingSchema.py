"""Tests sql/housingSchema.sql against a local MariaDB: creation (twice), a realistic hierarchy, and the queries the housing
module depends on. The inventory table comes from the server's real schema.

Usage: python3 testHousingSchema.py pok-schema-all.json   (a local MariaDB with a `peq` database, root without a password)
"""
import json, os, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
schema = open(os.path.join(here, '..', 'sql', 'housingSchema.sql')).read()


def sql(text, expectError=False):
  r = subprocess.run(['mariadb', '-uroot', '-N', 'peq'], input=text, capture_output=True, text=True)
  if (r.returncode != 0) != expectError:
    raise RuntimeError(f"unexpected result ({r.returncode}): {r.stderr[:400]}")
  return r.stderr if expectError else r.stdout.strip()


real = json.load(open(sys.argv[1]))
def definition(c):
  d = f"`{c['Field']}` {c['Type']} {'NOT NULL' if c['Null'] == 'NO' else 'NULL'}"
  return d + (f" DEFAULT '{c['Default']}'" if c['Default'] is not None else '')
inventory = ("DROP TABLE IF EXISTS inventory; CREATE TABLE inventory (" + ", ".join(definition(c) for c in real['inventory'])
             + ", PRIMARY KEY (`character_id`, `slot_id`)) ENGINE=InnoDB;")
tables = [l.split('`')[1] for l in schema.splitlines() if l.startswith('CREATE TABLE')]
sql("".join(f"DROP TABLE IF EXISTS `{t}`;" for t in tables) + inventory)
sql(schema); sql(schema)
print(f'created {len(tables)} tables; re-running the schema is safe')

sql("""
INSERT INTO real_estate_regions (id, name, x, y, z, size_x, size_y, size_z, heading) VALUES (1, 'plot 1', 100, 200, 0, 60, 40, 30, 128);
INSERT INTO real_estate_definitions (id, name, type, instance_zone_id) VALUES (900, 'Sunrise Hills neighborhood', 5, 712);
INSERT INTO real_estate_definitions (id, name, type, region_id, purchase_cost_id, upkeep_cost_id, max_items, max_non_real_estate_items, max_pet_items)
  VALUES (901, 'player plot 1', 4, 1, 1, 2, 200, 150, 20), (902, 'guild plot 1', 8, 0, 1, 2, 200, 150, 20);
INSERT INTO real_estate_definitions (id, name, type, instance_zone_id, max_items) VALUES (5, 'House, Stucco interior', 3, 714, 100), (31, 'Crate of Eviction', 7, 0, 500);
INSERT INTO real_estate_definition_children VALUES (900, 901), (900, 902), (901, 5), (901, 31);
INSERT INTO real_estate_costs (id, name) VALUES (1, 'plot price'), (2, 'plot upkeep');
INSERT INTO real_estate_cost_options VALUES (1, 1, 0), (2, 1, 0), (2, 2, 1);
INSERT INTO real_estate_cost_items VALUES (1, 1, 0, 0, 5000000), (1, 1, 9, 1, 25), (2, 1, 0, 0, 1000), (2, 2, 0, 0, -200);
INSERT INTO real_estate_properties (id, parent_id, definition_id, host_zone_id, host_instance_id, name) VALUES (1, 0, 900, 712, 1001, 'Sunrise Hills 1');
INSERT INTO real_estate_properties (id, parent_id, definition_id, owner_character_id, host_zone_id, host_instance_id, next_upkeep_at, name)
  VALUES (2, 1, 901, 7, 712, 1001, NOW() - INTERVAL 1 HOUR, 'Plot 1'), (3, 1, 902, 0, 712, 1001, NULL, 'Guild plot 1');
INSERT INTO real_estate_properties (id, parent_id, definition_id, owner_character_id, host_zone_id, host_instance_id, interior_instance_id)
  VALUES (4, 2, 5, 7, 712, 1001, 2001);
INSERT INTO real_estate_properties (id, parent_id, definition_id, owner_character_id, contained_in_item_guid, archived) VALUES (5, 0, 31, 7, 55501, 1);
INSERT INTO real_estate_property_balances VALUES (2, 0, 123456), (2, 9, 3);
INSERT INTO real_estate_items (property_id, owner_character_id, placed, x, y, z, item_id) VALUES (2, 7, 1, 101, 201, 0, 61410), (4, 7, 0, 0, 0, 0, 61441);
INSERT INTO real_estate_access_groups VALUES (2, 0, 255), (2, 1, 3);
INSERT INTO real_estate_access_names VALUES (2, 'Friendly', 1);
INSERT INTO inventory (character_id, slot_id, item_id, guid) VALUES (7, 23, 52147, 55501);
""")

def check(label, query, expected):
  got = sql(query)
  assert got == expected, f'{label}: expected {expected!r}, got {got!r}'
  print(f'{label}: {got!r}')

check('zone-in: properties hosted in 712/1001', "SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE host_zone_id = 712 AND host_instance_id = 1001", '1,2,3,4')
check('inside a house: the house and its parents', """
  WITH RECURSIVE chain AS (SELECT id, parent_id FROM real_estate_properties WHERE interior_instance_id = 2001
    UNION ALL SELECT p.id, p.parent_id FROM real_estate_properties p JOIN chain c ON p.id = c.parent_id)
  SELECT GROUP_CONCAT(id ORDER BY id) FROM chain""", '1,2,4')
check('children, derived from parent_id', "SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE parent_id = 1", '2,3')
check('list A (owned)', "SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE owner_character_id = 7", '2,4,5')
check('list C (archived, a subset of owned)', "SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE owner_character_id = 7 AND archived = 1", '5')
check('list B (contained in items the character carries)', """
  SELECT GROUP_CONCAT(p.id ORDER BY p.id) FROM real_estate_properties p JOIN inventory i ON i.guid = p.contained_in_item_guid
  WHERE p.contained_in_item_guid <> 0 AND i.character_id = 7""", '5')
check('neighborhood row: player plots, used, guild plots, used', """
  SELECT CONCAT_WS(',', SUM(d.type = 4), SUM(d.type = 4 AND (p.owner_character_id <> 0 OR p.guild_id <> 0)),
                        SUM(d.type = 8), SUM(d.type = 8 AND (p.owner_character_id <> 0 OR p.guild_id <> 0)))
  FROM real_estate_properties p JOIN real_estate_definitions d ON d.id = p.definition_id WHERE p.parent_id = 1""", '1,1,1,0')
check('revision-guarded update, fresh', "UPDATE real_estate_properties SET upkeep_option_key = 2, revision = revision + 1 WHERE id = 2 AND revision = 0; SELECT ROW_COUNT()", '1')
check('revision-guarded update, stale', "UPDATE real_estate_properties SET upkeep_option_key = 1, revision = revision + 1 WHERE id = 2 AND revision = 0; SELECT ROW_COUNT()", '0')
check('upkeep tick: due properties', "SELECT GROUP_CONCAT(id) FROM real_estate_properties WHERE next_upkeep_at <= NOW()", '2')
plan = sql("EXPLAIN SELECT id FROM real_estate_properties FORCE INDEX (next_upkeep_at) WHERE next_upkeep_at <= NOW()")
assert 'next_upkeep_at' in plan and 'range' in plan, plan
print('upkeep tick: the due query is an index range scan')
check('price with two currencies', "SELECT GROUP_CONCAT(CONCAT(currency, ':', amount) ORDER BY currency) FROM real_estate_cost_items WHERE cost_id = 1 AND option_key = 1", '0:5000000,9:25')
check('a reduction stays negative', "SELECT amount FROM real_estate_cost_items WHERE cost_id = 2 AND option_key = 2", '-200')
for label, bad in [('definition type out of range', "INSERT INTO real_estate_definitions (id, type) VALUES (999, 9)"),
                   ('pet items above max items', "INSERT INTO real_estate_definitions (id, type, max_items, max_pet_items) VALUES (998, 4, 10, 11)"),
                   ('access group above 6', "INSERT INTO real_estate_access_groups VALUES (2, 7, 1)"),
                   ('personal trophy slot above 9', "INSERT INTO character_trophy_tribute VALUES (7, 10, 100, 0)"),
                   ('guild trophy slot above 3', "INSERT INTO guild_trophy_tribute VALUES (1, 4, 100, 0)"),
                   ('poll closing before it opens', "INSERT INTO real_estate_polls (kind, selection_type, opens_at, closes_at) VALUES (2, 2, NOW(), NOW() - INTERVAL 1 DAY)"),
                   ('region without volume', "INSERT INTO real_estate_regions (id, size_x, size_y, size_z) VALUES (2, 0, 1, 1)"),
                   ('second pending offer for one property', "INSERT INTO real_estate_transfer_offers (property_id, from_character_id, to_character_id, expires_at) VALUES (2, 7, 8, NOW()), (2, 7, 9, NOW())")]:
  sql(bad, expectError=True)
  print(f'rejected: {label}')
