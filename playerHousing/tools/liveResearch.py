#!/usr/bin/env python3
"""Rebuilds research/live/master/ from every MQ2PeridotLive capture in research/live/."""
import collections
import csv
import pathlib
import re
import struct

from eqlibLayouts import Layouts

researchDirectory = pathlib.Path(__file__).resolve().parent.parent / 'research' / 'live'
masterDirectory = researchDirectory / 'master'
# Each Live client build's frozen eqlib headers; captures made before plugin 0.4 recorded no build and all came from Sep 17 2026.
layoutDirectories = {'Sep 17 2026 15:46:17': researchDirectory / 'eqlib' / '9efa0bc'}
buildBeforeHeaders = 'Sep 17 2026 15:46:17'

itemColumns = ['itemId', 'name', 'housepetFileName', 'npcRespawnInterval', 'realEstateDefinitionId', 'realEstateUpkeepId',
  'maxPerRealEstate', 'placeable', 'placementType', 'trophyBenefitId', 'interactiveObject', 'linkHash']
headerColumns = {'time': ['stamp'], 'client': ['date', 'time'], 'plugin': ['version'], 'layout': ['date', 'time'],
  'zone': ['shortName', 'longName', 'designId', 'zoneId', 'instance']}
itemOwners = ('placedItem', 'merchantItem', 'ground', 'heldItem')
appearanceOwners = ('spawn', 'merchant')
# PlayerClient offsets in eqlib's layout for Live's Sep 17 2026 client, for captures made before the plugin named these fields:
# RealEstateID (named from 0.5); mActorClient's ActorBase, NpcTintIndex, Light and Equipment (named from 0.6). Decoding checks
# race, class and gender against the named columns, so a wrong offset fails loudly.
spawnRealEstateIdOffset = 0x498
actorOffset = 0x0fc8
npcTintIndexOffset = 0x458
lightOffset = 0x384
equipmentOffset = 0x1df4
# Live varies an NPC's appearance between spawns (random gear, tints and per-spawn material and variation), so these are sets.
appearanceFields = ['actorDef', 'textureType', 'material', 'variation', 'headType', 'faceStyle', 'hairStyle', 'hairColor', 'facialHair',
  'facialHairColor', 'eyeColor1', 'eyeColor2', 'raceOverride', 'showHelm', 'heritage', 'tattoo', 'details', 'npcTintIndex', 'light', 'armorColors',
  'actorEquipment', 'equipment']
noProperty = ('0', '-1')


