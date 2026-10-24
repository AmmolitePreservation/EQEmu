"""Tests makeDbStringsSql.py on its data: the 27 training dummy labels, their ids and texts, the regenerated SQL and the refusals.

Usage: python testDbStrings.py --live <livedumps clone>
"""
import argparse, os, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeDbStringsSql as generator

parser = argparse.ArgumentParser(description='Test makeDbStringsSql.py.')
parser.add_argument('--live', required=True, help='a livedumps clone')
live = parser.parse_args().live


def check(label, actual, expected):
  if actual != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
  print(f'{label}: {actual!r}')


labels = generator.dummyLabels(live)
with open(generator.sqlPath, encoding='utf-8') as file:
  sql = file.read()
check("sql/dbStrings.sql is the generator's output", sql == generator.renderSql(labels), True)
check('27 labels: the four submenus, their 22 actions and Report Statistics', len(labels), 27)
check('the ids Live uses', sorted(labels), [100, 101, 102, 103, 104, 105, 106, 200, 201, 202, 203, 204, 205, 300, 400, 401, 402, 403, 404, 405, 406, 407, 500, 501, 502, 503, 504])
check('the submenus', (labels[100], labels[200], labels[400], labels[500]), ('Change Bodytype', 'Modify Armor Class', 'Modify Level', 'Modify HP'))
check('the body types', [labels[i] for i in range(101, 107)], ['No Bodytype', 'Animal', 'Construct', 'Humanoid', 'Plant', 'Undead'])
check('the hit point steps', (labels[501], labels[504]), ('Reset to Base HP', 'Set to x1000 of Base HP'))
check('Report Statistics stands alone', labels[300], 'Report Statistics')
check('every row is category 28', sql.count(', 28, '), 27)
check('the SQL refuses existing rows', "WHERE type = 28 AND id IN (100, 101" in sql, True)
check('a quote is doubled', generator.quote("Brell's"), "'Brell''s'")
try:
  generator.dummyLabels(os.path.join(live, 'nowhere'))
  raise AssertionError('a missing clone did not raise')
except FileNotFoundError:
  print('a missing clone raises')
print('all dbstr checks pass')
