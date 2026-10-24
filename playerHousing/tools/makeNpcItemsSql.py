"""Writes sql/npcItems.sql: the NPC behind every NPC item in Peridot's items table (NPC-1, NPC-2, NPC-8, NPC-9, NPC-24).

An NPC item is a placeable item with a house-pet name that is not an interactive object (the RoF2 client's rule), as the items
table stands after the housing migrations: an item Live lists takes Live's name (content/peridotExports, the same client records
and items.txt the adoption migration reads); an item only Peridot has is one when its captured tail carries a respawn interval,
and this content gives it the name of the NPC it spawns, as the owner decided. Each gets an npc_types row at 3000000 + its item
id, its look from one source in this order:
- content/npcItemsToAuthor.md section 1 (the guild services, the Device, Hyredel Swiftstride and the training dummies): the
  owner's race, gender, class, level and size, with the rest of the look read from Live's captures of that NPC in that race and
  gender (the most common complete look across every server's halls; owners re-skin copies with the Augmentater, the mode is the
  standard look);
- a click spell whose creature Peridot's horses or pets table has: that row's race, gender, texture and head texture (the yard
  look is the summoned creature, as on Live), with Live's name, level, size and race tag for the item, or, where Live never
  named the item's creature, those of the same creature (race, gender and texture) under another item, or section 4's;
- Live's captured look of the NPC the item's house-pet name names (the house pets with no spell and the summons Peridot can't);
- the Walking Death override (section 3: one of its sixteen forms).
A playable race's look is voted on whole (race, gender, face, hair, eyes, Drakkin marks, armor materials and colors, held models);
another race's on what the client reads for it (race, gender, texture, head texture, actor definition, held models), ties going
to the lowest texture.
Merchant lists are Live's General Supply Merchant's 24 items in Live's order and the Teleportation Assistant's keys, those the
portal module takes (makeGuildServicesSql) and Peridot's items table has; prices are EQEmu's from the items (NPC-9). A pet's
actions are the type-9 entries Live's menus showed for the item, or the four movement actions every captured mount and familiar
offers when no menu of it was captured; a training dummy's come with its flag. The race tag RoF2 previews an item with is the one
Live's client gives the race Peridot spawns (by gender, then by race), else the item's own Live look's. The SQL refuses existing
rows and loads in one transaction.

Usage: python makeNpcItemsSql.py --live <livedumps clone>
"""
import argparse, collections, csv, glob, os, re, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import itemTailDecode  # noqa: E402
import makeGuildServicesSql as guildServices  # noqa: E402

exportsDirectory = os.path.join(here, '..', 'content', 'peridotExports')
authoringPath = os.path.join(here, '..', 'content', 'npcItemsToAuthor.md')
sqlPath = os.path.join(here, '..', 'sql', 'npcItems.sql')

npcTypeBase, npcTypeLimit = 3000000, 3999999
petActions = (16175, 16176, 16177, 16178, 16179, 16394, 16492)
# Stay, Walk Around, Run Around and Follow Me: every mount and familiar whose menu Live's captures hold offers these four, so a pet
# with no captured menu gets them until its menu is captured (NPC-20); Play Dead, Sit Down and Kick the Gnome come only from a capture.
movementActions = (16175, 16176, 16177, 16178)
# EQEmu's own set for a decorative mount (zone/horse.cpp): MeleeImmunity 19, MagicImmunity 20 and AggroImmunity 24 (common/emu_constants.h).
unkillable = '19,1^20,1^24,1'
# A training dummy takes hits and never fights back.
unaggressive = '24,1'
generalSupplyMerchant, teleportationAssistant, teleportationDevice = 66639, 66640, 66646
parcelMerchants = (66647, 66648)
trainingDummies = range(66700, 66717)
peridotDummyPrefix, liveDummyPrefix = 'Decorative Dummy: ', 'Training Dummy: '
# RoF2 has no class 74, Live's Personal-Tribute Master: the owner's 63 (NPC-9).
classesPastRoF2 = {74: 63}
playableRaces = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 128, 130, 330, 522}
animalBodyType, humanoidBodyType = 21, 1
petRunSpeed = 1.25
hitPointsPerLevel, armorClassPerLevel = 100, 5
nameLimit = 31
# Walking Death shifts among sixteen looks on Live (content/npcItemsToAuthor.md, section 3): Peridot spawns the Banshee.
overrides = {52198: {'race': 487, 'gender': 1, 'texture': 0, 'helmtexture': 0, 'size': 3.0}}
vendorZone, generalSupplyVendor, assistantVendor = 'guildhalllrg_int', 'General_Supply_Merchant00', 'Teleportation_Assistant00'
actorPrefix = 'mActorClient.'
# The capture's complete look, voted on as one tuple so a face never pairs with another capture's hair.
lookColumns = ('Race', 'Gender', 'TextureType', 'HeadType', 'FaceStyle', 'HairStyle', 'HairColor', 'FacialHair', 'FacialHairColor',
               'EyeColor1', 'EyeColor2', 'Heritage', 'Tattoo', 'Details', 'ActorDef', 'ArmorColor', 'Class')
