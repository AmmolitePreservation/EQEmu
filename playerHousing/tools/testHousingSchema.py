"""Tests sql/housingSchema.sql against a local MariaDB: creation (twice), a realistic hierarchy, the queries the housing
module depends on, and the constraints that refuse bad rows.

Usage: python testHousingSchema.py   (a local MariaDB with a `peq` database, root without a password)
"""
import os, subprocess

here = os.path.dirname(os.path.abspath(__file__))
schema = open(os.path.join(here, '..', 'sql', 'housingSchema.sql')).read()


def sql(text, expectError=False):
  r = subprocess.run(['mariadb', '-uroot', '-N', 'peq'], input=text, capture_output=True, text=True)
  if (r.returncode != 0) != expectError:
    raise RuntimeError(f"unexpected result ({r.returncode}): {r.stderr[:400]}")
  return r.stderr if expectError else r.stdout.strip()


def check(label, query, expected):
  got = sql(query)
  assert got == expected, f'{label}: expected {expected!r}, got {got!r}'
  print(f'{label}: {got!r}')


tables = [l.split('`')[1] for l in schema.splitlines() if l.startswith('CREATE TABLE')]
sql("SET FOREIGN_KEY_CHECKS = 0;" + "".join(f"DROP TABLE IF EXISTS `{t}`;" for t in tables))
sql(schema); sql(schema)
print(f'created {len(tables)} tables; re-running the schema is safe')

