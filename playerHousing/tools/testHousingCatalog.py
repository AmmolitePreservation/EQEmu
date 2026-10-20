"""Tests sql/housingCatalog.sql against a local MariaDB (a `catalogTest` database, root without a password): loaded into the
tested schema; the counts; the zone loader's rules; every definition the item export names; values against their sources;
names composed from the catalog against Live's; zone templates and exit points; the plot regions under the client's own
containment rule; regeneration; a refused second run; and a failure part way leaving the catalog empty.

Usage: python testHousingCatalog.py --live <livedumps clone>
"""
import argparse, csv, json, math, os, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from regionMatrix import boxToMatrix, clientContains

catalogPath = os.path.join(here, '..', 'sql', 'housingCatalog.sql')
parser = argparse.ArgumentParser(description='Test sql/housingCatalog.sql.')
parser.add_argument('--live', required=True, help='a livedumps clone')
liveClone = parser.parse_args().live


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
generator = subprocess.run([sys.executable, os.path.join(here, 'makeHousingCatalogSql.py'), '--live', liveClone], capture_output=True, text=True, check=True)
if open(catalogPath).read() != catalog:
  raise AssertionError('sql/housingCatalog.sql differs from a fresh generation')
print('the committed catalog equals a fresh generation')

freshDatabase()
sql(catalog)
check('counts', "SELECT CONCAT_WS(' ', (SELECT COUNT(*) FROM real_estate_definitions), (SELECT COUNT(*) FROM real_estate_definition_children), (SELECT COUNT(*) FROM real_estate_costs), (SELECT COUNT(*) FROM real_estate_cost_options), (SELECT COUNT(*) FROM real_estate_cost_items), (SELECT COUNT(*) FROM real_estate_regions), (SELECT COUNT(*) FROM real_estate_zone_templates), (SELECT COUNT(*) FROM real_estate_neighborhood_words))", '119 75 29 79 79 75 11 30')
check('each cost has one ungated positive coin option', "SELECT COUNT(*) FROM real_estate_costs c WHERE (SELECT COUNT(*) FROM real_estate_cost_items i JOIN real_estate_cost_options o USING (cost_id, option_key) WHERE i.cost_id = c.id AND i.currency = 0 AND i.sub_key = 0 AND i.amount > 0 AND o.min_expansion = -1) <> 1", '0')
check('currency options open at their expansion (TBS 13, SoF 14, SoD 15)', "SELECT GROUP_CONCAT(DISTINCT CONCAT(i.currency, ':', o.min_expansion) ORDER BY i.currency) FROM real_estate_cost_items i JOIN real_estate_cost_options o USING (cost_id, option_key) WHERE i.currency <> 0", '10:13,11:13,12:13,14:14,15:15')
check('option keys run 0, 1, 2, ... in each cost', "SELECT COUNT(*) FROM (SELECT MAX(option_key) + 1 AS keyCount, COUNT(*) AS optionCount FROM real_estate_cost_options GROUP BY cost_id) k WHERE keyCount <> optionCount", '0')
check("Live's plot costs: upkeep 7-11, purchase 12-16, in copper", "SELECT GROUP_CONCAT(CONCAT(cost_id, ':', amount) ORDER BY cost_id) FROM real_estate_cost_items WHERE cost_id BETWEEN 7 AND 16 AND currency = 0", '7:8400,8:12600,9:16800,10:4200,11:2100,12:84000,13:126000,14:168000,15:42000,16:21000')
check('each plot and guild plot holds one house or hall; only Sunrise Hills holds more, 1711', "SELECT CONCAT_WS(' ', SUM(max_children = 1 AND type IN (4, 8)), GROUP_CONCAT(IF(type NOT IN (4, 8) AND max_children <> 0, CONCAT(id, ':', max_children), NULL))) FROM real_estate_definitions", '75 2:1711')
check('the 75 plots buy with costs 12-16 and pay upkeep 7-11', "SELECT CONCAT_WS(' ', COUNT(*), MIN(cost_definition_id), MAX(cost_definition_id), MIN(upkeep_definition_id), MAX(upkeep_definition_id)) FROM real_estate_definitions WHERE type IN (4, 8)", '75 12 16 7 11')
check('only plots are sold', "SELECT COUNT(*) FROM real_estate_definitions WHERE cost_definition_id <> 0 AND type NOT IN (4, 8)", '0')
check('every reference resolves', "SELECT (SELECT COUNT(*) FROM real_estate_definitions d WHERE (d.area_id <> 0 AND NOT EXISTS (SELECT 1 FROM real_estate_regions r WHERE r.id = d.area_id))) + (SELECT COUNT(*) FROM real_estate_definition_children h WHERE NOT EXISTS (SELECT 1 FROM real_estate_definitions d WHERE d.id = h.child_id) OR NOT EXISTS (SELECT 1 FROM real_estate_definitions d WHERE d.id = h.parent_id)) + (SELECT COUNT(*) FROM real_estate_definitions d WHERE (d.cost_definition_id <> 0 AND NOT EXISTS (SELECT 1 FROM real_estate_costs c WHERE c.id = d.cost_definition_id)) OR (d.upkeep_definition_id <> 0 AND NOT EXISTS (SELECT 1 FROM real_estate_costs c WHERE c.id = d.upkeep_definition_id))) + (SELECT COUNT(*) FROM real_estate_definitions d WHERE d.dz_id <> 0 AND NOT EXISTS (SELECT 1 FROM real_estate_zone_templates t WHERE t.dz_id = d.dz_id)) + (SELECT COUNT(*) FROM real_estate_zone_templates t WHERE NOT EXISTS (SELECT 1 FROM real_estate_definitions d WHERE d.dz_id = t.dz_id))", '0')