# What the client reads of a look for a race that is not playable; the other fields carry whatever bytes the spawn had.
modelLookColumns = ('Race', 'Gender', 'TextureType', 'HeadType', 'ActorDef', 'Class')
equipmentSlots = 9
heldSlots = (7, 8)
armorSlots = range(7)
tintColumns = ('1h', '2c', '3a', '4b', '5g', '6l', '7f', '8x', '9x')
npcTypeColumns = ('id', 'name', 'lastname', 'level', 'race', 'class', 'bodytype', 'hp', 'mana', 'gender', 'texture', 'helmtexture', 'size',
                  'loottable_id', 'merchant_id', 'npc_spells_id', 'npc_faction_id', 'mindmg', 'maxdmg', 'special_abilities', 'aggroradius',
                  'assistradius', 'face', 'luclin_hairstyle', 'luclin_haircolor', 'luclin_eyecolor', 'luclin_eyecolor2', 'luclin_beardcolor',
                  'luclin_beard', 'drakkin_heritage', 'drakkin_tattoo', 'drakkin_details', 'armortint_id', 'armortint_red', 'armortint_green',
                  'armortint_blue', 'd_melee_texture1', 'd_melee_texture2', 'runspeed', 'AC', 'findable', 'trackable', 'exclude', 'isquest',
                  'show_name', 'untargetable', 'armtexture', 'bracertexture', 'handtexture', 'legtexture', 'feettexture', 'light',
                  'is_parcel_merchant')


def readTsv(path, encoding='latin-1'):
  with open(path, newline='', encoding=encoding) as file:
    return list(csv.DictReader(file, delimiter='\t', quoting=csv.QUOTE_NONE))


def npcName(displayedName):
  name = displayedName.strip().replace(' ', '_')
  if not name or len(name) > nameLimit or not re.fullmatch(r"[A-Za-z0-9_`#'-]+", name):
    raise ValueError(f'{displayedName!r} does not make an NPC name of at most {nameLimit} characters')
  return name


def npcTypeId(itemId):
  return npcTypeBase + itemId


# --- Peridot's NPC items -------------------------------------------------------------------------------------------------

def peridotNpcItems(live):
  """Item id -> {name, itemName, clickEffect, housepetName, source}: Peridot's NPC items as the migrated items table holds them."""
  items = {int(row['item_id']): row for row in readTsv(os.path.join(exportsDirectory, 'placeableItems.tsv'), 'utf-8')}
  _, tailRows = itemTailDecode.readRows(os.path.join(exportsDirectory, 'placeableTails.tsv'))
  tails = {row['id']: itemTailDecode.decode(row) for row in tailRows}
  if set(tails) != set(items):
    raise ValueError('the placeable item export and the tail export name different items')
  captured = {int(row['itemId']): row for row in readTsv(os.path.join(live, 'master', 'fields', 'itemDefinitions.tsv')) if row['Placeable'] != '0'}
  listed = {int(row['id']): row for row in readTsv(os.path.join(live, 'itemsTxt', 'itemsTxtExtract.tsv')) if row['placeablebitfield'] != '0'}
  result = {}
  for itemId, item in sorted(items.items()):
    tail = tails[itemId]
    if itemId in captured:
      housepetName, interactive, source = captured[itemId]['HousepetFileName'], captured[itemId]['bInteractiveObject'] != '0', 'live'
    elif itemId in listed:
      housepetName, interactive, source = listed[itemId]['placeablenpcname'], tail['interactive'] != 0, 'live'
    else:
      housepetName, interactive, source = '', tail['interactive'] != 0, 'peridot'
    if interactive or not (housepetName or (source == 'peridot' and tail['respawn'] > 0)):
      continue
    result[itemId] = {'itemName': item['Name'], 'clickEffect': int(item['clickeffect']), 'housepetName': housepetName, 'source': source}
  return result