sql("""
INSERT INTO real_estate_regions (id, name, x, y, z, size_x, size_y, size_z, heading) VALUES (1, 'plot 1', 100, 200, 0, 60, 40, 30, 128);
INSERT INTO real_estate_definitions (id, name, type, dz_id) VALUES (900, 'Sunrise Hills neighborhood', 5, 20504);
INSERT INTO real_estate_definitions (id, name, type, area_id, cost_definition_id, upkeep_definition_id, max_items, max_non_real_estate_items, max_npc_items,
  address, exit_zone_id, exit_x, exit_y, exit_z, exit_heading)
  VALUES (901, 'player plot 1', 4, 1, 1, 2, 200, 150, 20, '101 Market Street', 712, 1931.75, -2461.25, 2.95, 2.25),
         (902, 'guild plot 1', 8, 0, 1, 2, 200, 150, -1, '200 Guild Way', 712, 2035, -2940, 6, 0),
         (903, 'player plot 2', 4, 0, 1, 2, 200, 0, 4, '101 Terminus Heights', 712, 2035, -2940, 6, 0);
INSERT INTO real_estate_definitions (id, name, name_segment, type, dz_id, max_items) VALUES (5, 'House, Stucco interior', 'Three Room House', 3, 23000, 100),
  (31, 'Crate of Eviction', 'Crate of Eviction', 7, 0, 500), (192, 'Scalebreaker''s crate', 'Scalebreaker''s Storage Crate', 7, 0, 100);
INSERT INTO real_estate_definition_children VALUES (900, 901), (900, 902), (900, 903), (901, 5), (901, 31), (5, 192);
INSERT INTO real_estate_zone_templates VALUES (23000, 714, 0, 0, -12, 5, 0), (20504, 712, 0, 2035, -2940, 6, 0);
INSERT INTO real_estate_costs (id, name) VALUES (1, 'plot price'), (2, 'plot upkeep'), (3, 'upkeep reduction');
INSERT INTO real_estate_cost_options (cost_id, option_key) VALUES (1, 0), (2, 0), (3, 0);
INSERT INTO real_estate_cost_options (cost_id, option_key, min_expansion) VALUES (1, 1, 13), (1, 2, 15);
INSERT INTO real_estate_cost_items VALUES (1, 0, 0, 0, 5000000), (1, 1, 10, 1, 6), (1, 2, 15, 1, 3), (2, 0, 0, 0, 1000), (3, 0, 0, 0, -200);
INSERT INTO real_estate_properties (parent_id, definition_id, name) VALUES (0, 900, 'Bridge Hills');
INSERT INTO real_estate_properties (id, parent_id, definition_id, owner_character_id, upkeep_settled_at, next_upkeep_at, name)
  VALUES (100001, 100000, 901, 7, '2026-10-14 12:00:00', NOW() - INTERVAL 1 HOUR, ''), (100002, 100000, 902, 0, NULL, NULL, '');
INSERT INTO real_estate_properties (id, parent_id, definition_id, owner_character_id) VALUES (100003, 100001, 5, 7);
INSERT INTO real_estate_properties (id, parent_id, definition_id, owner_character_id, contained_in_item, archived) VALUES (100004, 0, 31, 7, 1, 1);
INSERT INTO real_estate_properties (id, parent_id, definition_id, name) VALUES (100005, 0, 900, 'Prophetic Alliance Village II'),
  (100006, 100005, 903, ''), (100007, 100006, 5, ''), (100008, 100007, 192, '');
INSERT INTO real_estate_instances VALUES (1001, 100000), (1002, 100005), (1003, 100003);
INSERT INTO real_estate_instance_counter VALUES (1, 1004);
INSERT INTO real_estate_property_balances VALUES (100001, 0, 10000), (100001, 9, 3);
INSERT INTO real_estate_items (property_id, owner_character_id, placed, scale, x, y, z, item_id) VALUES (100001, 7, 1, 1, 101, 201, 0, 61410), (100003, 7, 0, 0, 0, 0, 0, 61441),
  (100003, 7, 1, 1.5, 3, 4, 0, 66641);
INSERT INTO real_estate_item_looks VALUES (3, 4, 0, 0, 0, 255, 0, 2, 4278222848);
INSERT INTO real_estate_access_groups VALUES (100001, 0, 255), (100001, 1, 3);
INSERT INTO real_estate_access_names VALUES (100001, 'Friendly', 1);
INSERT INTO real_estate_npc_items (item_id, npc_type_id, race_tag) VALUES (66639, 1001, 'DAF');
INSERT INTO real_estate_npc_items (item_id, npc_type_id, mount_npc_type_id, race_tag) VALUES (93850, 1002, 1003, 'FRM');
INSERT INTO real_estate_npc_items (item_id, npc_type_id, race_tag, training_dummy) VALUES (66700, 1004, 'TDU', 1);
INSERT INTO real_estate_npc_item_actions VALUES (93850, 16178), (93850, 16175), (93850, 16179);
INSERT INTO real_estate_neighborhood_words (position, word) VALUES (0, 'Maple'), (0, 'Walnut'), (1, 'Bridge'), (1, 'Circle'), (2, 'Point'), (2, 'Lake'), (2, 'Hills');
""")

composedName = """
  WITH RECURSIVE chain AS (SELECT id, parent_id, 0 AS depth FROM real_estate_properties WHERE id = {}
    UNION ALL SELECT p.id, p.parent_id, c.depth + 1 FROM real_estate_properties p JOIN chain c ON p.id = c.parent_id)
  SELECT GROUP_CONCAT(CASE WHEN c.parent_id = 0 THEN p.name WHEN d.type IN (4, 8) THEN d.address ELSE d.name_segment END ORDER BY c.depth DESC SEPARATOR ', ') AS n
  FROM chain c JOIN real_estate_properties p ON p.id = c.id JOIN real_estate_definitions d ON d.id = p.definition_id"""
check('property ids start at 100000, above every region id', "SELECT id FROM real_estate_properties WHERE name = 'Bridge Hills'", '100000')
check('zone-in: the property copy 1001 runs and the tree under it', """
  WITH RECURSIVE tree AS (SELECT p.id FROM real_estate_properties p JOIN real_estate_instances i ON i.property_id = p.id WHERE i.instance_id = 1001
    UNION ALL SELECT p.id FROM real_estate_properties p JOIN tree t ON p.parent_id = t.id)
  SELECT GROUP_CONCAT(id ORDER BY id) FROM tree""", '100000,100001,100002,100003')