class Capture:
  def __init__(self, path):
    self.path = path
    if path.name.startswith('peridotLiveZone_'):
      self.kind = 'zone'
    elif path.name.startswith('peridotLiveVendor_'):
      self.kind = 'vendor'
    elif path.name.startswith('peridotLiveWorld_'):
      self.kind = 'world'
    elif path.name.startswith('peridotLiveItems_'):
      self.kind = 'items'
    elif path.name.startswith('peridotLiveWindow_'):
      self.kind = 'window'
    else:
      raise ValueError(f'{path.name} is not an MQ2PeridotLive capture')
    self.header = {}
    self.headerRecords = {}
    self.records = []
    columns = {}
    current = None
    for lineNumber, line in enumerate(path.read_text(encoding='latin-1').rstrip('\n').split('\n'), 1):
      line = line.rstrip('\r')
      where = f'{path.name}:{lineNumber}'
      if not line or line.startswith('#'):
        continue
      parts = line.split('\t')
      head = parts[0]
      if head.startswith('[') or head.startswith('  '):
        if len(parts) == 1:
          continue
        label = head.split('] ', 1)[1] if head.startswith('[') else head.strip()
        columns[label] = [column.split(' (')[0].strip() for column in parts[1:]]
        continue
      if head == 'raw':
        if current is None or len(parts) != 3 or len(parts[2]) != int(parts[1]) * 2:
          raise ValueError(f'{where}: malformed raw line')
        current['raw'].append(bytes.fromhex(parts[2]))
        continue
      if head in headerColumns:
        if len(parts) - 1 != len(headerColumns[head]):
          raise ValueError(f'{where}: {head} has {len(parts) - 1} values')
        self.header[head] = dict(zip(headerColumns[head], parts[1:]))
        current = self.headerRecords[head] = {'label': head, 'raw': []}
        continue
      names = itemColumns if head == 'item' else columns.get(head)
      if names is None:
        raise ValueError(f'{where}: unknown record {head!r}')
      if len(parts) - 1 != len(names):
        raise ValueError(f'{where}: {head} has {len(parts) - 1} values for {len(names)} columns')
      record = {'label': head, 'fields': dict(zip(names, parts[1:])), 'raw': [], 'item': None, 'appearance': None}
      if head == 'appearance':
        owner = self.records[-1] if self.records else None
        if owner is None or owner['label'] not in appearanceOwners or owner['appearance'] is not None:
          raise ValueError(f'{where}: an appearance line must follow a spawn or merchant line')
        owner['appearance'] = record
      elif head == 'item':
        owner = self.records[-1] if self.records else None
        if owner is None or owner['label'] not in itemOwners or owner['item'] is not None:
          raise ValueError(f'{where}: an item line must follow a placed, merchant, ground or held item')
        owner['item'] = record
      else:
        self.records.append(record)
      current = record
    for required in ('time', 'zone'):
      if required not in self.header:
        raise ValueError(f'{path.name}: no {required} line')
    self.stamp = self.header['time']['stamp']
    self.zoneShortName = self.header['zone']['shortName']
    client = self.header.get('client')
    self.build = f'{client["date"]} {client["time"]}' if client else buildBeforeHeaders
    layout = self.header.get('layout')
    if layout and f'{layout["date"]} {layout["time"]}' != self.build:
      raise ValueError(f'{path.name}: plugin built for {layout["date"]} {layout["time"]} but captured on client {self.build}')
    if self.build not in layoutDirectories:
      raise ValueError(f'{path.name}: no eqlib headers for client {self.build}; freeze them under research/live/eqlib first')

  def all(self, label):
    return [record for record in self.records if record['label'] == label]


def tsvText(value):
  return ''.join('?' if ord(ch) < 0x20 else ch for ch in str(value))


def sortKey(key):
  return tuple((0, int(part), '') if part.lstrip('-').isdigit() else (1, 0, part) for part in key)


class Table:
  def __init__(self, name, keyFields, stableFields, volatileFields=(), setFields=()):
    self.name = name
    self.keyFields = list(keyFields)
    self.stableFields = list(stableFields)
    self.volatileFields = list(volatileFields)
    self.setFields = list(setFields)
    self.rows = {}
    self.conflicts = []

  def observe(self, key, capture, stable=None, volatile=None, sets=None):
    row = self.rows.get(key)
    if row is None:
      row = self.rows[key] = {'stable': {}, 'volatile': {}, 'sets': collections.defaultdict(set), 'firstSeen': capture.stamp,
        'lastSeen': capture.stamp, 'captures': set()}
    for field, value in (stable or {}).items():
      old = row['stable'].get(field)
      if old is not None and old != value:
        self.conflicts.append((self.name, '/'.join(key), field, old, value, capture.path.name))
      row['stable'][field] = value
    row['volatile'].update(volatile or {})
    for field, value in (sets or {}).items():
      row['sets'][field].add(value)
    row['lastSeen'] = capture.stamp
    row['captures'].add(capture.path.name)

  def write(self):
    with (masterDirectory / f'{self.name}.tsv').open('w', encoding='latin-1', newline='') as out:
      writer = csv.writer(out, delimiter='\t', lineterminator='\n')
      writer.writerow(self.keyFields + self.stableFields + self.volatileFields + self.setFields + ['firstSeen', 'lastSeen', 'captureCount'])
      for key in sorted(self.rows, key=sortKey):
        row = self.rows[key]
        writer.writerow([tsvText(value) for value in list(key) + [row['stable'].get(field, '') for field in self.stableFields]
          + [row['volatile'].get(field, '') for field in self.volatileFields]
          + [' | '.join(sorted(row['sets'][field], key=lambda value: sortKey((value,)))) for field in self.setFields]
          + [row['firstSeen'], row['lastSeen'], len(row['captures'])]])


