"""Writes sql/housingDoors.sql: the housing interiors' and halls' front doors, the Palatial hall's interior doors, the Guild Lobby gate's open type, and every housing door's RealEstateDoorID (doors.real_estate_door_id, DOR-19).

Doors are Live's (a livedumps clone's master/doors.tsv for name, type, position, heading and size; master/fields/doors.tsv for
state, remain-open and RealEstateDoorID). Each Live zone maps to its Peridot zone through makeHousingZonesSql.py's table
(guildhall_int is guildhall at version 2); the zones RoF2 lacks (dragonhouseinta, plhbixieint, plhpirateshipint, Q12) are
left out. Every column a door row does not set is copied from one of PEQ's neighborhood plot borders (OBP_LOTSQUARE, open
type 160, version 0), as makeGuildBorderDoorsSql.py does, since PEQ has no rows in these zones; each row sets zone, version,
door id, name, position, heading, size and open type, and is closed and uninverted (doorisopen 0, invert_state 0).

Every housing door carries Live's RealEstateDoorID (DOR-3, DOR-9, DOR-19), checked against its rule: a lot square's
door id plus 100, a guild square's plot (226-229 -> 187-190), 1 for the neighborhood list doors (the void portal and the
Guild Lobby gate) and 0 for a front door. The Guild Lobby is zone 344 (common/eq_constants.h, Zones::GUILDLOBBY); its gate
is door 38 of version 1, Live's lobby (guildLobby.sql, DOR-17). Each lot square is checked against PEQ's
(content/sunriseHillsLotSquares.tsv: same door id, within 1 unit of Live's); PEQ's void portal moves to Live's position
(DOR-15). The SQL checks the borders, the portal and the gate in the loaded doors table (so guildLobby.sql and
guildBorderDoors.sql load first).

The door rules hold (DOR-10) for every zone version written: ids 0-255 and at most 500 doors, names longer than 3 characters
and upper case, state 0 on types 159-161 and nothing remaining open. The SQL refuses doors that already carry a RealEstateDoorID or
existing rows at the doors it writes, and loads in one transaction.

Usage: python makeHousingDoorsSql.py --live <livedumps clone>
"""
import argparse, collections, csv, math, os, re, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from eqemuColumns import columnsOf
from makeHousingZonesSql import housingZones
from makeHousingCatalogSql import neighborhoodZoneId

frontDoorModels = {
  'phinterior3a1': 'OBJ_DOOR_PLHA', 'phinterior3a2': 'OBJ_DOOR_PLHA', 'phinterior3a3': 'OBJ_DOOR_PLHA',
  'phinterior1a1': 'OBJ_DOOR_PLHA', 'phinterior1a2': 'OBJ_DOOR_PLHA', 'phinterior1a3': 'OBJ_DOOR_PLHA',
  'phinterior1d1': 'OBJ_PH1D1_DOOR', 'phinteriortree': 'OBJ_EVANTIL_PLYRHOUSE_DOOR',
  'guildhall3_int': 'GUILDHALL_DOOR_SWITCH', 'guildhallsml_int': 'OBJ_GUILDHALL_SML_DOOR',
  'guildhalllrg_int': 'OBJ_FRONTENTRANCE', 'guildhall_int': 'OBJ_GUILD_DOOR',
}
zonesWithInteriorDoors = {'guildhalllrg_int'}
zonesRoF2Lacks = {'dragonhouseinta', 'plhbixieint', 'plhpirateshipint'}
typeNeighborhoodList, typePlotBorder, typeFrontDoor = 159, 160, 161
lotSquareModel, guildSquareModel, portalModel, gateModel = 'OBP_LOTSQUARE', 'OBP_GUILDSQUARE', 'OBJ_VOIDPORTAL', 'OBJ_IRONGATESWITCH'
guildSquarePlots = {226: 187, 227: 188, 228: 189, 229: 190}
portalDoorID, gateDoorID = 130, 38
# The gate is in the Guild Lobby's version 1, Live's lobby (guildLobby.sql, DOR-17).
neighborhoodVersion, lobbyVersion = 0, 1
lotSquareMatchUnits = 1.0
maxDoorID, maxDoors = 255, 500
decorationPositionUnits, decorationHeadingUnits = 0.05, 0.5


def doorsColumns():
  columns = columnsOf('doors')
  if 'real_estate_door_id' not in columns:
    raise ValueError(f'unexpected doors columns: {columns}')
  return columns