check("a neighborhood's zone is its definition's template's", """
  SELECT t.zone_id FROM real_estate_properties p JOIN real_estate_definitions d ON d.id = p.definition_id JOIN real_estate_zone_templates t ON t.dz_id = d.dz_id
  WHERE p.id = 100005""", '712')
check('inside a house: the house and its parents', """
  WITH RECURSIVE chain AS (SELECT id, parent_id FROM real_estate_properties WHERE id = 100003
    UNION ALL SELECT p.id, p.parent_id FROM real_estate_properties p JOIN chain c ON p.id = c.parent_id)
  SELECT GROUP_CONCAT(id ORDER BY id) FROM chain""", '100000,100001,100003')
check("a house's name composes three levels", composedName.format(100003), 'Bridge Hills, 101 Market Street, Three Room House')
check("Live's longest name (99 characters) composes from four levels", f"SELECT CONCAT(LENGTH(n), ' ', n) FROM ({composedName.format(100008)}) x",
      "99 Prophetic Alliance Village II, 101 Terminus Heights, Three Room House, Scalebreaker's Storage Crate")
check("entering a house finds its definition's zone template", """
  SELECT CONCAT_WS(' ', t.zone_id, t.zone_version, t.zone_in_x, t.zone_in_y, t.zone_in_z, t.zone_in_heading) FROM real_estate_properties p
  JOIN real_estate_definitions d ON d.id = p.definition_id JOIN real_estate_zone_templates t ON t.dz_id = d.dz_id WHERE p.id = 100003""", '714 0 0 -12 5 0')
check("leaving a house goes to its plot's exit point", """
  SELECT CONCAT_WS(' ', d.exit_zone_id, d.exit_zone_version, d.exit_x, d.exit_y, ROUND(d.exit_z, 2), d.exit_heading) FROM real_estate_properties h
  JOIN real_estate_properties p ON p.id = h.parent_id JOIN real_estate_definitions d ON d.id = p.definition_id WHERE h.id = 100003""", '712 0 1931.75 -2461.25 2.95 2.25')
check('children, derived from parent_id', "SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE parent_id = 100000", '100001,100002')
check('list A (owned)', "SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE owner_character_id = 7", '100001,100003,100004')
check('list C (archived, a subset of owned)', "SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE owner_character_id = 7 AND archived = 1", '100004')
check('a property inside an item is flagged and has no parent', """
  SELECT GROUP_CONCAT(id ORDER BY id) FROM real_estate_properties WHERE contained_in_item = 1 AND parent_id = 0""", '100004')
check('neighborhood row: player plots, used, guild plots, used', """
  SELECT CONCAT_WS(',', SUM(d.type = 4), SUM(d.type = 4 AND (p.owner_character_id <> 0 OR p.guild_id <> 0)),
                        SUM(d.type = 8), SUM(d.type = 8 AND (p.owner_character_id <> 0 OR p.guild_id <> 0)))
  FROM real_estate_properties p JOIN real_estate_definitions d ON d.id = p.definition_id WHERE p.parent_id = 100000""", '1,1,1,0')
