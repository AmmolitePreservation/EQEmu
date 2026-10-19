"""Tests sql/itemHousingTail.sql against a local MariaDB: the real `items` definition, the exported rows, every value checked
against itemTailDecode.py, the column order, the capture, a re-run after a mid-way failure, and each guard. Then
sql/itemHousingAdoption.sql on the result: Live's values adopted in ITM-1's counts, nothing else changed, and every run
leaving the same table.

Usage: python testItemHousingTail.py pok-schema-all.json itemTailFull.txt   (a local MariaDB with a `peq` database, root without a
password; itemTailFull.txt is the second result set of the itemTailFull.sql export)
"""
import contextlib, io, json, os, runpy, struct, subprocess, sys

schemaPath, exportPath = sys.argv[1], sys.argv[2]
here = os.path.dirname(os.path.abspath(__file__))
migration = open(os.path.join(here, '..', 'sql', 'itemHousingTail.sql')).read()


def sql(text, expectError=False):
  r = subprocess.run(['mariadb', '-uroot', '-N', 'peq'], input=text, capture_output=True, text=True)
  if (r.returncode != 0) != expectError:
    raise RuntimeError(f"unexpected result ({r.returncode}): {r.stderr[:300]}")
  return r.stdout if not expectError else r.stderr


schema = json.load(open(schemaPath))['items']
def definition(c):
  d = f"`{c['Field']}` {c['Type']} {'NOT NULL' if c['Null'] == 'NO' else 'NULL'}"
  if c['Default'] is not None:
    d += f" DEFAULT '{c['Default']}'" if not c['Default'].lower().startswith('current_timestamp') else f" DEFAULT {c['Default']}"
  return d + (' AUTO_INCREMENT' if 'auto_increment' in (c['Extra'] or '') else '')
createTable = ("DROP TABLE IF EXISTS items_housing_capture; DROP TABLE IF EXISTS items; CREATE TABLE items ("
               + ", ".join(definition(c) for c in schema) + ", PRIMARY KEY (`id`)) ENGINE=InnoDB;")

with contextlib.redirect_stdout(io.StringIO()):
  sys.argv = ['itemTailDecode.py', exportPath]
  decoder = runpy.run_path(os.path.join(here, 'itemTailDecode.py'))
rows = {r['id']: r for r in decoder['rows']}
exportColumns = ['id', 'Name', 'UNK223', 'UNK224', 'UNK225', 'UNK226', 'UNK227', 'UNK228', 'UNK229', 'UNK230', 'UNK231', 'UNK232',
                 'UNK233', 'UNK234', 'placeable', 'UNK236', 'UNK237', 'UNK238', 'UNK239', 'UNK240', 'UNK241']
patterns = [(1041, [0] * 17), (1001, [0, 0, 0, 0, 0, 0, -1, 0, -256, 255, 0, 0, 0, 0, 0, 0, 0]), (1104, [0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0])]
values = [f"({r['id']},'x',{','.join(str(r[c]) for c in exportColumns[2:])})" for r in rows.values()]
values += [f"({i},'x',0,0,{','.join(map(str, p))})" for i, p in patterns]
loadRows = f"INSERT INTO items ({','.join('`' + c + '`' for c in exportColumns)}) VALUES " + ",".join(values) + ";"


def fresh(extra=''):
  sql(createTable + loadRows + extra)


newColumns = ['placeable', 'placeable_ignore_collisions', 'placement_type', 'real_estate_definition_id', 'placeable_scale_min',
              'placeable_scale_max', 'real_estate_upkeep_id', 'max_per_real_estate', 'housepet_file_name', 'trophy_benefit_id',
              'disable_placement_rotation', 'disable_free_placement', 'npc_respawn_interval', 'placeable_default_scale',
              'placeable_default_heading', 'placeable_default_pitch', 'placeable_default_roll', 'interactive_object']
floats = {'placeable_scale_min', 'placeable_scale_max', 'placeable_default_scale', 'placeable_default_heading', 'placeable_default_pitch',
          'placeable_default_roll'}
bits = lambda x: struct.unpack('<I', struct.pack('<f', x))[0]


def expected(i):
  if i not in rows:
    return [0, 0, 0, 0, 0.0, 0.0, 0, -1, '', -1, 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0]
  r, d, f32 = rows[i], decoder['chosen'][i][1], decoder['f32']
  return [r['UNK224'], r['UNK225'], r['UNK226'], r['UNK227'], f32(r['UNK228']), f32(r['UNK229']), r['UNK230'], r['UNK231'], '',
          d['trophy'], d['rotation'], d['free'], d['respawn'], d['scale'], d['heading'], d['pitch'], d['roll'], d['interactive']]


