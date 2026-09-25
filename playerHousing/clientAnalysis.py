"""Reusable static-analysis helpers for the RoF2 client (eqgame.exe).

Consolidates the ad-hoc helpers used during the player housing research so they persist:
function disassembly, a cached call/reference index and call graph, "which function
contains this address", callers, references to a value, strings used by a function,
eqstr/dbstr lookups, and annotated disassembly.

The index is built with a linear sweep of .text (a few minutes) and cached to a file
outside the repo (default /tmp/clientAnalysisIndex.pkl). The owner's client files
(eqgame.exe, eqstr_us.txt, dbstr_us.txt) are not committed; pass their paths.

Command line:
  python clientAnalysis.py <eqgame.exe> fn <address> [--eqstr eqstr_us.txt]
  python clientAnalysis.py <eqgame.exe> callers <address>
  python clientAnalysis.py <eqgame.exe> refs <value>
  python clientAnalysis.py <eqgame.exe> strings <function address>
  python clientAnalysis.py <eqgame.exe> containing <address>
  python clientAnalysis.py <eqgame.exe> findString "<exact text>"

Requires: pefile, capstone (via realEstateOpcodeScan)
"""
import argparse
import bisect
import collections
import os
import pickle
import re
import struct
import sys

from realEstateOpcodeScan import ClientImage

defaultIndexPath = "/tmp/clientAnalysisIndex.pkl"


def functionDisassembly(client, start, maxBytes=0x3000):
  """Instructions from `start` until a ret that is followed by int3 padding."""
  instructions = client.linearDisassembly(start, start + maxBytes)
  for index, instruction in enumerate(instructions[:-1]):
    if instruction.mnemonic == "ret" and instructions[index + 1].mnemonic == "int3":
      return instructions[:index + 1]
  return instructions


def cString(client, address, maxLength=300):
  offset = address - client.imageBase
  if not (0 <= offset < len(client.image)):
    return None
  end = client.image.find(b"\x00", offset, offset + maxLength)
  raw = client.image[offset:end]
  if len(raw) < 2 or not all(32 <= c < 127 or c in (9, 10) for c in raw):
    return None
  return raw.decode()


class ClientIndex:
  """Call sites, immediate references and function boundaries for the whole of .text."""

  def __init__(self, client, cachePath=defaultIndexPath):
    self.client = client
    if os.path.exists(cachePath):
      with open(cachePath, "rb") as cacheFile:
        cached = pickle.load(cacheFile)
      if cached.get("imageSize") != len(client.image):
        raise RuntimeError(f"Index cache {cachePath} was built for a different image; delete it and rebuild")
      self.calls, self.references, self.functionStarts = cached["calls"], cached["references"], cached["functionStarts"]
    else:
      self.build()
      with open(cachePath, "wb") as cacheFile:
        pickle.dump({"imageSize": len(client.image), "calls": self.calls, "references": self.references,
                     "functionStarts": self.functionStarts}, cacheFile)
    self.callersOf = collections.defaultdict(list)
    for site, target in self.calls.items():
      self.callersOf[target].append(site)

  def build(self):
    client = self.client
    imageEnd = client.imageBase + len(client.image)
    immediate = re.compile(r"0x[0-9a-f]{6,8}")
    self.calls, self.references = {}, {}
    starts = set()
    for instruction in client.linearDisassembly(client.textStart, client.textEnd):
      if instruction.mnemonic == "call" and instruction.op_str.startswith("0x"):
        target = int(instruction.op_str, 16)
        self.calls[instruction.address] = target
        starts.add(target)
        continue
      values = [int(v, 16) for v in immediate.findall(instruction.op_str) if client.imageBase <= int(v, 16) < imageEnd]
      if values:
        self.references[instruction.address] = values
    code = client.image[client.offset(client.textStart):client.offset(client.textEnd)]
    for match in re.finditer(rb"\xcc+([^\xcc])", code):
      address = client.textStart + match.start(1)
      if address % 16 == 0:
        starts.add(address)
    rdata = next(s for s in client.pe.sections if s.Name.startswith(b".rdata"))
    for offset in range(rdata.VirtualAddress, rdata.VirtualAddress + rdata.Misc_VirtualSize - 4, 4):
      pointer = struct.unpack("<I", client.image[offset:offset + 4])[0]
      if client.textStart <= pointer < client.textEnd and pointer % 16 == 0:
        starts.add(pointer)
    self.functionStarts = sorted(starts)

  def containing(self, address):
    index = bisect.bisect_right(self.functionStarts, address) - 1
    return self.functionStarts[index] if index >= 0 else None

  def callers(self, target):
    return sorted((site, self.containing(site)) for site in self.callersOf.get(target, []))

  def referencesTo(self, value):
    return sorted((site, self.containing(site)) for site, values in self.references.items() if value in values)

  def stringsOf(self, function):
    end = self.functionStarts[bisect.bisect_right(self.functionStarts, function)] if function in self.functionStarts else function + 0x1000
    found = set()
    for site, values in self.references.items():
      if function <= site < end:
        for value in values:
          text = cString(self.client, value)
          if text:
            found.add(text)
    return sorted(found)


