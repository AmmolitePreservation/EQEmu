"""Builds the eqemu-client-files rof2/ tree from a clean RoF2 install and Live's client (PAT-9): the finished client files no
table drives, built and never edited by hand.

Resources/OnDemandResources.txt is the stock file plus Live's on-demand lines for the furniture19, furnitureiksar and lon16 models
(content/onDemandAdditionsFromLive.txt, PAT-4) and for the NPC race archives RoF2 ships but lists nowhere
(content/onDemandRaceAdditionsFromLive.txt, PAT-5); every line is checked against Live's own file and its archive against the
install. Resources/GlobalLoad_chr.txt is the stock file plus the S3D housepet races Live loads globally (content/zoneOnlyPetModels.tsv's
rows outside the on-demand archives, PAT-6), line 1's count raised, since the client's parser drops lines past the count.
guildlobby.eqg is Live's archive (DOR-17): its 73 models hold the lobby's OBJ_ door models, the gate among them, which the stock
archive lacks; guildlobby.zon is byte-identical in both clients and is not shipped. A stock file that already carries a line, a
Live file or archive without what the lists name, or a differing guildlobby.zon refuses the build.

Usage: python makeClientFiles.py <clean RoF2 install> <Live client> <eqemu-client-files clone>
"""
import csv, os, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from eqgV4Placements import readPFS  # noqa: E402
from makeHousingDoorsSql import gateModel  # noqa: E402

contentFolder = os.path.join(here, '..', 'content')
furnitureList, raceList, petModelsList = 'onDemandAdditionsFromLive.txt', 'onDemandRaceAdditionsFromLive.txt', 'zoneOnlyPetModels.tsv'
onDemandFile = os.path.join('Resources', 'OnDemandResources.txt')
globalChrFile = os.path.join('Resources', 'GlobalLoad_chr.txt')
lobbyArchive, lobbyZon = 'guildlobby.eqg', 'guildlobby.zon'
clientFolder = 'rof2'
liveLobbyModels = 73
onDemandSource = 'onDemand:'


def readLines(path):
  text = open(path, 'rb').read().decode('latin-1')
  lines = text.replace('\r\n', '\n').split('\n')
  if lines and lines[-1] == '':
    lines.pop()
  return lines


def contentLines(name):
  lines = readLines(os.path.join(contentFolder, name))
  if '' in lines or len(set(lines)) != len(lines):
    raise ValueError(f'content/{name} has an empty or repeated line')
  return lines


def petModelRows():
  with open(os.path.join(contentFolder, petModelsList), encoding='latin-1', newline='') as file:
    return list(csv.DictReader(file, delimiter='\t', quoting=csv.QUOTE_NONE))


def installNames(install):
  return {name.lower() for name in os.listdir(install)}


def onDemandResources(install, liveClient, names):
  stock = readLines(os.path.join(install, onDemandFile))
  stockSet, liveSet = set(stock), set(readLines(os.path.join(liveClient, onDemandFile)))
  additions = contentLines(furnitureList) + contentLines(raceList)
  for line in additions:
    if line in stockSet:
      raise ValueError(f'the stock {onDemandFile} already carries {line}; not a clean install')
    if line not in liveSet:
      raise ValueError(f"Live's {onDemandFile} lacks {line}")
    archive = line.split('^')[0]
    if archive.lower() not in names:
      raise ValueError(f'the install lacks the archive {archive} that {line} names')
  return stock + additions


def raceArchivesAgree():
  listed = {line.split('^')[0].lower() for line in contentLines(raceList)}
  rows = {row['liveSource'][len(onDemandSource):].lower() for row in petModelRows() if row['liveSource'].startswith(onDemandSource)}
  if listed != rows:
    raise ValueError(f'content/{raceList} names the archives {sorted(listed)} but content/{petModelsList} {sorted(rows)}')


def globalLoadChr(install, names):
  stock = readLines(os.path.join(install, globalChrFile))
  count, entries = int(stock[0]), stock[1:]
  if count != len(entries):
    raise ValueError(f'the stock {globalChrFile} counts {count} lines but holds {len(entries)}')
  tags = {entry.split(',')[0] for entry in entries}
  additions = []
  for row in petModelRows():
    if row['liveSource'].startswith(onDemandSource):
      continue
    tag, archive = row['chrLine'].split(',')
    if tag in tags:
      raise ValueError(f'the stock {globalChrFile} already carries the race {tag}; not a clean install')
    if os.path.splitext(row['rof2ChrArchive'])[0].lower() != archive.lower() or row['rof2ChrArchive'].lower() not in names:
      raise ValueError(f"{row['actorDef']}: the install lacks {row['rof2ChrArchive']} for the line {row['chrLine']}")
    tags.add(tag)
    additions.append(row['chrLine'])
  return [str(count + len(additions))] + entries + additions


def lobbyArchiveBytes(install, liveClient):
  liveModels = {name.lower() for name in readPFS(os.path.join(liveClient, lobbyArchive)) if name.lower().endswith('.mod')}
  gate = gateModel.lower() + '.mod'
  if len(liveModels) != liveLobbyModels or gate not in liveModels:
    raise ValueError(f"Live's {lobbyArchive} holds {len(liveModels)} models{'' if gate in liveModels else ' without ' + gateModel}, not Live's {liveLobbyModels}")
  if gate in {name.lower() for name in readPFS(os.path.join(install, lobbyArchive))}:
    raise ValueError(f'the stock {lobbyArchive} already holds {gateModel}; not a clean install')
  if open(os.path.join(install, lobbyZon), 'rb').read() != open(os.path.join(liveClient, lobbyZon), 'rb').read():
    raise ValueError(f'{lobbyZon} differs between the two clients; it is not shipped')
  return open(os.path.join(liveClient, lobbyArchive), 'rb').read()


def writeLines(path, lines):
  os.makedirs(os.path.dirname(path), exist_ok=True)
  with open(path, 'wb') as file:
    file.write(('\r\n'.join(lines) + '\r\n').encode('ascii'))


def build(install, liveClient, clone):
  names = installNames(install)
  raceArchivesAgree()
  onDemand = onDemandResources(install, liveClient, names)
  globalChr = globalLoadChr(install, names)
  archive = lobbyArchiveBytes(install, liveClient)
  root = os.path.join(clone, clientFolder)
  writeLines(os.path.join(root, onDemandFile), onDemand)
  writeLines(os.path.join(root, globalChrFile), globalChr)
  with open(os.path.join(root, lobbyArchive), 'wb') as file:
    file.write(archive)
  return {'onDemandLines': len(onDemand), 'globalChrCount': int(globalChr[0]), 'lobbyArchiveBytes': len(archive)}


def main():
  if len(sys.argv) != 4:
    raise SystemExit(__doc__.strip().splitlines()[-1])
  summary = build(*sys.argv[1:4])
  print(f"wrote {os.path.join(sys.argv[3], clientFolder)}: {onDemandFile} {summary['onDemandLines']} lines, {globalChrFile} count "
        f"{summary['globalChrCount']}, {lobbyArchive} {summary['lobbyArchiveBytes']} bytes")


if __name__ == '__main__':
  main()
