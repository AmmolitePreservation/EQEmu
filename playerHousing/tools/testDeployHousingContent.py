"""Tests deployHousingContent.py with a stand-in MariaDB client that records what it was asked: the file order, the migration
version gate, the stop at a missing or failing file, the one-database rule, the check mode, and the password's path.

Usage: python testDeployHousingContent.py
"""
import json, os, re, shutil, subprocess, sys, tempfile

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import deployHousingContent as deploy

work = tempfile.mkdtemp()
stubPath = os.path.join(work, 'mariadbStub.py')
logPath = os.path.join(work, 'calls.jsonl')
with open(stubPath, 'w', encoding='utf-8') as file:
  file.write(f'''import json, os, sys
stdin = sys.stdin.read() if not sys.stdin.isatty() else ''
defaults = [a for a in sys.argv if a.startswith('--defaults-extra-file=')][0].split('=', 1)[1]
with open({logPath!r}, 'a', encoding='utf-8') as log:
  log.write(json.dumps({{'args': sys.argv[1:], 'stdinStart': stdin[:60], 'defaults': open(defaults, encoding='utf-8').read()}}) + '\\n')
if '-e' in sys.argv:
  print(os.environ.get('STUB_VERSION', '11'))
elif 'FAIL' in stdin:
  print('ERROR 1644 (45000) at line 3: the stub refused this file', file=sys.stderr)
  sys.exit(1)
''')
stubCommand = f'"{sys.executable}" "{stubPath}"'
configPath = os.path.join(work, 'eqemu_config.json')
with open(configPath, 'w', encoding='utf-8') as file:
  json.dump({'server': {'database': {'host': 'db.example', 'port': '3307', 'username': 'eqemu', 'password': 'secret-word', 'db': 'peq'}}}, file)
sqlFolder = os.path.join(work, 'sql')
os.makedirs(sqlFolder)
for name in deploy.contentFiles:
  with open(os.path.join(sqlFolder, name), 'w', encoding='utf-8') as file:
    file.write(f'-- {name}\nSELECT 1;\n')


def check(label, actual, expected):
  if actual != expected:
    raise AssertionError(f'{label}: expected {expected!r}, got {actual!r}')
  print(f'{label}: {actual!r}')


def run(*extra, version=str(deploy.requiredCustomVersion)):
  if os.path.exists(logPath):
    os.remove(logPath)
  result = subprocess.run([sys.executable, os.path.join(here, 'deployHousingContent.py'), '--config', configPath, '--mariadb', stubCommand, '--sql', sqlFolder, *extra],
                          capture_output=True, text=True, env={**os.environ, 'STUB_VERSION': version})
  calls = [json.loads(line) for line in open(logPath, encoding='utf-8')] if os.path.exists(logPath) else []
  return result, calls


manifest = open(os.path.join(here, 'makeHousingManifest.py'), encoding='utf-8').read()
check('the required custom version is the manifest\'s highest', deploy.requiredCustomVersion, max(int(v) for v in re.findall(r"^\s+\((\d+), '", manifest, re.M)))
check('ten content files, the catalog first, the eqstr changes last', (len(deploy.contentFiles), deploy.contentFiles[0], deploy.contentFiles[-1]),
      (10, 'housingCatalog.sql', 'eqStrings.sql'))
check('the housing doors load after the guild border doors and the Guild Lobby', deploy.contentFiles.index('housingDoors.sql') > deploy.contentFiles.index('guildBorderDoors.sql')
      and deploy.contentFiles.index('housingDoors.sql') > deploy.contentFiles.index('guildLobby.sql'), True)
check('the guild services load after the NPC items', deploy.contentFiles.index('guildServices.sql') > deploy.contentFiles.index('npcItems.sql'), True)
check('every content file exists in the repository', deploy.contentPaths(os.path.join(here, '..', 'sql')) != [], True)

result, calls = run()
check('a full run succeeds', result.returncode, 0)
check('the version query comes first, then every file in order', [c['stdinStart'].split('\n')[0] or c['args'][-2] for c in calls],
      ['SELECT custom_version FROM db_version'] + [f'-- {name}' for name in deploy.contentFiles])
check('each call names the database and reads the defaults file, never a password argument',
      (all(c['args'][-1] == 'peq' for c in calls), any('secret-word' in a for c in calls for a in c['args'])), (True, False))
check('the defaults file carried the connection', calls[0]['defaults'], '[client]\nhost=db.example\nport=3307\nuser=eqemu\npassword=secret-word\n')
check('the defaults file is gone afterwards', os.path.exists(calls[0]['args'][0].split('=', 1)[1]), False)
check('the run reports each file', result.stdout.count('loaded '), len(deploy.contentFiles))

result, calls = run('--check')
check('check mode queries the version and loads nothing', (result.returncode, len(calls), 'content files in order' in result.stdout), (0, 1, True))

result, calls = run(version='4')
check('migrations behind the build are refused before any load', (result.returncode, len(calls), f'custom version 4, not {deploy.requiredCustomVersion}' in result.stderr), (1, 1, True))

with open(os.path.join(sqlFolder, 'guildBorderDoors.sql'), 'w', encoding='utf-8') as file:
  file.write('-- guildBorderDoors.sql\nFAIL;\n')
result, calls = run()
check('a failing file stops the deploy there, the files before it loaded', (result.returncode, [c['stdinStart'].split('\n')[0] for c in calls[1:]], 'guildBorderDoors.sql failed' in result.stderr),
      (1, ['-- housingCatalog.sql', '-- housingZones.sql', '-- housingZoneSettings.sql', '-- guildLobby.sql', '-- guildBorderDoors.sql'], True))

os.remove(os.path.join(sqlFolder, 'npcItems.sql'))
result, calls = run()
check('a missing file stops the deploy before any call', (result.returncode, len(calls), 'content files missing: npcItems.sql' in result.stderr), (1, 0, True))

with open(configPath, 'w', encoding='utf-8') as file:
  json.dump({'server': {'database': {'host': 'db.example', 'port': '3307', 'username': 'eqemu', 'password': 'x', 'db': 'peq'},
                        'content_database': {'host': 'content.example', 'port': '3307', 'username': 'eqemu', 'password': 'x', 'db': 'peq_content'}}}, file)
with open(os.path.join(sqlFolder, 'npcItems.sql'), 'w', encoding='utf-8') as file:
  file.write('-- npcItems.sql\nSELECT 1;\n')
result, calls = run()
check('a separate content database is refused', (result.returncode, len(calls), 'housing expects one database' in result.stderr), (1, 0, True))
shutil.rmtree(work)
print('all deploy checks pass')