class FieldTable:
  def __init__(self, name, keyFields):
    self.name = name
    self.keyFields = list(keyFields)
    self.rows = {}
    self.columns = {}

  def observe(self, key, capture, values):
    self.rows[key] = (capture.path.name, values)
    for column in values:
      self.columns.setdefault(column, None)

  def write(self):
    directory = masterDirectory / 'fields'
    directory.mkdir(exist_ok=True)
    with (directory / f'{self.name}.tsv').open('w', encoding='latin-1', newline='') as out:
      writer = csv.writer(out, delimiter='\t', lineterminator='\n')
      writer.writerow(self.keyFields + list(self.columns) + ['capture'])
      for key in sorted(self.rows, key=sortKey):
        captureName, values = self.rows[key]
        writer.writerow([tsvText(value) for value in list(key) + [values.get(column, '') for column in self.columns] + [captureName]])


layoutCache = {}


def decodeRecord(capture, typeName, raw, checks):
  directory = layoutDirectories[capture.build]
  if directory not in layoutCache:
    layoutCache[directory] = Layouts(directory)
  values = layoutCache[directory].decode(typeName, raw)
  for field, expected in checks.items():
    if values.get(field) != expected:
      raise ValueError(f'{capture.path.name}: {typeName} decodes {field} as {values.get(field)!r}, not the named {expected!r}')
  return values


def pick(fields, names):
  return {name: fields[name] for name in names}


definitionFields = ['id', 'name', 'description', 'address', 'zoneInX', 'zoneInY', 'zoneInZ', 'areaId', 'costDefinition', 'upkeepDefinition',
  'maxItems', 'maxNonRealEstateItems', 'maxChildren', 'zoneId', 'dzId', 'type', 'groupId', 'icon', 'maxNpcItems', 'switchX', 'switchY',
  'switchZ', 'switchHeading', 'childDefinitions']
# Link hashes cover an item's own data and transformation molds change a placeable's appearance, so an item id can show several of each.
itemStableFields = [column for column in itemColumns[1:] if column != 'linkHash']
# An item's spawned NPC is named after the item's house-pet name for every pet, mount, familiar and plushie seen; guild services
# spawn differently named NPCs, learned where an item list links the spawn to its item.
npcLookFields = ['displayedName', 'race', 'gender', 'class', 'level', 'height']
# The same NPC name spawns with different levels, races, genders and heights between instances (two guild halls' training
# dummies, randomized NPCs), and one item's copies spawn differently named NPCs, so these are kept as observed variants.
spawnVariantFields = ['lastname', 'race', 'gender', 'level', 'height', 'avatarHeight']
spawnLookFields = ['displayedName', 'lastname', 'race', 'class', 'gender', 'level', 'height', 'avatarHeight']
doorStableFields = ['name', 'type', 'defaultX', 'defaultY', 'defaultZ', 'defaultHeading', 'defaultState',
  'scaleFactor', 'key', 'spellId', 'dynDoorId', 'usable', 'visible']
# Before plugin 0.5 a door without a script wrote its script field's uninitialized bytes, which differ between sessions.
doorVolatileFields = ['state', 'script', 'x', 'y', 'z', 'heading']


def buildFieldTables():
  return {name: FieldTable(name, keys) for name, keys in (('itemDefinitions', ['itemId']), ('spawns', ['zone', 'name']), ('doors', ['zone', 'id']),
    ('zoneHeaders', ['zone']), ('ground', ['zone', 'dropId']), ('placedObjects', ['propertyId', 'realEstateItemId']), ('definitions', ['id']),
    ('properties', ['id']), ('propertyItems', ['propertyId', 'realEstateItemId']), ('worldZones', ['id']))}


