"""Tests makeHousingDoorsSql.py on its data: regeneration, the door map's rows and values, Live's front doors per zone, the Palatial hall's interior doors, the SQL's guards and gate update, and each door rule refusing a bad input.

Usage: python testHousingDoors.py --live <livedumps clone>
"""
import argparse, copy, os, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeHousingDoorsSql as generator

sqlPath = os.path.join(here, '..', 'sql', 'housingDoors.sql')
parser = argparse.ArgumentParser(description='Test makeHousingDoorsSql.py.')
parser.add_argument('--live', required=True, help='a livedumps clone')
liveClone = parser.parse_args().live


def check(label, actual, expected):
  if actual != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
  print(f'{label}: {actual!r}')


def expectRaises(label, action, fragment):
  try:
    action()
  except ValueError as error:
    if fragment not in str(error):
      raise AssertionError(f'{label}: raised {error!r}, which lacks {fragment!r}')
    print(f'{label}: raises ({error})')
    return
  raise AssertionError(f'{label}: did not raise')


committed = open(sqlPath).read()
subprocess.run([sys.executable, os.path.join(here, 'makeHousingDoorsSql.py'), '--live', liveClone], capture_output=True, text=True, check=True)
check('the committed SQL equals a fresh generation', open(sqlPath).read() == committed, True)

liveDoors, lotIDs, doorRows, mapRows = generator.generate(liveClone)
lotSquares = generator.readLotSquares()

neighborhoodRows = [row for row in mapRows if row[:2] == (712, 0)]
check('door map rows: 73 lot squares, 4 guild squares, the portal, the Guild Lobby gate, 12 front doors', len(mapRows), 91)
check('neighborhood and Guild Lobby map rows', len(neighborhoodRows) + sum(row[:2] == (344, 0) for row in mapRows), 79)
check('lot squares 1-73 map to plots 101-173', [row[3] for row in neighborhoodRows if row[2] <= 73], list(range(101, 174)))
check('guild squares map to plots 187-190', [(row[2], row[3]) for row in neighborhoodRows if 200 <= row[2] <= 255], [(226, 187), (227, 188), (228, 189), (229, 190)])
check('the void portal opens the neighborhood list', [row for row in neighborhoodRows if row[2] == 130], [(712, 0, 130, 1)])
check('the Guild Lobby gate opens the neighborhood list', [row for row in mapRows if row[0] == 344], [(344, 0, 38, 1)])
check('front doors map to 0', sorted(row for row in mapRows if row[3] == 0),
      [(345, 2, 3, 0), (714, 0, 1, 0), (715, 0, 1, 0), (716, 0, 1, 0), (717, 0, 1, 0), (718, 0, 1, 0), (719, 0, 1, 0), (723, 0, 1, 0),
       (737, 0, 1, 0), (738, 0, 2, 0), (751, 0, 2, 0), (766, 0, 1, 0)])

fronts = {(row['zone'], row['version']): (row['doorID'], row['name'], row['type'], row['x'], row['y'], row['z'], row['heading'], row['size'])
          for row in doorRows if row['type'] == 161}
