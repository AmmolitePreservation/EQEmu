"""Static scan of the RoF2 eqgame.exe for player housing (real estate) opcodes.

Reproduces the findings in README.md:
  1. Locates CEverQuest::HandleWorldMessage and decodes its opcode switch.
  2. Finds the switch case that forwards to RealEstateManagerClient::HandleMessage
     and decodes that manager's own opcode switch (server -> client).
  3. Finds every client -> server send inside the real estate windows and
     manager, and labels each with the window notification handler and the
     nearest preceding child-control comparison.

Anchor addresses are specific to the build this was written against
(May 10 2013 23:30:08). The script verifies the build stamp and every anchor
before using it, and raises if anything does not match.

Usage:
  python realEstateOpcodeScan.py <path to eqgame.exe> [--eqemuConf patch_RoF2.conf]

Requires: pefile, capstone
"""
import argparse
import bisect
import collections
import json
import re
import struct
import sys

import pefile
from capstone import CS_ARCH_X86, CS_MODE_32, Cs

expectedBuildStamps = (b"May 10 2013\x00", b"23:30:08\x00")

# Anchors (virtual addresses) for this build.
unknownMessageString = b"Received unknown message #%i (0x%x) (%s)!\n"
dispatcherStart = 0x4c3250          # CEverQuest::HandleWorldMessage
dispatcherSwitchStart = 0x4c3317    # first compare against the opcode local
dispatcherOpcodeLocal = "[ebp - 0x5360]"
dispatcherDefault = 0x4cd5d9
# HandleWorldMessage runs a SECOND opcode switch in its common tail (0x4cd5e0) when
# [0xdd2660]->0x488450() is nonzero. Unknown = handled by neither switch (flag [ebp-0xaa5]).
secondSwitchStart = 0x4cd625
secondSwitchOpcodeLocal = "[ebp - 0x55b0]"
secondSwitchDefault = 0x4d3f43
realEstateForwardCase = 0x4cc1d7    # case that forwards to the manager
managerHandleMessage = 0x5b2500     # RealEstateManagerClient::HandleMessage
managerSwitchStart = 0x5b2525
managerDefault = 0x5b3150
sendOpcodeStub = 0x7d03c0           # identity opcode stub called before every send
windowNotifyIndex = 34              # WndNotification slot in the CXWnd vtable

# Window vtables, keyed by the XML control prefix used in that window.
windowVtables = {
  "REIW": ("RealEstateItemsWnd", 0x9f82e0),
  "REMW": ("RealEstateManageWnd", 0x9f8600),
  "RENW": ("RealEstateNeighborhoodWnd", 0x9f8c28),
  "REPSW": ("RealEstatePlotSearchWnd", 0x9f8ea8),
  "REPW": ("RealEstatePurchaseWnd", 0x9f9200),
  "RELDW": ("RealEstateLayoutDetailsWnd", 0x9d8de8),
}

# Code ranges holding real estate send sites: manager, and the windows block.
sendScanRanges = (
  (0x5a9000, 0x5b8000, "RealEstateManagerClient"),
  (0x728000, 0x74e560, "RealEstate windows"),
)

conditionalJumps = ("ja", "jb", "je", "jne", "jae", "jbe", "jmp")


