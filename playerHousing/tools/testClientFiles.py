"""Tests makeClientFiles.py on stand-in clients: a small stock RoF2 install, a Live client holding the listed lines and a 73-model
lobby archive, and an empty clone; the built tree's exact bytes, then each refusal.

Usage: python testClientFiles.py
"""
import csv, os, shutil, struct, sys, tempfile, zlib

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import makeClientFiles as make  # noqa: E402

work = tempfile.mkdtemp()
content = os.path.join(here, '..', 'content')


def lines(path):
  return [line for line in open(path, 'rb').read().decode('latin-1').replace('\r\n', '\n').split('\n') if line != '']


furniture, races = lines(os.path.join(content, make.furnitureList)), lines(os.path.join(content, make.raceList))
petRows = list(csv.DictReader(open(os.path.join(content, make.petModelsList), encoding='latin-1', newline=''), delimiter='\t', quoting=csv.QUOTE_NONE))
globalRows = [row for row in petRows if not row['liveSource'].startswith('onDemand:')]
stockOnDemand = ['omensequip.eqg^GCST_BA_1_IT10748.ANI^GCST_BA_1_IT10748^EQGA', 'omensequip.eqg^GCST_BA_1_IT10750.ANI^GCST_BA_1_IT10750^EQGA', 'rafclothgold.eqg^IT83955.MOD^IT83955_ACTORDEF^EQGM']
stockChr = ['2', 'liz,liz_chr', 'rea,rea_chr']
stockModels = ['aframe_.mod', 'guild_door_.mod', 'trans_entry.mod', 'obj_altar_.mod']
liveModels = ['obj_irongateswitch.mod', 'obj_guild_door.mod', 'obj_aframe.mod', 'obj_trans_entry.mod'] + [f'obj_lamp_{k:02}.mod' for k in range(69)]
zon = b'*NAME guildlobby\r\n*VERSION 4\r\n'


def writePFS(path, files):
  body, entries, position = b'', [], 12
  for name, data in list(files.items()) + [(None, struct.pack('<I', len(files)) + b''.join(struct.pack('<I', len(n) + 1) + n.encode('ascii') + b'\0' for n in files))]:
    deflated = zlib.compress(data)
    entries.append((0x61580AC9 if name is None else zlib.crc32(name.encode('ascii')), position, len(data)))
    block = struct.pack('<II', len(deflated), len(data)) + deflated
    body += block
    position += len(block)
  directory = struct.pack('<I', len(entries)) + b''.join(struct.pack('<III', *entry) for entry in entries)
  with open(path, 'wb') as file:
    file.write(struct.pack('<I4sI', position, b'PFS ', 0x00020000) + body + directory)


def writeText(path, textLines, trailingNewline=True):
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'wb') as file:
    file.write(('\r\n'.join(textLines) + ('\r\n' if trailingNewline else '')).encode('ascii'))


def makeClients(stockOnDemandLines=stockOnDemand, stockChrLines=stockChr, liveOnDemandLines=None, stockModelNames=stockModels, liveModelNames=liveModels, liveZon=zon, dropArchive=None):
  shutil.rmtree(work, ignore_errors=True)
  install, live, clone = (os.path.join(work, name) for name in ('rof2Install', 'liveClient', 'clone'))
  writeText(os.path.join(install, make.onDemandFile), stockOnDemandLines)
  writeText(os.path.join(install, make.globalChrFile), stockChrLines)
  writeText(os.path.join(live, make.onDemandFile), liveOnDemandLines if liveOnDemandLines is not None else ['other.eqg^IT1.MOD^IT1_ACTORDEF^EQGM'] + races + furniture)
  writeText(os.path.join(live, make.globalChrFile), ['71', 'ghm,ghm_chr'], trailingNewline=False)
  archives = {line.split('^')[0] for line in furniture + races} | {row['rof2ChrArchive'] for row in petRows}
  for archive in archives - {dropArchive}:
    open(os.path.join(install, archive), 'wb').close()
  writePFS(os.path.join(install, make.lobbyArchive), {name: name.encode('ascii') * 3 for name in stockModelNames + ['guildlobby.zon']})
  writePFS(os.path.join(live, make.lobbyArchive), {name: name.encode('ascii') * 5 for name in liveModelNames + ['ab_crate_8_n.dds']})
  open(os.path.join(install, make.lobbyZon), 'wb').write(zon)
  open(os.path.join(live, make.lobbyZon), 'wb').write(liveZon)
  os.makedirs(clone)
  return install, live, clone