def buildTables():
  return {
    'captures': Table('captures', ['file'], ['kind', 'client', 'plugin', 'zone', 'zoneId', 'instance', 'zoneRealEstateId', 'zoneRealEstateType',
      'definitionsManagerEntries', 'costsManagerEntries', 'spawns', 'doors', 'definitions', 'costs', 'properties', 'placedItems', 'vendorItems',
      'placedObjects', 'mismatchedPlacedObjects', 'unlinkedItemSpawns', 'worldZones', 'vendorItems', 'heldItems', 'widgets', 'listRows']),
    'definitions': Table('definitions', ['id'], definitionFields[1:]),
    'costs': Table('costs', ['costId', 'componentId'], ['subComponents']),
    'unreadableCosts': Table('unreadableCosts', ['key'], ['costId', 'components']),
    'properties': Table('properties', ['id'], ['definitionId', 'staticId', 'parentId', 'guildId', 'fellowshipId', 'primaryOwner', 'primaryHandle',
      'defaultPermissionLevel', 'dynamicName', 'defaultUpkeepId', 'votingOptedOut'], ['upkeepExpiredTime', 'escrow', 'childIds']),
    'guildPlots': Table('guildPlots', ['guildId'], ['realEstateId']),
    'propertyItems': Table('propertyItems', ['propertyId', 'realEstateItemId'], ['itemId', 'itemName', 'ownerName'],
      ['placed', 'x', 'y', 'z', 'heading', 'pitch', 'roll', 'upkeepExpiredTime']),
    'items': Table('items', ['itemId'], itemStableFields, setFields=['actorTags', 'linkHashes', 'npcNames', 'seenAs', 'soldBy']),
    'npcLooks': Table('npcLooks', ['npcName'], ['displayedName', 'class'], setFields=[f for f in npcLookFields if f not in ('displayedName', 'class')]
      + appearanceFields + ['zones']),
    'placedObjects': Table('placedObjects', ['propertyId', 'realEstateItemId'], ['itemId', 'actorTag'],
      ['x', 'y', 'z', 'heading', 'angle', 'roll', 'scale'], setFields=['zones']),
    'vendors': Table('vendors', ['zone', 'name'], ['displayedName', 'class'], ['itemCount'], setFields=spawnVariantFields + ['positions'] + appearanceFields),
    'vendorItems': Table('vendorItems', ['zone', 'vendor', 'itemId'], ['itemName'], ['index', 'price', 'merchantQuantity', 'merchantSlot']),
    'doors': Table('doors', ['zone', 'id'], doorStableFields, doorVolatileFields),
    'spawns': Table('spawns', ['zone', 'name'], ['displayedName', 'class'], ['realEstateId', 'realEstateItemId'],
      setFields=spawnVariantFields + ['positions'] + appearanceFields),
    'worldZones': Table('worldZones', ['id'], ['shortName', 'longName', 'expansion', 'flags', 'minLevel', 'accessLevel', 'eqstrId']),
    'ground': Table('ground', ['zone', 'dropId'], ['name', 'x', 'y', 'z', 'heading', 'pitch', 'roll', 'scale', 'weight', 'itemId']),
    'windowWidgets': Table('windowWidgets', ['window', 'path'], [], ['depth', 'type', 'visible', 'text']),
    'windowRows': Table('windowRows', ['window', 'path', 'row'], [], ['cells']),
  }


def position(fields):
  return ','.join('%.1f' % float(fields[axis]) for axis in ('x', 'y', 'z', 'heading'))


def observeItem(tables, capture, itemRecord, seenAs, soldBy=None):
  fields = itemRecord['fields']
  tables['fields']['itemDefinitions'].observe((fields['itemId'],), capture, decodeRecord(capture, 'ItemDefinition', itemRecord['raw'][1],
    {'ItemNumber': fields['itemId']}))
  sets = {'seenAs': seenAs}
  if soldBy:
    sets['soldBy'] = soldBy
  tables['items'].observe((fields['itemId'],), capture, stable=pick(fields, itemStableFields[:-1]), sets={**sets, 'linkHashes': fields['linkHash']})


def equipmentText(raw, offset):
  return ','.join(':'.join(str(value) for value in struct.unpack_from('<5I', raw, offset + 20 * slot)) for slot in range(9))