class ClientImage:
  def __init__(self, exePath):
    self.pe = pefile.PE(exePath, fast_load=True)
    self.image = self.pe.get_memory_mapped_image()
    self.imageBase = self.pe.OPTIONAL_HEADER.ImageBase
    textSection = next(s for s in self.pe.sections if s.Name.startswith(b".text"))
    self.textStart = self.imageBase + textSection.VirtualAddress
    self.textEnd = self.textStart + textSection.Misc_VirtualSize
    self.disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    self.instructionCache = {}
    self.verifyBuild()

  def verifyBuild(self):
    for stamp in expectedBuildStamps:
      if stamp not in self.image:
        raise RuntimeError(f"Build stamp {stamp!r} not found -- this is not the expected RoF2 build.")

  def offset(self, address):
    return address - self.imageBase

  def readDword(self, address):
    return struct.unpack("<I", self.image[self.offset(address):self.offset(address) + 4])[0]

  def readByte(self, address):
    return self.image[self.offset(address)]

  def instructionAt(self, address):
    if address not in self.instructionCache:
      window = self.image[self.offset(address):self.offset(address) + 16]
      instruction = next(self.disassembler.disasm(window, address), None)
      if instruction is None:
        raise RuntimeError(f"Undecodable instruction at {address:#x}")
      self.instructionCache[address] = (instruction.mnemonic, instruction.op_str, instruction.size)
    return self.instructionCache[address]

  def linearDisassembly(self, start, end):
    # Capstone stops at the first undecodable byte; resume one byte later so a
    # stray data byte inside a function does not truncate the listing.
    instructions = []
    address = start
    while address < end:
      decodedAny = False
      for instruction in self.disassembler.disasm(self.image[self.offset(address):self.offset(end)], address):
        instructions.append(instruction)
        address = instruction.address + instruction.size
        decodedAny = True
      if not decodedAny:
        address += 1
    return instructions

  def stringAddresses(self, text):
    pattern = re.escape(text) + b"\x00"
    return [self.imageBase + m.start() for m in re.finditer(pattern, self.image)
            if m.start() == 0 or self.image[m.start() - 1] == 0]

  def codeReferences(self, value):
    needle = struct.pack("<I", value)
    references = []
    position = self.image.find(needle, self.offset(self.textStart), self.offset(self.textEnd))
    while position != -1:
      references.append(self.imageBase + position)
      position = self.image.find(needle, position + 1, self.offset(self.textEnd))
    return references

  def vtableEntries(self, vtableAddress):
    entries = []
    while True:
      pointer = self.readDword(vtableAddress + 4 * len(entries))
      if not (self.textStart <= pointer < self.textEnd):
        return entries
      entries.append(pointer)


def verifyAnchors(client):
  references = [r for s in client.stringAddresses(unknownMessageString) for r in client.codeReferences(s)]
  if len(references) != 1 or not (dispatcherStart < references[0] < dispatcherDefault + 0x8000):
    raise RuntimeError(f"Unknown-message string reference {references} is not inside HandleWorldMessage.")
  mnemonic, operands, _ = client.instructionAt(dispatcherSwitchStart)
  if mnemonic != "cmp" or dispatcherOpcodeLocal not in operands:
    raise RuntimeError(f"Dispatcher switch anchor {dispatcherSwitchStart:#x} is not an opcode compare.")
  mnemonic, operands, _ = client.instructionAt(managerSwitchStart)
  if mnemonic != "cmp" or not operands.startswith("eax,"):
    raise RuntimeError(f"Manager switch anchor {managerSwitchStart:#x} is not an opcode compare.")
  forwardCalls = [i.op_str for i in client.linearDisassembly(realEstateForwardCase, realEstateForwardCase + 0x18)
                  if i.mnemonic == "call"]
  if f"{managerHandleMessage:#x}" not in forwardCalls:
    raise RuntimeError(f"Forward case {realEstateForwardCase:#x} does not call HandleMessage {managerHandleMessage:#x}.")


def branchTaken(mnemonic, left, right):
  return {
    "ja": left > right, "jb": left < right, "je": left == right, "jne": left != right,
    "jae": left >= right, "jbe": left <= right, "jmp": True,
  }[mnemonic]


