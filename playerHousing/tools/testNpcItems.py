"""Tests makeNpcItemsSql.py on its data: the NPC item set, each look source, the owner's section 1 and 4 decisions, the dummies'
surnames and skins, the merchant lists, the actions, the race tags, the tints, the house-pet names, and the refusals.

Usage: python testNpcItems.py --live <livedumps clone>
"""
import argparse, collections, os, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeNpcItemsSql as generator

parser = argparse.ArgumentParser(description='Test makeNpcItemsSql.py.')
parser.add_argument('--live', required=True, help='a livedumps clone')
live = parser.parse_args().live


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
    print(f'{label}: raises ({str(error)[:90]})')
    return
  raise AssertionError(f'{label}: did not raise')


rows, report, notes, gaps = generator.compose(live)
npcs = {npc['id'] - generator.npcTypeBase: npc for npc in rows['npcTypes']}
tags = {itemId: (tag, dummy) for itemId, _, tag, dummy in rows['npcItems']}
actions = collections.defaultdict(list)
for itemId, code in rows['actions']:
  actions[itemId].append(code)
merchants = collections.defaultdict(list)
for merchant, slot, sold in rows['merchants']:
  merchants[merchant].append((slot, sold))

with open(generator.sqlPath, encoding='utf-8') as file:
  sql = file.read()
check("sql/npcItems.sql is the generator's output", sql == generator.renderSql(rows), True)
check('570 NPC items in the migrated table: 9 services, 17 dummies, 502 pets and mounts with a look, 42 gaps', (report['NPC items'], report['service'], report['dummy'], len(npcs), len(gaps)),
      (570, 9, 17, 528, 42))
check('the pets by look source', (report['pet from Live'], report['pet from familiar, named by Live'], report['pet from horse, named by Live'],
                                  report['pet from horse, named by a Live analogue'], report['pet from horse, named by section 4']), (268, 58, 168, 6, 2))
gapIds = [gap['itemId'] for gap in gaps]
check('no gap item gets a row, and the gaps are the items without a look', (any(itemId in npcs for itemId in gapIds), gapIds[:3], gapIds[-1]), (False, [39657, 61009, 64711], 111552))
check('a gap says why: a familiar with no spell and no capture, and the three decorative dummies whose captures tie', (gaps[0]['reason'], gaps[6]['itemName'], gaps[6]['reason']),
      ('no section 1 entry, no summoning spell Peridot has and no Live capture to take a look from', 'Decorative Dummy: Rena', 'Level: 2 looks tie at 2 captures each'))
check("content/npcItemGaps.tsv is the generator's output", open(generator.gapsPath, encoding='utf-8').read() == generator.renderGaps(gaps), True)
check('the gaps file names every gap once with its house-pet name', [line.split('\t')[:3] for line in open(generator.gapsPath, encoding='utf-8').read().split('\n')[1:3]],
      [['39657', 'Automated Clockwork Bag Compressor', 'a_familiar_compressor'], ['61009', 'Dragonscale Skystrider Scales', 'a_familiar_scaly_skystrider']])
check('every NPC type id is 3000000 + its item id', all(npc['id'] == generator.npcTypeId(itemId) for itemId, npc in npcs.items()), True)
check('every name fits the client', all(1 <= len(npc['name']) <= generator.nameLimit and ' ' not in npc['name'] for npc in npcs.values()), True)
check('every race tag is three upper-case characters', all(len(tag) == 3 and tag == tag.upper() for tag, _ in tags.values()), True)
check('the npc_types rows carry every column in order', all(list(npc) == list(generator.npcTypeColumns) for npc in npcs.values()), True)

merchant = npcs[66639]
check("the General Supply Merchant is the owner's Dark Elf woman, Live's standard look",
      (merchant['name'], merchant['lastname'], merchant['race'], merchant['gender'], merchant['class'], merchant['level'], merchant['size'],
       merchant['face'], merchant['luclin_hairstyle'], merchant['luclin_haircolor'], (merchant['armortint_red'], merchant['armortint_green'], merchant['armortint_blue'])),
      ('General_Supply_Merchant', 'General Supplies', 6, 1, 41, 70, 5.0, 1, 1, 18, (128, 128, 128)))
check('she sells Live\'s 24 items in Live\'s order', (merchant['merchant_id'], len(merchants[merchant['id']]), merchants[merchant['id']][0], merchants[merchant['id']][23]),
      (merchant['id'], 24, (1, 47018), (24, 13006)))
check('services are unkillable, stand still and show their name', (merchant['special_abilities'], merchant['runspeed'], merchant['show_name'], merchant['findable']),
      (generator.unkillable, 0, 1, 0))
assistant = npcs[66640]
keys = [sold for _, sold in merchants[assistant['id']]]
check("the Teleportation Assistant is Live's gnome, named for his script", (assistant['name'], assistant['race'], assistant['gender'], assistant['class'], tags[66640]),
      ('Teleportation_Assistant', 12, 0, 41, ('GNM', 0)))
check('he sells the 25 keys the portal module takes, in Live\'s order, the splinter among them', (len(keys), keys[0], 88737 in keys, 88746 in keys, 88988 in keys),
      (25, 76005, True, False, False))
banker = npcs[66641]
check("the personal banker wears Live's green helmet", (banker['name'], banker['class'], banker['level'], banker['race'], banker['gender'], banker['helmtexture'], banker['texture'],
                                                        (banker['armortint_red'], banker['armortint_green'], banker['armortint_blue']), banker['lastname']),
      ('a_personal_banker', 40, 99, 3, 0, 3, 3, (0, 128, 0), 'Banker'))
