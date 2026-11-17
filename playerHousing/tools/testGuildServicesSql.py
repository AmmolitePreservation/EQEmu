"""Tests sql/guildServices.sql against a local MariaDB: EQEmu's npc_types, merchantlist, auras and zone as Peridot defines them (the
schema export), the housing tables it names from sql/housingSchema.sql, Zeflmin Werlikanin's ProjectEQ list, and the landing zones'
rows from Peridot (content/peridotExports/keyZones.tsv). The pool's aura loads, both key sellers sell the same 25 keys in Live's
order, each from the expansion its landing belongs to, and every refusal leaves the database as it was.

Usage: python testGuildServicesSql.py <schema json> <housingSchema.sql> <guildServices.sql> <keyZones.tsv>   (a local MariaDB with a
`peq` database, root without a password)
"""
import csv, json, os, re, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeGuildServicesSql as generator  # noqa: E402

schemaPath, housingSchemaPath, guildServicesPath, keyZonesPath = sys.argv[1:5]
schema = json.load(open(schemaPath))
keys = generator.keys(generator.readDestinations())
sold = [key['itemId'] for key in generator.soldKeys(keys)]
assistantType = 3066640
# ProjectEQ's rows for Zeflmin, as Peridot holds them: item and min expansion.
projectEqRows = [(10092, 14), (10094, 14), (64191, 14), (76000, -1), (76001, -1), (76002, -1), (76003, -1), (76004, -1), (76005, -1),
                 (76006, -1), (76007, -1), (76009, -1), (76010, -1), (76011, -1), (76012, -1), (76070, 10), (88735, 11), (88736, 12),
                 (88737, 13), (88738, 13), (88739, -1), (88740, 16)]


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
  primary = [column['Field'] for column in columns if column['Key'] == 'PRI']
  return f"DROP TABLE IF EXISTS `{name}`; CREATE TABLE `{name}` ({', '.join(definition(c) for c in columns)}, PRIMARY KEY ({', '.join(f'`{k}`' for k in primary)})) ENGINE=InnoDB;"


def housingTables():
  text = open(housingSchemaPath, encoding='utf-8').read()
  statements = []
  for table in ('real_estate_npc_items', 'real_estate_auras'):
    start = text.index(f'CREATE TABLE IF NOT EXISTS `{table}`')
    statements.append(f'DROP TABLE IF EXISTS `{table}`; ' + text[start:text.index(';', start) + 1])
  return ' '.join(statements)


def sql(text, expectError=False):
  result = subprocess.run(['mariadb', '-uroot', '-N', 'peq'], input="SET SESSION sql_mode = 'STRICT_TRANS_TABLES';\n" + text, capture_output=True, text=True)
  if (result.returncode != 0) != expectError:
    raise RuntimeError(f'unexpected result ({result.returncode}): {result.stderr[:500]}')
  return result.stderr if expectError else result.stdout.strip()


def check(label, query, expected):
  got = sql(query)
  if got != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {got!r}')
  print(f'{label}: {got!r}')


def seed():
  sql('DROP TABLE IF EXISTS `real_estate_auras`; DROP TABLE IF EXISTS `real_estate_npc_item_actions`; DROP TABLE IF EXISTS `real_estate_npc_items`;')
  sql(createTable('npc_types') + createTable('merchantlist') + createTable('auras') + createTable('zone'))
  sql('DROP TABLE IF EXISTS `items`; CREATE TABLE `items` (`id` INT(11) NOT NULL, PRIMARY KEY (`id`)) ENGINE=InnoDB;')
  sql(housingTables())
  sql('INSERT INTO items (id) VALUES ' + ', '.join(f'({itemId})' for itemId in sold + [generator.teleportationAssistant, generator.poolItem]) + ';')
  sql(f"INSERT INTO npc_types (id, name, merchant_id) VALUES ({assistantType}, 'Teleportation_Assistant', {assistantType}),"
      f" ({generator.zeflminWerlikanin}, 'Zeflmin_Werlikanin', {generator.zeflminWerlikanin});")
  sql(f"INSERT INTO real_estate_npc_items (item_id, npc_type_id, mount_npc_type_id, race_tag, training_dummy) VALUES"
      f" ({generator.teleportationAssistant}, {assistantType}, 0, 'GNM', 0);")
  sql('INSERT INTO merchantlist (merchantid, slot, item, faction_required, level_required, min_status, max_status, alt_currency_cost, '
      'classes_required, probability, min_expansion, max_expansion) VALUES ' +
      ', '.join(f'({generator.zeflminWerlikanin}, {slot}, {item}, -100, 0, 0, 255, 0, 65535, 100, {minimum}, -1)'
                for slot, (item, minimum) in enumerate(projectEqRows, start=1)) + ';')
  with open(keyZonesPath, newline='', encoding='utf-8') as file:
    zones = list(csv.DictReader(file, delimiter='\t'))
  # The zone rows hold only the columns the SQL reads; the rest of the table's required columns take their types' defaults.
  sql("SET SESSION sql_mode = ''; INSERT INTO zone (zoneidnumber, version, short_name, expansion, min_expansion, max_expansion) VALUES " +
      ', '.join(f"({z['zoneidnumber']}, {z['version']}, '{z['short_name']}', {z['expansion']}, {z['min_expansion']}, {z['max_expansion']})" for z in zones) + ';')


