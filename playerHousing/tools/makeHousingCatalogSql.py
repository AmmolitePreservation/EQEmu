"""Writes sql/housingCatalog.sql: the housing catalog (serverPlan.md, the content sections).

Ids and costs are Live's, read from a livedumps clone's master/ (mqperidotlive's tools/liveResearch.py over MQ2PeridotLive's captures). Sunrise Hills
is definition 2 (Housing:RandomNeighborhoodDefinitions' default), its children Live's 71 player plots (101-173) and 4 guild
plots (187-190), each matched to its plot in content/sunriseHillsPlots.json by its switch and checked against the
template's price. Every definition keeps Live's area id, cost and upkeep ids, child limit, item caps, NPC item cap, switch,
address, icon, zone id and dz id; the plot regions are the template's boxes. Every definition an item names comes from
content/itemDefinitions.txt (tools/exportItemDefinitions.sql). Costs are Live's, copied for every id a definition names or
the plan's item upkeep table lists (the ids Peridot's placeable items carry, its amounts checked against Live's): each
option a way to pay, coin always, and each alternate currency gated by the expansion it arrived in.

Names are composed when sent, so a house, hall or crate carries Live's segment for it, checked against every property name
Live sent for its definition. Each dz id gets a zone template: the zone its definition's interior is (the Standard Guild
Hall's guildhall at version 2), entered at Live's arrival point. Each plot gets an exit point: Live's where it was measured
(101 Market Street's house, 200 Terminus Heights' hall), otherwise its neighborhood's, Live's neighborhood safe point.

Refuses to write when a plot has no single matching template plot, a price or kind disagrees, an item's definition can't be
placed, a named cost or definition is missing from Live's catalog, a currency has no known expansion, two plot footprints
overlap, a segment disagrees with Live's names, a dz id names two zones or a zone has no arrival point, or an id collides.
The SQL refuses non-empty catalog tables and loads in one transaction.
Usage: python makeHousingCatalogSql.py --live <livedumps clone>
"""
import argparse, collections, csv, json, math, os, re

here = os.path.dirname(os.path.abspath(__file__))
content = os.path.join(here, '..', 'content')

liveMaster = None
neighborhoodDefinitionId = 2
neighborhoodZoneId = 712
# Live's yard regions are boxes centered on the plot's ground, reaching this far above and below it: the client's containment test
# (eqgame 2026-09-17, 0x14066ef20) takes |local| <= 1 on each axis of a region's matrix, whose vertical axis reads 350 or 450.
plotRegionHalfHeight = 350.0
guildPlotRegionHalfHeight = 450.0
switchMatchUnits = 6.0
# Live's alternate currency ids (eqlib's ALTCURRENCY) and the EQEmu expansion (Expansion::ExpansionNumber) each arrived in.
currencyExpansions = {10: 13, 11: 13, 12: 13, 13: 13, 14: 14, 15: 15}

