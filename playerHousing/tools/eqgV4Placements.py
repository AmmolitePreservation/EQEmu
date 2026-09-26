"""Lists what a v4 zone (.eqg, House of Thule era and later) places: every tile's placeables, areas and placement groups.

A port of zone-utilities' EQG4Loader::ParseZoneDat (PhoenixCampfire/zone-utilities, src/common/eqg_v4_loader.cpp): the PFS
archive is unpacked, the .zon's terrain options read, and the .dat parsed tile by tile; world positions add each tile's
origin with x and y swapped, as the loader does. The parse must consume the whole .dat, or it fails.

Usage: python3 eqgV4Placements.py zone.eqg > placements.json
"""
import collections, json, struct, sys, zlib


def readPFS(path):
  data = open(path, 'rb').read()
  directory, magic, _ = struct.unpack_from('<I4sI', data, 0)
  if magic != b'PFS ':
    raise ValueError(f'{path}: not a PFS archive')
  count, = struct.unpack_from('<I', data, directory)
  entries = [struct.unpack_from('<III', data, directory + 4 + 12 * k) for k in range(count)]
  def inflate(offset, size):
    out, position = b'', offset
    while len(out) < size:
      deflated, _ = struct.unpack_from('<II', data, position)
      position += 8
      out += zlib.decompress(data[position:position + deflated])
      position += deflated
    return out
  nameEntry = max(entries, key=lambda e: e[1])
  names = inflate(nameEntry[1], nameEntry[2])
  n, = struct.unpack_from('<I', names, 0)
  position, nameList = 4, []
  for _ in range(n):
    length, = struct.unpack_from('<I', names, position)
    nameList.append(names[position + 4:position + 4 + length].rstrip(b'\0').decode('latin-1'))
    position += 4 + length
  dataEntries = sorted((e for e in entries if e is not nameEntry), key=lambda e: e[1])
  return {name: inflate(offset, size) for name, (_, offset, size) in zip(nameList, dataEntries)}


def zonOptions(text):
  tokens = text.replace('\r', ' ').split()
  if tokens[0] != 'EQTZP':
    raise ValueError('not a v4 .zon')
  value = lambda key: tokens[tokens.index(key) + 1]
  return dict(name=value('*NAME'), minLng=int(value('*MINLNG')), minLat=int(value('*MINLAT')),
              unitsPerVert=float(value('*UNITSPERVERT')), quadsPerTile=int(value('*QUADSPERTILE')))


def parseDat(buf, options):
  position = 0
  def read(fmt):
    nonlocal position
    values = struct.unpack_from('<' + fmt, buf, position)
    position += struct.calcsize('<' + fmt)
    return values if len(values) > 1 else values[0]
  def cstr():
    nonlocal position
    end = buf.index(b'\0', position)
    text = buf[position:end].decode('latin-1')
    position = end + 1
    return text
  flags, _, _ = read('III')
  cstr()
  tileSize = options['unitsPerVert'] * options['quadsPerTile']
  zoneMinX, zoneMinY = options['minLat'] * tileSize, options['minLng'] * tileSize
  verts, quads = (options['quadsPerTile'] + 1) ** 2, options['quadsPerTile'] ** 2
  placeables, areas, groups = [], [], []
  for _ in range(read('I')):
    lng, lat, _ = read('iii')
    startY = zoneMinY + (lng - 100000 - options['minLng']) * tileSize
    startX = zoneMinX + (lat - 100000 - options['minLat']) * tileSize
    position += 4 * verts * 3 + quads + 4
    extra = struct.unpack_from('<i', buf, position)[0]
    position += 4
    if extra > 0:
      if read('b') > 0:
        position += 16
      position += 4
    layers = read('I')
    if layers > 0:
      cstr()
      for _ in range(1, layers):
        cstr()
        dim = read('I')
        position += dim * dim
    for _ in range(read('I')):
      model, name = cstr().lower(), cstr()
      read('II')
      x, y, z, rx, ry, rz, sx, sy, sz = read('9f')
      read('B')
      if flags & 2:
        position += 4
      placeables.append(dict(model=model, name=name, position=[startY + x, startX + y, z], rotation=[rx, ry, rz], scale=[sx, sy, sz]))
    for _ in range(read('I')):
      name, kind, alternate = cstr(), read('i'), cstr()
      read('II')
      x, y, z, rx, ry, rz, sx, sy, sz, ex, ey, ez = read('12f')
      areas.append(dict(name=name, type=kind, alternateName=alternate, position=[startY + x, startX + y, z], rotation=[rx, ry, rz],
                        scale=[sx, sy, sz], size=[ex, ey, ez]))
    for _ in range(read('I')):
      cstr(); cstr(); read('b'); read('II'); position += 40
    for _ in range(read('I')):
      name = cstr()
      read('II')
      x, y, z = read('3f')
      position += 28
      groups.append(dict(name=name, position=[startY + x, startX + y, z]))
  if position != len(buf):
    raise ValueError(f'parsed {position} of {len(buf)} bytes')
  return dict(placeables=placeables, areas=areas, groups=groups)


if __name__ == '__main__':
  files = readPFS(sys.argv[1])
  zon = next(content for name, content in files.items() if name.endswith('.zon'))
  options = zonOptions(zon.decode('latin-1'))
  result = parseDat(files[options['name'] + '.dat'], options)
  print(json.dumps(result))
  counts = collections.Counter(p['model'] for p in result['placeables'])
  print(f"{len(result['placeables'])} placeables {dict(counts.most_common(8))}; {len(result['areas'])} areas; "
        f"{len(result['groups'])} placement groups {dict(collections.Counter(g['name'] for g in result['groups']))}", file=sys.stderr)