def guildLobbyZoneID():
  text = open(os.path.join(here, '..', '..', 'common', 'eq_constants.h'), encoding='utf-8').read()
  match = re.search(r'constexpr uint16 GUILDLOBBY = (\d+);', text)
  if match is None:
    raise ValueError('common/eq_constants.h has no Zones::GUILDLOBBY')
  return int(match.group(1))


def readLiveDoors(live):
  doors = {}
  for row in csv.DictReader(open(os.path.join(live, 'master', 'doors.tsv'), encoding='latin-1'), delimiter='\t'):
    key = (row['zone'], int(row['id']))
    if key in doors:
      raise ValueError(f"Live's doors.tsv repeats {key}")
    doors[key] = dict(zone=row['zone'], id=key[1], name=row['name'], type=int(row['type']), x=float(row['x']), y=float(row['y']),
                      z=float(row['z']), heading=float(row['heading']), size=row['scaleFactor'], fields=None)
  for row in csv.DictReader(open(os.path.join(live, 'master', 'fields', 'doors.tsv'), encoding='latin-1'), delimiter='\t'):
    key = (row['zone'], int(row['id']))
    if key not in doors or doors[key]['fields'] is not None:
      raise ValueError(f"Live's fields/doors.tsv: {key} is missing from doors.tsv or repeats")
    if row['Name'] != doors[key]['name'] or int(row['Type']) != doors[key]['type']:
      raise ValueError(f"Live door {key}: doors.tsv and fields/doors.tsv disagree on name or type")
    doors[key]['fields'] = dict(state=int(row['State']), defaultState=int(row['DefaultState']), remainOpen=int(row['bRemainOpen']),
                                realEstateDoorID=int(row['RealEstateDoorID']))
  return doors


def readPeqNeighborhoodDoors():
  doors = {}
  path = os.path.join(here, '..', 'content', 'peridotExports', 'neighborhoodDoors.tsv')
  for row in csv.DictReader(open(path, encoding='utf-8'), delimiter='\t'):
    doorID = int(row['doorid'])
    if doorID in doors:
      raise ValueError(f'neighborhoodDoors.tsv repeats door {doorID}')
    doors[doorID] = dict(id=doorID, name=row['name'], type=int(row['opentype']), x=float(row['pos_x']), y=float(row['pos_y']),
                         z=float(row['pos_z']), heading=float(row['heading']), size=int(row['size']))
  return doors


def isDecoration(door):
  return door['type'] not in (typeNeighborhoodList, typePlotBorder, typeFrontDoor)


def decorationDiffers(live, peq):
  return (live['name'] != peq['name'] or live['type'] != peq['type'] or int(live['size']) != peq['size']
          or math.dist((live['x'], live['y'], live['z']), (peq['x'], peq['y'], peq['z'])) > decorationPositionUnits
          or abs(live['heading'] - peq['heading']) > decorationHeadingUnits)


def neighborhoodDecorations(liveDoors, peqDoors):
  live = {door['id']: door for (zone, _), door in liveDoors.items() if zone == 'neighborhood' and isDecoration(door)}
  peq = {doorID: door for doorID, door in peqDoors.items() if isDecoration(door)}
  stray = sorted(set(peq) - set(live))
  if stray:
    raise ValueError(f"PEQ's neighborhood decorations {stray} are not among Live's")
  added = [doorRow(live[doorID], neighborhoodZoneId, neighborhoodVersion, 'neighborhood') for doorID in sorted(set(live) - set(peq))]
  changed = [(peq[doorID], doorRow(live[doorID], neighborhoodZoneId, neighborhoodVersion, 'neighborhood'))
             for doorID in sorted(set(live) & set(peq)) if decorationDiffers(live[doorID], peq[doorID])]
  return added, changed


def readLotSquares():
  squares = {}
  for line in open(os.path.join(here, '..', 'content', 'sunriseHillsLotSquares.tsv')):
    fields = line.rstrip('\n').split('\t')
    if len(fields) != 8 or int(fields[1]) in squares:
      raise ValueError(f'sunriseHillsLotSquares.tsv: unexpected row {fields}')
    squares[int(fields[1])] = (float(fields[2]), float(fields[3]), float(fields[4]))
  return squares


