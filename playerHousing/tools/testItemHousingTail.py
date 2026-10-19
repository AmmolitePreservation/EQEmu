"""Tests sql/itemHousingTail.sql against a local MariaDB: the real `items` definition, every exported row, every value
checked against itemTailDecode.py, the column order, the capture, a re-run after a mid-way failure, and each guard. Then
sql/itemHousingAdoption.sql on the result: Live's values on every Peridot item Live lists as placeable, in the counts measured
on Peridot's 117,944 items, the client record over items.txt, nothing else changed, and every run leaving the same table.

Usage: python testItemHousingTail.py pok-schema-all.json itemTailAll.txt   (a local MariaDB with a `peq` database, root without
a password)

Both inputs come from Peridot's `peq`, read-only. pok-schema-all.json is each table's SHOW COLUMNS as
{table: [{Field, Type, Null, Key, Default, Extra}]}; only `items` is read. itemTailAll.txt is every item's tail:
  echo 'SET SESSION TRANSACTION READ ONLY; SELECT id, Name, UNK223, UNK224, UNK225, UNK226, UNK227, UNK228, UNK229, UNK230,
    UNK231, UNK232, UNK233, UNK234, placeable, UNK236, UNK237, UNK238, UNK239, UNK240, UNK241 FROM items ORDER BY id;' |
    mariadb -B -uroot peq > itemTailAll.txt
"""
import contextlib, io, json, os, runpy, struct, subprocess, sys, tempfile

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

exportLines = [line for line in open(exportPath).read().split('\n') if line]
exportColumns = ['id', 'Name', 'UNK223', 'UNK224', 'UNK225', 'UNK226', 'UNK227', 'UNK228', 'UNK229', 'UNK230', 'UNK231', 'UNK232',
                 'UNK233', 'UNK234', 'placeable', 'UNK236', 'UNK237', 'UNK238', 'UNK239', 'UNK240', 'UNK241']
assert exportLines[0].split('\t') == exportColumns, 'the export does not start with its column names'
exported = [line.split('\t') for line in exportLines[1:]]
with tempfile.NamedTemporaryFile('w', suffix='.txt', delete=False) as placeableExport:
  placeableExport.write('\n'.join([exportLines[0]] + [line for line, p in zip(exportLines[1:], exported) if p[3] != '0']) + '\n')
with contextlib.redirect_stdout(io.StringIO()):
  sys.argv = ['itemTailDecode.py', placeableExport.name]
  decoder = runpy.run_path(os.path.join(here, 'itemTailDecode.py'))
os.unlink(placeableExport.name)
rows = {r['id']: r for r in decoder['rows']}
values = [f"({p[0]},'{p[1].replace(chr(39), chr(39) * 2)}',{','.join(p[2:])})" for p in exported]
loadRows = ''.join(f"INSERT INTO items ({','.join('`' + c + '`' for c in exportColumns)}) VALUES " + ','.join(values[k:k + 10000]) + ';'
                   for k in range(0, len(values), 10000))


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
  mismatches, checked = 0, 0
  for line in sql(f"SELECT id, {select} FROM items").splitlines():
    p = line.split('\t'); i = int(p[0]); checked += 1
    for name, e, v in zip(newColumns, expected(i), p[1:]):
      same = bits(e) == bits(float(v)) if name in floats else (str(e) == v if name == 'housepet_file_name' else e == int(v))
      mismatches += not same
  rename = dict(zip(exportColumns[3:], newColumns))
  order = sql("SELECT COLUMN_NAME FROM information_schema.COLUMNS WHERE TABLE_SCHEMA='peq' AND TABLE_NAME='items' ORDER BY ORDINAL_POSITION").split()
  assert order == [rename.get(c['Field'], c['Field']) for c in schema], 'column order changed'
  assert checked == len(exported) and mismatches == 0, f'{label}: {checked} rows, {mismatches} values differ from the decoder'
  print(f'{label}: {checked} rows match the decoder; {len(order)} columns in their original order')


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
sql('DROP TABLE IF EXISTS items_before; CREATE TABLE items_before AS SELECT * FROM items;')
sql(adoption); after = snapshot()
changed = lambda columns: sum(any(before[i][c] != after[i][c] for c in columns) for i in after)
counts = dict(items=changed(adopted), masks=changed(['placeable']), scaleMinimums=changed(['placeable_scale_min']),
              orientations=changed(['placeable_default_heading', 'placeable_default_pitch', 'placeable_default_roll']),
              rotationLocks=changed(['disable_placement_rotation']), limits=changed(['max_per_real_estate']),
              named=sum(after[i]['housepet_file_name'] != '' for i in after),
              newlyPlaceable=sum(before[i]['placeable'] == '0' and after[i]['placeable'] != '0' for i in after))
