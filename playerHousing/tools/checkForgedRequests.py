"""checkForgedRequests.py <zone log excerpt> <results.json> [...]: stage 9's forged-request runs.

Every case passed in at least one run (the owner's and the visitor's) and in none ended otherwise than passed or
skipped, and every zone log line a passing case expects is in the excerpt of the zone log written during the runs.
"""
import json
import sys


def main(logPath, resultPaths):
  with open(logPath, encoding='utf-8', errors='replace') as file:
    log = file.read()
  outcomes = {}
  missing = []
  for path in resultPaths:
    with open(path, encoding='utf-8') as file:
      results = json.load(file)
    if results.get('aborted'):
      raise SystemExit(f"{path} was aborted: {results['aborted']}")
    for case in results['cases']:
      outcomes.setdefault(case['case'], []).append(f"{case['outcome']} ({results['character']})")
      if case['outcome'] == 'passed':
        for line in case.get('recorded', {}).get('expectedLog', []):
          if line not in log:
            missing.append((case['case'], line))
  problems = 0
  for name, seen in outcomes.items():
    if any(not outcome.startswith(('passed', 'skipped')) for outcome in seen) or not any(outcome.startswith('passed') for outcome in seen):
      print(f"{name}: {', '.join(seen)}")
      problems += 1
  for name, line in missing:
    print(f'{name}: the zone log lacks "{line}"')
    problems += 1
  print(f'{len(outcomes)} cases in {len(resultPaths)} runs, {problems} problems')
  return 1 if problems else 0


if __name__ == '__main__':
  if len(sys.argv) < 3:
    raise SystemExit(__doc__)
  sys.exit(main(sys.argv[1], sys.argv[2:]))