def spawnAppearance(capture, spawn):
  fields = spawn['fields']
  if spawn['appearance'] is not None:
    return spawn['appearance']['fields']
  raw = spawn['raw'][0]
  signed = lambda offset: str(struct.unpack_from('<b', raw, actorOffset + offset)[0])
  unsigned = lambda offset: str(raw[actorOffset + offset])
  integer = lambda offset: str(struct.unpack_from('<i', raw, actorOffset + offset)[0])
  decoded = (integer(0x14), integer(0x1c), unsigned(0x20))
  if decoded != (fields['race'], fields['class'], fields['gender']):
    raise ValueError(f'{capture.path.name}: {fields["name"]} decodes as race, class, gender {decoded}, not the named {fields["race"]}, {fields["class"]}, {fields["gender"]}')
  return {'actorDef': raw[actorOffset + 0x21:actorOffset + 0x61].split(b'\0')[0].decode('latin-1'), 'textureType': signed(0x08),
    'material': signed(0x09), 'variation': signed(0x0a), 'headType': signed(0x0b), 'faceStyle': unsigned(0x0c), 'hairStyle': unsigned(0x11),
    'hairColor': unsigned(0x0d), 'facialHair': unsigned(0x12), 'facialHairColor': unsigned(0x0e), 'eyeColor1': unsigned(0x0f), 'eyeColor2': unsigned(0x10),
    'raceOverride': integer(0x18), 'showHelm': '1' if raw[actorOffset + 0x88] else '0', 'heritage': integer(0x8c), 'tattoo': integer(0x90),
    'details': integer(0x94), 'npcTintIndex': str(struct.unpack_from('<i', raw, npcTintIndexOffset)[0]), 'light': str(raw[lightOffset]),
    'armorColors': ','.join('%08X' % color for color in struct.unpack_from('<9I', raw, actorOffset + 0x64)),
    'actorEquipment': equipmentText(raw, actorOffset + 0x98), 'equipment': equipmentText(raw, equipmentOffset)}


def spawnPropertyId(capture, spawn):
  fields = spawn['fields']
  if 'realEstateId' in fields:
    propertyId = fields['realEstateId']
  else:
    propertyId = str(struct.unpack_from('<i', spawn['raw'][0], spawnRealEstateIdOffset)[0])
  isItemSpawn = int(fields['realEstateItemId']) > 0
  if isItemSpawn != (int(propertyId) > 0):
    raise ValueError(f'{capture.path.name}: spawn {fields["name"]} has real estate item {fields["realEstateItemId"]} but property {propertyId}')
  return propertyId


def ingestVendor(tables, capture):
  merchants = capture.all('merchant')
  if len(merchants) != 1:
    raise ValueError(f'{capture.path.name}: {len(merchants)} merchant lines')
  merchant = merchants[0]['fields']
  soldItems = capture.all('merchantItem')
  observeSpawnFields(tables, capture, merchants[0])
  tables['vendors'].observe((capture.zoneShortName, merchant['name']), capture,
    stable=pick(merchant, ['displayedName', 'class']), sets={**pick(merchant, spawnVariantFields), **pick(spawnAppearance(capture, merchants[0]), appearanceFields),
      'positions': position(merchant)},
    volatile={'itemCount': str(len(soldItems))})
  for sold in soldItems:
    if sold['item'] is None:
      raise ValueError(f'{capture.path.name}: merchant item {sold["fields"]["index"]} has no item line')
    item = sold['item']['fields']
    observeItem(tables, capture, sold['item'], 'vendor', f'{capture.zoneShortName}/{merchant["displayedName"]}')
    tables['vendorItems'].observe((capture.zoneShortName, merchant['name'], item['itemId']), capture, stable={'itemName': item['name']},
      volatile=pick(sold['fields'], ['index', 'price', 'merchantQuantity', 'merchantSlot']))
  return {'vendorItems': len(soldItems)}


def observeSpawnFields(tables, capture, spawn):
  fields = spawn['fields']
  tables['fields']['spawns'].observe((capture.zoneShortName, fields['name']), capture, decodeRecord(capture, 'PlayerClient', spawn['raw'][0],
    {'SpawnID': fields['spawnId'], 'Name': fields['name']}))


def observeZoneHeader(tables, capture):
  tables['fields']['zoneHeaders'].observe((capture.zoneShortName,), capture, decodeRecord(capture, 'zoneHeader',
    capture.headerRecords['zone']['raw'][0], {'ShortName': capture.zoneShortName}))


