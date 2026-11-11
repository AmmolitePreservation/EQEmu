"""The item_instances table the housing tables refer to, for the tools' scratch databases: the SQL of the code's migration 9328,
and the real_estate holder housing adds to it (sql/realEstateHolder.sql).
"""
import os, re

here = os.path.dirname(os.path.abspath(__file__))
holderSql = open(os.path.join(here, '..', 'sql', 'realEstateHolder.sql'), encoding='utf-8').read()


def createSql():
  manifest = open(os.path.join(here, '..', '..', 'common', 'database', 'database_update_manifest.cpp'), encoding='utf-8').read()
  migration = re.search(r'\.version = 9328,.*?\.sql = R"\((.*?)\)"', manifest, re.S)
  if not migration or 'CREATE TABLE `item_instances`' not in migration.group(1):
    raise ValueError('migration 9328 in database_update_manifest.cpp does not create item_instances')
  return 'DROP TABLE IF EXISTS `item_instances`;' + migration.group(1)