def loadEqstr(path):
  table = {}
  for line in open(path, encoding="latin-1"):
    parts = line.rstrip("\r\n").split(" ", 1)
    if len(parts) == 2 and parts[0].isdigit():
      table[int(parts[0])] = parts[1]
  return table


def loadDbstr(path):
  table = {}
  for line in open(path, encoding="latin-1"):
    parts = line.rstrip("\r\n").split("^")
    if len(parts) >= 3 and parts[0].isdigit() and parts[1].isdigit():
      table[(int(parts[1]), int(parts[0]))] = parts[2]
  return table


def annotate(client, instruction, eqstr=None, controlNames=None):
  notes = []
  for value in re.findall(r"0x[0-9a-f]{6,8}", instruction.op_str):
    text = cString(client, int(value, 16), 80)
    if text:
      notes.append(f'"{text}"')
  if eqstr and instruction.mnemonic == "push" and instruction.op_str.startswith("0x"):
    value = int(instruction.op_str, 16)
    if 0x200 < value in eqstr:
      notes.append("eqstr: " + eqstr[value][:90])
  if controlNames:
    for match in re.finditer(r"\[e\w\w \+ (0x[0-9a-f]+)\]", instruction.op_str):
      offset = int(match.group(1), 16)
      if offset in controlNames and "esp" not in instruction.op_str:
        notes.append(f"<{controlNames[offset]}>")
  return ("   ; " + "  ".join(notes)) if notes else ""


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("exePath")
  parser.add_argument("command", choices=["fn", "callers", "refs", "strings", "containing", "findString"])
  parser.add_argument("argument")
  parser.add_argument("--eqstr", default=None)
  parser.add_argument("--index", default=defaultIndexPath)
  arguments = parser.parse_args()
  client = ClientImage(arguments.exePath)
  eqstr = loadEqstr(arguments.eqstr) if arguments.eqstr else None
  if arguments.command == "fn":
    for instruction in functionDisassembly(client, int(arguments.argument, 16)):
      print(f"{instruction.address:x} {instruction.mnemonic} {instruction.op_str}{annotate(client, instruction, eqstr)}")
    return 0
  if arguments.command == "findString":
    for address in client.stringAddresses(arguments.argument.encode()):
      print(f"{address:#x}")
    return 0
  index = ClientIndex(client, arguments.index)
  value = int(arguments.argument, 16)
  if arguments.command == "callers":
    for site, function in index.callers(value):
      print(f"{site:#x} in {function:#x}")
  elif arguments.command == "refs":
    for site, function in index.referencesTo(value):
      print(f"{site:#x} in {function:#x}")
  elif arguments.command == "strings":
    for text in index.stringsOf(value):
      print(text)
  elif arguments.command == "containing":
    print(f"{index.containing(value):#x}")
  return 0


if __name__ == "__main__":
  sys.exit(main())
