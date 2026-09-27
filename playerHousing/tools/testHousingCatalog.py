"""Tests sql/housingCatalog.sql against a local MariaDB (a `catalogTest` database, root without a password): loaded into the
tested schema; the counts; the zone loader's rules; every definition the item export names; values against their sources;
the plot regions under the client's own containment rule; regeneration; a refused second run; and a failure part way leaving
the catalog empty.

Usage: python3 testHousingCatalog.py
"""
import csv, json, math, os, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from regionMatrix import boxToMatrix, clientContains

catalogPath = os.path.join(here, '..', 'sql', 'housingCatalog.sql')


def sql(text, expectError=False):
  r = subprocess.run(['mariadb', '-uroot', '-N', 'catalogTest'], input=text, capture_output=True, text=True)
  if (r.returncode != 0) != expectError:
    raise RuntimeError(f'unexpected result ({r.returncode}): {r.stderr[:400]}')
  return r.stderr if expectError else r.stdout.strip()


def check(label, query, expected):
  actual = sql(query)
  if actual != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
  print(f'{label}: {actual!r}')


def freshDatabase():
  subprocess.run(['mariadb', '-uroot', '-e', 'DROP DATABASE IF EXISTS catalogTest; CREATE DATABASE catalogTest'], check=True)
  sql(open(os.path.join(here, '..', 'sql', 'housingSchema.sql')).read())


catalog = open(catalogPath).read()
generator = subprocess.run([sys.executable, os.path.join(here, 'makeHousingCatalogSql.py')], capture_output=True, text=True, check=True)
if open(catalogPath).read() != catalog:
  raise AssertionError('sql/housingCatalog.sql differs from a fresh generation')
print('the committed catalog equals a fresh generation')

freshDatabase()
sql(catalog)
check('counts', "SELECT CONCAT_WS(' ', (SELECT COUNT(*) FROM real_estate_definitions), (SELECT COUNT(*) FROM real_estate_definition_children), (SELECT COUNT(*) FROM real_estate_costs), (SELECT COUNT(*) FROM real_estate_cost_options), (SELECT COUNT(*) FROM real_estate_cost_items), (SELECT COUNT(*) FROM real_estate_regions), (SELECT COUNT(*) FROM real_estate_neighborhood_words), (SELECT COUNT(*) FROM real_estate_currencies))", '119 75 29 29 29 75 30 0')
check('each cost stores one positive coin option', "SELECT COUNT(*) FROM real_estate_costs c WHERE (SELECT COUNT(*) FROM real_estate_cost_options o WHERE o.cost_id = c.id AND o.option_key = 0) <> 1 OR (SELECT COUNT(*) FROM real_estate_cost_items i WHERE i.cost_id = c.id AND i.option_key = 0 AND i.currency = 0 AND i.sub_key = 0 AND i.amount > 0) <> 1 OR EXISTS (SELECT 1 FROM real_estate_cost_items i WHERE i.cost_id = c.id AND (i.option_key <> 0 OR i.currency <> 0))", '0')
check('no cost is both a purchase and an upkeep cost', "SELECT COUNT(*) FROM real_estate_definitions a JOIN real_estate_definitions b ON a.purchase_cost_id <> 0 AND a.purchase_cost_id = b.upkeep_cost_id", '0')
check('every reference resolves', "SELECT (SELECT COUNT(*) FROM real_estate_definitions d WHERE (d.region_id <> 0 AND NOT EXISTS (SELECT 1 FROM real_estate_regions r WHERE r.id = d.region_id)) OR (d.purchase_cost_id <> 0 AND NOT EXISTS (SELECT 1 FROM real_estate_costs c WHERE c.id = d.purchase_cost_id)) OR (d.upkeep_cost_id <> 0 AND NOT EXISTS (SELECT 1 FROM real_estate_costs c WHERE c.id = d.upkeep_cost_id))) + (SELECT COUNT(*) FROM real_estate_definition_children h WHERE NOT EXISTS (SELECT 1 FROM real_estate_definitions d WHERE d.id = h.child_id) OR NOT EXISTS (SELECT 1 FROM real_estate_definitions d WHERE d.id = h.parent_id))", '0')