groups = [row for row in csv.reader(open(os.path.join(here, '..', 'content', 'itemDefinitions.txt')), delimiter='\t') if row and row[0] == 'items']
upkeepIds = sorted({int(row[2]) for row in groups if int(row[2])})
check("player plots are Live's 101-173", "SELECT CONCAT(MIN(id), '-', MAX(id), ' ', COUNT(*)) FROM real_estate_definitions WHERE type = 4", '101-173 71')
check("guild plots are Live's 187-190", "SELECT CONCAT(MIN(id), '-', MAX(id), ' ', COUNT(*)) FROM real_estate_definitions WHERE type = 8", '187-190 4')
check('every item-named definition exists', f"SELECT COUNT(*) FROM real_estate_definitions WHERE id IN ({', '.join(row[1] for row in groups)})", str(len(groups)))
check('every item upkeep id is a cost', f"SELECT COUNT(*) FROM real_estate_costs WHERE id IN ({', '.join(map(str, upkeepIds))})", str(len(upkeepIds)))
check('item definition types (houses, halls, crates)', "SELECT CONCAT_WS(' ', SUM(type = 3), SUM(type = 2), SUM(type = 7)) FROM real_estate_definitions WHERE type IN (2, 3, 7)", '27 4 12')
check('no NPC item cap on houses, halls, crates and the neighborhood; plots 4-6, guild plots 12', "SELECT CONCAT_WS(' ', SUM(type IN (2, 3, 5, 7) AND max_npc_items <> -1), (SELECT GROUP_CONCAT(DISTINCT max_npc_items ORDER BY max_npc_items) FROM real_estate_definitions WHERE type = 4), (SELECT GROUP_CONCAT(DISTINCT max_npc_items) FROM real_estate_definitions WHERE type = 8)) FROM real_estate_definitions", '0 4,5,6 12')