# --- Live's looks -------------------------------------------------------------------------------------------------------

def spawnCaptures(live):
  """Spawn name without its instance digits -> every server's and zone's captured row of it."""
  captures = collections.defaultdict(list)
  for path in sorted(glob.glob(os.path.join(live, 'master', 'fields', 'spawns', '*.tsv'))):
    for row in readTsv(path):
      captures[re.sub(r'\d+$', '', row['name'])].append(row)
  if not captures:
    raise ValueError(f'{live} has no master/fields/spawns tables')
  return captures


def single(row, column):
  values = row[column].split(' | ')
  return values[0] if len(values) == 1 else None


def lookTuple(row, playable):
  columns = lookColumns if playable else modelLookColumns
  slots = range(equipmentSlots) if playable else heldSlots
  values = [single(row, actorPrefix + column) for column in columns]
  values += [single(row, f'{actorPrefix}ActorEquipment.Item[{slot}].type') for slot in slots]
  return None if None in values else tuple(values)


def mode(counter, what, lowestOnTie=False):
  ranked = counter.most_common()
  if not ranked:
    raise ValueError(f'{what}: no capture shows it whole')
  tied = [value for value, count in ranked if count == ranked[0][1]]
  if len(tied) > 1:
    if not lowestOnTie:
      raise ValueError(f'{what}: {len(tied)} looks tie at {ranked[0][1]} captures each')
    return min(tied, key=lambda value: [int(part) if part.lstrip('-').isdigit() else part for part in value])
  return ranked[0][0]


def standardLook(rows, what, race=None, gender=None, withLevel=True, lastname=None):
  """The most common look among the captures (of the given race, gender and surname, else the most captured ones), with its height and surname."""
  if lastname is not None:
    rows = [row for row in rows if single(row, 'Lastname') == lastname]
  if race is None:
    pairs = collections.Counter((single(row, actorPrefix + 'Race'), single(row, actorPrefix + 'Gender')) for row in rows)
    pairs.pop((None, None), None)
    race, gender = (int(value) for value in mode(pairs, f'{what}: race and gender'))
  rows = [row for row in rows if single(row, actorPrefix + 'Race') == str(race) and single(row, actorPrefix + 'Gender') == str(gender)]
  playable = race in playableRaces
  tuples = collections.Counter(lookTuple(row, playable) for row in rows)
  tuples.pop(None, None)
  chosen = mode(tuples, f'{what} as race {race} gender {gender}', lowestOnTie=not playable)
  columns = lookColumns if playable else modelLookColumns
  look = {column: '0' for column in lookColumns}
  look.update(zip(columns, chosen))
  held = chosen[len(columns):]
  look['equipment'] = [int(value) for value in held] if playable else [0] * heldSlots[0] + [int(value) for value in held]
  look['armorColors'] = [int(value) for value in look['ArmorColor'].split(',')] if playable else [0] * equipmentSlots
  if len(look['armorColors']) != equipmentSlots:
    raise ValueError(f'{what}: ArmorColor holds {len(look["armorColors"])} colors')
  if not look['ActorDef'].endswith('_ACTORDEF') or len(look['ActorDef']) < 12:
    raise ValueError(f'{what}: actor definition {look["ActorDef"]!r} gives no race tag')
  look['raceTag'] = look['ActorDef'][:3]
  for column in (('Level',) if withLevel else ()) + ('Height', 'Lastname'):
    look[column] = mode(collections.Counter(single(row, column) for row in rows if single(row, column) is not None), f'{what}: {column}')
  return look


def creatureAnalogues(captures):
  """(race, gender, texture) -> the captures' (name, level, height, actor definition) of that creature as an NPC, by count."""
  seen = collections.defaultdict(collections.Counter)
  for rows in captures.values():
    for row in rows:
      key = tuple(single(row, actorPrefix + column) for column in ('Race', 'Gender', 'TextureType'))
      values = tuple(single(row, column) for column in ('DisplayedName', 'Level', 'Height', f'{actorPrefix}ActorDef'))
      if single(row, 'Type') != '1' or None in key or None in values or values[0].endswith('`s Mount'):
        continue
      seen[tuple(int(value) for value in key)][values] += 1
  return seen