def ingestWorld(tables, capture):
  zones = capture.all('worldZone')
  for zone in zones:
    fields = zone['fields']
    tables['worldZones'].observe((fields['id'],), capture, stable=pick(fields, tables['worldZones'].stableFields))
    tables['fields']['worldZones'].observe((fields['id'],), capture, decodeRecord(capture, 'EQZoneInfo', zone['raw'][0],
      {'Id': fields['id'], 'ShortName': fields['shortName']}))
  return {'worldZones': len(zones)}


def ingestItems(tables, capture):
  held = capture.all('heldItem')
  for record in held:
    if record['item'] is None:
      raise ValueError(f'{capture.path.name}: held item {record["fields"]["path"]} has no item line')
    observeItem(tables, capture, record['item'], 'held')
  return {'heldItems': len(held)}


def ingestWindow(tables, capture):
  window = capture.path.name.split('_')[1]
  widgets, rows = capture.all('widget'), capture.all('listRow')
  for widget in widgets:
    tables['windowWidgets'].observe((window, widget['fields']['path']), capture, volatile=pick(widget['fields'], ['depth', 'type', 'visible', 'text']))
  for row in rows:
    tables['windowRows'].observe((window, row['fields']['path'], row['fields']['row']), capture, volatile={'cells': row['fields']['cells']})
  return {'widgets': len(widgets), 'listRows': len(rows)}


