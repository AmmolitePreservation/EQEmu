"""Proves zone/housing/housingGeometry.h against the tested reference (regionMatrix.py).

Compiles a harness around the header, then for random boxes compares every matrix element with boxToMatrix and every
containment answer with the client's own rule (clientContains, the inverted matrix) at random points, including points near
the edges. Needs g++.

Usage: python3 testHousingGeometry.py
"""
import math, os, random, struct, subprocess, sys, tempfile

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from regionMatrix import boxToMatrix, clientContains

peridot = os.path.normpath(os.path.join(here, '..', '..'))
random.seed(7)
boxes = [(random.uniform(-3000, 3000), random.uniform(-3000, 3000), random.uniform(-200, 200), random.uniform(20, 400),
          random.uniform(20, 400), random.uniform(20, 400), random.choice([0.0, 128.0, 256.0, 384.0, 445.0, random.uniform(0, 512)]))
         for _ in range(400)]
points = []
for box in boxes:
  for _ in range(60):
    scale = random.choice([0.5, 0.95, 0.999, 1.001, 1.05, 2.0])
    angle = box[6] / 512.0 * 2.0 * math.pi
    across, along, up = (random.uniform(-1, 1) * scale * box[3] / 2, random.uniform(-1, 1) * scale * box[4] / 2, random.uniform(-1, 1) * scale * box[5] / 2)
    points.append((box, box[0] + across * math.cos(angle) + along * math.sin(angle), box[1] - across * math.sin(angle) + along * math.cos(angle), box[2] + up))

harness = '''
#include <cstdio>
#include "housingGeometry.h"
int main() {
  housing::RegionBox box;
  float px, py, pz;
  char kind;
  while (scanf(" %c", &kind) == 1) {
    if (scanf("%f %f %f %f %f %f %f", &box.x, &box.y, &box.z, &box.sizeX, &box.sizeY, &box.sizeZ, &box.heading) != 7) return 1;
    if (kind == 'm') {
      auto m = housing::regionMatrix(box);
      for (float v : m) printf("%.9g ", v);
      printf("\\n");
    } else {
      if (scanf("%f %f %f", &px, &py, &pz) != 3) return 1;
      printf("%d\\n", housing::regionContains(box, px, py, pz) ? 1 : 0);
    }
  }
}
'''
with tempfile.TemporaryDirectory() as work:
  open(os.path.join(work, 'harness.cpp'), 'w').write(harness)
  subprocess.run(['g++', '-std=c++20', '-O1', '-iquote', os.path.join(peridot, 'zone', 'housing'), '-o', os.path.join(work, 'harness'), os.path.join(work, 'harness.cpp')], check=True)
  commands = ''.join('m ' + ' '.join(repr(v) for v in box) + '\n' for box in boxes)
  commands += ''.join('c ' + ' '.join(repr(v) for v in box) + f' {x!r} {y!r} {z!r}\n' for box, x, y, z in points)
  output = subprocess.run([os.path.join(work, 'harness')], input=commands, capture_output=True, text=True, check=True).stdout.split('\n')

failures = []
f32 = lambda v: struct.unpack('<f', struct.pack('<f', v))[0]
for i, box in enumerate(boxes):
  got = [float(v) for v in output[i].split()]
  want = [f32(v) for v in boxToMatrix(*[f32(v) for v in box])]
  if any(abs(g - w) > 1e-3 * max(1.0, abs(w)) for g, w in zip(got, want)):
    failures.append(f'matrix differs for box {box}')
disagreements = 0
for j, (box, x, y, z) in enumerate(points):
  f = [f32(v) for v in box]
  got = output[len(boxes) + j] == '1'
  want = clientContains(boxToMatrix(*f), f32(x), f32(y), f32(z))
  disagreements += got != want
if disagreements:
  failures.append(f'containment disagrees with the client rule at {disagreements} of {len(points)} points')
if failures:
  sys.exit('FAILED:\n  ' + '\n  '.join(failures[:10]))
print(f'passed: {len(boxes)} matrices match the reference; containment agrees with the client rule at all {len(points)} points')