check('Sunrise Hills is definition 2', "SELECT CONCAT_WS(' ', name, type, max_children, max_npc_items, max_items, dz_id) FROM real_estate_definitions WHERE id = 2", 'Sunrise Hills 5 1711 -1 0 20504')
check('its children are the 75 plots', "SELECT CONCAT(COUNT(*), ' ', MIN(child_id), '-', MAX(child_id)) FROM real_estate_definition_children WHERE parent_id = 2", '75 101-190')
template = json.load(open(os.path.join(here, '..', 'content', 'sunriseHillsPlots.json')))
first = template[0]
stones = {int(r[1]): r for r in csv.reader(open(os.path.join(here, '..', 'content', 'sunriseHillsLotSquares.tsv')), delimiter='\t')}
liveDefinitions = {row['id']: row for row in csv.DictReader(open(os.path.join(liveClone, 'master', 'definitions.tsv'), encoding='latin-1'), delimiter='\t')}
live = liveDefinitions['171']
check(f"plot 171 is Live's {first['address']}", "SELECT CONCAT_WS(' ', name, type, area_id, max_items, max_non_real_estate_items, max_npc_items, cost_definition_id, upkeep_definition_id) FROM real_estate_definitions WHERE id = 171",
      f"{first['address']} 4 {live['areaId']} {live['maxItems']} 0 {live['maxNpcItems']} {live['costDefinition']} {live['upkeepDefinition']}")
check("plot 101's switch is Live's (-219.06, 332.21, 0.127) in Live's y-first order", "SELECT CONCAT_WS(' ', ROUND(switch_y, 2), ROUND(switch_x, 2), ROUND(switch_z, 3), switch_heading) FROM real_estate_definitions WHERE id = 101", '-219.06 332.21 0.127 256')
check("200 Guild Way's switch is Live's (2633.2, 2689.73, 108.872)", "SELECT CONCAT_WS(' ', ROUND(switch_y, 2), ROUND(switch_x, 2), ROUND(switch_z, 3), switch_heading) FROM real_estate_definitions WHERE name = '200 Guild Way'", '2633.20 2689.73 108.872 123')
switches = {int(line.split('\t')[0]): [float(v) for v in line.split('\t')[1:]] for line in sql('SELECT id, switch_x, switch_y, switch_heading FROM real_estate_definitions WHERE type = 4').splitlines()}
byAddress = {d['address']: int(i) for i, d in liveDefinitions.items() if d['type'] == '4'}
offsets = [(math.hypot(switches[byAddress[p['address']]][0] - float(stones[p['door']][2]), switches[byAddress[p['address']]][1] - float(stones[p['door']][3])),
            abs(switches[byAddress[p['address']]][2] - float(stones[p['door']][5]))) for p in template if p['kind'] == 'plot']
if len(offsets) != 71 or max(o[0] for o in offsets) > 0.02 or max(o[1] for o in offsets) > 0.5:
  raise AssertionError(f'a player plot switch is off its PEQ border stone: {max(offsets)}')
print(f'every player plot switch lies on its PEQ border stone: {len(offsets)} within 0.02 units and half a heading step')
check('its region is its box, 350 below its ground to 350 above', "SELECT CONCAT_WS(' ', ROUND(x, 2), ROUND(y, 2), ROUND(z, 2), ROUND(size_x, 2), ROUND(size_y, 2), size_z, heading) FROM real_estate_regions WHERE id = 171",
      f"{first['center'][0]:.2f} {first['center'][1]:.2f} {first['center'][2]:.2f} {first['sizeAcross']:.2f} {first['sizeAlong']:.2f} 700 {first['heading']:g}")
guildPlot = next(plot for plot in template if plot['address'] == '200 Guild Way')
check("a guild plot's region reaches 450 below and above its ground", "SELECT CONCAT_WS(' ', ROUND(r.z, 2), r.size_z) FROM real_estate_regions r JOIN real_estate_definitions d ON d.area_id = r.id WHERE d.name = '200 Guild Way'",
      f"{guildPlot['center'][2]:.2f} 900")
check('guild plot 200 Guild Way', "SELECT CONCAT_WS(' ', id BETWEEN 187 AND 190, type, max_items, max_non_real_estate_items, max_npc_items, cost_definition_id, upkeep_definition_id) FROM real_estate_definitions WHERE name = '200 Guild Way'",
      '1 8 210 0 12 16 11')