def ingestZone(tables, capture):
  zone = capture.zoneShortName
  observeZoneHeader(tables, capture)
  for definition in capture.all('definition'):
    fields = definition['fields']
    tables['definitions'].observe((fields['id'],), capture, stable=pick(fields, definitionFields[1:]))
    tables['fields']['definitions'].observe((fields['id'],), capture, decodeRecord(capture, 'RealEstateDefinition', definition['raw'][0], {'id': fields['id']}))
  for cost in capture.all('cost'):
    fields = cost['fields']
    tables['costs'].observe((fields['costId'], fields['componentId']), capture, stable={'subComponents': fields['subComponents']})
  for unreadable in capture.all('unreadableCost'):
    fields = unreadable['fields']
    tables['unreadableCosts'].observe((fields['key'],), capture, stable=pick(fields, ['costId', 'components']))
  propertyIds = set()
  for realEstate in capture.all('property'):
    fields = realEstate['fields']
    propertyIds.add(fields['id'])
    tables['properties'].observe((fields['id'],), capture, stable=pick(fields, tables['properties'].stableFields),
      volatile=pick(fields, tables['properties'].volatileFields))
    tables['fields']['properties'].observe((fields['id'],), capture, decodeRecord(capture, 'RealEstate', realEstate['raw'][0], {'id': fields['id']}))
  for guildPlot in capture.all('guildPlot'):
    fields = guildPlot['fields']
    tables['guildPlots'].observe((fields['guildId'],), capture, stable={'realEstateId': fields['realEstateId']})
  placedItems = {}
  for placed in capture.all('placedItem'):
    fields = placed['fields']
    key = (fields['realEstateId'], fields['realEstateItemId'])
    if placed['item'] is None:
      raise ValueError(f'{capture.path.name}: placed item {key} has no item line')
    placedItems[key] = placed['item']['fields']
    observeItem(tables, capture, placed['item'], 'placed')
    tables['fields']['propertyItems'].observe(key, capture, decodeRecord(capture, 'RealEstateItem', placed['raw'][0],
      {'ids.realEstateID': key[0], 'ids.realEstateItemID': key[1]}))
    tables['propertyItems'].observe(key, capture, stable={'itemId': placed['item']['fields']['itemId'], 'itemName': placed['item']['fields']['name'],
      'ownerName': fields['ownerName']}, volatile=pick(fields, tables['propertyItems'].volatileFields))
  mismatchedObjects = 0
  for placedObject in capture.all('placedObject'):
    fields = placedObject['fields']
    key = (fields['realEstateId'], fields['realEstateItemId'])
    listed = placedItems.get(key)
    if listed is not None and listed['itemId'] != fields['recordNum']:
      mismatchedObjects += 1
    tables['fields']['placedObjects'].observe(key, capture, decodeRecord(capture, 'EQPlacedItem', placedObject['raw'][0],
      {'RecordNum': fields['recordNum'], 'RealEstateID': key[0], 'RealEstateItemID': key[1]}))
    tables['placedObjects'].observe(key, capture, stable={'itemId': fields['recordNum'], 'actorTag': fields['name']},
      volatile=pick(fields, ['x', 'y', 'z', 'heading', 'angle', 'roll', 'scale']), sets={'zones': zone})
    tables['items'].observe((fields['recordNum'],), capture, sets={'seenAs': 'object', 'actorTags': fields['name']})
  unlinkedSpawns = 0
  for spawn in capture.all('spawn'):
    fields = spawn['fields']
    if fields['type'] == '0':
      continue
    propertyId = spawnPropertyId(capture, spawn)
    appearance = pick(spawnAppearance(capture, spawn), appearanceFields)
    observeSpawnFields(tables, capture, spawn)
    tables['spawns'].observe((zone, fields['name']), capture, stable=pick(fields, ['displayedName', 'class']),
      sets={**pick(fields, spawnVariantFields), **appearance, 'positions': position(fields)},
      volatile={'realEstateItemId': fields['realEstateItemId'], 'realEstateId': propertyId})
    if propertyId in noProperty:
      continue
    npcName = re.sub(r'\d+$', '', fields['name'])
    tables['npcLooks'].observe((npcName,), capture, stable=pick(fields, ['displayedName', 'class']),
      sets={**pick(fields, ['race', 'gender', 'level', 'height']), **appearance, 'zones': zone})
    item = placedItems.get((propertyId, fields['realEstateItemId']))
    if item is None:
      unlinkedSpawns += 1
      continue
    tables['items'].observe((item['itemId'],), capture, sets={'npcNames': npcName})
  for door in capture.all('door'):
    fields = door['fields']
    tables['doors'].observe((zone, fields['id']), capture, stable=pick(fields, doorStableFields), volatile=pick(fields, doorVolatileFields))
    tables['fields']['doors'].observe((zone, fields['id']), capture, decodeRecord(capture, 'EQSwitch', door['raw'][0], {'ID': fields['id']}))
  for ground in capture.all('ground'):
    fields = ground['fields']
    itemId = ground['item']['fields']['itemId'] if ground['item'] else ''
    if ground['item']:
      observeItem(tables, capture, ground['item'], 'ground')
    tables['fields']['ground'].observe((zone, fields['dropId']), capture, decodeRecord(capture, 'EQGroundItem', ground['raw'][0], {'DropID': fields['dropId']}))
    tables['ground'].observe((zone, fields['dropId']), capture,
      stable={**pick(fields, ['name', 'x', 'y', 'z', 'heading', 'pitch', 'roll', 'scale', 'weight']), 'itemId': itemId})
  managers = capture.all('manager')
  manager = managers[0]['fields'] if managers else {}
  entries = {label: capture.all(label)[0]['fields']['entries'] if capture.all(label) else '' for label in ('definitionsManager', 'costsManager')}
  return {'zoneRealEstateId': manager.get('zoneRealEstateId', ''), 'zoneRealEstateType': manager.get('zoneRealEstateType', ''),
    'definitionsManagerEntries': entries['definitionsManager'], 'costsManagerEntries': entries['costsManager'],
    'spawns': len(capture.all('spawn')), 'doors': len(capture.all('door')), 'definitions': len(capture.all('definition')),
    'costs': len(capture.all('cost')), 'properties': len(propertyIds), 'placedItems': len(placedItems),
    'placedObjects': len(capture.all('placedObject')), 'mismatchedPlacedObjects': mismatchedObjects, 'unlinkedItemSpawns': unlinkedSpawns}


def readItemsTxt():
  with (researchDirectory / 'itemsTxt' / 'itemsTxtExtract.tsv').open(encoding='latin-1', newline='') as source:
    return {row['id']: row for row in csv.DictReader(source, delimiter='\t')}


def itemsTxtConflicts(tables, extract):
  conflicts = []
  for (itemId,), row in tables['items'].rows.items():
    captured, listed = row['stable'].get('housepetFileName'), extract.get(itemId, {}).get('placeablenpcname')
    if 'name' in row['stable'] and listed is not None and captured != listed:
      conflicts.append(('itemsTxt', itemId, 'housepetFileName', captured, listed, 'itemsTxtExtract.tsv'))
  return conflicts


