"""compareProbes.py <before.json> <after.json>: two probes of one client hold the same values but the clocks (stage 9)."""
import json
import sys

# Readings that advance between any two probes of an idle client.
clockPaths = {'probe.time', 'manager.standingUpdatedTime'}
clockLeaves = {'nextFlushTime', 'nextRefreshTime'}
absent = object()


def flatten(value, prefix, flat):
  if isinstance(value, dict):
    for key, member in value.items():
      flatten(member, f'{prefix}.{key}' if prefix else key, flat)
  elif isinstance(value, list):
    for index, member in enumerate(value):
      flatten(member, f'{prefix}.{index}', flat)
  else:
    flat[prefix] = value


def isClock(path):
  return path in clockPaths or path.rsplit('.', 1)[-1] in clockLeaves


def main(beforePath, afterPath):
  with open(beforePath, encoding='utf-8') as file:
    before = json.load(file)
  with open(afterPath, encoding='utf-8') as file:
    after = json.load(file)
  if before['probe']['character'] != after['probe']['character']:
    raise SystemExit(f"the probes are of {before['probe']['character']} and {after['probe']['character']}")
  beforeValues = {}
  afterValues = {}
  flatten(before, '', beforeValues)
  flatten(after, '', afterValues)
  differences = [path for path in sorted(beforeValues.keys() | afterValues.keys())
                 if not isClock(path) and beforeValues.get(path, absent) != afterValues.get(path, absent)]
  for path in differences:
    print(f'{path}: {beforeValues.get(path, "(absent)")} -> {afterValues.get(path, "(absent)")}')
  print(f"{before['probe']['character']}: {len(differences)} differences")
  return 1 if differences else 0


if __name__ == '__main__':
  if len(sys.argv) != 3:
    raise SystemExit(__doc__)
  sys.exit(main(sys.argv[1], sys.argv[2]))
