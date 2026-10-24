"""Tests sql/guildLobby.sql against a local MariaDB: EQEmu's zone, doors, zone_points and instance_list as Peridot defines them (the
schema export), seeded as PEQ stands (the version 0 zone row and zone point, the 36 version 1 doors the SQL names, two version 0
doors); the content caps version 0, adds version 1 with its instance and zone point, renumbers the doors to Live's ids, and refuses a
second run. Then Peridot's own sql/peridot/guildLobbyExpansions.sql: refused before the split, moving the split to Planes of Power after
it, refused again.

Usage: python testGuildLobbySql.py <schema json> <sql folder>   (a local MariaDB with a `peq` database, root without a password; the folder
holds guildLobby.sql, peridot/guildLobbyExpansions.sql and the column migrations zoneHeaderColumns.sql and doorColumns.sql, which run
first as world's migrations do)
"""
import json, os, re, subprocess, sys

schemaPath, sqlFolder = sys.argv[1:3]
schema = json.load(open(schemaPath))


def definition(column):
  text = f"`{column['Field']}` {column['Type']}"
  text += ' NULL' if column['Null'] == 'YES' else ' NOT NULL'
  if column['Default'] is not None:
    default = column['Default']
    text += ' DEFAULT ' + (default if re.fullmatch(r'-?\d+(\.\d+)?', default) or default == 'CURRENT_TIMESTAMP' else "'" + default.replace("'", "''") + "'")
  if 'auto_increment' in column['Extra']:
    text += ' AUTO_INCREMENT'
  return text


def createTable(name):
  columns = schema[name]
  keys = [column['Field'] for column in columns if column['Key'] == 'PRI']
  return f"DROP TABLE IF EXISTS `{name}`; CREATE TABLE `{name}` ({', '.join(definition(c) for c in columns)}, PRIMARY KEY ({', '.join(f'`{k}`' for k in keys)})) ENGINE=InnoDB;"


def sql(text, expectError=False):
  result = subprocess.run(['mariadb', '-uroot', '-N', 'peq'], input=text, capture_output=True, text=True)
  if (result.returncode != 0) != expectError:
    raise RuntimeError(f'unexpected result ({result.returncode}): {result.stderr[:500]}')
  return result.stderr if expectError else result.stdout.strip()


def check(label, query, expected):
  got = sql(query)
  if got != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {got!r}')
  print(f'{label}: {got!r}')


content = open(os.path.join(sqlFolder, 'guildLobby.sql'), encoding='utf-8').read()
# The doors the SQL itself names: PEQ's version 1 rows as they stand before the renumbering.
liveDoors = re.findall(r"\(doorid = (\d+) AND name = '([A-Z_]+)' AND opentype = (\d+) AND ABS\(pos_x - \(([-\d.]+)\)\) <= 1\.0 AND ABS\(pos_y - \(([-\d.]+)\)\) <= 1\.0 AND ABS\(pos_z - \(([-\d.]+)\)\) <= 1\.0\)", content)
assert len(liveDoors) == 36, f'the SQL names {len(liveDoors)} doors'

sql(createTable('zone') + createTable('doors') + createTable('zone_points') + createTable('instance_list'))
for migration in ('zoneHeaderColumns.sql', 'doorColumns.sql'):
  sql(open(os.path.join(sqlFolder, migration), encoding='utf-8').read())
print('zone and doors carry the housing columns')
sql("INSERT INTO zone (zoneidnumber, short_name, version, long_name, min_expansion, max_expansion, expansion, safe_x, safe_y, safe_z, note) VALUES (344, 'guildlobby', 0, 'Guild Lobby', -1, -1, 9, 19, -55, 5, '');")
sql("INSERT INTO zone (zoneidnumber, short_name, version, long_name, min_expansion, max_expansion, expansion) VALUES (27, 'lavastorm', 1, 'Lavastorm Mountains', 9, 99, 0);")
sql("INSERT INTO zone_points (zone, version, number, target_y, target_x, target_z, target_heading, target_zone_id) VALUES ('guildlobby', 0, 20, -306, 1398, -124, 328, 202);")
sql("INSERT INTO doors (zone, version, doorid, name, opentype, pos_x, pos_y, pos_z) VALUES ('guildlobby', 0, 2, 'GUILD_DOOR_', 58, 48, 624, 4), ('guildlobby', 0, 38, 'AFRAME_', 58, 168, 174, -2);")
sql('INSERT INTO doors (zone, version, doorid, name, opentype, pos_x, pos_y, pos_z) VALUES ' + ', '.join(f"('guildlobby', 1, {d[0]}, '{d[1]}', {d[2]}, {d[3]}, {d[4]}, {d[5]})" for d in liveDoors) + ';')
sql("INSERT INTO instance_list (id, zone, version, is_global, start_time, duration, never_expires, notes) VALUES (5, 27, 1, 1, 0, 0, 1, 'Lavastorm - Dragons of Norrath');")
print('tables made and seeded as PEQ: one lobby zone row, one zone point, 2 version 0 and 36 version 1 doors')