def writeNpcItems(tables, extract):
  looks = tables['npcLooks'].rows
  captured = tables['items'].rows
  modelled = {key[0] for key, row in captured.items() if row['stable'].get('housepetFileName')}
  modelled |= {itemId for itemId, row in extract.items() if row['placeablenpcname'] and 'name' not in captured.get((itemId,), {'stable': {}})['stable']}
  with (masterDirectory / 'npcItems.tsv').open('w', encoding='latin-1', newline='') as out:
    writer = csv.writer(out, delimiter='\t', lineterminator='\n')
    writer.writerow(['itemId', 'name', 'housepetFileName', 'modelSource', 'npcRespawnInterval', 'placeable', 'npcName', 'lookSource'] + npcLookFields
                    + ['soldBy'])
    for itemId in sorted(modelled, key=int):
      row = captured.get((itemId,), {'stable': {}, 'sets': collections.defaultdict(set)})
      stable = row['stable']
      fromCapture = bool(stable.get('housepetFileName'))
      housepet = stable['housepetFileName'] if fromCapture else extract[itemId]['placeablenpcname']
      linked = sorted(name for name in row['sets']['npcNames'] if (name,) in looks)
      npcName, lookSource = (linked[0], 'itemList') if linked else (housepet, 'housepetName')
      look = looks.get((npcName,))
      if look is None:
        npcName, lookSource = '', ''
      field = lambda name: look['stable'].get(name) or ' | '.join(sorted(look['sets'].get(name, ())))
      values = [field(name) for name in npcLookFields] if look else [''] * len(npcLookFields)
      writer.writerow([tsvText(value) for value in [itemId, stable.get('name') or extract[itemId]['name'], housepet,
        'capture' if fromCapture else 'items.txt', stable.get('npcRespawnInterval', ''), stable.get('placeable') or extract[itemId]['placeablebitfield'],
        npcName, lookSource] + values + [','.join(sorted(row['sets']['soldBy']))]])


def main():
  captures = sorted((Capture(path) for path in researchDirectory.glob('peridotLive*.txt')), key=lambda capture: (capture.stamp, capture.path.name))
  tables = buildTables()
  fieldTables = tables['fields'] = buildFieldTables()
  ingest = {'vendor': ingestVendor, 'zone': ingestZone, 'world': ingestWorld, 'items': ingestItems, 'window': ingestWindow}
  for capture in captures:
    counts = ingest[capture.kind](tables, capture)
    client = capture.header.get('client')
    tables['captures'].observe((capture.path.name,), capture, stable={'kind': capture.kind,
      'client': f'{client["date"]} {client["time"]}' if client else '', 'plugin': capture.header.get('plugin', {}).get('version', ''),
      'zone': capture.zoneShortName, 'zoneId': capture.header['zone']['zoneId'], 'instance': capture.header['zone']['instance'],
      **{field: str(value) for field, value in counts.items()}})
  del tables['fields']
  extract = readItemsTxt()
  masterDirectory.mkdir(exist_ok=True)
  for table in list(tables.values()) + list(fieldTables.values()):
    table.write()
  writeNpcItems(tables, extract)
  conflicts = [conflict for table in tables.values() for conflict in table.conflicts] + itemsTxtConflicts(tables, extract)
  with (masterDirectory / 'conflicts.tsv').open('w', encoding='latin-1', newline='') as out:
    writer = csv.writer(out, delimiter='\t', lineterminator='\n')
    writer.writerow(['table', 'key', 'field', 'earlierValue', 'laterValue', 'laterCapture'])
    writer.writerows([[tsvText(value) for value in conflict] for conflict in conflicts])
  print(f'{len(captures)} captures -> ' + ', '.join(f'{name} {len(table.rows)}' for name, table in tables.items()) + f', conflicts {len(conflicts)}')
  print('fields: ' + ', '.join(f'{name} {len(table.rows)}x{len(table.columns)}' for name, table in fieldTables.items()))


if __name__ == '__main__':
  main()