def peridotZones():
  zones = {liveZone: (zoneID, version, shortName) for zoneID, version, shortName, _, liveZone, _ in housingZones if liveZone is not None}
  if set(zones) != set(frontDoorModels):
    raise ValueError(f'the housing zones Live sent {sorted(zones)} are not the zones with front door models {sorted(frontDoorModels)}')
  return zones


def withFields(door):
  if door['fields'] is None:
    raise ValueError(f"Live door {door['zone']} {door['id']} has no fields/doors.tsv row")
  return door


def doorRow(door, zoneID, version, shortName):
  withFields(door)
  if not re.fullmatch(r'\d+', door['size']):
    raise ValueError(f"Live door {door['zone']} {door['id']}: size {door['size']!r} is not a whole number")
  return dict(zone=shortName, zoneID=zoneID, version=version, doorID=door['id'], name=door['name'], type=door['type'], x=door['x'], y=door['y'],
              z=door['z'], heading=door['heading'], size=int(door['size']), **door['fields'])


def housingDoorRows(liveDoors):
  zones = peridotZones()
  handled = set(zones) | zonesRoF2Lacks
  stray = sorted(key for key, door in liveDoors.items()
                 if door['type'] == typeFrontDoor and key[0] not in handled)
  if stray:
    raise ValueError(f"Live's front doors in zones not handled here: {stray}")
  rows = []
  for liveZone, (zoneID, version, shortName) in zones.items():
    doors = sorted((door for (zone, _), door in liveDoors.items() if zone == liveZone), key=lambda door: door['id'])
    fronts = [door for door in doors if door['type'] == typeFrontDoor]
    if len(fronts) != 1 or fronts[0]['name'] != frontDoorModels[liveZone]:
      raise ValueError(f"Live's {liveZone}: front doors {[(d['id'], d['name']) for d in fronts]}, expected one {frontDoorModels[liveZone]}")
    if len(doors) > 1 and liveZone not in zonesWithInteriorDoors:
      raise ValueError(f"Live's {liveZone} has doors besides its front door: {[(d['id'], d['name']) for d in doors]}")
    rows.extend(doorRow(door, zoneID, version, shortName) for door in doors)
  return rows


def expectedRealEstateDoorID(door):
  if door['type'] == typePlotBorder and door['name'] == lotSquareModel:
    return {door['id'] + 100}
  if door['type'] == typePlotBorder and door['name'] == guildSquareModel and door['id'] in guildSquarePlots:
    return {guildSquarePlots[door['id']]}
  if door['type'] == typeNeighborhoodList:
    return {1}
  # Live sends -1 on the three-room interiors' doors; the column is unsigned and the client reads neither (DOR-5).
  if door['type'] == typeFrontDoor:
    return {0, -1}
  raise ValueError(f"Live door {door['zone']} {door['id']} ({door['name']}, type {door['type']}) is not a housing door")


def mapRow(door, zoneID, version):
  withFields(door)
  liveValue = door['fields']['realEstateDoorID']
  if liveValue not in expectedRealEstateDoorID(door):
    raise ValueError(f"Live door {door['zone']} {door['id']} ({door['name']}): RealEstateDoorID {liveValue}, the rule gives {sorted(expectedRealEstateDoorID(door))}")
  return (zoneID, version, door['id'], max(liveValue, 0))


def neighborhoodDoors(liveDoors, lotSquares):
  doors = [door for (zone, _), door in liveDoors.items() if zone == 'neighborhood' and door['type'] in (typeNeighborhoodList, typePlotBorder)]
  lots = {door['id']: door for door in doors if door['name'] == lotSquareModel}
  guilds = sorted(door['id'] for door in doors if door['name'] == guildSquareModel)
  portals = [door['id'] for door in doors if door['type'] == typeNeighborhoodList]
  if set(lots) != set(lotSquares) or guilds != sorted(guildSquarePlots) or portals != [portalDoorID] or liveDoors[('neighborhood', portalDoorID)]['name'] != portalModel:
    raise ValueError(f"Live's neighborhood borders and portal (lot squares {sorted(lots)}, guild squares {guilds}, list doors {portals}) "
                     f"are not PEQ's lot squares {sorted(lotSquares)}, guild squares {sorted(guildSquarePlots)} and {portalModel} {portalDoorID}")
  for doorID, door in lots.items():
    distance = math.dist((door['x'], door['y'], door['z']), lotSquares[doorID])
    if distance > lotSquareMatchUnits:
      raise ValueError(f"lot square {doorID}: PEQ's is {distance:.2f} units from Live's")
  return sorted(doors, key=lambda door: door['id'])


