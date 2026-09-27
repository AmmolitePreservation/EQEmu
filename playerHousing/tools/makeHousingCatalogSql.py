"""Writes sql/housingCatalog.sql: the housing catalog (serverPlan.md, the content sections).

The Sunrise Hills neighborhood is definition 1 (Housing:RandomNeighborhoodDefinitions' default), its children the 71 player
plots and 4 guild plots of content/sunriseHillsPlots.json as definitions 42-116, each with its price in copper, its region
(same id) and its border stone's position and heading as layout origin. Every definition an item names comes from content/itemDefinitions.txt
(tools/exportItemDefinitions.sql): houses and manors by their names' style, tier and finish, the guild halls, the storage
crates and the Crate of Eviction. New ids stay under 174, inside the range items prove the client accepts. The only stored
costs are the item upkeep costs of the plan's table, each its coin option: the zone derives each plot's purchase and upkeep
costs from its price (upkeep: Housing:PlotUpkeepPercent) and every currency option. A pet cap of 0 is no separate pet cap.

Refuses to write when an item's definition can't be placed, an item upkeep id has no amount, two plot footprints overlap
or an id collides. The SQL refuses non-empty catalog tables and loads in one transaction.
Usage: python3 makeHousingCatalogSql.py
"""
import collections, csv, json, math, os, re

here = os.path.dirname(os.path.abspath(__file__))
content = os.path.join(here, '..', 'content')

neighborhoodDefinitionId = 1
neighborhoodZoneId = 712
firstPlotDefinitionId = 42
itemProvenIdLimit = 174
regionBelowGround = 100.0
regionAboveGround = 900.0
evictionCrateCapacity = 3000

threeRoomStyles, oneRoomStyles = {'Monel', 'Zimel'}, {'Behroe', 'Cobbler'}
threeRoomCaps = {'Hut': (300, 0), 'House': (300, 40), 'Residence': (300, 80), 'Domicile': (300, 120), 'Manor': (500, 300)}
oneRoomCaps = {'Hut': (100, 0), 'House': (100, 20), 'Residence': (100, 40), 'Domicile': (100, 60)}
threeRoomZones = {'Stucco': 714, 'Wood': 716, 'Stone': 717}
oneRoomZones = {'Stucco': 715, 'Wood': 718, 'Stone': 719}
guildHalls = {'Standard Guild Hall': (345, 1000), 'Grand Guild Hall': (738, 1500), 'Palatial Guild Hall': (737, 2000), 'Modest Guild Hall': (751, 1500)}
storageCrates = {'Small': 50, 'Hut': 100, 'All Hallows Eve': 100, 'Medium': 150, 'Domicile': 200, 'Large': 250, 'House': 300,
                 'Giant': 350, 'Mansion': 400, 'Enormous': 450, 'Estate': 500}
typeGuildHall, typePlayerHouse, typePlot, typeNeighborhood, typeMovingCrate, typeGuildPlot = 2, 3, 4, 5, 7, 8
placeableInPlots, placeableInGuildPlots = 0x08, 0x80


def readItemDefinitions():
  groups = []
  with open(os.path.join(content, 'itemDefinitions.txt')) as source:
    for row in csv.reader(source, delimiter='\t'):
      if row and row[0] == 'items':
        groups.append(dict(definition=int(row[1]), upkeep=int(row[2]), placeable=int(row[3]), names=row[6].split(' | ')))
  return groups


def readItemUpkeeps():
  plan = open(os.path.join(here, '..', 'serverPlan.md'), encoding='utf-8').read().splitlines()
  start = plan.index('| Upkeep id | Kind | Daily upkeep |') + 2
  upkeeps = {}
  for line in plan[start:]:
    if not line.startswith('|'):
      break
    ids, _, amounts = [cell.strip() for cell in line.strip('|').split('|')]
    ids = [int(value) for value in re.findall(r'\d+', ids)]
    amounts = [int(value.replace(',', '')) for value in re.findall(r'\d[\d,]*', amounts.split('(')[0])]
    if len(ids) != len(amounts):
      raise ValueError(f'upkeep table row: {line}')
    upkeeps.update(zip(ids, amounts))
  return upkeeps