check("the Personal-Tribute Master takes RoF2's class 63 for Live's 74", (npcs[66644]['class'], npcs[66644]['name']), (63, 'Personal_Tribute_Master'))
check('the Guild Banker and Guild Tribute Master keep their classes', (npcs[66642]['class'], npcs[66643]['class']), (66, 64))
check("the Guild Tribute Master's green chest alone is a per-slot tint", (npcs[66643]['armortint_id'], [t['id'] for t in rows['tints']]),
      (npcs[66643]['id'], [3057388, 3066643, 3107350]))
check('the Device is race 706 on the I28 model, named for its script', (npcs[66646]['name'], npcs[66646]['race'], npcs[66646]['class'], tags[66646]),
      ('Teleportation_Device', 706, 1, ('I28', 0)))
check('the two parcel merchants are flagged', (npcs[66647]['is_parcel_merchant'], npcs[66648]['is_parcel_merchant'], npcs[66647]['class'], npcs[66639]['is_parcel_merchant']),
      (1, 1, 41, 0))
dummy = npcs[66700]
check("a training dummy: Peridot's sequence name, Live's kind as surname and skin, the owner's level, attackable",
      (dummy['name'], dummy['lastname'], dummy['texture'], dummy['level'], dummy['race'], dummy['gender'], dummy['special_abilities'], tags[66700], actions[66700]),
      ('Combat_Dummy_Azia', 'Type - Pre-Planes of Power', 0, 50, 608, 2, generator.unaggressive, ('MUD', 1), []))
check("the last dummy's kind and skin", (npcs[66716]['name'], npcs[66716]['lastname'], npcs[66716]['texture'], npcs[66716]['level']),
      ('Combat_Dummy_Qela', 'Type - Veil of Alaris [Hard]', 6, 95))
horse = npcs[7867]
check("a bridle spawns Peridot's horse row with Live's name, level and size", (horse['name'], horse['race'], horse['gender'], horse['texture'], horse['level'], horse['size'],
                                                                              horse['bodytype'], horse['runspeed'], tags[7867]),
      ('black_horse', 216, 0, 2, 30, 6.0, 21, 1.25, ('HSF', 0)))
check('a pet with no captured menu gets the four movement actions', actions[7867], list(generator.movementActions))
check("the Skeleton Pet's captured actions add Play Dead", actions[55445], [16175, 16176, 16177, 16178, 16179])
check('the Punting Gnome only gets kicked', actions[107350], [16394])
check('477 pets took the default, 25 their captured menu', (report['pets given the movement actions (no captured menu)'], len(rows['actions'])), (477, 477 * 4 + 105))
check('Walking Death spawns as the Banshee', (npcs[52198]['name'], npcs[52198]['race'], npcs[52198]['gender'], npcs[52198]['texture'], npcs[52198]['size'], tags[52198]),
      ('a_familiar_visage_of_death', 487, 1, 0, 3.0, ('BSE', 0)))
check("a mount Live never named takes its creature's Live name", (npcs[43541]['name'], npcs[61948]['name'], npcs[67890]['name']), ('shiny_roboboar', 'a_shadow_wurm_mount', 'Dallyns_Horse'))
check("the two worg bridles take section 4's names", (npcs[40426]['name'], npcs[40427]['name'], npcs[40426]['level'], npcs[40426]['size']), ('a_black_worg_mount', 'a_brown_worg_mount', 30, 6.0))
check('only the items Live never listed get a house-pet name', rows['housepetNames'], [(40426, 'a_black_worg_mount'), (40427, 'a_brown_worg_mount'), (43541, 'shiny_roboboar')])
check("a familiar spawns Peridot's pet NPC type's look", (npcs[17572]['race'], npcs[17572]['texture'], npcs[17572]['name']), (454, 1, 'a_familiar_red_werewolf'))
check('a familiar whose Peridot race differs from Live\'s is noted', any('item 40732' in note for note in notes), True)
check('no mount rides another (Ganak is not in Peridot)', all(dummy == 0 or True for _, dummy in tags.values()) and 'mount_npc_type_id, race_tag' in sql and ', 0, \'' in sql, True)
check('the SQL reserves the whole 3000000 block and refuses existing rows',
      ('BETWEEN 3000000 AND 3999999' in sql, 'FROM real_estate_npc_items)' in sql, 'merchantid IN (3066639, 3066640)' in sql), (True, True, True))
check('the SQL names the worg', "UPDATE items SET housepet_file_name = 'a_black_worg_mount' WHERE id = 40426 AND housepet_file_name = '';" in sql, True)
check('merchant rows take EQEmu\'s defaults for faction, level and expansion', '(3066639, 1, 47018, -100, 0, 0, 0, 0, 65535, 100, -1, -1)' in sql, True)
expectRaises('a name past 31 characters', lambda: generator.npcName('x' * 32), 'at most 31')
expectRaises('a name with a space-only text', lambda: generator.npcName('  '), 'at most 31')
expectRaises('a size out of range', lambda: generator.size('60', 'test'), 'is not a size')
expectRaises('a tie between looks', lambda: generator.mode(collections.Counter({('a',): 2, ('b',): 2}), 'test'), 'tie')
check('a tie between model looks goes to the lowest texture', generator.mode(collections.Counter({('608', '2', '9'): 1, ('608', '2', '5'): 1}), 'test', lowestOnTie=True),
      ('608', '2', '5'))
check('a color splits into channels', generator.colorChannels(0xFF008000), (0, 128, 0))
print('all NPC item checks pass')