peridotContent = open(os.path.join(sqlFolder, 'peridot', 'guildLobbyExpansions.sql'), encoding='utf-8').read()
error = sql(peridotContent, expectError=True)
assert 'not split at House of Thule' in error, f"Peridot's SQL ran before the split: {error[:300]}"
print("Peridot's lobby SQL is refused before the split")
sql(content)
check('version 0 ends at Underfoot, version 1 runs from House of Thule with no end', "SELECT version, min_expansion, max_expansion FROM zone WHERE short_name = 'guildlobby' ORDER BY version", '0\t-1\t16\n1\t17\t-1')
check("version 1 copies version 0's row and carries the note", "SELECT zoneidnumber, long_name, expansion, safe_x, safe_y, safe_z, LEFT(note, 23) FROM zone WHERE short_name = 'guildlobby' AND version = 1",
      "344\tGuild Lobby\t9\t19\t-55\t5\tLive's lobby from House")
check("version 1 doors carry Live's ids, 1 to 38 without 5 and 25", "SELECT COUNT(*), MIN(doorid), MAX(doorid), SUM(doorid IN (5, 25)) FROM doors WHERE zone = 'guildlobby' AND version = 1", '36\t1\t38\t0')
check('the gate is door 38, type 159, at its position', "SELECT name, opentype, pos_x, pos_y FROM doors WHERE zone = 'guildlobby' AND version = 1 AND doorid = 38", 'OBJ_IRONGATESWITCH\t159\t293.331\t984.187')
check('the guild doors keep their ids 1 and 3', "SELECT GROUP_CONCAT(doorid ORDER BY doorid) FROM doors WHERE zone = 'guildlobby' AND version = 1 AND name = 'OBJ_GUILD_DOOR'", '1,3')
check('version 0 doors stay', "SELECT GROUP_CONCAT(CONCAT(doorid, ':', name) ORDER BY doorid) FROM doors WHERE zone = 'guildlobby' AND version = 0", '2:GUILD_DOOR_,38:AFRAME_')
check('the zone point to the Plane of Knowledge exists for both versions', "SELECT version, number, target_zone_id, target_x FROM zone_points WHERE zone = 'guildlobby' ORDER BY version", '0\t20\t202\t1398\n1\t20\t202\t1398')
check('the global never-expiring instance of version 1', "SELECT zone, version, is_global, start_time, duration, never_expires FROM instance_list WHERE zone = 344", '344\t1\t1\t0\t0\t1')
check("lavastorm's instance is untouched", 'SELECT COUNT(*) FROM instance_list', '2')
error = sql(content, expectError=True)
assert 'already has a version 1 zone row' in error, f'a second run was not refused: {error[:300]}'
print('a second run is refused by the first guard')
check('and changes nothing', "SELECT COUNT(*) FROM zone WHERE short_name = 'guildlobby'", '2')
sql(peridotContent)
check("Peridot's SQL moves the split to Planes of Power: version 0 through Luclin, version 1 from Planes of Power",
      "SELECT version, min_expansion, max_expansion FROM zone WHERE short_name = 'guildlobby' ORDER BY version", '0\t-1\t3\n1\t4\t-1')
check('the other zone rows are untouched', "SELECT min_expansion, max_expansion FROM zone WHERE short_name = 'lavastorm'", '9\t99')
error = sql(peridotContent, expectError=True)
assert 'or this ran already' in error, f"Peridot's SQL ran twice: {error[:300]}"
print("Peridot's lobby SQL is refused a second time")
print('all Guild Lobby SQL checks passed')