def check(label, actual, expected):
  if actual != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
  print(f'{label}: {str(actual)[:90]!r}')


def refuses(label, fragment, **clients):
  install, live, clone = makeClients(**clients)
  try:
    make.build(install, live, clone)
  except ValueError as error:
    if fragment not in str(error):
      raise AssertionError(f'{label}: refused for another reason: {error}')
    print(f'{label}: raises ({str(error)[:110]})')
    return
  raise AssertionError(f'{label}: built')


install, live, clone = makeClients()
summary = make.build(install, live, clone)
root = os.path.join(clone, 'rof2')
check('the summary counts the lines, the count and the archive', summary, {'onDemandLines': 3 + 56 + 255, 'globalChrCount': 2 + 55, 'lobbyArchiveBytes': os.path.getsize(os.path.join(live, make.lobbyArchive))})
check('the tree holds the three files and nothing else', sorted(os.path.relpath(os.path.join(folder, name), root).replace(os.sep, '/') for folder, _, names in os.walk(root) for name in names),
      ['Resources/GlobalLoad_chr.txt', 'Resources/OnDemandResources.txt', 'guildlobby.eqg'])
check('OnDemandResources.txt is the stock lines, the furniture lines, then the race lines, CRLF', open(os.path.join(root, make.onDemandFile), 'rb').read(),
      ('\r\n'.join(stockOnDemand + furniture + races) + '\r\n').encode('ascii'))
check('GlobalLoad_chr.txt raises the count and appends the 55 S3D races in the table order', open(os.path.join(root, make.globalChrFile), 'rb').read(),
      ('\r\n'.join(['57', 'liz,liz_chr', 'rea,rea_chr'] + [row['chrLine'] for row in globalRows]) + '\r\n').encode('ascii'))
check('the first added race is the spirit wolf from sro_chr', [row['chrLine'] for row in globalRows][0], 'spi,sro_chr')
check("guildlobby.eqg is Live's archive byte for byte", open(os.path.join(root, make.lobbyArchive), 'rb').read(), open(os.path.join(live, make.lobbyArchive), 'rb').read())
check('a second build rewrites the same tree', make.build(install, live, clone), summary)

refuses('a stock OnDemandResources.txt that already carries a listed line', 'already carries', stockOnDemandLines=stockOnDemand + [furniture[0]])
refuses("a Live OnDemandResources.txt that lacks a listed line", "Live's", liveOnDemandLines=furniture + races[1:])
refuses('an install without an archive the lines name', 'lacks the archive lon16.eqg', dropArchive='lon16.eqg')
refuses('a stock GlobalLoad_chr.txt that already carries a race', 'already carries the race spi', stockChrLines=['3', 'liz,liz_chr', 'rea,rea_chr', 'spi,sro_chr'])
refuses('a stock GlobalLoad_chr.txt whose count is off', 'counts 3 lines but holds 2', stockChrLines=['3', 'liz,liz_chr', 'rea,rea_chr'])
refuses('a Live archive with 72 models', 'holds 72 models', liveModelNames=liveModels[:-1])
refuses('a Live archive without the gate', 'without OBJ_IRONGATESWITCH', liveModelNames=liveModels[1:] + ['obj_extra.mod'])
refuses('a stock archive that already holds the gate', 'already holds OBJ_IRONGATESWITCH', stockModelNames=stockModels + ['obj_irongateswitch.mod'])
refuses('a guildlobby.zon that differs between the clients', 'differs between the two clients', liveZon=zon + b'*LAYER 1\r\n')
shutil.rmtree(work, ignore_errors=True)
print('all client file checks pass')