def lobbyGate(liveDoors):
  gate = liveDoors.get(('guildlobby', gateDoorID))
  if gate is None or gate['name'] != gateModel or gate['type'] != typeNeighborhoodList:
    raise ValueError(f"Live's guildlobby door {gateDoorID} is not a type-{typeNeighborhoodList} {gateModel}")
  others = sorted(key for key, door in liveDoors.items() if door['type'] == typeNeighborhoodList and key not in (('guildlobby', gateDoorID), ('neighborhood', portalDoorID)))
  if others:
    raise ValueError(f"Live's neighborhood list doors besides the portal and the gate: {others}")
  return gate


def realEstateDoorRows(liveDoors, lotSquares, doorRows):
  lobbyZoneID = guildLobbyZoneID()
  rows = [mapRow(door, neighborhoodZoneId, neighborhoodVersion) for door in neighborhoodDoors(liveDoors, lotSquares)]
  rows.append(mapRow(lobbyGate(liveDoors), lobbyZoneID, lobbyVersion))
  fronts = [row for row in doorRows if row['type'] == typeFrontDoor]
  rows.extend(mapRow(dict(zone=row['zone'], id=row['doorID'], name=row['name'], type=row['type'], fields=dict(realEstateDoorID=row['realEstateDoorID'])),
                     row['zoneID'], row['version']) for row in fronts)
  if len(set(row[:3] for row in rows)) != len(rows):
    raise ValueError('the door map repeats a (zone, version, door id)')
  return sorted(rows)


def checkDoorRules(doorRows):
  perZone = collections.Counter((row['zone'], row['version']) for row in doorRows)
  for zoneVersion, count in perZone.items():
    if count > maxDoors:
      raise ValueError(f'{zoneVersion}: {count} doors, more than {maxDoors}')
  if len({(row['zone'], row['version'], row['doorID']) for row in doorRows}) != len(doorRows):
    raise ValueError('two doors share a (zone, version, door id)')
  for row in doorRows:
    label = f"{row['zone']} version {row['version']} door {row['doorID']} ({row['name']})"
    if not 0 <= row['doorID'] <= maxDoorID:
      raise ValueError(f'{label}: door id outside 0-{maxDoorID}')
    if not re.fullmatch(r'[A-Z0-9_]{4,}', row['name']):
      raise ValueError(f'{label}: the name is not upper case and longer than 3 characters')
    if row['type'] in (typeNeighborhoodList, typePlotBorder, typeFrontDoor) and (row['state'] != 0 or row['defaultState'] != 0):
      raise ValueError(f"{label}: type {row['type']} with state {row['state']} (default {row['defaultState']}), not 0")
    if row['remainOpen'] != 0:
      raise ValueError(f'{label}: remains open')
    if not 0 <= row['heading'] < 512:
      raise ValueError(f"{label}: heading {row['heading']} is outside [0, 512)")


def mappedZoneNames():
  return {neighborhoodZoneId: 'neighborhood', guildLobbyZoneID(): 'guildlobby'}


def ruleRowsForMappedDoors(liveDoors, mapRows):
  zoneNames = mappedZoneNames()
  return [doorRow(liveDoors[(zoneNames[zoneID], doorID)], zoneID, version, zoneNames[zoneID])
          for zoneID, version, doorID, _ in mapRows if zoneID in zoneNames]


def insertStatement(columns, row, template='templateId'):
  values = dict(zone=f"'{row['zone']}'", version=str(row['version']), doorid=str(row['doorID']), name=f"'{row['name']}'", pos_x=repr(row['x']),
                pos_y=repr(row['y']), pos_z=repr(row['z']), heading=repr(row['heading']), size=str(row['size']), opentype=str(row['type']),
                doorisopen='0', invert_state='0', real_estate_door_id=str(max(row['realEstateDoorID'], 0)))
  selected = ', '.join(values.get(column, f'`{column}`') for column in columns)
  return (f"  INSERT INTO doors ({', '.join(f'`{column}`' for column in columns)})\n"
          f"    SELECT {selected} FROM doors WHERE id = {template};\n")