def raceTags(captures):
  """(race, gender) and (race, None) -> the actor definition's first three characters Live's client gives that race, by count."""
  tags = collections.defaultdict(collections.Counter)
  for rows in captures.values():
    for row in rows:
      race, gender, actorDef = single(row, actorPrefix + 'Race'), single(row, actorPrefix + 'Gender'), single(row, actorPrefix + 'ActorDef')
      if race and gender and actorDef and actorDef.endswith('_ACTORDEF'):
        tags[(int(race), int(gender))][actorDef[:3]] += 1
        tags[(int(race), None)][actorDef[:3]] += 1
  return {key: counter.most_common(1)[0][0] for key, counter in tags.items()}


def raceTag(tags, race, gender, look):
  """The tag RoF2 previews the item with: Live's for the spawned race and gender, else for the race, else the item's own Live look's."""
  return tags.get((race, gender)) or tags.get((race, None)) or (look['raceTag'] if look else None)


# --- the owner's section 1 ---------------------------------------------------------------------------------------------

def authoredServices():
  """Item id -> the owner's race, gender, class, level and size for section 1 of content/npcItemsToAuthor.md."""
  text = open(authoringPath, encoding='utf-8').read()
  section = text[text.index('## 1.'):text.index('## 2.')]
  genders = {'male': 0, 'female': 1, 'neutral': 2}
  result = {}
  for line in section.split('\n'):
    cells = [cell.strip() for cell in line.strip().strip('|').split('|')]
    if len(cells) != 8 or not cells[0].isdigit():
      continue
    itemId, decision = int(cells[0]), cells[7]
    found = re.search(r'race (\d+)[^,]*, (male|female|neutral), class (\d+), level (\d+), size ([\d.]+)', decision)
    if not found:
      if decision.startswith('none:'):
        continue
      raise ValueError(f'section 1 gives item {itemId} no race, gender, class, level and size: {decision!r}')
    liveClass = int(found.group(3))
    result[itemId] = {'race': int(found.group(1)), 'gender': genders[found.group(2)], 'liveClass': liveClass,
                      'class': classesPastRoF2.get(liveClass, liveClass), 'level': int(found.group(4)), 'size': float(found.group(5))}
  if not result:
    raise ValueError('section 1 of content/npcItemsToAuthor.md lists no NPC to use')
  return result


def authoredMounts():
  """Item id -> the name, level and size of a mount Live never named, from section 4 of content/npcItemsToAuthor.md."""
  text = open(authoringPath, encoding='utf-8').read()
  section = text[text.index('## 4.'):text.index('## Reference')]
  result = {}
  for line in section.split('\n'):
    cells = [cell.strip() for cell in line.strip().strip('|').split('|')]
    if len(cells) == 6 and cells[0].isdigit():
      result[int(cells[0])] = {'displayedName': cells[2], 'level': int(cells[3]), 'size': float(cells[4])}
  return result


# --- Live's names, spells, menus and vendors ------------------------------------------------------------------------------

def liveNpcItems(live):
  return {int(row['itemId']): row for row in readTsv(os.path.join(live, 'master', 'npcItems.tsv')) if row['itemId'].isdigit()}


def clickCreatures():
  """Spell id -> ('horse', horses row) or ('familiar', pets row joined to its NPC type), for the click spells Peridot can summon."""
  spells = {int(row['id']): row for row in readTsv(os.path.join(exportsDirectory, 'clickSpells.tsv'), 'utf-8')}
  horses = {row['filename']: row for row in readTsv(os.path.join(exportsDirectory, 'horses.tsv'), 'utf-8')}
  familiars = collections.defaultdict(list)
  for row in readTsv(os.path.join(exportsDirectory, 'familiarPets.tsv'), 'utf-8'):
    familiars[int(row['spell_id'])].append(row)
  creatures = {}
  for spellId, spell in spells.items():
    if spell['teleport_zone'] in horses:
      creatures[spellId] = ('horse', horses[spell['teleport_zone']])
    elif spellId in familiars:
      rows = familiars[spellId]
      if len({row['npcID'] for row in rows}) != 1:
        raise ValueError(f'spell {spellId} summons more than one familiar NPC type')
      creatures[spellId] = ('familiar', rows[0])
  return creatures


def menuActions(live):
  """Item id -> the pet actions (type-9 entry codes) Live's menus showed for it, in code order."""
  actions = collections.defaultdict(set)
  for row in readTsv(os.path.join(live, 'master', 'menus.tsv')):
    if not row['itemId'].isdigit():
      continue
    for entry in row['entries'].split(' | '):
      found = re.match(r'(\d+):(-?\d+):', entry)
      if found and found.group(1) == '9' and int(found.group(2)) in petActions:
        actions[int(row['itemId'])].add(int(found.group(2)))
  return {itemId: sorted(codes) for itemId, codes in actions.items()}