content = open(guildServicesPath, encoding='utf-8').read()
seed()
print(f'tables made: Zeflmin holds ProjectEQ\'s {len(projectEqRows)} keys, {len(keys)} keys in the destinations')
sql(content)
check('the pool keeps its aura', 'SELECT (SELECT COUNT(*) FROM auras WHERE type = 31874), (SELECT aura_type FROM real_estate_auras WHERE item_id = 66645),'
      ' (SELECT name FROM npc_types WHERE id = 2000100)', '1\t31874\tIOAuraOfTheHall')
check('each seller sells the 25 keys', f'SELECT merchantid, COUNT(*) FROM merchantlist GROUP BY merchantid ORDER BY merchantid',
      f'{generator.zeflminWerlikanin}\t25\n{assistantType}\t25')
check("in Live's order", f'SELECT GROUP_CONCAT(item ORDER BY slot) FROM merchantlist WHERE merchantid = {generator.zeflminWerlikanin}',
      ','.join(map(str, sold)))
check("Zeflmin's rows are the Assistant's", f'SELECT COUNT(*) FROM merchantlist a JOIN merchantlist z ON z.slot = a.slot AND z.item = a.item'
      f' AND z.min_expansion = a.min_expansion AND z.max_expansion = a.max_expansion WHERE a.merchantid = {assistantType} AND z.merchantid = {generator.zeflminWerlikanin}', '25')
check("Lavastorm's key from expansion 9, where Live's version of the zone serves; North Karana's from the start",
      f'SELECT item, min_expansion FROM merchantlist WHERE merchantid = {assistantType} AND item IN (76000, 76002) ORDER BY item', '76000\t0\n76002\t9')
check("Barindu's, Brell's Rest's and the Dreadlands' from their zones' expansions; the Plane of Hate's from the start",
      f'SELECT item, min_expansion FROM merchantlist WHERE merchantid = {assistantType} AND item IN (10092, 76005, 76012, 88740) ORDER BY item',
      '10092\t0\n76005\t1\n76012\t7\n88740\t16')
check('the splinter from The Buried Sea', f'SELECT min_expansion FROM merchantlist WHERE merchantid = {assistantType} AND item = 88737', '13')
with open(keyZonesPath, newline='', encoding='utf-8') as file:
  zoneRows = list(csv.DictReader(file, delimiter='\t'))
expected = {}
for key in keys:
  rowsOfZone = [z for z in zoneRows if int(z['zoneidnumber']) == key['zoneId']]
  expected[key['itemId']] = generator.guildStandardExpansion if key['guildStandard'] else max(
    max(int(z['expansion']) for z in rowsOfZone), max(int(z['min_expansion']) for z in rowsOfZone))
check("every key starts at the greater of its zone's expansion and its newest version's start",
      f'SELECT GROUP_CONCAT(CONCAT(item, ":", min_expansion) ORDER BY item) FROM merchantlist WHERE merchantid = {assistantType}',
      ','.join(f'{itemId}:{expected[itemId]}' for itemId in sorted(expected)))
check('at era 4 only the keys whose landings are in that era show', f'SELECT COUNT(*) FROM merchantlist WHERE merchantid = {assistantType} AND min_expansion <= 4',
      str(sum(1 for value in expected.values() if value <= 4)))
check('every row shows to every status and never ends', 'SELECT DISTINCT min_status, max_status, max_expansion FROM merchantlist', '0\t255\t-1')
error = sql(content, expectError=True)
assert 'the guild regeneration pool already has an aura row' in error, f'a second run was not refused: {error[:300]}'
print('a second run is refused by the first guard')


stateQuery = ('SELECT (SELECT COUNT(*) FROM auras), (SELECT COUNT(*) FROM npc_types), (SELECT COUNT(*) FROM real_estate_auras),'
              ' (SELECT GROUP_CONCAT(CONCAT(merchantid, ":", slot, ":", item) ORDER BY merchantid, slot) FROM merchantlist)')


def refused(label, change, fragment):
  seed()
  sql(change)
  before = sql(stateQuery)
  error = sql(content, expectError=True)
  assert fragment in error, f'{label}: {error[:300]}'
  check(f'{label} is refused and changes nothing', stateQuery, before)


refused('a list for Zeflmin other than ProjectEQ\'s', f'UPDATE merchantlist SET item = 13006 WHERE merchantid = {generator.zeflminWerlikanin} AND slot = 4',
        "Zeflmin Werlikanin's list is not ProjectEQ's 22 keys")
refused('a key that is no item', 'DELETE FROM items WHERE id = 76002', 'a key the halls sell is not in items')
refused('a landing zone without a row', 'DELETE FROM zone WHERE zoneidnumber = 27', 'a key lands in a zone with no zone row')
refused('an Assistant who already sells keys', f'INSERT INTO merchantlist (merchantid, slot, item) VALUES ({assistantType}, 1, 76005)',
        'the Teleportation Assistant already sells keys')
refused('an Assistant with no merchant list', f'UPDATE npc_types SET merchant_id = 0 WHERE id = {assistantType}',
        'the Teleportation Assistant or Zeflmin Werlikanin has no merchant list')
print('all guild services SQL checks passed')