def updateStatement(zoneName, version, doorID, value, liveDoors):
  columns = f'real_estate_door_id = {value}'
  if (zoneName, doorID) == ('guildlobby', gateDoorID):
    columns = f'opentype = {typeNeighborhoodList}, {columns}'
  if (zoneName, doorID) == ('neighborhood', portalDoorID):
    # PEQ's portal stands 8 units from Live's (DOR-15): it moves to Live's spot.
    portal = liveDoors[(zoneName, doorID)]
    columns = f"pos_x = {portal['x']!r}, pos_y = {portal['y']!r}, pos_z = {portal['z']!r}, heading = {portal['heading']!r}, {columns}"
  return f"  UPDATE doors SET {columns} WHERE zone = '{zoneName}' AND version = {version} AND doorid = {doorID};\n"


def decorationUpdateStatement(peq, live):
  return (f"  UPDATE doors SET name = '{live['name']}', opentype = {live['type']}, pos_x = {live['x']!r}, pos_y = {live['y']!r}, "
          f"pos_z = {live['z']!r}, heading = {live['heading']!r}, size = {live['size']} WHERE zone = 'neighborhood' AND version = "
          f"{neighborhoodVersion} AND doorid = {peq['id']};\n")


def peqDecorationCondition(peq):
  return (f"(doorid = {peq['id']} AND name = '{peq['name']}' AND opentype = {peq['type']} AND size = {peq['size']} AND "
          + ' AND '.join(f"ABS(pos_{axis} - ({peq[axis]!r})) <= {decorationPositionUnits!r}" for axis in 'xyz')
          + f" AND ABS(heading - ({peq['heading']!r})) <= {decorationHeadingUnits!r})")


def nearCondition(door):
  return ' AND '.join(f'ABS(pos_{axis} - ({door[axis]!r})) <= {lotSquareMatchUnits!r}' for axis in 'xyz')


def buildSql(columns, liveDoors, lotIDs, doorRows, mapRows, added, changed):
  byZone = collections.defaultdict(list)
  for row in doorRows:
    byZone[(row['zone'], row['version'])].append(row['doorID'])
  idList = lambda ids: ', '.join(map(str, sorted(ids)))
  existing = ' OR '.join(f"(zone = '{zone}' AND version = {version} AND doorid IN ({idList(ids)}))" for (zone, version), ids in sorted(byZone.items()))
  limits = ''.join(
    f"  IF (SELECT COUNT(*) + {len(ids)} > {maxDoors} OR COALESCE(MAX(doorid), 0) > {maxDoorID} FROM doors WHERE zone = '{zone}' AND version = {version}) THEN\n"
    f"    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = '{zone} version {version} would pass {maxDoors} doors or already has a door id above {maxDoorID}';\n  END IF;\n"
    for (zone, version), ids in sorted(byZone.items()))
  borderCount = len(lotIDs) + len(guildSquarePlots)
  gate = liveDoors[('guildlobby', gateDoorID)]
  inserts = ''.join(insertStatement(columns, row) for row in doorRows)
  zoneNames = mappedZoneNames()
  updates = ''.join(updateStatement(zoneNames[zoneID], version, doorID, value, liveDoors) for zoneID, version, doorID, value in mapRows if zoneID in zoneNames)
  closed = 'doorisopen = 0 AND invert_state = 0'
  addedIDs = idList(row['doorID'] for row in added)
  decorationInserts = ''.join(insertStatement(columns, row, 'decorationTemplateId') for row in added)
  decorationUpdates = ''.join(decorationUpdateStatement(peq, live) for peq, live in changed)
  peqDecorations = ' OR '.join(peqDecorationCondition(peq) for peq, _ in changed)
  return f"""-- Generated by playerHousing/tools/makeHousingDoorsSql.py from Live's doors and content/sunriseHillsLotSquares.tsv; loads after guildLobby.sql and guildBorderDoors.sql.
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE templateId INT DEFAULT NULL;
  DECLARE decorationTemplateId INT DEFAULT NULL;
  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
  IF EXISTS (SELECT 1 FROM doors WHERE real_estate_door_id <> 0) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'a door already carries a RealEstateDoorID';
  END IF;
  IF EXISTS (SELECT 1 FROM doors WHERE {existing}) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'a housing front or interior door row already exists';
  END IF;
{limits}  IF (SELECT COUNT(*) FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion} AND opentype = {typePlotBorder} AND {closed}
      AND ((name = '{lotSquareModel}' AND doorid IN ({idList(lotIDs)})) OR (name = '{guildSquareModel}' AND doorid IN ({idList(guildSquarePlots)})))) <> {borderCount} THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the neighborhood lacks a closed type-{typePlotBorder} lot or guild square the door map names; load guildBorderDoors.sql first';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion} AND doorid = {portalDoorID} AND name = '{portalModel}'
      AND opentype = {typeNeighborhoodList} AND {closed}) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'neighborhood door {portalDoorID} is not a closed type-{typeNeighborhoodList} {portalModel}';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM doors WHERE zone = 'guildlobby' AND version = {lobbyVersion} AND doorid = {gateDoorID} AND name = '{gateModel}'
      AND {closed} AND {nearCondition(gate)}) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'guildlobby door {gateDoorID} is not a closed {gateModel} at Live''s position';
  END IF;
  IF EXISTS (SELECT 1 FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion} AND doorid IN ({addedIDs})) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'a door id of Live''s neighborhood decorations is taken; load guildBorderDoors.sql first';
  END IF;
  IF (SELECT COUNT(*) FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion} AND ({peqDecorations})) <> {len(changed)} THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the neighborhood''s decorations that differ from Live''s are not as PEQ ships them';
  END IF;
  IF (SELECT COUNT(*) + {len(added)} > {maxDoors} FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion}) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the neighborhood would pass {maxDoors} doors';
  END IF;
  SELECT id INTO decorationTemplateId FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion} AND opentype = 55 AND name LIKE 'IT%' ORDER BY doorid LIMIT 1;
  IF decorationTemplateId IS NULL THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'no neighborhood decoration of open type 55 to copy';
  END IF;
  SELECT id INTO templateId FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion} AND name = '{lotSquareModel}' AND opentype = {typePlotBorder} ORDER BY doorid LIMIT 1;
  IF templateId IS NULL THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'no {lotSquareModel} door with open type {typePlotBorder} to copy';
  END IF;
  START TRANSACTION;
{inserts}{updates}{decorationInserts}{decorationUpdates}  COMMIT;
END //
DELIMITER ;
SELECT zone, version, doorid, name, pos_x, pos_y, pos_z, heading, opentype, size FROM doors WHERE {existing} OR (zone = 'guildlobby' AND version = {lobbyVersion} AND doorid = {gateDoorID}) ORDER BY zone, version, doorid;
SELECT COUNT(*), MIN(doorid), MAX(doorid) FROM doors WHERE zone = 'neighborhood' AND version = {neighborhoodVersion};
SELECT zone, version, COUNT(*), MIN(real_estate_door_id), MAX(real_estate_door_id) FROM doors WHERE real_estate_door_id <> 0 OR opentype IN ({typeNeighborhoodList}, {typePlotBorder}, {typeFrontDoor}) GROUP BY zone, version ORDER BY zone, version;
"""


