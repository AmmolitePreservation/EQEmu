"""Generates the housing entries of common/database/database_update_manifest_custom.cpp from the tested SQL files.

Version 1 is the item tail (sql/itemHousingTail.sql, content database); version 2 the catalog tables and version 3 the state
tables of sql/housingSchema.sql; version 4 Live's item housing values (sql/itemHousingAdoption.sql, content database); each
version after them adds columns to one EQEmu table (columnFiles: the zone header fields RoF2 reads, the doors' RealEstateDoorID,
the safe return's property, the trophy tribute columns) and runs while the table lacks the first column it adds. Each statement becomes its own raw string
literal (MSVC limits one literal to about 16 KB). A schema migration checks for the last table it creates, so one stopped part
way runs again (every table is IF NOT EXISTS); version 4 runs whenever it is pending, every statement in it repeatable.

Usage: python makeHousingManifest.py   (prints the entries)
"""
import os, re

here = os.path.dirname(os.path.abspath(__file__))
catalogTables = ['real_estate_definitions', 'real_estate_definition_children', 'real_estate_costs', 'real_estate_cost_options',
                 'real_estate_cost_items', 'real_estate_regions', 'real_estate_zone_templates', 'real_estate_writs',
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


# (version, description, SQL file, the table, the first column the file adds, content database)
columnFiles = [
  (5, '2026_10_20_zone_header_columns', 'zoneHeaderColumns.sql', 'zone', 'ceiling', 'true'),
  (6, '2026_10_23_doors_real_estate_door_id', 'doorColumns.sql', 'doors', 'real_estate_door_id', 'true'),
  (7, '2026_10_23_safe_return_property_id', 'safeReturnColumns.sql', 'character_instance_safereturns', 'property_id', 'false'),
  (8, '2026_10_23_character_tribute_trophy_columns', 'characterTributeColumns.sql', 'character_tribute', 'kind', 'false'),
  (9, '2026_10_23_character_data_trophy_columns', 'characterDataColumns.sql', 'character_data', 'trophy_tribute_active', 'false'),
  (10, '2026_10_23_guild_members_trophy_column', 'guildMembersColumns.sql', 'guild_members', 'trophy_tribute_enable', 'false'),
  (11, '2026_10_23_guild_tributes_trophy_columns', 'guildTributesColumns.sql', 'guild_tributes', 'trophy_id_1', 'false'),
]

itemTail = statements(open(os.path.join(here, '..', 'sql', 'itemHousingTail.sql')).read())
itemAdoption = statements(open(os.path.join(here, '..', 'sql', 'itemHousingAdoption.sql')).read())
schema = tableStatements(open(os.path.join(here, '..', 'sql', 'housingSchema.sql')).read())
stateTables = [t for t in schema if t not in catalogTables]
if set(catalogTables) - set(schema):
  raise ValueError(f'catalog tables missing from the schema: {set(catalogTables) - set(schema)}')
print(entry(1, '2026_09_26_items_housing_tail', "SHOW COLUMNS FROM `items` LIKE 'UNK224'", 'not_empty', itemTail, 'true')
      + entry(2, '2026_09_26_real_estate_catalog', f"SHOW TABLES LIKE '{catalogTables[-1]}'", 'empty', [schema[t] for t in catalogTables], 'true')
      + entry(3, '2026_09_26_real_estate_state', f"SHOW TABLES LIKE '{stateTables[-1]}'", 'empty', [schema[t] for t in stateTables], 'false')
      + entry(4, '2026_10_19_items_housing_live_values', "SHOW TABLES LIKE 'items'", 'not_empty', itemAdoption, 'true')
      + ''.join(entry(version, description, f"SHOW COLUMNS FROM `{table}` LIKE '{column}'", 'empty',
                      statements(open(os.path.join(here, '..', 'sql', file)).read()), content)
                for version, description, file, table, column, content in columnFiles), end='')
