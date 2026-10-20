"""Tests makeGuildServicesSql.py on its data: regeneration, the pool aura's values, which keys are written, Live's landings and
the heading conversion, and the refusals.

Usage: python testGuildServices.py
"""
import os, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeGuildServicesSql as generator


def check(label, actual, expected):
  if actual != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
  print(f'{label}: {actual!r}')


def expectRaises(label, action, fragment):
  try:
    action()
  except ValueError as error:
    if fragment not in str(error):
      raise AssertionError(f'{label}: raised {error!r}, which lacks {fragment!r}')
    print(f'{label}: raises ({error})')
    return
  raise AssertionError(f'{label}: did not raise')


rows = generator.readDestinations()
keys = generator.keys(rows)
byItem = {key['itemId']: key for key in keys}
with open(generator.sqlPath, encoding='utf-8') as file:
  check('sql/guildServices.sql is the generator\'s output', file.read() == generator.render(keys), True)
check('Live lists 37 keys', len(rows), 37)
check('24 keys to zones RoF2 has and the guild standard splinter are written', len(keys), 25)
check('no key lands in a zone RoF2 lacks', sorted(r['zone'] for r in rows if int(r['itemId']) not in byItem and r['zone']), sorted(generator.zonesRoF2Lacks))
check('the two keys Live refused are left out', sorted(int(r['itemId']) for r in rows if not r['zone'] and int(r['itemId']) != generator.guildStandardKey), [88994, 88995])
check('the splinter goes to the guild standard, not a point', (byItem[88737]['guildStandard'], byItem[88737]['zoneId'], byItem[88737]['destination']), (1, 0, 'your Guild Standard'))
check("the Commonlands pebble lands at Live's point", (byItem[76001]['zoneId'], byItem[76001]['x'], byItem[76001]['y'], byItem[76001]['z']), (408, 1783.0, 223.0, 38.23))
check('a heading of 135 degrees is 320', byItem[76001]['heading'], 320.0)
check('a heading of 180 degrees is 256', byItem[76000]['heading'], 256.0)
check('a heading of 360 degrees is 0', byItem[88736]['heading'], 0.0)
check('a heading of 90 degrees is 384', generator.eqHeading(90.0), 384.0)
check("the dialog's and the alignment's text is Live's", byItem[76001]['destination'], 'the Commonlands')
check('a name with an apostrophe is quoted', generator.quote("Brell's Rest"), "'Brell''s Rest'")
check("the pool keeps Aura of the Hall within 35 units", (generator.poolItem, generator.poolSpell, generator.poolRadius), (66645, 31874, 35.0))
expectRaises('a heading above 360 degrees', lambda: generator.eqHeading(361.0), 'outside 0-360')
landed = dict(rows[0])
expectRaises('a landing without a zone id', lambda: generator.keys([dict(landed, zoneId='')]), 'with no zone id')
expectRaises('a key listed twice', lambda: generator.keys([landed, landed]), 'listed twice')
splinter = next(r for r in rows if int(r['itemId']) == generator.guildStandardKey)
expectRaises('a splinter with a landing', lambda: generator.keys([dict(splinter, zone='commonlands')]), 'fixed point')
print('all guild services checks pass')