def generate(live):
  liveDoors = readLiveDoors(live)
  lotSquares = readLotSquares()
  doorRows = housingDoorRows(liveDoors)
  mapRows = realEstateDoorRows(liveDoors, lotSquares, doorRows)
  checkDoorRules(doorRows)
  checkDoorRules(ruleRowsForMappedDoors(liveDoors, mapRows))
  added, changed = neighborhoodDecorations(liveDoors, readPeqNeighborhoodDoors())
  checkDoorRules(added + [live for _, live in changed])
  return liveDoors, sorted(lotSquares), doorRows, mapRows, added, changed


def main():
  parser = argparse.ArgumentParser(description='Write sql/housingDoors.sql.')
  parser.add_argument('--live', required=True, help='a livedumps clone')
  live = parser.parse_args().live
  if not os.path.isdir(os.path.join(live, 'master')):
    raise SystemExit(f'{live} is not a livedumps clone')
  liveDoors, lotIDs, doorRows, mapRows, added, changed = generate(live)
  sql = buildSql(doorsColumns(), liveDoors, lotIDs, doorRows, mapRows, added, changed)
  path = os.path.join(here, '..', 'sql', 'housingDoors.sql')
  open(path, 'w', encoding='ascii').write(sql)
  fronts = sum(row['type'] == typeFrontDoor for row in doorRows)
  print(f'wrote {os.path.relpath(path)}: {len(doorRows)} doors ({fronts} front doors), {len(mapRows)} RealEstateDoorIDs, the Guild Lobby gate, '
        f"{len(added)} of Live's neighborhood decorations added and {len(changed)} set to Live's")


if __name__ == '__main__':
  main()
