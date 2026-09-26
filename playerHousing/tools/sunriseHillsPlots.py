"""Derives the Sunrise Hills plot template from the neighborhood map's plot layer (content/neighborhood_2.txt).

The layer draws each of the 71 player plots as four line segments and labels it with its street address. Each plot's box
(center, size, orientation) is fitted to its four sides, and its purchase price, daily upkeep, item capacity and pet cap
come from Live's plot table (Allakhazam, "EQ:Player Housing in EverQuest").

World coordinates: the map stores (-x, -y) (verified: every plot center lands within 1.2 units of one of Live's plot border
stones, the OBP_LOTSQUARE doors in content/sunriseHillsLotSquares.tsv, exported from PEQ's doors table). EQ headings (512 per
turn, 0 facing +y) are 90 degrees minus the box edge's angle from +x (verified: every plot within 0.7 degrees of its stone's
heading, the doors' own rounding). Each plot takes its stone's door id and ground height. The two stones no player plot claims
lie outside the zone's terrain and aren't plots.

The four guild plots come from Live: /loc (which prints y, x, z) at their border's corner spires, in opposite-corner pairs,
with the side the entrance opens to and the ground height of the pad the border stands on (the zone's terrain). Each pair
fits the border (obp_guildsquare.mod: 699.8 x 351.2, its center 4.89, -1.30 from its origin) in one of two mirror-image
orientations; the entrance picks one. A border's entrance faces opposite its heading (107 Vanward Street's, heading 128,
opens east), and a model point (a, b) lands at (-b cos t - a sin t, b sin t - a cos t) from the door, t the angle of the
heading plus a half turn. The guild plots have no border stone in PEQ's doors, so each gives the position for a new one.

Usage: python3 sunriseHillsPlots.py > sunriseHillsPlots.json
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

# (section, /loc readings in opposite-corner pairs, the entrance's side, the pad's ground height)
liveGuildPlots = [
  ('Guild', [(2439.51, 2356.74, 114.46), (2830.33, 3032.31, 114.54), (2482.55, 3053.34, 114.46), (2787.87, 2336.02, 113.99)], 'east', 110),
  ('Terminus', [(2248.72, -745.32, 114.35), (2605.96, -1438.99, 114.45)], 'west', 110),
  ('Brimming', [(-1872.99, 732.14, 4.86), (-2583.03, 409.50, 48.46)], 'north', 0),
  ('Vanward', [(-602.29, 2672.69, 4.46), (-908.07, 3390.33, 5.69)], 'east', 0),
]
guildBorderLength, guildBorderWidth = 699.8, 351.2
guildBorderCenter = (4.89, -1.30)
compassSides = dict(north=(0, 1), south=(0, -1), west=(1, 0), east=(-1, 0))


def guildPlotRow(section, readings, entrance, ground):
  corners = [(x, y) for y, x, _ in readings]
  if len(corners) % 2:
    raise ValueError(f'{section}: corners come in opposite pairs')
  entranceX, entranceY = compassSides[entrance]
  diagonalOffset = math.atan2(guildBorderWidth, guildBorderLength)
  facings = []
  for (x1, y1), (x2, y2) in zip(corners[0::2], corners[1::2]):
    diagonal = math.hypot(x2 - x1, y2 - y1)
    if abs(diagonal - math.hypot(guildBorderLength, guildBorderWidth)) > 6:
      raise ValueError(f"{section}: corners {diagonal:.1f} apart, not the border's diagonal")
    fits = []
    for angle in (math.atan2(y2 - y1, x2 - x1) - diagonalOffset, math.atan2(y2 - y1, x2 - x1) + diagonalOffset):
      alignment = math.cos(angle) * entranceX + math.sin(angle) * entranceY
      if abs(alignment) > math.cos(math.radians(20)):
        fits.append(math.atan2(math.sin(angle), math.cos(angle)) if alignment > 0 else math.atan2(-math.sin(angle), -math.cos(angle)))
    if len(fits) != 1:
      raise ValueError(f'{section}: {len(fits)} border orientations open to the {entrance}')
    facings.append(fits[0])
  facing = math.atan2(sum(math.sin(f) for f in facings), sum(math.cos(f) for f in facings))
  if any(abs(math.atan2(math.sin(f - facing), math.cos(f - facing))) > math.radians(2) for f in facings):
    raise ValueError(f'{section}: its corner pairs disagree on the orientation')
  centerX, centerY = sum(x for x, _ in corners) / len(corners), sum(y for _, y in corners) / len(corners)
  heading = math.degrees(math.atan2(-math.cos(facing), -math.sin(facing))) % 360 * 512 / 360
  turn = math.radians((heading + 256) * 360 / 512)
  def place(a, b):
    return (-b * math.cos(turn) - a * math.sin(turn), b * math.sin(turn) - a * math.cos(turn))
  expected = [place(a * guildBorderLength / 2, b * guildBorderWidth / 2) for a in (-1, 1) for b in (-1, 1)]
  for x, y in corners:
    if min(math.hypot(x - centerX - ex, y - centerY - ey) for ex, ey in expected) > 8:
      raise ValueError(f'{section}: corner ({x}, {y}) is off the fitted border')
  offsetX, offsetY = place(*guildBorderCenter)
  return dict(kind='guild plot', section=section, door=None, doorPosition=[round(centerX - offsetX, 1), round(centerY - offsetY, 1), ground],
              center=[round(centerX, 1), round(centerY, 1), ground], heading=round(heading, 1),
              sizeAcross=guildBorderWidth, sizeAlong=guildBorderLength)


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


def loadStones(path):
  stones = []
  for line in open(path):
    f = line.rstrip('\n').split('\t')
    stones.append(dict(door=int(f[1]), position=(float(f[2]), float(f[3]), float(f[4])), heading=float(f[5])))
  return stones


if __name__ == '__main__':
  here = os.path.dirname(os.path.abspath(__file__))
  segments, labels = parse(os.path.join(here, '..', 'content', 'neighborhood_2.txt'))
  stones = loadStones(os.path.join(here, '..', 'content', 'sunriseHillsLotSquares.tsv'))
  plots = matchLabels(fitBoxes(segments), labels)
  counts = collections.Counter()
  rows = []
  for plot in plots:
    group = liveGroup(plot['address'])
    counts[group[:2] if group[1] is None else (group[0], tuple(sorted(group[1])))] += 1
    box = plot['box']
    worldX, worldY = -box['center'][0], -box['center'][1]
    stone = min(stones, key=lambda st: math.hypot(st['position'][0] - worldX, st['position'][1] - worldY))
    if math.hypot(stone['position'][0] - worldX, stone['position'][1] - worldY) > 5:
      raise ValueError(f"{plot['address']}: no border stone at its center")
    heading = (90 - box['sideAngle']) % 90 * 512 / 360
    turn = (stone['heading'] - heading) % 128
    if min(turn, 128 - turn) > 1:
      raise ValueError(f"{plot['address']}: box heading {heading:.1f} disagrees with its stone's {stone['heading']:.0f}")
    rows.append(dict(kind='plot', address=plot['address'], door=stone['door'], center=[round(worldX, 3), round(worldY, 3), stone['position'][2]],
                     heading=stone['heading'], sizeAcross=round(box['sizeAcross'], 2), sizeAlong=round(box['sizeAlong'], 2),
                     pricePlatinum=group[2], upkeepCopperPerDay=group[3], capacity=group[4], pets=group[5]))
  for group in liveGroups:
    key = group[:2] if group[1] is None else (group[0], tuple(sorted(group[1])))
    if counts[key] != group[6]:
      raise ValueError(f'{group[0]} {group[1] or ""}: {counts[key]} plots on the map, {group[6]} in Live\'s table')
  claimed = {row['door'] for row in rows}
  if len(claimed) != len(rows):
    raise ValueError('two plots claimed one border stone')
  unclaimed = len(stones) - len(claimed)
  rows += [guildPlotRow(*plot) for plot in liveGuildPlots]
  print(json.dumps(rows, indent=1))
  print(f"{len(claimed)} plots, each on its own border stone with a matching heading; every group matches Live's table; "
        f"{unclaimed} unclaimed stones; {len(liveGuildPlots)} guild plots fitted to Live's corners", file=sys.stderr)