check('revision-guarded update, fresh', "UPDATE real_estate_properties SET upkeep_currency = 9, revision = revision + 1 WHERE id = 100001 AND revision = 0; SELECT ROW_COUNT()", '1')
check('revision-guarded update, stale', "UPDATE real_estate_properties SET upkeep_currency = 0, revision = revision + 1 WHERE id = 100001 AND revision = 0; SELECT ROW_COUNT()", '0')
check('a new property and a new item start unexpired', "SELECT CONCAT_WS(' ', (SELECT upkeep_expired_time FROM real_estate_properties WHERE id = 100003), (SELECT MAX(upkeep_expired_at) FROM real_estate_items))", '0 0')
check('upkeep charge: due properties', "SELECT GROUP_CONCAT(id) FROM real_estate_properties WHERE next_upkeep_at <= NOW()", '100001')
plan = sql("EXPLAIN SELECT id FROM real_estate_properties FORCE INDEX (next_upkeep_at) WHERE next_upkeep_at <= NOW()")
assert 'next_upkeep_at' in plan and 'range' in plan, plan
print('upkeep charge: the due query is an index range scan')
check("a charge prorates the daily cost by the second (Live: 12,600 a day over 42,021 s is 6,128.0625)", """
  UPDATE real_estate_properties SET next_upkeep_at = upkeep_settled_at + INTERVAL 42021 SECOND WHERE id = 100001;
  UPDATE real_estate_property_balances b JOIN real_estate_properties p ON p.id = b.property_id
    SET b.amount = b.amount - 12600 * TIMESTAMPDIFF(SECOND, p.upkeep_settled_at, p.next_upkeep_at) / 86400 WHERE p.id = 100001 AND b.currency = 0;
  SELECT amount FROM real_estate_property_balances WHERE property_id = 100001 AND currency = 0""", '3871.937500')
check('an item stored without being placed keeps an all-zero transform', "SELECT CONCAT_WS(' ', heading, pitch, roll, scale, x, y, z) FROM real_estate_items WHERE id = 2", '0 0 0 0 0 0 0')
check("a placed copy's look", "SELECT CONCAT_WS(' ', race, gender, face, hair_style, beard, hair_color, body_material, body_tint) FROM real_estate_item_looks WHERE real_estate_item_id = 3",
      '4 0 0 0 255 0 2 4278222848')
check('picking the copy up deletes its look', "DELETE FROM real_estate_items WHERE id = 3; SELECT COUNT(*) FROM real_estate_item_looks", '0')
check('the next free generated neighborhood name skips one in use', """
  SELECT n.name FROM (
    SELECT CONCAT(t.word, ' ', m.word, ' ', p.word) AS name FROM real_estate_neighborhood_words t JOIN real_estate_neighborhood_words m JOIN real_estate_neighborhood_words p
      WHERE t.position = 0 AND m.position = 1 AND p.position = 2
    UNION SELECT CONCAT(t.word, ' ', p.word) FROM real_estate_neighborhood_words t JOIN real_estate_neighborhood_words p WHERE t.position = 0 AND p.position = 2
    UNION SELECT CONCAT(m.word, ' ', p.word) FROM real_estate_neighborhood_words m JOIN real_estate_neighborhood_words p WHERE m.position = 1 AND p.position = 2
  ) n LEFT JOIN real_estate_properties pr ON pr.name = n.name AND pr.definition_id IN (SELECT id FROM real_estate_definitions WHERE type = 5)
  WHERE pr.id IS NULL ORDER BY n.name LIMIT 1""", 'Bridge Lake')
check('an NPC item spawns its NPC type', "SELECT npc_type_id FROM real_estate_npc_items WHERE item_id = 66639", '1001')
check('a familiar riding a mount spawns both, a service no mount', "SELECT GROUP_CONCAT(CONCAT(item_id, ':', npc_type_id, ':', mount_npc_type_id) ORDER BY item_id) FROM real_estate_npc_items WHERE item_id IN (66639, 93850)", '66639:1001:0,93850:1002:1003')
check("a pet's actions in Live's order", "SELECT GROUP_CONCAT(action_id ORDER BY action_id) FROM real_estate_npc_item_actions WHERE item_id = 93850", '16175,16178,16179')
check('only a training dummy is one', "SELECT GROUP_CONCAT(item_id ORDER BY item_id) FROM real_estate_npc_items WHERE training_dummy = 1", '66700')
check("an NPC item's actions go with its row", "DELETE FROM real_estate_npc_items WHERE item_id = 93850; SELECT COUNT(*) FROM real_estate_npc_item_actions", '0')
check('an upkeep reduction stays negative', "SELECT amount FROM real_estate_cost_items WHERE cost_id = 3", '-200')
check('a cost stores each option it can be paid with', "SELECT GROUP_CONCAT(CONCAT(option_key, ':', currency, ':', amount) ORDER BY option_key) FROM real_estate_cost_items WHERE cost_id = 1", '0:0:5000000,1:10:6,2:15:3')
enabledAt = lambda expansion: (f"SELECT GROUP_CONCAT(option_key ORDER BY option_key) FROM real_estate_cost_options WHERE cost_id = 1"
  f" AND (min_expansion <= {expansion} OR min_expansion = -1) AND (max_expansion >= {expansion} OR max_expansion = -1)")
