"""The item_instances table the housing tables refer to, for the tools' scratch databases: the SQL of the code's migration that
creates it, and the real_estate holder housing adds to it (sql/realEstateHolder.sql).
"""
import os, re

here = os.path.dirname(os.path.abspath(__file__))
holderSql = open(os.path.join(here, '..', 'sql', 'realEstateHolder.sql'), encoding='utf-8').read()


def createSql():
  manifest = open(os.path.join(here, '..', '..', 'common', 'database', 'database_update_manifest.cpp'), encoding='utf-8').read()
  creating = [sql for sql in re.findall(r'\.sql = R"\((.*?)\)"', manifest, re.S) if 'CREATE TABLE `item_instances`' in sql]
  if len(creating) != 1:
    raise ValueError(f'database_update_manifest.cpp has {len(creating)} migrations creating item_instances, not one')
  return 'DROP TABLE IF EXISTS `item_instances`;' + creating[0]