def verify(label):
  select = ", ".join(f"CAST(`{c}` AS DOUBLE)" if c in floats else f"`{c}`" for c in newColumns)
  mismatches = 0
  for line in sql(f"SELECT id, {select} FROM items").splitlines():
    p = line.split('\t'); i = int(p[0])
    for name, e, v in zip(newColumns, expected(i), p[1:]):
      same = bits(e) == bits(float(v)) if name in floats else (str(e) == v if name == 'housepet_file_name' else e == int(v))
      mismatches += not same
  rename = dict(zip(['UNK224', 'UNK225', 'UNK226', 'UNK227', 'UNK228', 'UNK229', 'UNK230', 'UNK231', 'UNK232', 'UNK233', 'UNK234',
                     'placeable', 'UNK236', 'UNK237', 'UNK238', 'UNK239', 'UNK240', 'UNK241'], newColumns))
  order = sql("SELECT COLUMN_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='peq' AND TABLE_NAME='items' ORDER BY ORDINAL_POSITION").split()
  assert order == [rename.get(c['Field'], c['Field']) for c in schema], 'column order changed'
  assert mismatches == 0, f'{label}: {mismatches} values differ from the decoder'
  print(f'{label}: {len(rows) + len(patterns)} rows match the decoder; {len(order)} columns in their original order')


fresh(); sql(migration); verify('fresh run')
statements = [s for s in migration.split(';\n') if s.strip()]
fresh(); sql(';\n'.join(statements[:12]) + ';\n'); sql(migration); verify('re-run after stopping mid-way')
for name, extra in [('non_placeable_item_outside_known_patterns', "INSERT INTO items (id, Name, UNK224, UNK226) VALUES (999001, 'x', 0, 5);"),
                    ('placeable_item_without_one_plausible_layout',
                     "INSERT INTO items (id, Name, UNK224, UNK233, UNK234, placeable, UNK236, UNK237) VALUES (999002, 'x', 12, 7, 99, 9, 12345, 5);")]:
  fresh(extra)
  error = sql(migration, expectError=True)
  intact = sql("SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='peq' AND TABLE_NAME='items' AND COLUMN_NAME='UNK224'").strip()
  assert name in error and intact == '1', f'guard {name} did not stop the migration cleanly: {error[:200]}'
  print(f'guard {name}: stopped before any change')

adoption = open(os.path.join(here, '..', 'sql', 'itemHousingAdoption.sql')).read()
adopted = ['placeable', 'placeable_scale_min', 'placeable_default_heading', 'placeable_default_pitch', 'placeable_default_roll',
           'disable_placement_rotation', 'max_per_real_estate', 'housepet_file_name']
selectAll = 'SELECT id, ' + ', '.join(f'CAST(`{c}` AS DOUBLE)' if c in floats else f'`{c}`' for c in newColumns) + ' FROM items ORDER BY id'


def snapshot():
  return {int(line.split('\t')[0]): dict(zip(newColumns, line.split('\t')[1:])) for line in sql(selectAll).splitlines()}


fresh(); sql(migration); before = snapshot()
sql(adoption); after = snapshot()
changed = lambda columns: sum(any(before[i][c] != after[i][c] for c in columns) for i in after)
counts = dict(masks=changed(['placeable']), scaleMinimums=changed(['placeable_scale_min']),
              orientations=changed(['placeable_default_heading', 'placeable_default_pitch', 'placeable_default_roll']),
              limits=changed(['max_per_real_estate']), named=sum(after[i]['housepet_file_name'] != '' for i in after))
assert counts == dict(masks=485, scaleMinimums=56, orientations=46, limits=4, named=113), f"adopted counts differ from ITM-1's: {counts}"
print(f"adoption: {counts['masks']} masks, {counts['scaleMinimums']} scale minimums, {counts['orientations']} orientations, {counts['limits']} limits, "
      f"{counts['named']} NPC items named; {changed(['disable_placement_rotation'])} rotation locks")
outside = [i for i in after if any(before[i][c] != after[i][c] for c in newColumns if c not in adopted) or (before[i]['placeable'] == '0' and before[i] != after[i])]
assert not outside, f'the adoption changed a column it does not adopt, or an item that is not placeable: {outside[:10]}'
print('adoption: no other column and no non-placeable item changed')
named = {i: (after[i]['placeable_default_heading'], after[i]['placeable_default_pitch'], after[i]['placeable_default_roll']) for i in (67906, 66865, 66866, 66867, 66868)}
assert after[93435]['housepet_file_name'] == 'aquatic_mephit' and named[67906] == ('128', '0', '128') and all(named[i] == ('0', '0', '384') for i in range(66865, 66869)), (after[93435], named)
print("adoption: 93435 is aquatic_mephit; 67906 faces 128, 0, 128 and 66865-66868 0, 0, 384, Live's off-grid orientations")
sql(adoption)
assert snapshot() == after, 'a second run of the adoption changed the table'
pieces = [s for s in adoption.split(';\n') if s.strip()]
fresh(); sql(migration); sql(';\n'.join(pieces[:len(pieces) // 2]) + ';\n'); sql(adoption)
assert snapshot() == after, 'the adoption after stopping mid-way left a different table'
print('adoption: a second run and a run after stopping mid-way leave the same table')
