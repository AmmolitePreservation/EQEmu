"""Tests sql/npcItems.sql against a local MariaDB: EQEmu's npc_types, merchantlist and npc_types_tint as Peridot defines them (the
schema export), the two housing tables from sql/housingSchema.sql, and an items table holding the NPC items; the content loads
once, every row lands with its values, the house-pet names fill only where empty, the constraints hold, and a second run is
refused whole.

Usage: python testNpcItemsSql.py <schema json> <housingSchema.sql> <npcItems.sql> <npcItemIds.txt>   (a local MariaDB with a `peq`
database, root without a password; npcItemIds.txt lists the NPC items' ids, one per line, with a tab and the name the migration
left them, empty for the ones the content names)
"""
import json, re, subprocess, sys

schemaPath, housingSchemaPath, npcItemsPath, idsPath = sys.argv[1:5]
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


def housingTables():
  text = open(housingSchemaPath, encoding='utf-8').read()
  statements = []
  for table in ('real_estate_npc_items', 'real_estate_npc_item_actions'):
    start = text.index(f'CREATE TABLE IF NOT EXISTS `{table}`')
    end = text.index(';', start) + 1
    statements.append(f'DROP TABLE IF EXISTS `{table}`; ' + text[start:end])
  return ' '.join(reversed(statements)) if False else ' '.join(statements)


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


items = [line.split('\t') for line in open(idsPath, encoding='utf-8').read().split('\n') if line]
sql('DROP TABLE IF EXISTS `real_estate_npc_item_actions`; DROP TABLE IF EXISTS `real_estate_npc_items`;')
sql(createTable('npc_types') + createTable('merchantlist') + createTable('npc_types_tint'))
sql("DROP TABLE IF EXISTS `items`; CREATE TABLE `items` (`id` INT(11) NOT NULL, `housepet_file_name` VARCHAR(31) NOT NULL DEFAULT '', PRIMARY KEY (`id`)) ENGINE=InnoDB;")
sql(housingTables())
sql('INSERT INTO items (id, housepet_file_name) VALUES ' + ', '.join(f"({itemId}, '{name.replace(chr(39), chr(39) * 2)}')" for itemId, name in items) + ';')
print(f'tables made; {len(items)} NPC items in the items table, {sum(1 for _, name in items if not name)} without a name')

content = open(npcItemsPath, encoding='utf-8').read()
sql(content)
check('274 NPC types in the housing block', 'SELECT COUNT(*) FROM npc_types WHERE id BETWEEN 3000000 AND 3999999', '274')
check('274 NPC item rows, each naming its type', 'SELECT COUNT(*) FROM real_estate_npc_items i JOIN npc_types n ON n.id = i.npc_type_id AND n.id = 3000000 + i.item_id', '274')
check('49 merchant rows for the two vendors', 'SELECT merchantid, COUNT(*) FROM merchantlist GROUP BY merchantid ORDER BY merchantid', '3066639\t24\n3066640\t25')
check('3 tint sets, each on its NPC type', 'SELECT COUNT(*) FROM npc_types_tint t JOIN npc_types n ON n.armortint_id = t.id', '3')
check('1000 actions, all on NPC items', 'SELECT COUNT(*) FROM real_estate_npc_item_actions a JOIN real_estate_npc_items i ON i.item_id = a.item_id', '1000')
check('17 training dummies', 'SELECT COUNT(*) FROM real_estate_npc_items WHERE training_dummy = 1', '17')
check("the General Supply Merchant's row", "SELECT name, lastname, race, gender, class, level, size, merchant_id, armortint_green FROM npc_types WHERE id = 3066639",
      'General_Supply_Merchant\tGeneral Supplies\t6\t1\t41\t70\t5\t3066639\t128')
check("the first dummy's surname and skin", "SELECT name, lastname, texture, level, special_abilities FROM npc_types WHERE id = 3066700",
      'Combat_Dummy_Azia\tType - Pre-Planes of Power\t0\t50\t24,1')
check("the Device's race and tag", "SELECT n.race, i.race_tag FROM npc_types n JOIN real_estate_npc_items i ON i.npc_type_id = n.id WHERE n.id = 3066646", '706\tI28')
check('the house-pet names filled only where empty', "SELECT COUNT(*) FROM items WHERE housepet_file_name = ''", '0')
check('a name the migration gave stays', "SELECT housepet_file_name FROM items WHERE id = 7867", 'black_horse')
check('the worg got its name', "SELECT housepet_file_name FROM items WHERE id = 40426", 'a_black_worg_mount')
check("the Assistant's first key is Live's first", 'SELECT item FROM merchantlist WHERE merchantid = 3066640 AND slot = 1', '76005')
error = sql(content, expectError=True)
assert 'npc_types already holds NPC item types' in error, f'a second run was not refused by the guard: {error[:300]}'
print('a second run is refused by the first guard')
check('and changes nothing', 'SELECT COUNT(*) FROM npc_types WHERE id BETWEEN 3000000 AND 3999999', '274')
error = sql("INSERT INTO real_estate_npc_items (item_id, npc_type_id, race_tag) VALUES (1, 3000001, 'TOOLONG')", expectError=True)
assert 'real_estate_npc_item_race_tag' in error or 'Data too long' in error, f'a long race tag was not refused: {error[:200]}'
print('a long race tag is refused')
error = sql('INSERT INTO real_estate_npc_item_actions (item_id, action_id) VALUES (7867, 1)', expectError=True)
assert 'real_estate_npc_item_action_id' in error, f'an unknown action was not refused: {error[:200]}'
print('an unknown action code is refused')
print('all NPC item SQL checks passed')
