"""Tests sql/housingZones.sql and sql/housingZoneSettings.sql against a local MariaDB: EQEmu's zone table as Peridot defines it (the
schema export) after the zone header migration, seeded as Peridot stands: the 24 housing interior and hall rows with their own
values, the classic guild hall (the template) and the neighborhood (the gates). housingZones.sql adds only guildhall version 2,
copying the template and the neighborhood's gates, adds nothing a second time, and refuses a clashing row; housingZoneSettings.sql
then sets Live's header values, the discard delay and the bypass on all 25 and the neighborhood.

Usage: python testHousingZonesSql.py <schema json> <sql folder>   (a local MariaDB with a `peq` database, root without a password; the
folder holds housingZones.sql, housingZoneSettings.sql and zoneHeaderColumns.sql, which runs first as world's migration does)
"""
import json, os, re, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from makeHousingZonesSql import housingZones  # noqa: E402

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


zones = open(os.path.join(sqlFolder, 'housingZones.sql'), encoding='utf-8').read()
settings = open(os.path.join(sqlFolder, 'housingZoneSettings.sql'), encoding='utf-8').read()
sql(createTable('zone'))
sql(open(os.path.join(sqlFolder, 'zoneHeaderColumns.sql'), encoding='utf-8').read())
# Peridot's rows: the template and the gate rows, then the 24 housing rows as PEQ ships them (its own safe points, an hour's delay).
sql("INSERT INTO zone (zoneidnumber, short_name, version, long_name, min_expansion, max_expansion, expansion, safe_x, safe_y, safe_z, ztype, canbind, shutdowndelay, bypass_expansion_check) VALUES (345, 'guildhall', 0, 'Guild Hall', -1, -1, 9, 0, 1, 3, 1, 0, 3600000, 1);")
sql("INSERT INTO zone (zoneidnumber, short_name, version, long_name, min_expansion, max_expansion, expansion, safe_x, safe_y, safe_z, ztype, canbind, shutdowndelay, bypass_expansion_check) VALUES (712, 'neighborhood', 0, 'Sunrise Hills', -1, -1, 17, -1800, -1900, 6, 0, 0, 3600000, 1);")
existing = [(zoneId, version, shortName, title) for zoneId, version, shortName, title, *_ in housingZones if (zoneId, version) != (345, 2)]
assert len(existing) == 24, len(existing)
sql('INSERT INTO zone (zoneidnumber, short_name, version, long_name, min_expansion, max_expansion, expansion, safe_x, safe_y, safe_z, shutdowndelay, bypass_expansion_check) VALUES '
    + ', '.join(f"({zoneId}, '{shortName}', {version}, '{title.replace(chr(39), chr(39) * 2)}', -1, -1, 17, -7, 6.2, 3, 3600000, 1)" for zoneId, version, shortName, title in existing) + ';')
print('zone table made and seeded as Peridot: the classic guild hall, the neighborhood and the 24 housing rows')

sql(zones)
check('only guildhall version 2 is added, 27 rows in all', 'SELECT COUNT(*) FROM zone', '27')
check("version 2 copies the classic hall's row, its own title kept, the neighborhood's gates and expansion, no file names",
      "SELECT zoneidnumber, long_name, file_name, map_file_name, min_expansion, max_expansion, expansion, bypass_expansion_check FROM zone WHERE short_name = 'guildhall' AND version = 2",
      '345\tGuild Hall\t\t\t-1\t-1\t17\t1')
check('the 24 rows Peridot has keep their own values until the settings run', "SELECT COUNT(*) FROM zone WHERE shutdowndelay = 3600000 AND safe_x = -7 AND zoneidnumber BETWEEN 714 AND 766", '24')
sql(zones)
check('a second run adds nothing', 'SELECT COUNT(*) FROM zone', '27')
sql("INSERT INTO zone (zoneidnumber, short_name, version, long_name) VALUES (714, 'otherzone', 0, 'Other');")
error = sql(zones, expectError=True)
assert 'clashes with a housing interior or hall row' in error, f'a clashing id was not refused: {error[:300]}'
sql("DELETE FROM zone WHERE short_name = 'otherzone';")
sql("INSERT INTO zone (zoneidnumber, short_name, version, long_name) VALUES (999, 'phinterior3a1', 0, 'Other');")
error = sql(zones, expectError=True)
assert 'clashes with a housing interior or hall row' in error, f'a clashing short name was not refused: {error[:300]}'
sql("DELETE FROM zone WHERE zoneidnumber = 999;")
print('a row holding a housing id or short name under another name or id refuses the file')

sql(settings)
check("the settings put the discard delay, the bypass and no binding on every housing row",
      "SELECT COUNT(*) FROM zone WHERE shutdowndelay = 1920000 AND bypass_expansion_check = 1 AND canbind = 0 AND (zoneidnumber BETWEEN 714 AND 766 OR (zoneidnumber = 345 AND version = 2))", '25')
check("the houses are Live's zone type 1, the Standard Guild Hall's interior Live's type 2",
      "SELECT GROUP_CONCAT(DISTINCT ztype ORDER BY ztype) FROM zone WHERE short_name LIKE 'phinterior%' OR short_name LIKE 'plh%' UNION ALL SELECT ztype FROM zone WHERE short_name = 'guildhall' AND version = 2", '1\n2')
check("phinterior3a1 gets Live's safe point and floor", "SELECT safe_x, safe_y, safe_z, safe_heading, underworld, ceiling FROM zone WHERE short_name = 'phinterior3a1'", '0\t-12\t5\t128\t-60\t100')
check("a zone Live never sent keeps its derived safe point", "SELECT safe_x, safe_y, safe_z FROM zone WHERE short_name = 'plhogrinteriors1a1'", '-1.6\t2.9\t-1.4')
check("the neighborhood gets Live's header values, its safe point among them, and the bypass", "SELECT safe_x, safe_y, bypass_expansion_check, canbind FROM zone WHERE short_name = 'neighborhood'", '2035\t-2940\t1\t0')
check('the classic guild hall itself is untouched', "SELECT shutdowndelay FROM zone WHERE short_name = 'guildhall' AND version = 0", '3600000')
sql("DELETE FROM zone WHERE zoneidnumber = 766;")
error = sql(settings, expectError=True)
assert 'not every housing interior and hall zone row is present' in error, f'a missing row was not refused: {error[:300]}'
print('the settings refuse a database missing a housing row')
print('all housing zone SQL checks passed')
