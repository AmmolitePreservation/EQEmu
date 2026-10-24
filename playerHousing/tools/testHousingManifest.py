"""Tests that the custom database manifest in the code is what makeHousingManifest.py generates, and that the two places
naming its last version agree with it: common/version.h's CUSTOM_BINARY_DATABASE_VERSION, which world compares with
db_version.custom_version to decide whether to run the updates at boot, and deployHousingContent.py's requiredCustomVersion.

Usage: python testHousingManifest.py
"""
import os, re, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
root = os.path.join(here, '..', '..')
sys.path.insert(0, here)
import deployHousingContent  # noqa: E402

generated = subprocess.run([sys.executable, os.path.join(here, 'makeHousingManifest.py')], capture_output=True, text=True, check=True).stdout.replace('\r\n', '\n')
versions = [int(v) for v in re.findall(r'^\t\t\.version = (\d+),$', generated, re.M)]
if versions != list(range(1, len(versions) + 1)):
  raise AssertionError(f'the generated versions are not 1..n: {versions}')
print(f'the generator writes versions 1 to {versions[-1]}')

manifest = open(os.path.join(root, 'common', 'database', 'database_update_manifest_custom.cpp'), encoding='utf-8').read().replace('\r\n', '\n')
head, tail = 'std::vector<ManifestEntry> manifest_entries_custom = {\n', '// Used for testing\n'
inCode = manifest[manifest.index(head) + len(head):manifest.index(tail)]
if inCode != generated:
  raise AssertionError('database_update_manifest_custom.cpp differs from what makeHousingManifest.py generates; regenerate it')
print('the manifest in the code is the generated one')

versionHeader = open(os.path.join(root, 'common', 'version.h'), encoding='utf-8').read()
binaryVersion = int(re.search(r'^#define CUSTOM_BINARY_DATABASE_VERSION (\d+)$', versionHeader, re.M).group(1))
if binaryVersion != versions[-1]:
  raise AssertionError(f"version.h's CUSTOM_BINARY_DATABASE_VERSION is {binaryVersion}, the manifest's last version {versions[-1]}; world would stop its updates at {binaryVersion}")
print(f"version.h's CUSTOM_BINARY_DATABASE_VERSION is {binaryVersion}")
if deployHousingContent.requiredCustomVersion != versions[-1]:
  raise AssertionError(f'deployHousingContent.py requires custom version {deployHousingContent.requiredCustomVersion}, the manifest ends at {versions[-1]}')
print(f'deployHousingContent.py requires {deployHousingContent.requiredCustomVersion}')
print('all manifest checks pass')