check("front doors at Live's positions", fronts, {
  ('phinterior3a1', 0): (1, 'OBJ_DOOR_PLHA', 161, 0.0, -19.2559, 0.0, 128.0, 100),
  ('phinterior3a2', 0): (1, 'OBJ_DOOR_PLHA', 161, 0.0, -19.2559, 0.0, 128.0, 100),
  ('phinterior3a3', 0): (1, 'OBJ_DOOR_PLHA', 161, 0.0, -19.2559, 0.0, 128.0, 100),
  ('phinterior1a1', 0): (1, 'OBJ_DOOR_PLHA', 161, 0.0, -19.3599, 0.0, 128.0, 100),
  ('phinterior1a2', 0): (1, 'OBJ_DOOR_PLHA', 161, 0.0, -19.3599, 0.0, 128.0, 100),
  ('phinterior1a3', 0): (1, 'OBJ_DOOR_PLHA', 161, 0.0, -19.3599, 0.0, 128.0, 100),
  ('phinterior1d1', 0): (1, 'OBJ_PH1D1_DOOR', 161, 0.0, -19.3599, 0.0, 128.0, 100),
  ('phinteriortree', 0): (1, 'OBJ_EVANTIL_PLYRHOUSE_DOOR', 161, -0.00308799, -5.42191, -0.173446, 128.0, 100),
  ('guildhall3', 0): (2, 'GUILDHALL_DOOR_SWITCH', 161, 265.698, 1.95192, -0.683093, 384.0, 100),
  ('guildhallsml', 0): (2, 'OBJ_GUILDHALL_SML_DOOR', 161, 265.698, 1.95192, -0.683093, 384.0, 100),
  ('guildhalllrg', 0): (1, 'OBJ_FRONTENTRANCE', 161, 88.0, 0.699999, 8.23681, 384.0, 100),
  ('guildhall', 2): (3, 'OBJ_GUILD_DOOR', 161, -3.4, -162.899, 6.9, 128.0, 100),
})
interiorDoors = [row for row in doorRows if row['zone'] == 'guildhalllrg' and row['type'] != 161]
check('the Palatial hall has 26 interior doors, ids 2-27, all OBJ_INTDOOR of type 27',
      (len(interiorDoors), [row['doorID'] for row in interiorDoors], {(row['name'], row['type']) for row in interiorDoors}),
      (26, list(range(2, 28)), {('OBJ_INTDOOR', 27)}))
check("interior door 14 at Live's position", next((row['x'], row['y'], row['z'], row['heading']) for row in interiorDoors if row['doorID'] == 14), (-115.57, 3.31411, 21.0, 128.0))
check('door rows: 12 front doors and 26 interior doors', len(doorRows), 38)
check('no door row in a zone RoF2 lacks', sorted({row['zone'] for row in doorRows} & generator.zonesRoF2Lacks), [])
check('every door row is closed and does not remain open', {(row['state'], row['defaultState'], row['remainOpen']) for row in doorRows}, {(0, 0, 0)})
check("Live's -1 on the three-room interiors' doors is written as 0", sorted((row['zone'], row['realEstateDoorID']) for row in doorRows if row['realEstateDoorID'] != 0),
      [('phinterior3a1', -1), ('phinterior3a2', -1), ('phinterior3a3', -1)])

sql = open(sqlPath).read()
check('the gate update sets only its open type', [line.strip() for line in sql.splitlines() if line.lstrip().startswith('UPDATE')],
      ["UPDATE doors SET opentype = 159 WHERE zone = 'guildlobby' AND version = 0 AND doorid = 38;"])
check('the SQL refuses a non-empty door map', "IF EXISTS (SELECT 1 FROM real_estate_doors) THEN" in sql, True)
check('one insert per door row', sql.count('INSERT INTO doors '), 38)
check('the 345 version 2 door row', "SELECT 3, 'guildhall', 2, 'OBJ_GUILD_DOOR', -162.899, -3.4, 6.9, 128.0, 161," in sql, True)
check('every door row is written closed and uninverted', sql.count(", 0, `door_param`,") == 38 and sql.count('`dest_heading`, 0, `incline`') == 38, True)