groups = [row for row in csv.reader(open(os.path.join(here, '..', 'content', 'itemDefinitions.txt')), delimiter='\t') if row and row[0] == 'items']
upkeepIds = sorted({int(row[2]) for row in groups if int(row[2])})
check('ids no item names stay under 174', f"SELECT COUNT(*) FROM real_estate_definitions WHERE id >= 174 AND id NOT IN ({', '.join(row[1] for row in groups)})", '0')
check('every item-named definition exists', f"SELECT COUNT(*) FROM real_estate_definitions WHERE id IN ({', '.join(row[1] for row in groups)})", str(len(groups)))
check('every item upkeep id is a cost', f"SELECT COUNT(*) FROM real_estate_costs WHERE id IN ({', '.join(map(str, upkeepIds))})", str(len(upkeepIds)))
check('item definition types (houses, halls, crates)', "SELECT CONCAT_WS(' ', SUM(type = 3), SUM(type = 2), SUM(type = 7)) FROM real_estate_definitions WHERE id < 42 OR id > 116", '27 4 12')

check('Sunrise Hills', "SELECT CONCAT_WS(' ', name, type, zone_id, max_items) FROM real_estate_definitions WHERE id = 1", 'Sunrise Hills 5 712 0')
check('its children are the 75 plots', "SELECT CONCAT(MIN(child_id), '-', MAX(child_id)) FROM real_estate_definition_children WHERE parent_id = 1", '42-116')
template = json.load(open(os.path.join(here, '..', 'content', 'sunriseHillsPlots.json')))
first = template[0]
stones = {int(r[1]): r for r in csv.reader(open(os.path.join(here, '..', 'content', 'sunriseHillsLotSquares.tsv')), delimiter='\t')}
stone = stones[first['door']]
check(f"plot 42, {first['address']}", "SELECT CONCAT_WS(' ', d.name, d.type, d.max_items, d.max_non_real_estate_items, d.max_pet_items, p.amount, u.amount) FROM real_estate_definitions d JOIN real_estate_cost_items p ON p.cost_id = d.purchase_cost_id JOIN real_estate_cost_items u ON u.cost_id = d.upkeep_cost_id WHERE d.id = 42",
      f"{first['address']} 4 {first['capacity']} 0 6 {first['pricePlatinum'] * 1000} {first['upkeepCopperPerDay']}")
check('its layout origin is its border stone', "SELECT CONCAT_WS(' ', ROUND(layout_origin_x, 2), ROUND(layout_origin_y, 2), ROUND(layout_heading)) FROM real_estate_definitions WHERE id = 42",
      f'{float(stone[2]):.2f} {float(stone[3]):.2f} {round(float(stone[5]))}')
check('its region is its box, 100 below its ground to 900 above', "SELECT CONCAT_WS(' ', ROUND(x, 2), ROUND(y, 2), ROUND(z, 2), ROUND(size_x, 2), ROUND(size_y, 2), size_z, heading) FROM real_estate_regions WHERE id = 42",
      f"{first['center'][0]:.2f} {first['center'][1]:.2f} {first['center'][2] + 400:.2f} {first['sizeAcross']:.2f} {first['sizeAlong']:.2f} 1000 {first['heading']:g}")
check('guild plot 113, 200 Guild Way', "SELECT CONCAT_WS(' ', d.name, d.type, d.max_items, d.max_non_real_estate_items, d.max_pet_items, p.amount, u.amount, ROUND(d.layout_origin_x, 1), ROUND(d.layout_origin_y, 1), d.layout_heading) FROM real_estate_definitions d JOIN real_estate_cost_items p ON p.cost_id = d.purchase_cost_id JOIN real_estate_cost_items u ON u.cost_id = d.upkeep_cost_id WHERE d.id = 113",
      '200 Guild Way 8 210 0 12 21000 2100 2689.8 2633.5 123')
check('houses: three-room House, one-room House, Stone Manor', "SELECT GROUP_CONCAT(CONCAT_WS(':', id, zone_id, max_items, max_non_real_estate_items, max_pet_items, purchase_cost_id, upkeep_cost_id) ORDER BY id) FROM real_estate_definitions WHERE id IN (5, 18, 34)",
      '5:714:300:40:0:0:0,18:715:100:20:0:0:0,34:717:500:300:0:0:0')