def resolveDispatcherCase(client, opcode, switchStart=None, opcodeLocal=None, defaultTarget=None):
  """Walk the compiled binary-search + jump-table tree for one opcode value."""
  switchStart = switchStart or dispatcherSwitchStart
  opcodeLocal = opcodeLocal or dispatcherOpcodeLocal
  defaultTarget = defaultTarget or dispatcherDefault
  address = switchStart
  opcodeValue = opcode
  comparison = (0, 0)
  registers = {}
  for _ in range(500):
    mnemonic, operands, size = client.instructionAt(address)
    parts = [p.strip() for p in operands.split(",")]
    if mnemonic == "cmp" and opcodeLocal in parts[0]:
      comparison = (opcodeValue, int(parts[1], 0))
    elif mnemonic == "mov" and opcodeLocal in parts[1]:
      registers[parts[0]] = opcodeValue
    elif mnemonic == "mov" and opcodeLocal in parts[0]:
      opcodeValue = registers[parts[1]]
    elif mnemonic == "sub" and parts[0] in registers:
      registers[parts[0]] = (registers[parts[0]] - int(parts[1], 0)) & 0xffffffff
    elif mnemonic == "movzx":
      match = re.search(r"\[(\w+) \+ (0x[0-9a-f]+)\]", operands)
      registers[parts[0]] = client.readByte(registers[match.group(1)] + int(match.group(2), 16))
    elif mnemonic == "jmp" and "*4" in operands:
      match = re.search(r"\[(\w+)\*4 \+ (0x[0-9a-f]+)\]", operands)
      target = client.readDword(int(match.group(2), 16) + 4 * registers[match.group(1)])
      return None if target == defaultTarget else target
    elif mnemonic in conditionalJumps:
      target = int(operands, 16)
      if branchTaken(mnemonic, *comparison):
        if target == defaultTarget:
          return None
        if opcodeLocal not in client.instructionAt(target)[1]:
          return target
        address = target
        continue
    else:
      raise RuntimeError(f"Unexpected instruction in dispatcher tree at {address:#x}: {mnemonic} {operands}")
    address += size
  raise RuntimeError(f"Dispatcher walk for opcode {opcode:#x} did not terminate.")


def resolveManagerCase(client, opcode):
  """Walk RealEstateManagerClient::HandleMessage's compare tree for one opcode.

  The compiler mixes cmp/ja/je chains on eax with sub-dispatch blocks that copy
  the opcode into ecx and walk it down with sub/dec (for example 0x2aa6, 0x2aa7,
  0x2cf7), so register values are tracked per register rather than assumed to
  live in eax.
  """
  address = managerSwitchStart
  registers = {"eax": opcode}
  comparison = (0, 0)
  for _ in range(300):
    mnemonic, operands, size = client.instructionAt(address)
    parts = [p.strip() for p in operands.split(",")]
    if mnemonic == "cmp" and parts[0] in registers and parts[1].startswith("0x"):
      comparison = (registers[parts[0]], int(parts[1], 0))
    elif mnemonic == "cmp" and parts[0] in registers and parts[1].isdigit():
      comparison = (registers[parts[0]], int(parts[1]))
    elif mnemonic == "sub" and parts[0] in registers:
      registers[parts[0]] = (registers[parts[0]] - int(parts[1], 0)) & 0xffffffff
      comparison = (registers[parts[0]], 0)
    elif mnemonic == "dec" and parts[0] in registers:
      registers[parts[0]] = (registers[parts[0]] - 1) & 0xffffffff
      comparison = (registers[parts[0]], 0)
    elif mnemonic == "mov" and parts[1] in registers and parts[0] in ("eax", "ecx", "edx"):
      registers[parts[0]] = registers[parts[1]]
    elif mnemonic in conditionalJumps:
      target = int(operands, 16)
      if branchTaken(mnemonic, *comparison):
        if target == managerDefault:
          return None
        address = target
        continue
    else:
      return address
    address += size
  raise RuntimeError(f"Manager walk for opcode {opcode:#x} did not terminate.")


def controlOffsets(client):
  """Map (window prefix, member offset) -> control name from GetChildItem bindings."""
  names = set(m.group(0).decode() for m in re.finditer(rb"(REIW|REMW|RENW|REPSW|REPW|RELDW)_\w+(?=\x00)", client.image))
  offsets = collections.defaultdict(dict)
  for name in names:
    for stringAddress in [client.imageBase + m.start() for m in re.finditer(re.escape(name.encode()) + b"\x00", client.image)
                          if m.start() == 0 or not (chr(client.image[m.start() - 1]).isalnum() or client.image[m.start() - 1] == ord("_"))]:
      for reference in client.codeReferences(stringAddress):
        for instruction in client.linearDisassembly(reference, reference + 60)[1:10]:
          match = re.match(r"dword ptr \[e\w\w \+ (0x[0-9a-f]+)\], eax$", instruction.op_str)
          if instruction.mnemonic == "mov" and match:
            offsets[name.split("_")[0]][int(match.group(1), 16)] = name
            break
  return offsets


