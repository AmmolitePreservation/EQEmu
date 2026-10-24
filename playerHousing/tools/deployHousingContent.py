"""Loads the housing content into a server's database, in order, once the build's world has run the housing migrations.

The migrations (world runs them at its first boot on the build: the item tail, the schema, Live's item values and the column
files and the eq_str table, versions 1 to 12 of db_version.custom_version) must be in; then each content file loads through the MariaDB client in this
order, each refusing a second run itself: the catalog, the housing zone rows, their settings, the Guild Lobby's version 1, the guild
border doors, the housing doors, the NPC items, the guild services, the training dummies' dbstr labels, the eqstr changes (eq_str must already hold the
client's eqstr_us.txt, from import_client_files). A missing file stops the deploy before anything loads; a failing file stops it
there, the files before it loaded. Afterwards export_client_files writes dbstr_us.txt and eqstr_us.txt for players. The connection
is the server's own, from eqemu_config.json; the password reaches the client through a defaults file, never an argument. Housing
expects one database: a content_database that differs from the database is refused.

Usage: python deployHousingContent.py --config <eqemu_config.json> [--mariadb <client command>] [--sql <folder>] [--check]
"""
import argparse, json, os, shlex, subprocess, sys, tempfile

here = os.path.dirname(os.path.abspath(__file__))
contentFiles = ('housingCatalog.sql', 'housingZones.sql', 'housingZoneSettings.sql', 'guildLobby.sql', 'guildBorderDoors.sql', 'housingDoors.sql',
                'npcItems.sql', 'guildServices.sql', 'dbStrings.sql', 'eqStrings.sql')
requiredCustomVersion = 12


class DeployError(Exception):
  pass


def databaseSettings(configPath):
  with open(configPath, encoding='utf-8') as file:
    server = json.load(file)['server']
  database = server['database']
  content = server.get('content_database', {})
  if any(content.get(key) for key in ('host', 'db')) and any(content.get(key) != database.get(key) for key in ('host', 'port', 'db')):
    raise DeployError('eqemu_config.json names a content_database apart from the database; housing expects one database, where world runs its migrations')
  return {'host': database.get('host', 'localhost'), 'port': str(database.get('port', '3306')), 'user': database.get('username', 'eq'),
          'password': database.get('password', 'eq'), 'db': database.get('db', 'eq')}


def contentPaths(sqlFolder):
  paths = [os.path.join(sqlFolder, name) for name in contentFiles]
  missing = [path for path in paths if not os.path.isfile(path)]
  if missing:
    raise DeployError('content files missing: ' + ', '.join(os.path.basename(path) for path in missing))
  return paths


class Client:
  def __init__(self, command, settings):
    self.command = shlex.split(command)
    self.settings = settings
    handle, self.defaultsPath = tempfile.mkstemp(suffix='.cnf', text=True)
    with os.fdopen(handle, 'w', encoding='utf-8') as file:
      file.write(f"[client]\nhost={settings['host']}\nport={settings['port']}\nuser={settings['user']}\npassword={settings['password']}\n")
    os.chmod(self.defaultsPath, 0o600)

  def close(self):
    os.remove(self.defaultsPath)

  def run(self, arguments, stdin):
    return subprocess.run(self.command + [f'--defaults-extra-file={self.defaultsPath}', '--batch'] + arguments, input=stdin, capture_output=True, text=True)

  def customVersion(self):
    result = self.run(['-N', '-e', 'SELECT custom_version FROM db_version', self.settings['db']], None)
    if result.returncode != 0:
      raise DeployError(f'the database did not answer the version query: {result.stderr.strip()}')
    try:
      return int(result.stdout.strip())
    except ValueError:
      raise DeployError(f'db_version.custom_version is not a number: {result.stdout.strip()!r}')

  def load(self, path):
    with open(path, encoding='utf-8') as file:
      result = self.run([self.settings['db']], file.read())
    if result.returncode != 0:
      raise DeployError(f'{os.path.basename(path)} failed: {result.stderr.strip()}')


def deploy(configPath, clientCommand, sqlFolder, checkOnly):
  paths = contentPaths(sqlFolder)
  client = Client(clientCommand, databaseSettings(configPath))
  try:
    version = client.customVersion()
    if version < requiredCustomVersion:
      raise DeployError(f'the housing migrations are at custom version {version}, not {requiredCustomVersion}: start the build\'s world once first')
    print(f'housing migrations at custom version {version}; {len(paths)} content files in order: {", ".join(contentFiles)}')
    if checkOnly:
      return
    for path in paths:
      client.load(path)
      print(f'loaded {os.path.basename(path)}')
  finally:
    client.close()
  print('housing content loaded; world creates the random neighborhoods at its next boot; export_client_files now writes dbstr_us.txt and eqstr_us.txt for players')


def main():
  parser = argparse.ArgumentParser(description='Load the housing content into the server database.')
  parser.add_argument('--config', required=True, help="the server's eqemu_config.json")
  parser.add_argument('--mariadb', default='mariadb', help='the MariaDB client command')
  parser.add_argument('--sql', default=os.path.join(here, '..', 'sql'), help='the folder of content files')
  parser.add_argument('--check', action='store_true', help='verify the files and the migrations, load nothing')
  arguments = parser.parse_args()
  try:
    deploy(arguments.config, arguments.mariadb, arguments.sql, arguments.check)
  except DeployError as error:
    print(f'deploy stopped: {error}', file=sys.stderr)
    sys.exit(1)


if __name__ == '__main__':
  main()