check('halls: Standard, Grand, Palatial, Modest', "SELECT GROUP_CONCAT(CONCAT_WS(':', id, zone_id, max_items, max_non_real_estate_items, max_pet_items) ORDER BY id) FROM real_estate_definitions WHERE type = 2",
      '38:345:1000:0:0,39:738:1500:0:0,40:737:2000:0:0,41:751:1500:0:0')
check('crates: Small, Estate, All Hallows Eve, the Crate of Eviction', "SELECT GROUP_CONCAT(CONCAT_WS(':', id, max_items, max_non_real_estate_items, max_pet_items) ORDER BY id) FROM real_estate_definitions WHERE id IN (31, 174, 183, 184)",
      '31:3000:3000:3000,174:50:0:0,183:500:0:0,184:100:0:0')
check('item upkeep: station, three-room Domicile, Estate crate', "SELECT GROUP_CONCAT(CONCAT(cost_id, ':', amount) ORDER BY cost_id) FROM real_estate_cost_items WHERE cost_id IN (6, 20, 35)", '6:100,20:7350,35:220')
check('neighborhood words by position', "SELECT GROUP_CONCAT(n ORDER BY position) FROM (SELECT position, COUNT(*) AS n FROM real_estate_neighborhood_words GROUP BY position) w", '10,5,15')

regions = [dict(zip(['id', 'x', 'y', 'z', 'sizeX', 'sizeY', 'sizeZ', 'heading'], map(float, line.split('\t'))))
           for line in sql('SELECT id, x, y, z, size_x, size_y, size_z, heading FROM real_estate_regions ORDER BY id').splitlines()]
matrices = {r['id']: boxToMatrix(r['x'], r['y'], r['z'], r['sizeX'], r['sizeY'], r['sizeZ'], r['heading']) for r in regions}
tested = 0
for r in regions:
  angle = r['heading'] * 2 * math.pi / 512
  along, across = (math.sin(angle), math.cos(angle)), (math.cos(angle), -math.sin(angle))
  reach = math.hypot(r['sizeX'], r['sizeY']) / 2
  near = [o for o in regions if o is not r and math.hypot(o['x'] - r['x'], o['y'] - r['y']) <= reach + math.hypot(o['sizeX'], o['sizeY']) / 2]
  for a in (-0.98, -0.5, 0, 0.5, 0.98):
    for b in (-0.98, -0.5, 0, 0.5, 0.98):
      for c in (-0.98, 0, 0.98):
        x = r['x'] + a * along[0] * r['sizeY'] / 2 + b * across[0] * r['sizeX'] / 2
        y = r['y'] + a * along[1] * r['sizeY'] / 2 + b * across[1] * r['sizeX'] / 2
        z = r['z'] + c * r['sizeZ'] / 2
        if not clientContains(matrices[r['id']], x, y, z):
          raise AssertionError(f"region {r['id']} doesn't contain its own point")
        for other in near:
          if clientContains(matrices[other['id']], x, y, z):
            raise AssertionError(f"regions {r['id']} and {other['id']} overlap at ({x:.1f}, {y:.1f}, {z:.1f})")
        tested += 1
print(f'client containment: {tested} points, each in its own plot region only')

sql(catalog, expectError=True)
check('a second run is refused and changes nothing', 'SELECT COUNT(*) FROM real_estate_definitions', '119')
freshDatabase()
broken = catalog.replace('  COMMIT;\n', "  INSERT INTO real_estate_regions (id, size_x, size_y, size_z) VALUES (999, 0, 1, 1);\n  COMMIT;\n")
assert broken != catalog
error = sql(broken, expectError=True)
check('a failure part way leaves the catalog empty', "SELECT (SELECT COUNT(*) FROM real_estate_definitions) + (SELECT COUNT(*) FROM real_estate_regions) + (SELECT COUNT(*) FROM real_estate_costs)", '0')
subprocess.run(['mariadb', '-uroot', '-e', 'DROP DATABASE catalogTest'], check=True)
print('all catalog checks passed')