threeRoomStyles, oneRoomStyles = {'Monel', 'Zimel'}, {'Behroe', 'Cobbler'}
threeRoomTiers, oneRoomTiers = {'Hut', 'House', 'Residence', 'Domicile', 'Manor'}, {'Hut', 'House', 'Residence', 'Domicile'}
threeRoomZones = {'Stucco': 714, 'Wood': 716, 'Stone': 717}
oneRoomZones = {'Stucco': 715, 'Wood': 718, 'Stone': 719}
guildHallZones = {'Standard Guild Hall': 345, 'Grand Guild Hall': 738, 'Palatial Guild Hall': 737, 'Modest Guild Hall': 751}
storageCrates = {'Small', 'Hut', 'All Hallows Eve', 'Medium', 'Domicile', 'Large', 'House', 'Giant', 'Mansion', 'Enormous', 'Estate'}
typeGuildHall, typePlayerHouse, typePlot, typeNeighborhood, typeMovingCrate, typeGuildPlot = 2, 3, 4, 5, 7, 8
placeableInPlots, placeableInGuildPlots = 0x08, 0x80
# Live's arrival points (x, y, z, heading), EQEmu's x and y (INT-2, INT-3): the house interiors' and the Grand and Modest halls'
# are their zone headers' safe points, the houses facing 0; the Palatial's and the Standard's were measured on entering; the
# neighborhood's is its header's safe point.
zoneVersions = {345: 2}
arrivalPoints = {
  712: (2035.0, -2940.0, 6.0, 0.0),
  714: (0.0, -12.0, 5.0, 0.0), 715: (0.0, -12.0, 5.0, 0.0), 716: (0.0, -12.0, 5.0, 0.0),
  717: (0.0, -12.0, 5.0, 0.0), 718: (0.0, -12.0, 5.0, 0.0), 719: (0.0, -12.0, 5.0, 0.0),
  345: (-1.0, -1.0, 3.4, 0.0), 737: (71.0, -1.0, 1.56, 386.0), 738: (255.0, 2.0, -10.0, 386.0), 751: (255.0, 2.0, -10.0, 386.0),
}
# Where Live's front doors put a player (x, y, z, heading), by plot definition (INT-11).
measuredExits = {171: (1931.75, -2461.25, 2.94661, 2.25), 188: (-1444.37, 2256.5, 112.95, 511.25)}


def readItemDefinitions():
  groups = []
  with open(os.path.join(content, 'itemDefinitions.txt')) as source:
    for row in csv.reader(source, delimiter='\t'):
      if row and row[0] == 'items':
        groups.append(dict(definition=int(row[1]), upkeep=int(row[2]), placeable=int(row[3]), names=row[6].split(' | ')))
  return groups


def readPlanItemUpkeeps():
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


def readLiveCatalog():
  definitions = {int(row['id']): row for row in csv.DictReader(open(os.path.join(liveMaster, 'definitions.tsv'), encoding='latin-1'), delimiter='\t')}
  costs = collections.defaultdict(dict)
  for row in csv.DictReader(open(os.path.join(liveMaster, 'costs.tsv'), encoding='latin-1'), delimiter='\t'):
    parts = [tuple(int(value) for value in part.split(':')) for part in row['subComponents'].split(',')]
    costs[int(row['costId'])][int(row['componentId'])] = parts
  return definitions, costs


def readLiveSegments():
  segments = collections.defaultdict(set)
  for row in csv.DictReader(open(os.path.join(liveMaster, 'properties.tsv'), encoding='latin-1'), delimiter='\t'):
    if ', ' in row['dynamicName']:
      segments[int(row['definitionId'])].add(row['dynamicName'].rsplit(', ', 1)[1])
  return segments


def liveSwitch(live):
  return (float(live['switchY']), float(live['switchX']), float(live['switchZ']), float(live['switchHeading']))


def matchLivePlots(livePlots, plots):
  matches = {}
  for liveId, live in livePlots.items():
    x, y = liveSwitch(live)[:2]
    distance, plot = min((math.hypot(plot['center'][0] - x, plot['center'][1] - y), index) for index, plot in enumerate(plots))
    if distance > switchMatchUnits:
      raise ValueError(f'Live plot {liveId}: no template plot within {switchMatchUnits} units of its switch ({distance:.1f})')
    if (live['type'] == str(typeGuildPlot)) != (plots[plot]['kind'] == 'guild plot'):
      raise ValueError(f"Live plot {liveId} and template plot {plots[plot]['address']} disagree on plot or guild plot")
    matches[liveId] = plots[plot]
  if len({id(plot) for plot in matches.values()}) != len(plots):
    raise ValueError('Live plots and template plots do not match one to one')
  return matches


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
    tiers, zones = (threeRoomTiers, threeRoomZones) if threeRoom else (oneRoomTiers, oneRoomZones)
    if tier not in tiers:
      raise ValueError(f"definition {group['definition']}: no {tier} in its size: {names}")
    name = f"{'Manor' if tier == 'Manor' else ('Three' if threeRoom else 'One') + '-room ' + tier}, {finish} interior"
    return dict(name=name, segment='Three Room House' if threeRoom else 'One Room House', type=typePlayerHouse, zone=zones[finish])
  if len(names) != 1:
    raise ValueError(f"definition {group['definition']}: unrecognized items {names}")
  name = names[0]
  if name in guildHallZones:
    if not group['placeable'] & placeableInGuildPlots:
      raise ValueError(f'{name}: not placeable in guild yards')
    return dict(name=name, segment=name, type=typeGuildHall, zone=guildHallZones[name])
  crate = re.fullmatch(r'(.+) Storage Crate', name)
  if (crate and crate.group(1) in storageCrates) or name == 'Crate of Eviction':
    return dict(name=name, segment=name, type=typeMovingCrate, zone=0)
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


