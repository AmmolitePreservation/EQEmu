"""Proves the RoF2 item serializer's housing tail (T6) against the client's read order.

Compiles the new code in common/patches/rof2.cpp (the else branch after `if (packet_type == ItemPacketInvalid)`) with the
verbatim ItemQuaternaryBodyStruct from rof2_structs.h and common/itemHousingTail.cpp, serializes a housing item, an ordinary
item and today's struct, then reads each in the client reader's order (0x7f6bf0, windows/RealEstateItemsWnd.md): every
housing field must read back, an ordinary item must read today's defaults with an empty name, and the bytes before the tail
and the 14 after it must equal today's. Needs g++.

Usage: python3 testItemTailSerializer.py
"""
import os, struct, subprocess, sys, tempfile

peridot = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
source = open(os.path.join(peridot, 'common', 'patches', 'rof2.cpp')).read()
start = source.index('\t\telse {\n\t\t\t// the client reads the housing tail')
bodyStart = source.index('\n', start) + 1
body = source[bodyStart:source.index('\n\t\t}\n', bodyStart)]
if 'static_assert' not in body or 'trailer_size, trailer_size' not in body:
  sys.exit('could not find the serializer code in rof2.cpp')
structs = open(os.path.join(peridot, 'common', 'patches', 'rof2_structs.h')).read()
definition = structs[structs.index('struct ItemQuaternaryBodyStruct'):]
definition = definition[:definition.index('\n};') + 3]

harness = r'''
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <string>
#include "types.h"
#pragma pack(1)
namespace RoF2 { namespace structs {
DEFINITION
}}
#pragma pack()
#include "itemHousingTail.h"
struct FakeItem { uint32 ID; };
static EQ::ItemHousingTail housingItem;
static const EQ::ItemHousingTail *provider(uint32 id) { return id == 7 ? &housingItem : nullptr; }
static void today(RoF2::structs::ItemQuaternaryBodyStruct &iqbs) {
  memset(&iqbs, 0, sizeof(iqbs));
  for (size_t i = 0; i < offsetof(RoF2::structs::ItemQuaternaryBodyStruct, Heirloom); i++) ((uint8 *)&iqbs)[i] = (uint8)(0xA0 + i % 16);
  iqbs.unknown28 = -1; iqbs.unknown30 = -1; iqbs.NoZone = 0x11; iqbs.NoGround = 0x22; iqbs.unknown_RoF_7b = 0x33; iqbs.unknown_RoF_8d = 0x44; iqbs.unknown39 = 1;
}
static std::string serialize(uint32 itemId) {
  std::stringstream ob;
  RoF2::structs::ItemQuaternaryBodyStruct iqbs;
  today(iqbs);
  FakeItem fake{itemId}; FakeItem *item = &fake;
  {
BODY
  }
  return ob.str();
}
int main() {
  EQ::setItemHousingTailProvider(provider);
  housingItem.placeable = 0x0c; housingItem.placeableIgnoreCollisions = 1; housingItem.placementType = 3; housingItem.realEstateDefinitionId = 5;
  housingItem.placeableScaleMin = 0.5f; housingItem.placeableScaleMax = 2.0f; housingItem.realEstateUpkeepId = 17; housingItem.maxPerRealEstate = 1;
  strcpy(housingItem.housepetFileName, "IT20300"); housingItem.interactiveObject = 1; housingItem.trophyBenefitId = 100;
  housingItem.disablePlacementRotation = 1; housingItem.npcRespawnInterval = 4500; housingItem.placeableDefaultScale = 1.25f;
  housingItem.placeableDefaultHeading = 90.0f; housingItem.placeableDefaultPitch = -3.5f; housingItem.placeableDefaultRoll = 7.0f;
  RoF2::structs::ItemQuaternaryBodyStruct t; today(t);
  std::string a = serialize(7), b = serialize(8), c((const char *)&t, sizeof(t));
  uint32 lengths[4] = {(uint32)a.size(), (uint32)b.size(), (uint32)c.size(), (uint32)offsetof(RoF2::structs::ItemQuaternaryBodyStruct, Heirloom)};
  fwrite(lengths, 4, 4, stdout); fwrite(a.data(), 1, a.size(), stdout); fwrite(b.data(), 1, b.size(), stdout); fwrite(c.data(), 1, c.size(), stdout);
  return 0;
}
'''.replace('DEFINITION', definition).replace('BODY', body)

