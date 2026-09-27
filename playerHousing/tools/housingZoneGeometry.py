"""Derives each housing zone's entry point (a safe point) from the RoF2 client's zone files.

EQG version 1-3 zones: the terrain mesh (.ter, in the zone's .eqg) is used in its own coordinates, and the .zon's object
placements are in the same frame (their first stored angle is the turn about the vertical, as azone reads it); EQEmu's
coordinates are that frame's (y, x, z) (azone and EQEmu's map loader both swap x and y), so /loc, which prints y first,
reads the file's coordinates directly. Checked on Live: both Palatial Guild Hall readings stand 2.95 above a floor.

Floors are the upward-facing surfaces (the raw frame mirrors nothing, so floors face up and ceilings down). The entry is
the first point at least 10 units inside the zone's entrance object (FrontEntrance, else a door), walking toward the main
floor's centre (the level with the most floor), over a floor no more than 5 below the entrance object's height; 2.95
above the lowest such floor. A zone with neither object uses its main floor's centre.

Usage: python3 housingZoneGeometry.py <folder with the zones' .eqg and .zon files> [zone ...]
"""
import math, os, struct, sys
import numpy as np
from eqgV4Placements import readPFS

playerAboveFloor = 2.95
stepInside = 10.0


def readZon(path):
  buf = open(path, 'rb').read()
  magic, version, pool, modelCount, objectCount, regionCount, lightCount = struct.unpack_from('<4s6I', buf, 0)
  if magic != b'EQGZ':
    raise ValueError(f'{path}: not an EQGZ zone file')
  strings = buf[28:28 + pool]
  text = lambda offset: strings[offset:strings.index(b'\0', offset)].decode('latin-1')
  position = 28 + pool
  models = [text(struct.unpack_from('<I', buf, position + 4 * i)[0]) for i in range(modelCount)]
  position += 4 * modelCount
  objects = []
  for _ in range(objectCount):
    modelIndex, nameOffset, x, y, z, turn, tilt, roll, scale = struct.unpack_from('<iI7f', buf, position)
    position += 36
    if version > 1:
      position += 4 + 4 * struct.unpack_from('<I', buf, position)[0]
    objects.append(dict(model=models[modelIndex], name=text(nameOffset), position=(x, y, z), turn=turn, scale=scale))
  return objects


def readTer(buf):
  magic, version, pool, materialCount, vertexCount, triangleCount = struct.unpack_from('<4s5I', buf, 0)
  if magic != b'EQGT':
    raise ValueError('not an EQGT terrain mesh')
  position = 24 + pool
  for _ in range(materialCount):
    position += 16 + 12 * struct.unpack_from('<4I', buf, position)[3]
  for vertexSize in (44, 40, 36, 32):
    trianglesAt = position + vertexCount * vertexSize
    if trianglesAt + triangleCount * 20 > len(buf):
      continue
    records = np.frombuffer(buf, dtype=np.uint8, count=triangleCount * 20, offset=trianglesAt).reshape(triangleCount, 20)
    indices = records[:, :12].copy().view('<u4').reshape(triangleCount, 3)
    if (indices < vertexCount).all():
      vertices = np.frombuffer(buf, dtype=np.uint8, count=vertexCount * vertexSize, offset=position).reshape(vertexCount, vertexSize)
      return vertices[:, :12].copy().view('<f4').reshape(vertexCount, 3).astype(float), indices
  raise ValueError('no vertex layout gives valid triangle indices')


def floorSurfaces(vertices, indices):
  a, b, c = vertices[indices[:, 0]], vertices[indices[:, 1]], vertices[indices[:, 2]]
  normal = np.cross(b - a, c - a)
  area = np.linalg.norm(normal, axis=1) / 2
  upward = normal[:, 2] > 0.7 * np.maximum(2 * area, 1e-9)
  return a[upward], b[upward], c[upward], area[upward]


def surfaceHeightsAt(a, b, c, x, y):
  v0, v1, v2 = c[:, :2] - a[:, :2], b[:, :2] - a[:, :2], np.array([x, y]) - a[:, :2]
  d00, d01, d11 = (v0 * v0).sum(1), (v0 * v1).sum(1), (v1 * v1).sum(1)
  d20, d21 = (v2 * v0).sum(1), (v2 * v1).sum(1)
  denominator = d00 * d11 - d01 * d01
  valid = np.abs(denominator) > 1e-9
  u = np.where(valid, (d11 * d20 - d01 * d21) / np.where(valid, denominator, 1), -1)
  v = np.where(valid, (d00 * d21 - d01 * d20) / np.where(valid, denominator, 1), -1)
  inside = (u >= 0) & (v >= 0) & (u + v <= 1)
  return a[inside, 2] + u[inside] * (c[inside, 2] - a[inside, 2]) + v[inside] * (b[inside, 2] - a[inside, 2])


def entryPoint(zoneDirectory, zoneName):
  objects = readZon(os.path.join(zoneDirectory, f'{zoneName}.zon'))
  terrains = [o for o in objects if o['model'].lower().endswith('.ter')]
  if len(terrains) != 1:
    raise ValueError(f'{zoneName}: {len(terrains)} terrain placements')
  archive = readPFS(os.path.join(zoneDirectory, f'{zoneName}.eqg'))
  terrainFile = {name.lower(): name for name in archive}[terrains[0]['model'].lower()]
  a, b, c, area = floorSurfaces(*readTer(archive[terrainFile]))
  centres = (a + b + c) / 3
  heights = np.round(centres[:, 2] / 2) * 2
  levels, inverse = np.unique(heights, return_inverse=True)
  mainLevel = levels[np.argmax(np.bincount(inverse, weights=area))]
  onMain = np.abs(centres[:, 2] - mainLevel) <= 2
  mainCentre = (centres[onMain, :2] * area[onMain, None]).sum(0) / area[onMain].sum()
  entrances = [o for o in objects if 'frontentrance' in o['model'].lower()] or [o for o in objects if 'door' in o['model'].lower()]
  if entrances:
    door = np.array(entrances[0]['position'][:2])
    lowest = entrances[0]['position'][2] - 5
    distance = np.linalg.norm(mainCentre - door)
    candidates = [door + (mainCentre - door) / distance * step for step in np.arange(stepInside, distance + 1.0, 1.0)]
    source = entrances[0]['name']
  else:
    lowest = mainLevel - 2
    candidates = [mainCentre]
    source = 'main floor centre'
  for x, y in candidates:
    heightsHere = surfaceHeightsAt(a, b, c, x, y)
    usable = heightsHere[heightsHere >= lowest]
    if len(usable):
      floor = usable.min()
      break
  else:
    raise ValueError(f'{zoneName}: no floor between {source} and the main floor centre')
  return dict(zone=zoneName, source=source, fileX=float(x), fileY=float(y), floor=float(floor),
              safeX=round(float(y), 1), safeY=round(float(x), 1), safeZ=round(float(floor) + playerAboveFloor, 1))


if __name__ == '__main__':
  directory, names = sys.argv[1], sys.argv[2:]
  names = names or sorted(n[:-4] for n in os.listdir(directory) if n.endswith('.zon'))
  for zoneName in names:
    point = entryPoint(directory, zoneName)
    print(f"{point['zone']:20} EQEmu safe point ({point['safeX']:7.1f}, {point['safeY']:7.1f}, {point['safeZ']:6.1f})  "
          f"/loc ({point['fileX']:7.1f}, {point['fileY']:7.1f})  from {point['source']}")
