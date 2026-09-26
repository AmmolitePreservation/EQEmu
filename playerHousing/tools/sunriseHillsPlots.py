"""Derives the Sunrise Hills plot template from the neighborhood map's plot layer (content/neighborhood_2.txt).

The layer draws each of the 71 player plots as four line segments and labels it with its street address. Each plot's box
(center, size, orientation) is fitted to its four sides, and its purchase price, daily upkeep, item capacity and pet cap
come from Live's plot table (Allakhazam, "EQ:Player Housing in EverQuest"). Coordinates are the map file's.

Usage: python3 sunriseHillsPlots.py [neighborhood_2.txt] > sunriseHillsPlots.json
"""
import collections, json, math, os, re, sys

# Live's plot table: (street, the house numbers the row covers or None for the rest of the street) ->
# (price in platinum, daily upkeep in copper, item capacity, pet cap, plot count).
liveGroups = [
  ('Betwixt Avenue', None, 84, 8400, 105, 5, 7),
  ('Brimming Heights', None, 126, 12600, 120, 6, 8),
  ('Brimming Way', {101, 102, 107}, 168, 16800, 120, 6, 3),
  ('Brimming Way', {113}, 42, 4200, 90, 4, 1),
  ('Brimming Way', None, 84, 8400, 105, 5, 9),
  ('Guild Way', None, 84, 8400, 105, 5, 5),
  ('Market Street', {101, 102}, 42, 4200, 90, 4, 2),
  ('Market Street', {103, 104}, 21, 2100, 90, 4, 2),
  ('Market Heights', None, 168, 16800, 120, 6, 3),
  ('Park Circle', None, 126, 12600, 105, 6, 8),
  ('Terminus Street', {107}, 126, 12600, 120, 6, 1),
  ('Terminus Street', None, 84, 8400, 105, 5, 5),
  ('Terminus Heights', None, 168, 16800, 120, 6, 1),
  ('Vanward Street', {105, 116}, 21, 2100, 90, 4, 2),
  ('Vanward Street', None, 84, 8400, 105, 5, 13),
  ('Vanward Heights', None, 168, 16800, 120, 6, 1),
]


def parse(path):
  segments, labels = [], []
  for raw in open(path, encoding='latin-1'):
    raw = raw.strip()
    if raw.startswith('L '):
      v = [float(x) for x in raw[2:].split(',')[:6]]
      segments.append(((v[0], v[1], v[2]), (v[3], v[4], v[5])))
    elif raw.startswith('P '):
      p = [x.strip() for x in raw[2:].split(',')]
      labels.append(((float(p[0]), float(p[1]), float(p[2])), p[7].replace('_', ' ')))
  return segments, labels


def fitBoxes(segments, joinTolerance=6.0):
  parent = list(range(len(segments)))
  def find(i):
    while parent[i] != i:
      parent[i] = parent[parent[i]]; i = parent[i]
    return i
  near = lambda p, q: math.hypot(p[0] - q[0], p[1] - q[1]) < joinTolerance
  for i in range(len(segments)):
    for j in range(i + 1, len(segments)):
      if any(near(p, q) for p in segments[i] for q in segments[j]):
        parent[find(i)] = find(j)
  groups = collections.defaultdict(list)
  for i, segment in enumerate(segments):
    groups[find(i)].append(segment)
  boxes = []
  for sides in groups.values():
    if len(sides) != 4:
      raise ValueError(f'a plot outline with {len(sides)} sides')
    corners = []
    for segment in sides:
      for p in segment:
        if not any(near(p, c) for c in corners):
          corners.append(p)
    if len(corners) != 4:
      raise ValueError(f'a plot outline with {len(corners)} corners')
    cx, cy, cz = (sum(c[k] for c in corners) / 4 for k in range(3))
    a, b = max(sides, key=lambda s: math.hypot(s[1][0] - s[0][0], s[1][1] - s[0][1]))
    angle = math.atan2(b[1] - a[1], b[0] - a[0])
    u, v = (math.cos(angle), math.sin(angle)), (-math.sin(angle), math.cos(angle))
    extent = lambda w: 2 * max(abs((c[0] - cx) * w[0] + (c[1] - cy) * w[1]) for c in corners)
    boxes.append(dict(center=(cx, cy, cz), sideAngle=math.degrees(angle) % 180, sizeAlong=extent(u), sizeAcross=extent(v),
                      zRange=(min(c[2] for c in corners), max(c[2] for c in corners)), u=u, v=v))
  return boxes


def matchLabels(boxes, labels, margin=20.0):
  def near(box, p):
    cx, cy, _ = box['center']
    return (abs((p[0] - cx) * box['u'][0] + (p[1] - cy) * box['u'][1]) <= box['sizeAlong'] / 2 + margin and
            abs((p[0] - cx) * box['v'][0] + (p[1] - cy) * box['v'][1]) <= box['sizeAcross'] / 2 + margin)
  plots = []
  for position, address in labels:
    candidates = [box for box in boxes if near(box, position)]
    if not candidates:
      raise ValueError(f'{address}: no plot outline near its label')
    box = min(candidates, key=lambda b: math.hypot(b['center'][0] - position[0], b['center'][1] - position[1]))
    plots.append(dict(address=address, label=position, box=box))
  if len({id(p['box']) for p in plots}) != len(plots):
    raise ValueError('two labels matched one plot outline')
  return plots


def liveGroup(address):
  match = re.match(r'(?:(\d+)\s+)?(.+)$', address)
  number, street = (int(match.group(1)) if match.group(1) else None), match.group(2)
  for group in liveGroups:
    if group[0] == street and (group[1] is None or number in group[1]):
      return group
  raise ValueError(f'{address}: not in Live\'s plot table')


if __name__ == '__main__':
  here = os.path.dirname(os.path.abspath(__file__))
  segments, labels = parse(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, '..', 'content', 'neighborhood_2.txt'))
  plots = matchLabels(fitBoxes(segments), labels)
  counts = collections.Counter()
  rows = []
  for plot in plots:
    group = liveGroup(plot['address'])
    counts[group[:2] if group[1] is None else (group[0], tuple(sorted(group[1])))] += 1
    box = plot['box']
    rows.append(dict(address=plot['address'], mapCenter=[round(c, 3) for c in box['center']], sizeAlong=round(box['sizeAlong'], 2),
                     sizeAcross=round(box['sizeAcross'], 2), mapSideAngle=round(box['sideAngle'], 3),
                     mapZRange=[round(z, 3) for z in box['zRange']], pricePlatinum=group[2], upkeepCopperPerDay=group[3],
                     capacity=group[4], pets=group[5]))
  for group in liveGroups:
    key = group[:2] if group[1] is None else (group[0], tuple(sorted(group[1])))
    if counts[key] != group[6]:
      raise ValueError(f'{group[0]} {group[1] or ""}: {counts[key]} plots on the map, {group[6]} in Live\'s table')
  print(json.dumps(rows, indent=1))
  print(f'{len(rows)} plots; every group matches Live\'s table', file=sys.stderr)
