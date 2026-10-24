"""Tests makeGuildLobbySql.py on its data: Live's 36 lobby doors and the gate, the renumbering, the version rows and their expansions,
the instance, the zone point, the guards and the refusals.

Usage: python testGuildLobby.py --live <livedumps clone>
"""
import argparse, os, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeGuildLobbySql as generator
from eqemuColumns import columnsOf
from makeHousingDoorsSql import readLiveDoors

parser = argparse.ArgumentParser(description='Test makeGuildLobbySql.py.')
parser.add_argument('--live', required=True, help='a livedumps clone')
live = parser.parse_args().live


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
    print(f'{label}: raises ({str(error)[:80]})')
    return
  raise AssertionError(f'{label}: did not raise')


liveDoors = readLiveDoors(live)
doors = generator.liveLobbyDoors(liveDoors)
with open(generator.sqlPath, encoding='utf-8') as file:
  sql = file.read()
check("sql/guildLobby.sql is the generator's output", sql == generator.renderSql(doors, columnsOf('zone'), columnsOf('zone_points')), True)
check("Live's lobby has 36 doors, ids 1-38 without 5 and 25", (len(doors), sorted(set(range(1, 39)) - set(doors))), (36, [5, 25]))
check('the gate is door 38, type 159, at its position', (doors[38]['name'], doors[38]['type'], doors[38]['x'], doors[38]['y'], doors[38]['z']),
      ('OBJ_IRONGATESWITCH', 159, 293.331, 984.187, -22.0072))
check('the other doors are the two guild doors, the entry and 32 A-frames',
      sorted((door['name'] for door in doors.values()), key=lambda name: (name != 'OBJ_GUILD_DOOR', name)).count('OBJ_AFRAME'), 32)
check("version 0 ends at Underfoot, version 1 starts at House of Thule, open-ended", (generator.underfoot, generator.houseOfThule), (16, 17))
check('the SQL caps version 0 and copies it as version 1 with its note',
      ('UPDATE zone SET max_expansion = 16 WHERE id = templateId;' in sql, "SELECT `zoneidnumber`, 1, `short_name`, `long_name`, `min_status`, `map_file_name`, 'Live" in sql,
       ', 17, -1, `content_flags`' in sql), (True, True, True))
check("the doors renumber by PEQ's offset, the gate landing on 38", "UPDATE doors SET doorid = doorid - 39 WHERE zone = 'guildlobby' AND version = 1 AND doorid BETWEEN 40 AND 77;" in sql, True)
check('the zone point to the Plane of Knowledge is copied to version 1', 'INSERT INTO zone_points (' in sql and "SELECT `zone`, 1, `number`" in sql, True)
check('the global never-expiring instance EQEmu routes by', "INSERT INTO instance_list (zone, version, is_global, start_time, duration, never_expires, notes) VALUES (344, 1, 1, 0, 0, 1, " in sql, True)
check("the SQL refuses a second run and doors that are not PEQ's 36",
      ("already has a version 1 zone row" in sql, 'already has its global instance' in sql, "<> 36" in sql and "(doorid = 77 AND name = 'OBJ_IRONGATESWITCH' AND opentype = 159" in sql), (True, True, True))
check('every door is matched by id, name, type and position within a unit', sql.count('(doorid = ') , 36)
noGate = {key: door for key, door in liveDoors.items() if key != ('guildlobby', 38)}
expectRaises('a lobby without its gate', lambda: generator.liveLobbyDoors(noGate), 'is not a type-159 OBJ_IRONGATESWITCH')
expectRaises("no lobby doors at all", lambda: generator.liveLobbyDoors({key: door for key, door in liveDoors.items() if key[0] != 'guildlobby'}), 'no guildlobby door')
tooMany = dict(liveDoors)
tooMany[('guildlobby', 250)] = dict(liveDoors[('guildlobby', 4)], id=250)
expectRaises("a door id past PEQ's offset room", lambda: generator.liveLobbyDoors(tooMany), 'do not fit')
print('all guild lobby checks pass')
