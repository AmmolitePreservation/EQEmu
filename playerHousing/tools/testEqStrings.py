"""Tests makeEqStringsSql.py: the three coin lines, the two reclaim strings (Live's text with 13333's guild clause left for Q15), the guards,
the refusals.

Usage: python testEqStrings.py
"""
import os, sys, tempfile

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeEqStringsSql as generator


def check(label, actual, expected):
  if actual != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
  print(f'{label}: {actual!r}')


def expectRaises(label, rows, fragment):
  path = os.path.join(tempfile.mkdtemp(), 'changes.tsv')
  with open(path, 'w', encoding='utf-8', newline='\n') as file:
    file.write('id\ttext\tsource\n' + ''.join(f'{i}\t{t}\ts\n' for i, t in rows))
  try:
    generator.readChanges(path)
  except ValueError as error:
    if fragment not in str(error):
      raise AssertionError(f'{label}: raised {error!r}, which lacks {fragment!r}')
    print(f'{label}: raises ({error})')
    return
  raise AssertionError(f'{label}: did not raise')


changes = generator.readChanges()
with open(generator.sqlPath, encoding='utf-8') as file:
  sql = file.read()
check("sql/eqStrings.sql is the generator's output", sql == generator.renderSql(changes), True)
check('the three coin lines and the two reclaim labels change', sorted(changes), [6424, 6426, 6429, 13333, 13334])
check('each coin line runs its word into the coin text, which RoF2 starts with a space',
      (changes[6424], changes[6426], changes[6429]), ('You deposit%$1 into escrow.', 'You withdraw%$1 from escrow.', 'You spend%$1 to purchase the plot.'))
check("13333 is Live's escrow rule without the guild-village clause",
      changes[13333], 'Plots will pack up all items, send them to the owner, and become available for purchase again if escrow has been expired for 90 days.')
check("13334 is Live's guild plot rule", changes[13334].startswith('Guild plots will pack up all items') and changes[13334].endswith('or 90 days after escrow expires.'), True)
check('the SQL refuses an empty table and a missing id', ('IF NOT EXISTS (SELECT 1 FROM eq_str)' in sql, 'WHERE id IN (6424, 6426, 6429, 13333, 13334)) <> 5' in sql), (True, True))
check('each change is one update', sql.count('UPDATE eq_str SET value = '), 5)
expectRaises('an id that is not a number', [('abc', 'x')], 'not a number')
expectRaises('an empty text', [('5', '')], 'no text')
expectRaises('a repeated id', [('5', 'a'), ('5', 'b')], 'listed twice')
expectRaises('no change at all', [], 'lists no change')
print('all eqstr checks pass')