with tempfile.TemporaryDirectory() as work:
  open(os.path.join(work, 'harness.cpp'), 'w').write(harness)
  subprocess.run(['g++', '-std=c++20', '-iquote', os.path.join(peridot, 'common'), '-o', os.path.join(work, 'harness'),
                  os.path.join(work, 'harness.cpp'), os.path.join(peridot, 'common', 'itemHousingTail.cpp')], check=True)
  raw = subprocess.run([os.path.join(work, 'harness')], check=True, capture_output=True).stdout

la, lb, lt, heirloom = struct.unpack_from('<4I', raw)
housing, ordinary, current = raw[16:16 + la], raw[16 + la:16 + la + lb], raw[16 + la + lb:16 + la + lb + lt]


def clientRead(buf):
  p = heirloom + 1
  out = {}
  for name, fmt in (('placeable', '<I'), ('ignoreCollisions', '<B'), ('placementType', '<I'), ('definition', '<i'), ('scaleMin', '<f'),
                    ('scaleMax', '<f'), ('upkeep', '<i'), ('maxPer', '<i')):
    out[name] = struct.unpack_from(fmt, buf, p)[0]; p += struct.calcsize(fmt)
  end = buf.index(b'\0', p); out['housepet'] = buf[p:end][:31]; p = end + 1
  for name, fmt in (('interactive', '<B'), ('trophy', '<i'), ('rotationOff', '<B'), ('freeOff', '<B'), ('respawn', '<I'), ('scale', '<f'),
                    ('heading', '<f'), ('pitch', '<f'), ('roll', '<f'), ('x609', '<B')):
    out[name] = struct.unpack_from(fmt, buf, p)[0]; p += struct.calcsize(fmt)
  out['after'] = buf[p:]
  return out


h, o, c = clientRead(housing), clientRead(ordinary), clientRead(current)
failures = []
expected = dict(placeable=0x0c, ignoreCollisions=1, placementType=3, definition=5, scaleMin=0.5, scaleMax=2.0, upkeep=17, maxPer=1, housepet=b'IT20300',
                interactive=1, trophy=100, rotationOff=1, freeOff=0, respawn=4500, scale=1.25, heading=90.0, pitch=-3.5, roll=7.0, x609=0)
defaults = dict(placeable=0, ignoreCollisions=0, placementType=0, definition=0, scaleMin=0.0, scaleMax=0.0, upkeep=0, maxPer=0, housepet=b'',
                interactive=0, trophy=-1, rotationOff=0, freeOff=0, respawn=0, scale=0.0, heading=0.0, pitch=0.0, roll=0.0, x609=0)
failures += [f'housing item {k}: {h[k]!r}' for k, v in expected.items() if h[k] != v]
failures += [f'ordinary item {k}: {o[k]!r}' for k, v in defaults.items() if o[k] != v]
if c['housepet'] != b'\xff\xff\xff' or c['trophy'] != -1:
  failures.append("today's bytes no longer read as the research describes")
if not (h['after'] == o['after'] == c['after'] and len(c['after']) == 14):
  failures.append('the 14 bytes after the tail differ from today')
if not (housing[:heirloom + 1] == ordinary[:heirloom + 1] == current[:heirloom + 1]):
  failures.append('the bytes before the tail differ from today')
if failures:
  sys.exit('FAILED:\n  ' + '\n  '.join(failures))
print(f"passed: a housing item reads back every field; an ordinary item reads today's defaults with an empty name; the head and the 14 "
      f"bytes after the tail equal today's (today {lt} bytes, ordinary {lb}, housing {la})")
