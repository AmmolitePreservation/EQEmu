"""Generates the housing entries of common/database/database_update_manifest_custom.cpp from the tested SQL files.

Version 1 is the item tail (sql/itemHousingTail.sql, content database); version 2 the catalog tables and version 3 the state
tables of sql/housingSchema.sql; version 4 Live's item housing values (sql/itemHousingAdoption.sql, content database); version 5
the zone header columns RoF2 reads (sql/zoneHeaderColumns.sql, content database). Each
statement becomes its own raw string literal (MSVC limits one literal to about 16 KB). A schema migration checks for the last
table it creates, so one stopped part way runs again (every table is IF NOT EXISTS); version 4 runs whenever it is pending,
every statement in it repeatable; version 5 runs while the zone table has no ceiling column.

Usage: python makeHousingManifest.py   (prints the entries)
"""
import os, re

here = os.path.dirname(os.path.abspath(__file__))
catalogTables = ['real_estate_definitions', 'real_estate_definition_children', 'real_estate_costs', 'real_estate_cost_options',
                 'real_estate_cost_items', 'real_estate_regions', 'real_estate_zone_templates', 'real_estate_doors', 'real_estate_writs',
                 'real_estate_neighborhood_words',
                 'real_estate_npc_items', 'real_estate_npc_item_actions', 'real_estate_auras', 'real_estate_teleport_keys']


def statements(sql):
  lines = [line for line in sql.splitlines() if not line.startswith('--')]
  return [s.strip().rstrip(';') + ';' for s in '\n'.join(lines).split(';\n') if s.strip()]


def tableStatements(sql):
  byTable = {}
  for statement in statements(sql):
    name = re.match(r'CREATE TABLE IF NOT EXISTS `(\w+)`', statement)
    if not name:
      raise ValueError(f'unexpected statement in the schema: {statement[:60]}')
    byTable[name.group(1)] = statement.rstrip(';').rstrip() + ';'
  return byTable


def literal(statement):
  if len(statement) > 16000:
    raise ValueError(f'a statement of {len(statement)} characters exceeds one literal')
  if ')"' in statement:
    raise ValueError('a statement contains the raw string terminator')
  return 'R"(' + statement + '\n)"'


def entry(version, description, check, condition, pieces, content):
  body = '\n'.join('\t\t\t' + literal(p) for p in pieces)
  return (f'\tManifestEntry{{\n\t\t.version = {version},\n\t\t.description = "{description}",\n\t\t.check = "{check}",\n'
          f'\t\t.condition = "{condition}",\n\t\t.match = "",\n\t\t.sql =\n{body},\n\t\t.content_schema_update = {content},\n\t}},\n')


itemTail = statements(open(os.path.join(here, '..', 'sql', 'itemHousingTail.sql')).read())
itemAdoption = statements(open(os.path.join(here, '..', 'sql', 'itemHousingAdoption.sql')).read())
zoneHeaderColumns = statements(open(os.path.join(here, '..', 'sql', 'zoneHeaderColumns.sql')).read())
schema = tableStatements(open(os.path.join(here, '..', 'sql', 'housingSchema.sql')).read())
stateTables = [t for t in schema if t not in catalogTables]
if set(catalogTables) - set(schema):
  raise ValueError(f'catalog tables missing from the schema: {set(catalogTables) - set(schema)}')
print(entry(1, '2026_09_26_items_housing_tail', "SHOW COLUMNS FROM `items` LIKE 'UNK224'", 'not_empty', itemTail, 'true')
      + entry(2, '2026_09_26_real_estate_catalog', f"SHOW TABLES LIKE '{catalogTables[-1]}'", 'empty', [schema[t] for t in catalogTables], 'true')
      + entry(3, '2026_09_26_real_estate_state', f"SHOW TABLES LIKE '{stateTables[-1]}'", 'empty', [schema[t] for t in stateTables], 'false')
      + entry(4, '2026_10_19_items_housing_live_values', "SHOW TABLES LIKE 'items'", 'not_empty', itemAdoption, 'true')
      + entry(5, '2026_10_20_zone_header_columns', "SHOW COLUMNS FROM `zone` LIKE 'ceiling'", 'empty', zoneHeaderColumns, 'true'), end='')