def itemDefinition(group):
  names = group['names']
  houses = [re.fullmatch(r'(\w+) Style \w+ (House|Hut|Residence|Domicile|Manor), (Stucco|Wood|Stone) interior', name) for name in names]
  if all(houses):
    kinds = {(m.group(1) in threeRoomStyles, m.group(1) in oneRoomStyles, m.group(2), m.group(3)) for m in houses}
    if len(kinds) != 1:
      raise ValueError(f"definition {group['definition']}: its items disagree on size, tier or finish: {names}")
    threeRoom, oneRoom, tier, finish = kinds.pop()
    if threeRoom == oneRoom or not group['placeable'] & placeableInPlots:
      raise ValueError(f"definition {group['definition']}: not a one- or three-room house placeable in yards: {names}")
    caps, zones = (threeRoomCaps, threeRoomZones) if threeRoom else (oneRoomCaps, oneRoomZones)
    if tier not in caps:
      raise ValueError(f"definition {group['definition']}: no {tier} in its size: {names}")
    maxItems, vault = caps[tier]
    name = f"{'Manor' if tier == 'Manor' else ('Three' if threeRoom else 'One') + '-room ' + tier}, {finish} interior"
    return dict(name=name, type=typePlayerHouse, maxItems=maxItems, vault=vault, pets=0, zone=zones[finish])
  if len(names) != 1:
    raise ValueError(f"definition {group['definition']}: unrecognized items {names}")
  name = names[0]
  if name in guildHalls:
    if not group['placeable'] & placeableInGuildPlots:
      raise ValueError(f'{name}: not placeable in guild yards')
    zone, maxItems = guildHalls[name]
    return dict(name=name, type=typeGuildHall, maxItems=maxItems, vault=0, pets=0, zone=zone)
  crate = re.fullmatch(r'(.+) Storage Crate', name)
  if crate and crate.group(1) in storageCrates:
    return dict(name=name, type=typeMovingCrate, maxItems=storageCrates[crate.group(1)], vault=0, pets=0, zone=0)
  if name == 'Crate of Eviction':
    return dict(name=name, type=typeMovingCrate, maxItems=evictionCrateCapacity, vault=evictionCrateCapacity, pets=evictionCrateCapacity, zone=0)
  raise ValueError(f"definition {group['definition']}: unrecognized item {name}")


def footprint(region):
  angle = region['heading'] * 2 * math.pi / 512
  along, across = (math.sin(angle), math.cos(angle)), (math.cos(angle), -math.sin(angle))
  return [(region['x'] + a * along[0] * region['sizeY'] / 2 + b * across[0] * region['sizeX'] / 2,
           region['y'] + a * along[1] * region['sizeY'] / 2 + b * across[1] * region['sizeX'] / 2) for a, b in ((1, 1), (1, -1), (-1, -1), (-1, 1))]


def overlapDepth(first, second):
  depth = math.inf
  for corners in (first, second):
    for i in range(2):
      edge = (corners[i + 1][0] - corners[i][0], corners[i + 1][1] - corners[i][1])
      axis = (-edge[1] / math.hypot(*edge), edge[0] / math.hypot(*edge))
      a = [p[0] * axis[0] + p[1] * axis[1] for p in first]
      b = [p[0] * axis[0] + p[1] * axis[1] for p in second]
      depth = min(depth, min(max(a), max(b)) - max(min(a), min(b)))
  return depth