def liveFields(live, **fields):
  return dict(area=int(live['areaId']), purchase=int(live['costDefinition']), upkeep=int(live['upkeepDefinition']), children=int(live['maxChildren']),
              maxItems=int(live['maxItems']), vault=int(live['maxNonRealEstateItems']), npcItems=int(live['maxNpcItems']), icon=int(live['icon']),
              liveZone=int(live['zoneId']), dz=int(live['dzId']), address=live['address'], switch=liveSwitch(live), **fields)


def main():
  global liveMaster
  parser = argparse.ArgumentParser(description='Write sql/housingCatalog.sql.')
  parser.add_argument('--live', required=True, help='a livedumps clone')
  liveMaster = os.path.join(parser.parse_args().live, 'master')
  if not os.path.isdir(liveMaster):
    raise SystemExit(f'{liveMaster} does not exist: --live must be a livedumps clone')
  template = json.load(open(os.path.join(content, 'sunriseHillsPlots.json')))
  words = json.load(open(os.path.join(content, 'neighborhoodWords.json')))
  groups = readItemDefinitions()
  liveDefinitions, liveCosts = readLiveCatalog()
  liveSegments = readLiveSegments()

  coin = lambda costId: next(amount for currency, _, amount in liveCosts[costId][1] if currency == 0)
  livePlots = {i: d for i, d in liveDefinitions.items() if d['type'] in (str(typePlot), str(typeGuildPlot))}
  matches = matchLivePlots(livePlots, template)
  neighborhood = liveDefinitions[neighborhoodDefinitionId]
  children = [int(child) for child in neighborhood['childDefinitions'].split(',')]
  if neighborhood['type'] != str(typeNeighborhood) or sorted(children) != sorted(livePlots):
    raise ValueError(f'Live definition {neighborhoodDefinitionId} is not the neighborhood of its plots')
  unknownExits = set(measuredExits) - set(children)
  if unknownExits:
    raise ValueError(f'measured exits for definitions that are not plots of the neighborhood: {sorted(unknownExits)}')
  noExit = (0, 0, 0.0, 0.0, 0.0, 0.0)
  exitIn = lambda point: (neighborhoodZoneId, zoneVersions.get(neighborhoodZoneId, 0), *point)

  definitions, regions = [], []
  definitions.append(liveFields(neighborhood, id=neighborhoodDefinitionId, name='Sunrise Hills', segment='', type=typeNeighborhood,
                                zone=neighborhoodZoneId, exit=noExit))
  for liveId in children:
    live, plot = livePlots[liveId], matches[liveId]
    guild = plot['kind'] == 'guild plot'
    if coin(int(live['costDefinition'])) != plot['pricePlatinum'] * 1000:
      raise ValueError(f"Live plot {liveId} ({plot['address']}): Live price {coin(int(live['costDefinition']))} copper, template {plot['pricePlatinum']}pp")
    if live['address'] != plot['address']:
      raise ValueError(f"Live plot {liveId}: Live's address {live['address']!r}, the template's {plot['address']!r}")
    regions.append(dict(id=int(live['areaId']), name=plot['address'], x=plot['center'][0], y=plot['center'][1],
                        z=plot['center'][2], sizeX=plot['sizeAcross'], sizeY=plot['sizeAlong'],
                        sizeZ=2 * (guildPlotRegionHalfHeight if guild else plotRegionHalfHeight), heading=plot['heading']))
    exit = exitIn(measuredExits.get(liveId, arrivalPoints[neighborhoodZoneId]))
    definitions.append(liveFields(live, id=liveId, name=plot['address'], segment='', type=typeGuildPlot if guild else typePlot, zone=0, exit=exit))
  itemUpkeeps = readPlanItemUpkeeps()
  for costId, amount in sorted(itemUpkeeps.items()):
    if costId not in liveCosts or coin(costId) != amount:
      raise ValueError(f"item upkeep {costId}: the plan's {amount} copper is not Live's {coin(costId) if costId in liveCosts else 'missing cost'}")
  for group in sorted(groups, key=lambda g: g['definition']):
    entry = itemDefinition(group)
    live = liveDefinitions.get(group['definition'])
    if live is None:
      raise ValueError(f"definition {group['definition']} ({entry['name']}) is not in Live's catalog")
    if group['upkeep'] and group['upkeep'] not in itemUpkeeps:
      raise ValueError(f"definition {group['definition']}: upkeep id {group['upkeep']} is not in the plan's item upkeep table")
    definitions.append(liveFields(live, id=group['definition'], exit=noExit, **entry))
  if len({d['id'] for d in definitions}) != len(definitions):
    raise ValueError('duplicate definition ids')
  for d in definitions:
    expected = d['address'] if d['type'] in (typePlot, typeGuildPlot) else d['segment']
    if liveSegments[d['id']] - {expected}:
      raise ValueError(f"definition {d['id']}: Live names its properties ...{sorted(liveSegments[d['id']])}, not ...{expected!r}")
  corners = [footprint(region) for region in regions]
  for i in range(len(regions)):
    for j in range(i + 1, len(regions)):
      if overlapDepth(corners[i], corners[j]) > 0.01:
        raise ValueError(f"plot footprints overlap: {regions[i]['name']} and {regions[j]['name']}")

  templateZones = {}
  for d in definitions:
    if d['dz'] and templateZones.setdefault(d['dz'], d['zone']) != d['zone']:
      raise ValueError(f"dz id {d['dz']} names zones {templateZones[d['dz']]} and {d['zone']}")
  unplaced = sorted(dz for dz, zone in templateZones.items() if zone not in arrivalPoints)
  if unplaced:
    raise ValueError(f'zone templates without a zone or an arrival point: {unplaced}')
  templates = [(dz, zone, zoneVersions.get(zone, 0), *arrivalPoints[zone]) for dz, zone in sorted(templateZones.items())]

  costNames = {}
  for d in definitions:
    if d['purchase']:
      costNames.setdefault(d['purchase'], f"Plot purchase, {coin(d['purchase']) / 1000:g}pp")
    if d['upkeep']:
      kind = 'House' if d['type'] == typePlayerHouse else 'Plot'
      costNames.setdefault(d['upkeep'], f"{kind} upkeep, {coin(d['upkeep']) / 1000:g}pp a day")
  for costId in itemUpkeeps:
    costNames.setdefault(costId, f'Item upkeep, {coin(costId)} copper a day')
  missing = sorted(set(costNames) - set(liveCosts))
  if missing:
    raise ValueError(f"costs named but not in Live's catalog: {missing}")
  optionRows, itemRows = [], []
  for costId in sorted(costNames):
    for componentId, parts in sorted(liveCosts[costId].items()):
      currencies = {currency for currency, _, _ in parts}
      if currencies - {0} - set(currencyExpansions):
        raise ValueError(f'cost {costId}: currency {sorted(currencies - {0} - set(currencyExpansions))} has no known expansion')
      minimum = max([currencyExpansions[c] for c in currencies if c] or [-1])
      optionRows.append((str(costId), str(componentId - 1), str(minimum)))
      itemRows.extend((str(costId), str(componentId - 1), str(c), str(sub), str(amount)) for c, sub, amount in parts)
  wordRows = [(0, w) for w in words['trees']] + [(1, w) for w in words['middles']] + [(2, w) for w in words['places']]

  quoted = lambda text: "'" + text.replace("'", "''") + "'"
  number = lambda value: repr(round(float(value), 3))
  values = lambda rows: ',\n'.join('    (' + ', '.join(row) + ')' for row in rows) + ';\n'
  definitionRows = [(str(d['id']), quoted(d['name']), quoted(d['segment']), str(d['type']), '0', str(d['area']), str(d['purchase']), str(d['upkeep']),
                     str(d['children']), str(d['maxItems']), str(d['vault']), str(d['npcItems']), str(d['icon']), str(d['liveZone']), str(d['dz']),
                     quoted(d['address']), *[number(v) for v in d['switch']], str(d['exit'][0]), str(d['exit'][1]), *[number(v) for v in d['exit'][2:]])
                    for d in sorted(definitions, key=lambda d: d['id'])]
  catalogTables = ['real_estate_definitions', 'real_estate_definition_children', 'real_estate_costs', 'real_estate_cost_options',
                   'real_estate_cost_items', 'real_estate_regions', 'real_estate_zone_templates', 'real_estate_neighborhood_words']
  sql = ('-- Generated by playerHousing/tools/makeHousingCatalogSql.py.\nDELIMITER //\nBEGIN NOT ATOMIC\n'
         '  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;\n'
         f"  IF {' OR '.join(f'EXISTS (SELECT 1 FROM {t})' for t in catalogTables)} THEN\n"
         "    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the housing catalog tables are not empty';\n  END IF;\n  START TRANSACTION;\n"
         '  INSERT INTO real_estate_definitions (id, name, name_segment, type, placement_group_id, area_id, cost_definition_id, upkeep_definition_id,\n'
         '    max_children, max_items, max_non_real_estate_items, max_npc_items, icon, zone_id, dz_id, address, switch_x, switch_y, switch_z,\n'
         '    switch_heading, exit_zone_id, exit_zone_version, exit_x, exit_y, exit_z, exit_heading) VALUES\n'
         + values(definitionRows)
         + '  INSERT INTO real_estate_definition_children (parent_id, child_id) VALUES\n' + values([(str(neighborhoodDefinitionId), str(i)) for i in children])
         + '  INSERT INTO real_estate_costs (id, name) VALUES\n' + values([(str(c), quoted(costNames[c])) for c in sorted(costNames)])
         + '  INSERT INTO real_estate_cost_options (cost_id, option_key, min_expansion) VALUES\n' + values(optionRows)
         + '  INSERT INTO real_estate_cost_items (cost_id, option_key, currency, sub_key, amount) VALUES\n' + values(itemRows)
         + '  INSERT INTO real_estate_regions (id, name, x, y, z, size_x, size_y, size_z, heading) VALUES\n'
         + values([(str(r['id']), quoted(r['name']), number(r['x']), number(r['y']), number(r['z']), number(r['sizeX']), number(r['sizeY']),
                    number(r['sizeZ']), number(r['heading'])) for r in regions])
         + '  INSERT INTO real_estate_zone_templates (dz_id, zone_id, zone_version, zone_in_x, zone_in_y, zone_in_z, zone_in_heading) VALUES\n'
         + values([(str(dz), str(zone), str(version), *[number(v) for v in point]) for dz, zone, version, *point in templates])
         + '  INSERT INTO real_estate_neighborhood_words (position, word) VALUES\n' + values([(str(p), quoted(w)) for p, w in wordRows])
         + '  COMMIT;\nEND //\nDELIMITER ;\n'
         + 'SELECT ' + ', '.join(f"(SELECT COUNT(*) FROM {t}) AS {t.replace('real_estate_', '')}" for t in catalogTables) + ';\n')
  assert all(ord(ch) < 128 for ch in sql)
  open(os.path.join(here, '..', 'sql', 'housingCatalog.sql'), 'w').write(sql)
  print(f'wrote sql/housingCatalog.sql: {len(definitions)} definitions ({len(children)} plots, {len(groups)} named by items), '
        f'{len(costNames)} costs with {len(optionRows)} options, {len(regions)} regions, {len(templates)} zone templates, {len(wordRows)} words')


if __name__ == '__main__':
  main()