check('houses: three-room House, one-room House, Stone Manor, each with its interior zone', "SELECT GROUP_CONCAT(CONCAT_WS(':', d.id, t.zone_id, t.zone_version, d.max_items, d.max_non_real_estate_items, d.max_npc_items, d.upkeep_definition_id) ORDER BY d.id) FROM real_estate_definitions d JOIN real_estate_zone_templates t USING (dz_id) WHERE d.id IN (5, 18, 34)",
      '5:714:0:500:75:-1:17,18:715:0:300:60:-1:21,34:717:0:500:300:-1:0')
check('halls: Standard (345 version 2), Grand, Palatial, Modest', "SELECT GROUP_CONCAT(CONCAT_WS(':', d.id, d.dz_id, t.zone_id, t.zone_version, d.max_items, d.max_non_real_estate_items, d.max_npc_items) ORDER BY d.id) FROM real_estate_definitions d JOIN real_estate_zone_templates t USING (dz_id) WHERE d.type = 2",
      '38:35633:345:2:1000:0:-1,39:35634:738:0:1500:0:-1,40:35635:737:0:2000:0:-1,41:35642:751:0:1500:0:-1')
check("zone templates: Live's dz ids, their zones and arrival points", "SELECT GROUP_CONCAT(CONCAT_WS(':', dz_id, zone_id, zone_version, zone_in_x, zone_in_y, zone_in_z, zone_in_heading) ORDER BY dz_id SEPARATOR ' ') FROM real_estate_zone_templates",
      '20504:712:0:2042:-2958:3.193:481 23000:714:0:0:-12:5:0 23001:716:0:0:-12:5:0 23002:717:0:0:-12:5:0 23003:715:0:0:-12:5:0 23004:718:0:0:-12:5:0 23005:719:0:0:-12:5:0 '
      '35633:345:2:-1:-1:3.4:0 35634:738:0:255:2:-10:386 35635:737:0:71:-1:1.56:386 35642:751:0:255:2:-10:386')
check("the neighborhood's arrival is Live's (2042, -2958, 3.19254) facing 481, not its header's safe point (2035, -2940, 6)", "SELECT CONCAT_WS(' ', zone_in_x, zone_in_y, ROUND(zone_in_z, 3), zone_in_heading) FROM real_estate_zone_templates WHERE dz_id = 20504", '2042 -2958 3.193 481')
check("exit points: Live's measured ones on 101 Market Street and 200 Terminus Heights", "SELECT GROUP_CONCAT(CONCAT_WS(':', id, exit_zone_id, exit_zone_version, exit_x, exit_y, ROUND(exit_z, 2), exit_heading) ORDER BY id SEPARATOR ' ') FROM real_estate_definitions WHERE id IN (171, 188)",
      '171:712:0:1931.75:-2461.25:2.95:2.25 188:712:0:-1444.37:2256.5:112.95:511.25')
check("every other plot exits at a corner of its outline, at its height, facing north, and no other definition has an exit", "SELECT CONCAT_WS(' ', (SELECT COUNT(*) FROM real_estate_definitions d JOIN real_estate_regions r ON r.id = d.area_id WHERE d.type IN (4, 8) AND d.id NOT IN (171, 188) AND d.exit_zone_id = 712 AND d.exit_zone_version = 0 AND ABS(SQRT(POW(d.exit_x - r.x, 2) + POW(d.exit_y - r.y, 2)) - SQRT(POW(r.size_x, 2) + POW(r.size_y, 2)) / 2) < 0.01 AND d.exit_z = r.z AND d.exit_heading = 0), (SELECT COUNT(*) FROM real_estate_definitions WHERE type NOT IN (4, 8) AND exit_zone_id <> 0))", '73 0')
check('every plot and guild plot sends its street address, and no other definition any', "SELECT CONCAT_WS(' ', (SELECT address FROM real_estate_definitions WHERE id = 101), (SELECT COUNT(*) FROM real_estate_definitions WHERE type IN (4, 8) AND address = ''), (SELECT COUNT(*) FROM real_estate_definitions WHERE type NOT IN (4, 8) AND address <> ''))", '108 Brimming Way 0 0')
check("name segments: Live's Three Room House on 5-17 and 32-34, One Room House on 18-29", "SELECT CONCAT_WS(' / ', (SELECT GROUP_CONCAT(DISTINCT name_segment) FROM real_estate_definitions WHERE id BETWEEN 5 AND 17 OR id BETWEEN 32 AND 34), (SELECT GROUP_CONCAT(DISTINCT name_segment) FROM real_estate_definitions WHERE id BETWEEN 18 AND 29))", 'Three Room House / One Room House')
check('halls and crates take their item names as segments', "SELECT GROUP_CONCAT(CONCAT(id, ':', name_segment) ORDER BY id) FROM real_estate_definitions WHERE id IN (31, 38, 174, 184)", '31:Crate of Eviction,38:Standard Guild Hall,174:Small Storage Crate,184:All Hallows Eve Storage Crate')