def vendorLists(live):
  """Vendor spawn name -> Live's item ids in the vendor's slot order, for the two guild hall vendors."""
  lists = collections.defaultdict(dict)
  for row in readTsv(os.path.join(live, 'master', 'vendorItems.tsv')):
    if row['zone'] == vendorZone and row['vendor'] in (generalSupplyVendor, assistantVendor):
      lists[row['vendor']][int(row['index'])] = int(row['itemId'])
  result = {}
  for vendor, byIndex in lists.items():
    if sorted(byIndex) != list(range(len(byIndex))):
      raise ValueError(f"{vendor}'s captured list skips a slot")
    result[vendor] = [byIndex[index] for index in range(len(byIndex))]
  for vendor in (generalSupplyVendor, assistantVendor):
    if vendor not in result:
      raise ValueError(f'{vendor} has no captured list in {vendorZone}')
  return result


def peridotItemIds():
  return {int(row['id']) for row in readTsv(os.path.join(exportsDirectory, 'vendorItems.tsv'), 'utf-8')}


# --- composing each NPC --------------------------------------------------------------------------------------------------

def colorChannels(argb):
  return (argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff


def tint(npc, look):
  """A uniform body tint as the three channels; per-slot colors as an npc_types_tint row (the helmet's own color, say)."""
  colors = look['armorColors']
  body = colors[1:7]
  if len(set(body)) == 1 and colors[0] in (0, body[0]):
    npc.update(zip(('armortint_red', 'armortint_green', 'armortint_blue'), colorChannels(body[0])))
    return None
  npc['armortint_id'] = npc['id']
  return {'id': npc['id'], 'tint_set_name': npc['name'], 'colors': [colorChannels(color) for color in colors]}


def applyLook(npc, look, playable):
  """Live's capture onto the npc_types row: equipment materials for a playable race, the model textures otherwise."""
  equipment = look['equipment']
  if playable:
    npc.update(helmtexture=equipment[0], texture=equipment[1], armtexture=equipment[2], bracertexture=equipment[3], handtexture=equipment[4],
               legtexture=equipment[5], feettexture=equipment[6])
  else:
    npc.update(texture=max(int(look['TextureType']), 0), helmtexture=max(int(look['HeadType']), 0))
  npc.update(d_melee_texture1=equipment[7], d_melee_texture2=equipment[8], face=int(look['FaceStyle']), luclin_hairstyle=int(look['HairStyle']),
             luclin_haircolor=int(look['HairColor']), luclin_beard=int(look['FacialHair']), luclin_beardcolor=int(look['FacialHairColor']),
             luclin_eyecolor=int(look['EyeColor1']), luclin_eyecolor2=int(look['EyeColor2']), drakkin_heritage=int(look['Heritage']),
             drakkin_tattoo=int(look['Tattoo']), drakkin_details=int(look['Details']), lastname=look['Lastname'])
  return tint(npc, look)


def baseNpc(itemId, name, level, race, gender, kind, size):
  return {'id': npcTypeId(itemId), 'name': name, 'lastname': '', 'level': level, 'race': race, 'class': 1, 'bodytype': humanoidBodyType,
          'hp': level * hitPointsPerLevel, 'mana': 0, 'gender': gender, 'texture': 0, 'helmtexture': 0, 'size': size, 'loottable_id': 0,
          'merchant_id': 0, 'npc_spells_id': 0, 'npc_faction_id': 0, 'mindmg': 0, 'maxdmg': 0,
          'special_abilities': unaggressive if kind == 'dummy' else unkillable, 'aggroradius': 0, 'assistradius': 0, 'face': 0,
          'luclin_hairstyle': 0, 'luclin_haircolor': 0, 'luclin_eyecolor': 0, 'luclin_eyecolor2': 0, 'luclin_beardcolor': 0, 'luclin_beard': 0,
          'drakkin_heritage': 0, 'drakkin_tattoo': 0, 'drakkin_details': 0, 'armortint_id': 0, 'armortint_red': 0, 'armortint_green': 0,
          'armortint_blue': 0, 'd_melee_texture1': 0, 'd_melee_texture2': 0, 'runspeed': petRunSpeed if kind == 'pet' else 0,
          'AC': level * armorClassPerLevel, 'findable': 0, 'trackable': 0, 'exclude': 1, 'isquest': 0, 'show_name': 1, 'untargetable': 0,
          'armtexture': 0, 'bracertexture': 0, 'handtexture': 0, 'legtexture': 0, 'feettexture': 0, 'light': 0, 'is_parcel_merchant': 0}


def size(text, what):
  value = round(float(text), 2)
  if not 0.1 <= value <= 50:
    raise ValueError(f'{what}: size {value} is not a size')
  return value


def compose(live):
  """Every NPC item's rows: npc_types, tints, merchant lists, real_estate_npc_items, actions and house-pet names, plus the report."""
  npcItems = peridotNpcItems(live)
  captures = spawnCaptures(live)
  sources = {'tags': raceTags(captures), 'analogues': creatureAnalogues(captures), 'liveItems': liveNpcItems(live), 'creatures': clickCreatures(),
             'services': authoredServices(), 'mounts': authoredMounts(), 'actions': menuActions(live), 'vendors': vendorLists(live),
             'peridotIds': peridotItemIds(), 'moduleKeys': {key['itemId'] for key in guildServices.keys(guildServices.readDestinations())},
             'captures': captures}
  report = collections.Counter()
  notes, problems = [], []
  rows = {'npcTypes': [], 'tints': [], 'merchants': [], 'npcItems': [], 'actions': [], 'housepetNames': []}
  for itemId, item in npcItems.items():
    try:
      composeItem(itemId, item, sources, rows, report, notes)
    except ValueError as error:
      problems.append(str(error))
  if problems:
    raise ValueError(f'{len(problems)} NPC items cannot be composed:\n  ' + '\n  '.join(problems))
  report['NPC items'] = len(npcItems)
  return rows, report, notes


def summonedPet(itemId, item, what, sources, notes):
  """The npc_types row and race tag of a pet or mount whose click spell Peridot's horses or pets table resolves."""
  creatureKind, creature = sources['creatures'][item['clickEffect']]
  race, gender, texture = int(creature['race']), int(creature['gender']), int(creature['texture'])
  liveItem = sources['liveItems'].get(itemId)
  captured = sources['captures'].get(liveItem['npcName']) if liveItem and liveItem['npcName'] else None
  look = standardLook(captured, what) if captured else None
  displayedName = liveItem['displayedName'] if liveItem else ''
  if displayedName:
    level = int(look['Level']) if look else int(single(liveItem, 'level') or 0)
    height = look['Height'] if look else single(liveItem, 'height')
    if not level or not height:
      raise ValueError(f'{what}: Live gives no single level or height for it')
    if look and (int(look['Race']), int(look['Gender'])) != (race, gender):
      notes.append(f'{what}: Live shows race {look["Race"]} gender {look["Gender"]}, Peridot\'s {creatureKind} row race {race} gender {gender}')
    source = 'Live'
  elif (race, gender, texture) in sources['analogues']:
    displayedName, level, height, actorDef = mode(sources['analogues'][(race, gender, texture)], f'{what}: the creature\'s names on Live')
    level, source = int(level), 'a Live analogue'
  elif itemId in sources['mounts']:
    authored = sources['mounts'][itemId]
    displayedName, level, height, source = authored['displayedName'], authored['level'], authored['size'], 'section 4'
  else:
    raise ValueError(f'{what}: Live never named it or its creature (race {race} gender {gender} texture {texture}); author it in section 4')
  tag = raceTag(sources['tags'], race, gender, look)
  if not tag:
    raise ValueError(f'{what}: no capture gives race {race} an actor tag')
  npc = baseNpc(itemId, npcName(' '.join(displayedName.split())), level, race, gender, 'pet', size(height, what))
  npc.update(texture=max(texture, 0), helmtexture=max(int(creature['helmtexture']), 0))
  npc['bodytype'] = int(creature['bodytype']) if creatureKind == 'familiar' else animalBodyType
  return npc, tag, f'pet from {creatureKind}, named by {source}'


def composeItem(itemId, item, sources, rows, report, notes):
  if True:
    services, actions, vendors, peridotIds, moduleKeys = (sources[key] for key in ('services', 'actions', 'vendors', 'peridotIds', 'moduleKeys'))
    liveItem = sources['liveItems'].get(itemId)
    captured = sources['captures'].get(liveItem['npcName']) if liveItem and liveItem['npcName'] else None
    displayedName = liveItem['displayedName'] if liveItem and liveItem['displayedName'] else ''
    what = f'item {itemId} ({item["itemName"]})'
    npc = None
    if itemId in services:
      decision = services[itemId]
      kind = 'dummy' if itemId in trainingDummies else 'service'
      if kind == 'dummy':
        # Live names a hall's dummies in sequence (Combat Dummy Azia, Beza, ...) and shows the dummy's kind as its surname, 'Type - <expansion>',
        # with the kind's skin as its texture; Peridot's items carry the sequence names, Live's the kinds.
        if not liveItem or not liveItem['name'].startswith(liveDummyPrefix) or not item['itemName'].startswith(peridotDummyPrefix):
          raise ValueError(f'{what}: not a training dummy on both sides ({liveItem["name"] if liveItem else "unlisted on Live"})')
        displayedName = f'Combat Dummy {item["itemName"][len(peridotDummyPrefix):]}'
        surname = f'Type - {liveItem["name"][len(liveDummyPrefix):]}'
        captured = [row for name, rows in sources['captures'].items() if name.startswith('Combat_Dummy_') for row in rows]
        look = standardLook(captured, what, decision['race'], decision['gender'], withLevel=False, lastname=surname)
      else:
        if not captured:
          raise ValueError(f'{what}: section 1 names it and Live has no capture of {liveItem["npcName"] if liveItem else "it"}')
        look = standardLook(captured, what, decision['race'], decision['gender'], withLevel=False)
      if int(look['Class']) != decision['liveClass']:
        raise ValueError(f'{what}: the captures show class {look["Class"]}, section 1 says {decision["liveClass"]}')
      npc = baseNpc(itemId, npcName(displayedName), decision['level'], decision['race'], decision['gender'], kind, decision['size'])
      npc['class'] = decision['class']
      tintRow = applyLook(npc, look, decision['race'] in playableRaces)
      raceTag = look['raceTag']
      report[kind] += 1
    elif item['clickEffect'] in sources['creatures']:
      kind = 'pet'
      npc, raceTag, source = summonedPet(itemId, item, what, sources, notes)
      tintRow = None
      report[source] += 1
    elif captured:
      kind = 'pet'
      override = overrides.get(itemId)
      look = standardLook(captured, what, override['race'], override['gender']) if override else standardLook(captured, what)
      npc = baseNpc(itemId, npcName(displayedName), int(look['Level']), int(look['Race']), int(look['Gender']), kind, size(look['Height'], what))
      tintRow = applyLook(npc, look, int(look['Race']) in playableRaces)
      if override:
        npc.update({key: value for key, value in override.items()})
      raceTag = look['raceTag']
      report['pet from Live'] += 1
    else:
      raise ValueError(f'{what}: no section 1 entry, no summoning spell Peridot has and no Live capture to take a look from')
    if itemId == generalSupplyMerchant:
      npc['merchant_id'] = npc['id']
      missing = [sold for sold in vendors[generalSupplyVendor] if sold not in peridotIds]
      if missing:
        raise ValueError(f"the General Supply Merchant sells items Peridot lacks: {missing}")
      rows['merchants'].extend((npc['id'], slot + 1, sold) for slot, sold in enumerate(vendors[generalSupplyVendor]))
    if itemId == teleportationAssistant:
      npc['merchant_id'] = npc['id']
      sold = [key for key in vendors[assistantVendor] if key in moduleKeys and key in peridotIds]
      notes.append(f'the Teleportation Assistant sells {len(sold)} of Live\'s {len(vendors[assistantVendor])} keys: '
                   f'{len([k for k in vendors[assistantVendor] if k not in moduleKeys])} go to zones RoF2 lacks or Live refused, '
                   f'{len([k for k in vendors[assistantVendor] if k in moduleKeys and k not in peridotIds])} are items Peridot lacks')
      rows['merchants'].extend((npc['id'], slot + 1, key) for slot, key in enumerate(sold))
    if itemId in parcelMerchants:
      npc['is_parcel_merchant'] = 1
    if len(raceTag) != 3 or not all(character.isdigit() or character.isupper() for character in raceTag):
      raise ValueError(f'{what}: race tag {raceTag!r} is not three upper-case characters')
    rows['npcTypes'].append(npc)
    if tintRow:
      rows['tints'].append(tintRow)
    rows['npcItems'].append((itemId, npc['id'], raceTag, 1 if kind == 'dummy' else 0))
    if kind == 'pet' and itemId not in actions:
      rows['actions'].extend((itemId, code) for code in movementActions)
      report['pets given the movement actions (no captured menu)'] += 1
    else:
      rows['actions'].extend((itemId, code) for code in actions.get(itemId, []))
    if item['source'] == 'peridot':
      rows['housepetNames'].append((itemId, npc['name']))
      report['names given'] += 1


# --- rendering ----------------------------------------------------------------------------------------------------------

def sqlValue(value):
  if isinstance(value, str):
    return guildServices.quote(value)
  if isinstance(value, float):
    return repr(value)
  return str(value)


def renderSql(rows):
  merchantIds = sorted({merchant for merchant, _, _ in rows['merchants']})
  out = [f'-- Generated by playerHousing/tools/makeNpcItemsSql.py: the NPC each NPC item spawns, their merchant lists and actions (NPC-8).',
         'DELIMITER //', 'BEGIN NOT ATOMIC', '  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;',
         f'  IF EXISTS (SELECT 1 FROM npc_types WHERE id BETWEEN {npcTypeBase} AND {npcTypeLimit}) THEN',
         "    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'npc_types already holds NPC item types';", '  END IF;',
         '  IF EXISTS (SELECT 1 FROM real_estate_npc_items) THEN',
         "    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'real_estate_npc_items already has rows';", '  END IF;',
         f'  IF EXISTS (SELECT 1 FROM merchantlist WHERE merchantid IN ({", ".join(map(str, merchantIds))})) THEN',
         "    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'merchantlist already holds the guild services'' lists';", '  END IF;',
         '  START TRANSACTION;']
  out.append(f'  INSERT INTO npc_types ({", ".join(f"`{column}`" for column in npcTypeColumns)}) VALUES')
  out.append(',\n'.join(f'    ({", ".join(sqlValue(npc[column]) for column in npcTypeColumns)})' for npc in rows['npcTypes']) + ';')
  if rows['tints']:
    columns = ['id', 'tint_set_name'] + [f'{channel}{slot}' for slot in tintColumns for channel in ('red', 'grn', 'blu')]
    out.append(f'  INSERT INTO npc_types_tint ({", ".join(f"`{column}`" for column in columns)}) VALUES')
    out.append(',\n'.join(f'    ({tintRow["id"]}, {guildServices.quote(tintRow["tint_set_name"])}, '
                          f'{", ".join(str(channel) for color in tintRow["colors"] for channel in color)})' for tintRow in rows['tints']) + ';')
  out.append('  INSERT INTO merchantlist (merchantid, slot, item, faction_required, level_required, min_status, max_status, alt_currency_cost, '
             'classes_required, probability, min_expansion, max_expansion) VALUES')
  out.append(',\n'.join(f'    ({merchant}, {slot}, {sold}, -100, 0, 0, 0, 0, 65535, 100, -1, -1)' for merchant, slot, sold in rows['merchants']) + ';')
  out.append('  INSERT INTO real_estate_npc_items (item_id, npc_type_id, mount_npc_type_id, race_tag, training_dummy) VALUES')
  out.append(',\n'.join(f"    ({itemId}, {npcType}, 0, '{raceTag}', {dummy})" for itemId, npcType, raceTag, dummy in rows['npcItems']) + ';')
  out.append('  INSERT INTO real_estate_npc_item_actions (item_id, action_id) VALUES')
  out.append(',\n'.join(f'    ({itemId}, {code})' for itemId, code in rows['actions']) + ';')
  for itemId, name in rows['housepetNames']:
    out.append(f"  UPDATE items SET housepet_file_name = {guildServices.quote(name)} WHERE id = {itemId} AND housepet_file_name = '';")
  out += ['  COMMIT;', 'END //', 'DELIMITER ;', '']
  return '\n'.join(out)


def main():
  parser = argparse.ArgumentParser(description='Write sql/npcItems.sql.')
  parser.add_argument('--live', required=True, help='a livedumps clone')
  live = parser.parse_args().live
  if not os.path.isdir(os.path.join(live, 'master', 'fields', 'spawns')):
    raise SystemExit(f'{live} is not a livedumps clone with master tables')
  rows, report, notes = compose(live)
  with open(sqlPath, 'w', newline='\n', encoding='utf-8') as file:
    file.write(renderSql(rows))
  print(f'wrote {sqlPath}: {len(rows["npcTypes"])} NPC types, {len(rows["tints"])} tint sets, {len(rows["merchants"])} merchant rows, '
        f'{len(rows["actions"])} actions, {len(rows["housepetNames"])} house-pet names')
  for key, count in sorted(report.items()):
    print(f'  {key}: {count}')
  for note in notes:
    print(f'  note: {note}')


if __name__ == '__main__':
  main()
