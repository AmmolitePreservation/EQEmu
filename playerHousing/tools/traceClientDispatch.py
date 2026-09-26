"""Finds which case of a client dispatcher an opcode reaches, by simulating the dispatcher's own comparisons.

A dispatcher compares the opcode register with constants (cmp / ja / jb / je / jne ...), possibly after sub / add, and may use
a jump table; this follows those instructions for one opcode value and reports the address where the case's code begins, so
the opcode-to-handler map comes from the binary, not from a document.

Usage: python3 traceClientDispatch.py 0x5b2500 0x2698 [0x...]
"""
import os, re, struct, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..'))
from realEstateOpcodeScan import ClientImage
from clientAnalysis import functionDisassembly

client = ClientImage('/mnt/user-data/uploads/eqgame.exe')
listings = {}


def listing(function):
  if function not in listings:
    ins = functionDisassembly(client, function, 0x6000)
    listings[function] = {j.address: (k, j) for k, j in enumerate(ins)}, ins
  return listings[function]


def dword(address):
  return struct.unpack('<I', bytes(client.readByte(address + i) for i in range(4)))[0]


def dispatch(function, opcode, register='eax', start=None):
  byAddress, ins = listing(function)
  k = byAddress[start][0] if start else 0
  value = opcode
  flags = None
  for _ in range(400):
    j = ins[k]
    op = j.op_str
    if j.mnemonic == 'cmp' and op.startswith(register + ', ') and re.fullmatch(r'-?(0x[0-9a-f]+|\d+)', op.split(', ')[1]):
      flags = (value & 0xffffffff, int(op.split(', ')[1], 0) & 0xffffffff)
      k += 1
      continue
    if j.mnemonic in ('sub', 'add') and op.startswith(register + ', ') and re.fullmatch(r'-?(0x[0-9a-f]+|\d+)', op.split(', ')[1]):
      value = (value - int(op.split(', ')[1], 0)) if j.mnemonic == 'sub' else (value + int(op.split(', ')[1], 0))
      value &= 0xffffffff
      flags = (value, 0)
      k += 1
      continue
    copy = re.fullmatch(rf'(\w+), {register}', op) if j.mnemonic == 'mov' else None
    if copy and copy.group(1) in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi'):
      register = copy.group(1)
      k += 1
      continue
    if j.mnemonic == 'dec' and op == register:
      value = (value - 1) & 0xffffffff
      flags = (value, 0)
      k += 1
      continue
    if j.mnemonic.startswith('j') and j.mnemonic != 'jmp' and flags is not None and op.startswith('0x'):
      a, b = flags
      sa, sb = (a - (1 << 32) if a >> 31 else a), (b - (1 << 32) if b >> 31 else b)
      taken = {'je': a == b, 'jne': a != b, 'ja': a > b, 'jae': a >= b, 'jb': a < b, 'jbe': a <= b,
               'jg': sa > sb, 'jge': sa >= sb, 'jl': sa < sb, 'jle': sa <= sb}.get(j.mnemonic)
      if taken is None:
        return ('unhandled condition', j.address)
      if taken:
        target = int(op, 16)
        if target not in byAddress:
          return ('left the function', target)
        k = byAddress[target][0]
      else:
        k += 1
      continue
    table = re.fullmatch(rf'dword ptr \[{register}\*4 \+ (0x[0-9a-f]+)\]', op) if j.mnemonic == 'jmp' else None
    if table:
      target = dword(int(table.group(1), 16) + 4 * value)
      if target not in byAddress:
        return ('left the function', target)
      k = byAddress[target][0]
      continue
    byteTable = re.fullmatch(rf'(\w+), byte ptr \[{register} \+ (0x[0-9a-f]+)\]', op) if j.mnemonic == 'movzx' else None
    if byteTable:
      value = client.readByte(int(byteTable.group(2), 16) + value)
      register = {'ecx': 'ecx', 'edx': 'edx', 'eax': 'eax'}.get(byteTable.group(1), byteTable.group(1))
      k += 1
      continue
    if j.mnemonic == 'jmp' and op.startswith('0x'):
      target = int(op, 16)
      if target not in byAddress:
        return ('left the function', target)
      k = byAddress[target][0]
      continue
    return ('case', j.address)
  return ('too long', ins[k].address)


if __name__ == '__main__':
  function = int(sys.argv[1], 16)
  for arg in sys.argv[2:]:
    print(arg, dispatch(function, int(arg, 16), start=0x5b2525 if function == 0x5b2500 else None))