assert counts == dict(items=11095, masks=10846, scaleMinimums=1365, orientations=100, rotationLocks=76, limits=24, named=568,
                      newlyPlaceable=9119), f'adopted counts differ from those measured: {counts}'
print(f"adoption: {counts['items']} items changed: {counts['masks']} masks ({counts['newlyPlaceable']} newly placeable), "
      f"{counts['scaleMinimums']} scale minimums, {counts['orientations']} orientations, {counts['rotationLocks']} rotation locks, "
      f"{counts['limits']} limits; {counts['named']} NPC items named")
kept = [c['Field'] for c in schema if c['Field'] not in exportColumns[3:]] + [c for c in newColumns if c not in adopted]
differing = sql('SELECT COUNT(*) FROM items a JOIN items_before b ON b.id = a.id WHERE NOT ('
                + ' AND '.join(f'a.`{c}` <=> b.`{c}`' for c in kept) + ')').strip()
total = sql('SELECT COUNT(*) FROM items').strip()
sql('DROP TABLE items_before;')
assert differing == '0' and total == str(len(exported)) and sorted(after) == sorted(before), (differing, total)
print(f'adoption: {len(kept)} other columns unchanged on all {total} items; no item added')
housing = lambda i: tuple(after[i][c] for c in ('placeable', 'placeable_default_heading', 'placeable_default_pitch', 'placeable_default_roll'))
assert before[67906]['placeable'] == '0' and housing(67906) == ('142', '128', '0', '128'), after[67906]
assert before[66868]['placeable'] == '0' and all(housing(i) == ('142', '0', '0', '384') for i in range(66865, 66869)), \
  [after[i] for i in range(66865, 66869)]
print('adoption: 67906, not placeable before, is 142 facing 128, 0, 128; 66865-66868 are 142 facing 0, 0, 384')
pets = {i: (before[i]['placeable'], after[i]['placeable'], after[i]['housepet_file_name']) for i in (47715, 11164)}
assert pets == {47715: ('0', '142', 'a_familiar_floating_skull'), 11164: ('0', '142', 'a_familiar_of_Zatozia')}, pets
assert 93435 not in after, 'an item Live lists and Peridot lacks appeared'
print('adoption: 47715 named a_familiar_floating_skull from its client record and 11164 a_familiar_of_Zatozia from items.txt, '
      'neither placeable before; 93435 stays absent')
masks = {i: (before[i]['placeable'], after[i]['placeable']) for i in (55446, 55448)}
assert masks == {55446: ('-1', '-1'), 55448: ('0', '-1')}, masks
print("adoption: 55446 and 55448 take the client record's mask -1 over items.txt's 255")
sql(adoption)
assert snapshot() == after, 'a second run of the adoption changed the table'
pieces = [s for s in adoption.split(';\n') if s.strip()]
fresh(); sql(migration); sql(';\n'.join(pieces[:len(pieces) // 2]) + ';\n'); sql(adoption)
assert snapshot() == after, 'the adoption after stopping mid-way left a different table'
print('adoption: a second run and a run after stopping mid-way leave the same table')
