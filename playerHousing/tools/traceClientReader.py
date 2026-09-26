"""Lists what a client message reader consumes, from its code: the audit of every server message against the binary.

The client inlines each stream read as `lea r, [pos + N]` then `cmp r, [cursor + 4]` (the bounds check), a load, and the
position advanced by N (cursor = {buffer, length, position}). This prints, in address order, each read's size and whether its
load is integer or floating point, and every call (sub-readers such as the string reader), so a reader's layout comes from the
binary, not from a document.

Usage: python3 traceClientReader.py 0x7f4c90 [0x...]
"""
import os, re, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..'))
from realEstateOpcodeScan import ClientImage
from clientAnalysis import functionDisassembly

client = ClientImage('/mnt/user-data/uploads/eqgame.exe')


def trace(function, limit=0x3000):
  ins = functionDisassembly(client, function, limit)
  events = []
  for k, j in enumerate(ins):
    m = re.fullmatch(r'(\w+), \[(\w+) \+ (0x[0-9a-f]+|\d+)\]', j.op_str) if j.mnemonic == 'lea' else None
    check = next((c for c in ins[k + 1:k + 4] if c.mnemonic == 'cmp' and c.op_str.startswith(m.group(1) + ',')), None) if m else None
    if check:
      size = int(m.group(3), 0)
      loaded, destination, kind = None, None, 'int'
      for later in ins[k + 2:k + 16]:
        if loaded is None and later.mnemonic in ('mov', 'movzx', 'movsx') and re.search(r'ptr \[\w+ \+ \w+\]$', later.op_str):
          loaded = later.op_str.split(',')[0]
          kind = 'byte' if 'byte ptr' in later.op_str else ('short' if 'word ptr' in later.op_str and 'dword' not in later.op_str else 'int')
          continue
        if loaded and later.mnemonic == 'mov' and later.op_str.endswith(', ' + loaded) and 'ptr [' in later.op_str:
          destination = later.op_str.split(',')[0].replace('dword ptr ', '').replace('byte ptr ', '').replace('word ptr ', '')
          break
      events.append((j.address, f'read {size} ({kind}) -> {destination}'))
    if j.mnemonic == 'call':
      events.append((j.address, f'call {j.op_str}'))
    if j.mnemonic == 'fstp' and 'ptr [' in j.op_str:
      events.append((j.address, f'  float store {j.op_str.replace("dword ptr ", "")}'))
  return events


if __name__ == '__main__':
  for arg in sys.argv[1:]:
    print(f'== {arg}')
    for address, event in trace(int(arg, 16)):
      print(f'  {address:x}  {event}')