good = next(row for row in doorRows if row['zone'] == 'phinterior1a1')
variant = lambda **changes: [dict(good, **changes)]
generator.checkDoorRules(variant())
expectRaises('door id 256', lambda: generator.checkDoorRules(variant(doorID=256)), 'door id outside 0-255')
expectRaises('door id -1', lambda: generator.checkDoorRules(variant(doorID=-1)), 'door id outside 0-255')
generator.checkDoorRules([dict(good, doorID=i % 256, version=i // 256) for i in range(500)])
expectRaises('501 doors in one zone version', lambda: generator.checkDoorRules([dict(good, doorID=i % 256, name=f'OBJ_DOOR_{i}') for i in range(501)]), 'more than 500')
expectRaises('two doors with one id', lambda: generator.checkDoorRules(variant() + variant()), 'share a (zone, version, door id)')
expectRaises('a three-character name', lambda: generator.checkDoorRules(variant(name='OBJ')), 'upper case and longer than 3')
expectRaises('a lower case name', lambda: generator.checkDoorRules(variant(name='OBJ_door_plhA')), 'upper case and longer than 3')
for doorType in (159, 160, 161):
  expectRaises(f'type {doorType} with state 1', lambda: generator.checkDoorRules(variant(type=doorType, state=1)), 'not 0')
  expectRaises(f'type {doorType} with default state 1', lambda: generator.checkDoorRules(variant(type=doorType, defaultState=1)), 'not 0')
generator.checkDoorRules(variant(type=27, state=1))
expectRaises('a door that remains open', lambda: generator.checkDoorRules(variant(remainOpen=1)), 'remains open')
expectRaises('a heading of 512', lambda: generator.checkDoorRules(variant(heading=512.0)), 'outside [0, 512)')


def withLiveChange(key, **fields):
  changed = copy.deepcopy(liveDoors)
  changed[key]['fields'].update(fields)
  return changed


expectRaises("a lot square whose RealEstateDoorID is not its id plus 100", lambda: generator.realEstateDoorRows(withLiveChange(('neighborhood', 5), realEstateDoorID=999), lotSquares, doorRows), 'RealEstateDoorID 999')
expectRaises("a guild square mapped to the wrong plot", lambda: generator.realEstateDoorRows(withLiveChange(('neighborhood', 227), realEstateDoorID=187), lotSquares, doorRows), 'RealEstateDoorID 187')
expectRaises("the portal not mapped to 1", lambda: generator.realEstateDoorRows(withLiveChange(('neighborhood', 130), realEstateDoorID=0), lotSquares, doorRows), 'RealEstateDoorID 0')
expectRaises("the gate not mapped to 1", lambda: generator.realEstateDoorRows(withLiveChange(('guildlobby', 38), realEstateDoorID=0), lotSquares, doorRows), 'RealEstateDoorID 0')
badFront = [dict(row, realEstateDoorID=5) if row['zone'] == 'guildhall' else row for row in doorRows]
expectRaises("a front door not mapped to 0", lambda: generator.realEstateDoorRows(liveDoors, lotSquares, badFront), 'RealEstateDoorID 5')
moved = {**lotSquares, 12: (lotSquares[12][0] + 1.5, lotSquares[12][1], lotSquares[12][2])}
expectRaises("a PEQ lot square 1.5 units from Live's", lambda: generator.realEstateDoorRows(liveDoors, moved, doorRows), "lot square 12: PEQ's is 1.50 units")
missing = {doorID: position for doorID, position in lotSquares.items() if doorID != 40}
expectRaises("a lot square PEQ lacks", lambda: generator.realEstateDoorRows(liveDoors, missing, doorRows), 'are not PEQ')
stray = copy.deepcopy(liveDoors)
stray[('qeynos', 250)] = dict(stray[('phinterior1a1', 1)], zone='qeynos', id=250)
expectRaises("a Live front door in a zone not handled", lambda: generator.housingDoorRows(stray), "('qeynos', 250)")
extra = copy.deepcopy(liveDoors)
extra[('guildhall3_int', 7)] = dict(extra[('guildhalllrg_int', 2)], zone='guildhall3_int', id=7)
expectRaises("a hall with a door besides its front door", lambda: generator.housingDoorRows(extra), 'besides its front door')
renamed = copy.deepcopy(liveDoors)
renamed[('phinterior1d1', 1)]['name'] = 'OBJ_DOOR_PLHA'
expectRaises("a front door with another zone's model", lambda: generator.housingDoorRows(renamed), 'expected one OBJ_PH1D1_DOOR')
print('all housing door checks passed')
