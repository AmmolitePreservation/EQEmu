"""The region matrix the client receives in 0x49df, from a region box (center, size, heading), and its test.

The client maps the cube [-1, 1]^3 onto a region with this 4x4 matrix (row vector times matrix, translation in elements
12-14) and tests the player's LastCollision point in the order Y, X, Z (0x4b3040 -> 0x4b2f40 -> 0x8d2ad0). Headings are
EQ units (512 per turn); heading 0 faces +y and 128 faces +x (EQEmu's own convention: forward = (sin, cos) in (x, y)).
A box's size_y runs along its heading and size_x across it.

Usage: python3 regionMatrix.py   (runs the tests)
"""
import math, random


def boxToMatrix(x, y, z, sizeX, sizeY, sizeZ, heading):
  angle = heading / 512.0 * 2.0 * math.pi
  sine, cosine = math.sin(angle), math.cos(angle)
  halfX, halfY, halfZ = sizeX / 2.0, sizeY / 2.0, sizeZ / 2.0
  # Rows in the client's axis order (Y, X, Z): across (right = (cos, -sin) in (x, y)), along (forward), up, center.
  return [-sine * halfX, cosine * halfX, 0.0, 0.0,
          cosine * halfY, sine * halfY, 0.0, 0.0,
          0.0, 0.0, halfZ, 0.0,
          y, x, z, 1.0]


def invert(m):
  a = [m[r * 4:(r + 1) * 4] + [1.0 if r == c else 0.0 for c in range(4)] for r in range(4)]
  for col in range(4):
    pivot = max(range(col, 4), key=lambda r: abs(a[r][col]))
    a[col], a[pivot] = a[pivot], a[col]
    p = a[col][col]
    a[col] = [v / p for v in a[col]]
    for r in range(4):
      if r != col:
        factor = a[r][col]
        a[r] = [v - factor * w for v, w in zip(a[r], a[col])]
  return [v for row in a for v in row[4:]]


def clientContains(matrix, x, y, z):
  inverse = invert(matrix)
  point = (y, x, z)
  local = [sum(point[k] * inverse[k * 4 + axis] for k in range(3)) + inverse[12 + axis] for axis in range(3)]
  return all(abs(v) <= 1.0 for v in local)


def geometryContains(box, x, y, z):
  cx, cy, cz, sizeX, sizeY, sizeZ, heading = box
  angle = heading / 512.0 * 2.0 * math.pi
  dx, dy = x - cx, y - cy
  along = dx * math.sin(angle) + dy * math.cos(angle)
  across = dx * math.cos(angle) - dy * math.sin(angle)
  return abs(across) <= sizeX / 2 and abs(along) <= sizeY / 2 and abs(z - cz) <= sizeZ / 2


if __name__ == '__main__':
  rng = random.Random(1)
  checked = 0
  for _ in range(2000):
    box = (rng.uniform(-3000, 3000), rng.uniform(-3000, 3000), rng.uniform(-200, 200), rng.uniform(5, 300), rng.uniform(5, 300),
           rng.uniform(5, 100), rng.uniform(0, 512))
    matrix = boxToMatrix(*box)
    for _ in range(20):
      point = (box[0] + rng.uniform(-250, 250), box[1] + rng.uniform(-250, 250), box[2] + rng.uniform(-80, 80))
      g, c = geometryContains(box, *point), clientContains(matrix, *point)
      assert g == c, (box, point, g, c)
      checked += 1
  print(f'{checked} random points: the client test agrees with the box geometry')
  for heading in (0, 64, 128, 256, 384, 500):
    box = (100.0, 200.0, 10.0, 40.0, 60.0, 20.0, heading)
    matrix = boxToMatrix(*box)
    angle = heading / 512.0 * 2.0 * math.pi
    ahead = lambda d: (100.0 + d * math.sin(angle), 200.0 + d * math.cos(angle), 10.0)
    right = lambda d: (100.0 + d * math.cos(angle), 200.0 - d * math.sin(angle), 10.0)
    assert clientContains(matrix, *ahead(29)) and not clientContains(matrix, *ahead(31)), heading
    assert clientContains(matrix, *right(19)) and not clientContains(matrix, *right(21)), heading
  print('facing the heading from the center: 29 ahead is in, 31 out; 19 to the right is in, 21 out, at six headings')