def main():
  template = json.load(open(os.path.join(content, 'sunriseHillsPlots.json')))
  stones = {}
  with open(os.path.join(content, 'sunriseHillsLotSquares.tsv')) as source:
    for row in csv.reader(source, delimiter='\t'):
      stones[int(row[1])] = (float(row[2]), float(row[3]), float(row[4]), float(row[5]))
  words = json.load(open(os.path.join(content, 'neighborhoodWords.json')))
  groups = readItemDefinitions()
  itemUpkeeps = readItemUpkeeps()

  plots = [plot for plot in template if plot['kind'] == 'plot'] + [plot for plot in template if plot['kind'] == 'guild plot']
  if len(plots) != len(template):
    raise ValueError('the template holds a kind other than plot and guild plot')
  itemDefinitionIds = {group['definition'] for group in groups}
  plotIds = list(range(firstPlotDefinitionId, firstPlotDefinitionId + len(plots)))
  if plotIds[-1] >= itemProvenIdLimit or itemDefinitionIds & set(plotIds) or neighborhoodDefinitionId in itemDefinitionIds | set(plotIds):
    raise ValueError('plot definition ids collide with item definitions or leave the proven range')
  for group in groups:
    if group['upkeep'] and group['upkeep'] not in itemUpkeeps:
      raise ValueError(f"definition {group['definition']}: upkeep id {group['upkeep']} has no amount in the plan's table")

  definitions, regions = [], []
  definitions.append(dict(id=neighborhoodDefinitionId, name='Sunrise Hills', type=typeNeighborhood, region=0, price=0,
                          maxItems=0, vault=0, pets=0, origin=(0.0, 0.0, 0.0, 0.0), zone=neighborhoodZoneId))
  for definitionId, plot in zip(plotIds, plots):
    guild = plot['kind'] == 'guild plot'
    ground = plot['center'][2]
    regions.append(dict(id=definitionId, name=plot['address'], x=plot['center'][0], y=plot['center'][1],
                        z=ground + (regionAboveGround - regionBelowGround) / 2, sizeX=plot['sizeAcross'], sizeY=plot['sizeAlong'],
                        sizeZ=regionBelowGround + regionAboveGround, heading=plot['heading']))
    origin = (*plot['doorPosition'], plot['heading']) if guild else stones[plot['door']]
    definitions.append(dict(id=definitionId, name=plot['address'], type=typeGuildPlot if guild else typePlot, region=definitionId,
                            price=plot['pricePlatinum'] * 1000,
                            maxItems=plot['capacity'], vault=plot.get('vault', 0), pets=plot['pets'], origin=origin, zone=0))
  for group in sorted(groups, key=lambda g: g['definition']):
    entry = itemDefinition(group)
    definitions.append(dict(id=group['definition'], name=entry['name'], type=entry['type'], region=0, price=0,
                            maxItems=entry['maxItems'], vault=entry['vault'], pets=entry['pets'], origin=(0.0, 0.0, 0.0, 0.0), zone=entry['zone']))
  if len({d['id'] for d in definitions}) != len(definitions):
    raise ValueError('duplicate definition ids')
  corners = [footprint(region) for region in regions]
  for i in range(len(regions)):
    for j in range(i + 1, len(regions)):
      if overlapDepth(corners[i], corners[j]) > 0.01:
        raise ValueError(f"plot footprints overlap: {regions[i]['name']} and {regions[j]['name']}")

  costs = [(costId, f'Item upkeep, {amount} copper a day', amount) for costId, amount in sorted(itemUpkeeps.items())]
  wordRows = [(0, w) for w in words['trees']] + [(1, w) for w in words['middles']] + [(2, w) for w in words['places']]

  quoted = lambda text: "'" + text.replace("'", "''") + "'"
  number = lambda value: repr(round(float(value), 3))
  values = lambda rows: ',\n'.join('    (' + ', '.join(row) + ')' for row in rows) + ';\n'
  definitionRows = [(str(d['id']), quoted(d['name']), str(d['type']), '0', str(d['region']), str(d['price']),
                     str(d['maxItems']), str(d['vault']), str(d['pets']), *[number(v) for v in d['origin']], str(d['zone']), '0')
                    for d in sorted(definitions, key=lambda d: d['id'])]
  catalogTables = ['real_estate_definitions', 'real_estate_definition_children', 'real_estate_costs', 'real_estate_cost_options',
                   'real_estate_cost_items', 'real_estate_regions', 'real_estate_neighborhood_words']
  sql = ('-- Generated by playerHousing/tools/makeHousingCatalogSql.py.\nDELIMITER //\nBEGIN NOT ATOMIC\n'
         '  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;\n'
         f"  IF {' OR '.join(f'EXISTS (SELECT 1 FROM {t})' for t in catalogTables)} THEN\n"
         "    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the housing catalog tables are not empty';\n  END IF;\n  START TRANSACTION;\n"
         '  INSERT INTO real_estate_definitions (id, name, type, category, region_id, price, max_items,\n'
         '    max_non_real_estate_items, max_pet_items, layout_origin_x, layout_origin_y, layout_origin_z, layout_heading, zone_id, zone_version) VALUES\n'
         + values(definitionRows)
         + '  INSERT INTO real_estate_definition_children (parent_id, child_id) VALUES\n' + values([(str(neighborhoodDefinitionId), str(i)) for i in plotIds])
         + '  INSERT INTO real_estate_costs (id, name) VALUES\n' + values([(str(c), quoted(n)) for c, n, _ in sorted(costs)])
         + '  INSERT INTO real_estate_cost_options (cost_id, option_key) VALUES\n' + values([(str(c), '0') for c, _, _ in sorted(costs)])
         + '  INSERT INTO real_estate_cost_items (cost_id, option_key, currency, sub_key, amount) VALUES\n'
         + values([(str(c), '0', '0', '0', str(a)) for c, _, a in sorted(costs)])
         + '  INSERT INTO real_estate_regions (id, name, x, y, z, size_x, size_y, size_z, heading) VALUES\n'
         + values([(str(r['id']), quoted(r['name']), number(r['x']), number(r['y']), number(r['z']), number(r['sizeX']), number(r['sizeY']),
                    number(r['sizeZ']), number(r['heading'])) for r in regions])
         + '  INSERT INTO real_estate_neighborhood_words (position, word) VALUES\n' + values([(str(p), quoted(w)) for p, w in wordRows])
         + '  COMMIT;\nEND //\nDELIMITER ;\n'
         + 'SELECT ' + ', '.join(f"(SELECT COUNT(*) FROM {t}) AS {t.replace('real_estate_', '')}" for t in catalogTables) + ';\n')
  assert all(ord(ch) < 128 for ch in sql)
  open(os.path.join(here, '..', 'sql', 'housingCatalog.sql'), 'w').write(sql)
  print(f'wrote sql/housingCatalog.sql: {len(definitions)} definitions ({len(plots)} plots, {len(groups)} named by items), '
        f'{len(costs)} costs, {len(regions)} regions, {len(wordRows)} words')


if __name__ == '__main__':
  main()
