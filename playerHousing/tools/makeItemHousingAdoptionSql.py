"""Writes sql/itemHousingAdoption.sql, the migration that gives every Peridot item Live lists as placeable Live's housing values (ITM-1).

Sources, from a livedumps clone: master/fields/itemDefinitions.tsv (the Live client's own item records) and
itemsTxt/itemsTxtExtract.tsv (MQ2LinkDB's items.txt, its housing fields decoded under the client's names). For an id in both,
the client's record wins (ITM-11, ITM-12). Every Live id with a placeable mask is staged; each item in Peridot's table,
whatever its own mask, takes the mask, placement type, ignore-collisions, scale range and default, default heading, pitch and
roll, rotation and free-placement locks, per-property limit and house-pet name; no other column changes. A record whose scale
minimum is above its maximum is refused. Every statement can be repeated.

Usage: python makeItemHousingAdoptionSql.py --live <livedumps clone>
"""
import argparse, csv, math, os

here = os.path.dirname(os.path.abspath(__file__))
statementLimit = 15000
# Each adopted column: its definition, the client record's field and the items.txt extract's column.
adopted = [('placeable', 'INT(11) NOT NULL', 'Placeable', 'placeablebitfield', int),
           ('placement_type', 'INT(11) NOT NULL', 'PlacementType', 'PlacementType', int),
           ('placeable_ignore_collisions', 'TINYINT(3) UNSIGNED NOT NULL', 'bPlaceableIgnoreCollisions', 'bPlaceableIgnoreCollisions', int),
           ('placeable_scale_min', 'FLOAT NOT NULL', 'PlaceableScaleRangeMin', 'PlaceableScaleRangeMin', float),
           ('placeable_scale_max', 'FLOAT NOT NULL', 'PlaceableScaleRangeMax', 'PlaceableScaleRangeMax', float),
           ('placeable_default_scale', 'FLOAT NOT NULL', 'PlaceableDefScale', 'PlaceableDefScale', float),
           ('placeable_default_heading', 'FLOAT NOT NULL', 'PlaceableDefHeading', 'PlaceableDefHeading', float),
           ('placeable_default_pitch', 'FLOAT NOT NULL', 'PlaceableDefPitch', 'PlaceableDefPitch', float),
           ('placeable_default_roll', 'FLOAT NOT NULL', 'PlaceableDefRoll', 'PlaceableDefRoll', float),
           ('disable_placement_rotation', 'TINYINT(3) UNSIGNED NOT NULL', 'bDisablePlacementRotation', 'bDisablePlacementRotation', int),
           ('disable_free_placement', 'TINYINT(3) UNSIGNED NOT NULL', 'bDisableFreePlacement', 'bDisableFreePlacement', int),
           ('max_per_real_estate', 'INT(11) NOT NULL', 'MaxPerRealEstate', 'MaxPerRealEstate', int),
           ('housepet_file_name', 'VARCHAR(31) NOT NULL', 'HousepetFileName', 'placeablenpcname', str)]
capturedFields = [(column, definition, client, kind) for column, definition, client, _, kind in adopted]
listedFields = [(column, definition, extract, kind) for column, definition, _, extract, kind in adopted]


def literal(value, kind, itemId, field):
  if kind is str:
    if len(value) > 31 or not all(32 <= ord(ch) < 127 for ch in value):
      raise ValueError(f'item {itemId}: {field} {value!r} is not a printable ASCII name of at most 31 characters')
    return "'" + value.replace('\\', '\\\\').replace("'", "''") + "'"
  if kind is float:
    if not math.isfinite(float(value)):
      raise ValueError(f'item {itemId}: {field} {value!r} is not a finite number')
    return value
  if str(int(value)) != value:
    raise ValueError(f'item {itemId}: {field} {value!r} is not an integer')
  return value


def stagedRows(rows, idColumn, fields):
  staged = []
  source = {name: field for name, _, field, _ in fields}
  for row in rows:
    itemId = row[idColumn]
    if not itemId.isdigit():
      raise ValueError(f'item id {itemId!r} is not a number')
    minimum, maximum = float(row[source['placeable_scale_min']]), float(row[source['placeable_scale_max']])
    if minimum > maximum:
      raise ValueError(f'item {itemId}: its scale minimum {minimum} is above its maximum {maximum}')
    staged.append(f"({itemId}, {', '.join(literal(row[source], kind, itemId, name) for name, _, source, kind in fields)})")
  return staged


def inserts(table, fields, rows):
  head = f"REPLACE INTO `{table}` (`id`, {', '.join(f'`{name}`' for name, *_ in fields)}) VALUES "
  statements, current = [], []
  for row in rows:
    if current and len(head) + sum(len(r) + 2 for r in current) + len(row) + 1 > statementLimit:
      statements.append(head + ', '.join(current) + ';')
      current = []
    current.append(row)
  if current:
    statements.append(head + ', '.join(current) + ';')
  return statements


def main():
  parser = argparse.ArgumentParser(description='Write sql/itemHousingAdoption.sql.')
  parser.add_argument('--live', required=True, help='a livedumps clone')
  live = parser.parse_args().live
  captured = list(csv.DictReader(open(os.path.join(live, 'master', 'fields', 'itemDefinitions.tsv'), encoding='latin-1'), delimiter='\t'))
  listed = list(csv.DictReader(open(os.path.join(live, 'itemsTxt', 'itemsTxtExtract.tsv'), encoding='latin-1'), delimiter='\t', quoting=csv.QUOTE_NONE))
  capturedIds = {row['itemId'] for row in captured}
  if len(capturedIds) != len(captured):
    raise ValueError('an item id repeats in the client records')
  captured = sorted((row for row in captured if row['Placeable'] != '0'), key=lambda row: int(row['itemId']))
  listed = [row for row in listed if row['placeablebitfield'] != '0']
  if len({row['id'] for row in listed}) != len(listed):
    raise ValueError('a placeable item id repeats in items.txt')
  listed = sorted((row for row in listed if row['id'] not in capturedIds), key=lambda row: int(row['id']))
  tables = [('items_housing_live_captured', capturedFields, stagedRows(captured, 'itemId', capturedFields)),
            ('items_housing_live_listed', listedFields, stagedRows(listed, 'id', listedFields))]
  out = ['-- Generated by playerHousing/tools/makeItemHousingAdoptionSql.py: Live\'s housing values for every Peridot item Live lists as placeable (ITM-1).']
  for table, fields, _ in tables:
    out.append(f"CREATE TABLE IF NOT EXISTS `{table}` (`id` INT(11) NOT NULL, "
               + ', '.join(f'`{name}` {definition}' for name, definition, *_ in fields) + ', PRIMARY KEY (`id`));')
  for table, fields, rows in tables:
    out += inserts(table, fields, rows)
  for table, fields, _ in tables:
    out.append(f"UPDATE `items` i JOIN `{table}` l ON l.`id` = i.`id` SET "
               + ', '.join(f'i.`{name}` = l.`{name}`' for name, *_ in fields) + ';')
  out += [f'DROP TABLE IF EXISTS `{table}`;' for table, *_ in tables]
  sql = '\n'.join(out) + '\n'
  open(os.path.join(here, '..', 'sql', 'itemHousingAdoption.sql'), 'w', encoding='ascii').write(sql)
  print(f'wrote sql/itemHousingAdoption.sql: {len(tables[0][2])} items from client records, {len(tables[1][2])} from items.txt')


if __name__ == '__main__':
  main()