def functionBounds(client, start):
  """End of a function: first int3 padding run after a ret."""
  instructions = client.linearDisassembly(start, start + 0x4000)
  for index, instruction in enumerate(instructions[:-1]):
    if instruction.mnemonic == "ret" and instructions[index + 1].mnemonic == "int3":
      return instruction.address + instruction.size
  raise RuntimeError(f"No function end found within 0x4000 bytes of {start:#x}")


def syncedDecodesEndingAt(client, address, minLookback=0x10, maxLookback=0x40):
  """All linear decodes that start somewhere in [address-maxLookback, address-minLookback]
  and land exactly on an instruction boundary at `address`. Each result is the list of
  instructions strictly before `address`. A decode that begins mid-instruction can still
  resynchronize, so callers vote across these rather than trusting one."""
  decodes = []
  for lookback in range(minLookback, maxLookback + 1):
    start = address - lookback
    decoded = []
    for instruction in client.disassembler.disasm(client.image[client.offset(start):client.offset(address) + 16], start):
      if instruction.address >= address:
        if instruction.address == address:
          decodes.append(decoded)
        break
      decoded.append(instruction)
  return decodes


def isInstructionBoundary(client, address):
  return len(syncedDecodesEndingAt(client, address)) >= 3


def blockTail(instructions):
  """Instructions after the last unconditional control transfer (the block that falls into
  the end of `instructions`)."""
  for index in range(len(instructions) - 1, -1, -1):
    if instructions[index].mnemonic in ("jmp", "ret"):
      return instructions[index + 1:]
  return instructions


def votedTail(client, address):
  """Majority-voted block tail ending at `address`, as a list of (address, mnemonic, operands).

  Synchronized decodes start at different lookbacks, so whole tails differ in length and
  can't be compared directly. Each decode instead votes for its anchor: the address and
  value of its last push, or the address of its block start when it has no push. When no
  decode reaches a real block boundary (jmp/ret) the lookback is widened.
  """
  for maxLookback in (0x40, 0x100, 0x200):
    votes = collections.Counter()
    tails = {}
    for decoded in syncedDecodesEndingAt(client, address, maxLookback=maxLookback):
      tail = blockTail(decoded)
      reachedBoundary = len(tail) < len(decoded)
      pushes = [i for i in tail if i.mnemonic == "push"]
      if pushes:
        key = ("push", pushes[-1].address, pushes[-1].op_str)
      elif reachedBoundary:
        key = ("block", tail[0].address if tail else address, None)
      else:
        continue
      votes[key] += 1
      candidate = [(i.address, i.mnemonic, i.op_str) for i in tail]
      if key not in tails or len(candidate) > len(tails[key]):
        tails[key] = candidate
    if votes:
      ranked = votes.most_common(2)
      if len(ranked) == 2 and ranked[0][1] == ranked[1][1]:
        raise RuntimeError(f"Ambiguous block tail before {address:#x}: {ranked}")
      return tails[ranked[0][0]]
  raise RuntimeError(f"No synchronized decode with a push or block boundary ends at {address:#x}")


def lastPushOpcode(tail):
  pushes = [operands for _, mnemonic, operands in tail if mnemonic == "push"]
  if not pushes:
    return "none"
  return int(pushes[-1], 16) if pushes[-1].startswith("0x") else None