segments = {int(line.split('\t')[0]): line.split('\t')[1:] for line in sql("SELECT id, type, address, name_segment FROM real_estate_definitions").splitlines()}
liveProperties = {(row['server'], row['id']): row for row in csv.DictReader(open(os.path.join(liveClone, 'master', 'properties.tsv'), encoding='latin-1'), delimiter='	')}
compared, mismatches = 0, []
for row in liveProperties.values():
  chain, current = [], row
  while current is not None and int(current['definitionId']) in segments and segments[int(current['definitionId'])][0] != '5':
    chain.append(current)
    current = liveProperties.get((current['server'], current['parentId']))
  if current is None or int(current['definitionId']) not in segments or not chain:
    continue
  composed = ', '.join([current['dynamicName']] + [segments[int(link['definitionId'])][1] if segments[int(link['definitionId'])][0] in ('4', '8') else segments[int(link['definitionId'])][2] for link in reversed(chain)])
  compared += 1
  if composed != row['dynamicName']:
    mismatches.append((row['id'], composed, row['dynamicName']))
if mismatches or compared != 212:
  raise AssertionError(f'{compared} Live names compared; composed from the catalog differ: {mismatches[:5]}')
print(f"Live's property names: all {compared} whose definitions the catalog holds compose from the catalog exactly")
check('crates: Small, Estate, All Hallows Eve, the Crate of Eviction', "SELECT GROUP_CONCAT(CONCAT_WS(':', id, max_items, max_non_real_estate_items, max_npc_items) ORDER BY id) FROM real_estate_definitions WHERE id IN (31, 174, 183, 184)",
      '31:0:0:-1,174:50:0:-1,183:500:0:-1,184:100:0:-1')
check('item upkeep: station, three-room Domicile, Estate crate', "SELECT GROUP_CONCAT(CONCAT(cost_id, ':', amount) ORDER BY cost_id) FROM real_estate_cost_items WHERE cost_id IN (6, 20, 35) AND currency = 0", '6:100,20:7350,35:220')
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

error = sql(catalog, expectError=True)
assert 'the housing catalog tables are not empty' in error, error[:300]
error = sql("INSERT INTO real_estate_definitions (id, type) VALUES (997, 9)", expectError=True)
assert 'real_estate_definition_type' in error, error[:300]
print('rejected: a definition type out of range')
check('a second run is refused and changes nothing', 'SELECT COUNT(*) FROM real_estate_definitions', '119')
freshDatabase()
broken = catalog.replace('  COMMIT;\n', "  INSERT INTO real_estate_regions (id, size_x, size_y, size_z) VALUES (999, 0, 1, 1);\n  COMMIT;\n")
assert broken != catalog
error = sql(broken, expectError=True)
assert 'real_estate_region_size' in error, error[:300]
check('a failure part way leaves the catalog empty', "SELECT (SELECT COUNT(*) FROM real_estate_definitions) + (SELECT COUNT(*) FROM real_estate_regions) + (SELECT COUNT(*) FROM real_estate_costs) + (SELECT COUNT(*) FROM real_estate_zone_templates)", '0')
subprocess.run(['mariadb', '-uroot', '-e', 'DROP DATABASE catalogTest'], check=True)
print('all catalog checks passed')