check("EQEmu's expansion filter leaves only coin at PoP", enabledAt(4), '0')
check("EQEmu's expansion filter adds the TBS option at TBS", enabledAt(13), '0,1')
check("EQEmu's expansion filter adds the SoD option at SoD", enabledAt(15), '0,1,2')
check('an access name may be a Co-Owner (5)', "INSERT INTO real_estate_access_names VALUES (100001, '_Guild2', 5); SELECT group_number FROM real_estate_access_names WHERE property_id = 100001 AND name = '_Guild2'", '5')
check('no NPC item cap is -1, the default', "INSERT INTO real_estate_definitions (id, name_segment, type, max_items) VALUES (990, 'Hut', 3, 10); SELECT max_npc_items FROM real_estate_definitions WHERE id = 990", '-1')
check('an NPC item cap may equal max items', "INSERT INTO real_estate_definitions (id, type, max_items, max_npc_items) VALUES (991, 7, 10, 10); SELECT max_npc_items FROM real_estate_definitions WHERE id = 991", '10')
check('a deactivated aura goes with its placed row', "INSERT INTO real_estate_inactive_auras VALUES (2); DELETE FROM real_estate_items WHERE id = 2; SELECT COUNT(*) FROM real_estate_inactive_auras", '0')
for label, bad, reason in [('definition type out of range', "INSERT INTO real_estate_definitions (id, type) VALUES (999, 9)", 'real_estate_definition_type'),
                   ('vault items above max items', "INSERT INTO real_estate_definitions (id, type, max_items, max_non_real_estate_items) VALUES (998, 3, 10, 11)", 'real_estate_definition_item_caps'),
                   ('NPC items above max items', "INSERT INTO real_estate_definitions (id, type, max_items, max_npc_items) VALUES (998, 3, 10, 11)", 'real_estate_definition_item_caps'),
                   ('NPC items below -1', "INSERT INTO real_estate_definitions (id, type, max_items, max_npc_items) VALUES (998, 3, 10, -2)", 'real_estate_definition_item_caps'),
                   ('an area id on a house', "INSERT INTO real_estate_definitions (id, type, area_id) VALUES (998, 3, 1)", 'real_estate_definition_area'),
                   ('a name segment on a plot', "INSERT INTO real_estate_definitions (id, type, name_segment, exit_zone_id) VALUES (998, 4, 'Plot', 712)", 'real_estate_definition_name_segment'),
                   ('a plot without an exit point', "INSERT INTO real_estate_definitions (id, type) VALUES (998, 4)", 'real_estate_definition_exit'),
                   ('an exit point on a house', "INSERT INTO real_estate_definitions (id, type, exit_zone_id) VALUES (998, 3, 712)", 'real_estate_definition_exit'),
                   ('a zone template without a zone', "INSERT INTO real_estate_zone_templates VALUES (23001, 0, 0, 0, 0, 0, 0)", 'real_estate_zone_template_zone'),
                   ('a zone template heading of 512', "INSERT INTO real_estate_zone_templates VALUES (23001, 716, 0, 0, 0, 0, 512)", 'real_estate_zone_template_heading'),
                   ('access group above 6', "INSERT INTO real_estate_access_groups VALUES (100001, 7, 1)", 'real_estate_access_group_number'),
                   ('access name in the Owner group (6)', "INSERT INTO real_estate_access_names VALUES (100001, 'Stranger', 6)", 'real_estate_access_name_group'),
                   ('access name in the Everyone group (0)', "INSERT INTO real_estate_access_names VALUES (100001, 'Stranger', 0)", 'real_estate_access_name_group'),
                   ('poll kind 3', "INSERT INTO real_estate_polls (kind, selection_type, opens_at, closes_at) VALUES (3, 2, NOW(), NOW() + INTERVAL 1 DAY)", 'real_estate_poll_kind'),
                   ('poll selection type 4', "INSERT INTO real_estate_polls (kind, selection_type, opens_at, closes_at) VALUES (2, 4, NOW(), NOW() + INTERVAL 1 DAY)", 'real_estate_poll_selection_type'),
                   ('poll closing before it opens', "INSERT INTO real_estate_polls (kind, selection_type, opens_at, closes_at) VALUES (2, 2, NOW(), NOW() - INTERVAL 1 DAY)", 'real_estate_poll_dates'),
                   ('region without volume', "INSERT INTO real_estate_regions (id, size_x, size_y, size_z) VALUES (2, 0, 1, 1)", 'real_estate_region_size'),
                   ('region id at the property id floor', "INSERT INTO real_estate_regions (id, size_x, size_y, size_z) VALUES (100000, 1, 1, 1)", 'real_estate_region_below_property_ids'),
                   ('an item without a scale', "INSERT INTO real_estate_items (property_id, item_id) VALUES (100001, 61410)", "Field 'scale' doesn't have a default value"),
                   ('a look for no placed item', "INSERT INTO real_estate_item_looks VALUES (999, 4, 0, 0, 0, 0, 0, 0, 0)", 'real_estate_item_look_item'),
                   ('a look with gender 3', "INSERT INTO real_estate_item_looks VALUES (1, 4, 3, 0, 0, 0, 0, 0, 0)", 'real_estate_item_look_gender'),
                   ('a look with race 0', "INSERT INTO real_estate_item_looks VALUES (1, 0, 0, 0, 0, 0, 0, 0, 0)", 'real_estate_item_look_race'),
                   ('an NPC item race tag of two characters', "INSERT INTO real_estate_npc_items (item_id, npc_type_id, race_tag) VALUES (66641, 1001, 'ER')", 'real_estate_npc_item_race_tag'),
                   ('an NPC item without a race tag', "INSERT INTO real_estate_npc_items (item_id, npc_type_id) VALUES (66641, 1001)", "Field 'race_tag' doesn't have a default value"),
                   ('a training dummy flag of 2', "INSERT INTO real_estate_npc_items (item_id, npc_type_id, race_tag, training_dummy) VALUES (66641, 1001, 'ERM', 2)", 'real_estate_npc_item_dummy'),
                   ('a deactivated aura for no placed item', "INSERT INTO real_estate_inactive_auras VALUES (999)", 'real_estate_inactive_aura_item'),
                   ('a pet action Live has none of', "INSERT INTO real_estate_npc_item_actions VALUES (66639, 25754)", 'real_estate_npc_item_action_id'),
                   ('an action for no NPC item', "INSERT INTO real_estate_npc_item_actions VALUES (61410, 16175)", 'real_estate_npc_item_action_item'),
                   ('duplicate neighborhood word in one position', "INSERT INTO real_estate_neighborhood_words (position, word) VALUES (0, 'Maple')", "for key 'position_word'"),
                   ('a second copy of one property', "INSERT INTO real_estate_instances VALUES (1004, 100000)", "for key 'property_id'"),
                   ('a second copy on one instance id', "INSERT INTO real_estate_instances VALUES (1001, 100005)", "for key 'PRIMARY'"),
                   ('a second instance counter', "INSERT INTO real_estate_instance_counter VALUES (2, 5)", 'real_estate_instance_counter_single'),
                   ('neighborhood word position above 2', "INSERT INTO real_estate_neighborhood_words (position, word) VALUES (3, 'Vale')", 'neighborhood_word_position')]:
  error = sql(bad, expectError=True)
  assert reason in error, f'{label}: refused for another reason: {error[:300]}'
  print(f'rejected: {label} ({reason})')