def jumpIndex(client, targets):
  """Every verified jmp/jcc in .text whose target is in `targets` -> {target: [jumpAddress]}."""
  image = client.image
  textOffset = client.offset(client.textStart)
  textEndOffset = client.offset(client.textEnd)
  found = collections.defaultdict(list)
  for offset in range(textOffset, textEndOffset - 6):
    opcode = image[offset]
    if opcode == 0xE9:
      size, relative = 5, struct.unpack("<i", image[offset + 1:offset + 5])[0]
    elif opcode == 0xEB or 0x70 <= opcode <= 0x7F:
      size, relative = 2, struct.unpack("<b", image[offset + 1:offset + 2])[0]
    elif opcode == 0x0F and 0x80 <= image[offset + 1] <= 0x8F:
      size, relative = 6, struct.unpack("<i", image[offset + 2:offset + 6])[0]
    else:
      continue
    jumpAddress = client.imageBase + offset
    target = jumpAddress + size + relative
    if target in targets and isInstructionBoundary(client, jumpAddress):
      instruction = next(client.disassembler.disasm(image[offset:offset + 8], jumpAddress), None)
      if instruction is not None and instruction.mnemonic.startswith("j") and instruction.op_str == f"{target:#x}":
        found[target].append(jumpAddress)
  return found


def allSendSites(client):
  """Every call to the send opcode stub, with every opcode that can reach it.

  Returns {callAddress: [(opcode or None, via)]} where `via` is "fallthrough" or the address
  of a jump that enters the call's block. The compiler shares one send tail between several
  branches (each pushes its opcode, then jumps to the common call), so reading only the
  pushes immediately before the call misses opcodes; jumps into the block are followed up
  to three levels back.
  """
  callSites = []
  textOffset = client.offset(client.textStart)
  textEndOffset = client.offset(client.textEnd)
  position = client.image.find(b"\xe8", textOffset, textEndOffset)
  while position != -1:
    callAddress = client.imageBase + position
    relative = struct.unpack("<i", client.image[position + 1:position + 5])[0]
    if callAddress + 5 + relative == sendOpcodeStub and isInstructionBoundary(client, callAddress):
      callSites.append(callAddress)
    position = client.image.find(b"\xe8", position + 1, textEndOffset)

  entryWindows = {}
  for callAddress in callSites:
    tail = votedTail(client, callAddress)
    pushAddresses = [address for address, mnemonic, _ in tail if mnemonic == "push"]
    windowStart = (pushAddresses[-1] + 1) if pushAddresses else (tail[0][0] if tail else callAddress)
    entryWindows[callAddress] = (tail, windowStart)
  targets = set()
  for callAddress, (tail, windowStart) in entryWindows.items():
    targets.update(address for address, _, _ in tail if address >= windowStart)
    targets.add(callAddress)
  jumps = jumpIndex(client, targets)

  def opcodesThrough(jumpAddress, depth):
    tail = votedTail(client, jumpAddress)
    opcode = lastPushOpcode(tail)
    if opcode != "none" or depth == 0:
      return [(None if opcode == "none" else opcode, f"{jumpAddress:#x}")]
    blockStart = tail[0][0] if tail else jumpAddress
    results = []
    for target in [address for address, _, _ in tail] + [jumpAddress]:
      for predecessor in jumpIndex(client, {target}).get(target, []):
        results.extend(opcodesThrough(predecessor, depth - 1))
    return results or [(None, f"{jumpAddress:#x}")]

  sites = {}
  for callAddress, (tail, windowStart) in entryWindows.items():
    opcode = lastPushOpcode(tail)
    sources = [] if opcode == "none" else [(opcode, "fallthrough")]
    for address in sorted({a for a, _, _ in tail if a >= windowStart} | {callAddress}):
      for jumpAddress in jumps.get(address, []):
        sources.extend(opcodesThrough(jumpAddress, 2))
    sites[callAddress] = sources or [(None, "fallthrough")]
  return sites


def loadEQEmuNames(confPath):
  names = {}
  if confPath is None:
    return names
  with open(confPath) as confFile:
    for line in confFile:
      match = re.match(r"\s*(OP_\w+)\s*=\s*0x([0-9a-fA-F]+)", line)
      if match:
        names[int(match.group(2), 16)] = match.group(1)
  return names


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("exePath")
  parser.add_argument("--eqemuConf", default=None, help="EQEmu patch_RoF2.conf for cross-reference")
  parser.add_argument("--jsonOut", default=None, help="write the full send-call inventory as JSON")
  parser.add_argument("--casesOut", default=None, help="write both HandleWorldMessage switches (opcode -> handler) as JSON")
  arguments = parser.parse_args()

  client = ClientImage(arguments.exePath)
  verifyAnchors(client)
  eqemuNames = loadEQEmuNames(arguments.eqemuConf)

  dispatcherCases = {}
  for opcode in range(0x10000):
    handler = resolveDispatcherCase(client, opcode)
    if handler is not None:
      dispatcherCases[opcode] = handler
  print(f"HandleWorldMessage switch 1: {len(dispatcherCases)} opcodes, {len(set(dispatcherCases.values()))} distinct handlers")
  secondCases = {}
  for opcode in range(0x10000):
    handler = resolveDispatcherCase(client, opcode, secondSwitchStart, secondSwitchOpcodeLocal, secondSwitchDefault)
    if handler is not None:
      secondCases[opcode] = handler
  print(f"HandleWorldMessage switch 2: {len(secondCases)} opcodes, {len(set(secondCases.values()))} distinct handlers; "
        f"union {len(set(dispatcherCases) | set(secondCases))} opcodes")
  if arguments.casesOut:
    with open(arguments.casesOut, "w") as outFile:
      json.dump({"switch1": {f"{o:#06x}": f"{h:#x}" for o, h in sorted(dispatcherCases.items())},
                 "switch2": {f"{o:#06x}": f"{h:#x}" for o, h in sorted(secondCases.items())}}, outFile, indent=0)
  if eqemuNames:
    known = sum(1 for opcode in dispatcherCases if opcode in eqemuNames)
    print(f"  {known} of {len(dispatcherCases)} match names in the EQEmu conf")

  forwarded = sorted(o for o, h in dispatcherCases.items() if h == realEstateForwardCase)
  print(f"\nServer -> client: {len(forwarded)} opcodes forwarded to RealEstateManagerClient::HandleMessage")
  for opcode in forwarded:
    handler = resolveManagerCase(client, opcode)
    label = f"case {handler:#x}" if handler else "manager default (ignored)"
    print(f"  {opcode:#06x}  {label}  {eqemuNames.get(opcode, '')}")

  offsets = controlOffsets(client)
  notifyHandlers = {client.vtableEntries(vtable)[windowNotifyIndex]: (prefix, name)
                    for prefix, (name, vtable) in windowVtables.items()}
  sites = allSendSites(client)
  opcodeSet = {o for sources in sites.values() for o, _ in sources if o is not None}
  shared = {a: srcs for a, srcs in sites.items() if len({o for o, _ in srcs}) > 1}
  computed = [a for a, srcs in sites.items() if all(o is None for o, _ in srcs)]
  print(f"\nClient -> server: {len(sites)} send calls, {len(opcodeSet)} distinct literal opcodes, "
        f"{len(shared)} calls reached by more than one opcode (shared tails), {len(computed)} with only computed opcodes")
  if eqemuNames:
    print(f"  {len({o for o in opcodeSet if o in eqemuNames})} distinct opcodes named in the EQEmu conf")
  if arguments.jsonOut:
    with open(arguments.jsonOut, "w") as outFile:
      json.dump({f"{a:#x}": [[o, via] for o, via in srcs] for a, srcs in sites.items()}, outFile, indent=0)
  print(f"\nReal estate send calls:")
  for callAddress in sorted(a for a in sites if any(lo <= a < hi for lo, hi, _ in sendScanRanges)):
    rangeName = next(n for lo, hi, n in sendScanRanges if lo <= callAddress < hi)
    described = ", ".join(f"{o:#06x}" if o is not None else "(computed)" for o, _ in sites[callAddress])
    names = " ".join(eqemuNames.get(o, "") for o, _ in sites[callAddress] if o is not None).strip()
    print(f"  call {callAddress:#x} [{rangeName}] {described} {names}")


if __name__ == "__main__":
  sys.exit(main())
