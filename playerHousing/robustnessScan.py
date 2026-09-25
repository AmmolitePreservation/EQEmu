"""Flag crash-relevant assumptions in the real estate server -> client handlers.

For each handler (and the functions it calls, two levels deep, inside the housing code ranges),
report: lookup results used before a null check, allocations whose size is not a constant,
memory writes indexed by a register, and window pointers used before a null check. Every
flag is a candidate for a manual read, not a verdict.

Usage:
  python robustnessScan.py <path to eqgame.exe> <output .md>
"""
import re
import sys

from clientAnalysis import ClientIndex, functionDisassembly
from realEstateOpcodeScan import ClientImage

handlers = [
  ("0x2698 definitions and costs", [0x7f5b60, 0x7fc6b0]),
  ("0x1cef placed-item list", [0x5b0d00]),
  ("0x087f", [0x5b0e30]),
  ("0x1097 item moved", [0x5ab4b0]),
  ("0x2cf7", [0x5abd20]),
  ("0x1baf", [0x5afe80]),
  ("0x1967", [0x5ab030]),
  ("0x21ec interaction menu", [0x7375f0]),
  ("0x6fd0 / 0x4f09 / 0x63fd property records", [0x7fdf00]),
  ("0x2aa6 account reply", [0x741670]),
  ("0x2691 search reply", [0x5b0f80, 0x74ab50, 0x74c4c0]),
  ("0x7d28", [0x5b4680]),
  ("0x3968 / 0x485d access lists", [0x5b5250, 0x5b7a90]),
  ("0x626e begin placement", [0x558bb0, 0x505400]),
  ("0x49df regions", [0x4b2740, 0x4b2ab0]),
  ("0x1045 world object", [0x504b60, 0x504fd0]),
  ("0x2a9e / 0x2523 world object remove / update", [0x5061a0, 0x556e10]),
  ("0x7bb1 anchor teleport", [0x4d5cb0]),
  ("polls", [0x5a7a20, 0x5a70c0, 0x7fb780, 0x5a7440, 0x5a75f0, 0x5a7840, 0x7fb020, 0x5a7420, 0x5a7360]),
  ("0x1d3c guild neighborhood", [0x425c60]),
  ("OP_GuildUpdate action 5 rank permission", [0x7d95e0]),
  ("OP_ClickObjectAction housing container", [0x4c19a0, 0x65d890]),
  ("OP_SendMembershipDetails", [0x7dbe90]),
]

inlineHandlers = [
  ("0x626e begin placement (dispatcher part)", 0x4d058a, 0x4d0800),
  ("0x1045 world object (dispatcher part)", 0x4d080a, 0x4d0998),
  ("0x2a9e world object remove (dispatcher part)", 0x4d0998, 0x4d0a10),
  ("0x2523 placed item update (dispatcher part)", 0x4d0a10, 0x4d0a30),
  ("0x49df regions (dispatcher part)", 0x4ca7a5, 0x4ca800),
  ("0x7bb1 anchor teleport (dispatcher part)", 0x4d3798, 0x4d3800),
  ("0x7a4d open vote window", 0x4c5bb8, 0x4c5c82),
  ("manager cases 0x21ea / 0x58c7 / 0x45be / 0x5a63", 0x5b26fa, 0x5b2d40),
]

housingRanges = [(0x4b2000, 0x4b4000), (0x504000, 0x50a000), (0x556000, 0x55a000), (0x5a6000, 0x5a9000),
                 (0x5aa000, 0x5b8000), (0x72f000, 0x74f000), (0x792000, 0x797000), (0x7d9000, 0x7dc000),
                 (0x7f4000, 0x7fe000), (0x65c000, 0x65e000), (0x425000, 0x427000), (0x4c1000, 0x4c2000)]

lookups = {
  0x466030: "record by id", 0x5b01c0: "placed entry", 0x7f4fa0: "definition", 0x5996e0: "spawn by id",
  0x7fa5d0: "poll by id", 0x7fa690: "poll results", 0x506120: "world object by ids",
  0x5060c0: "world object by GUID", 0x4ff100: "access list",
}
allocators = {0x8dbb3b: "operator new", 0x8da58b: "malloc"}
windowGlobal = re.compile(r"dword ptr \[(0xd1f[c-d][0-9a-f]{2})\]$")


def inHousing(address):
  return any(low <= address < high for low, high in housingRanges)


def reachable(client, roots, depth=2):
  seen = []
  frontier = list(roots)
  for _ in range(depth + 1):
    nextFrontier = []
    for function in frontier:
      if function in seen:
        continue
      seen.append(function)
      for instruction in functionDisassembly(client, function, 0x3000):
        if instruction.mnemonic == "call" and instruction.op_str.startswith("0x"):
          target = int(instruction.op_str, 16)
          if inHousing(target) and target not in lookups and target not in seen:
            nextFrontier.append(target)
    frontier = nextFrontier
  return seen


def usedBeforeCheck(instructions, start, register):
  aliases = {register}
  for instruction in instructions[start:start + 16]:
    text = f"{instruction.mnemonic} {instruction.op_str}"
    if instruction.mnemonic in ("test", "cmp") and any(re.search(rf"\b{alias}\b", instruction.op_str) for alias in aliases):
      return None
    if instruction.mnemonic.startswith("j") or instruction.mnemonic == "ret":
      return None
    copy = re.match(r"mov (e\w\w), (e\w\w)$", text)
    if copy and copy.group(2) in aliases:
      aliases.add(copy.group(1))
      continue
    if any(re.search(rf"\[{alias}(?: \+ [^\]]+)?\]", instruction.op_str) for alias in aliases):
      return instruction
    if instruction.mnemonic == "call" and "ecx" in aliases and not instruction.op_str.startswith("0x"):
      return instruction
    overwritten = re.match(r"(?:mov|lea|xor|pop) (e\w\w),", text)
    if overwritten and overwritten.group(1) in aliases and not (copy and copy.group(2) in aliases):
      aliases.discard(overwritten.group(1))
      if not aliases:
        return None
  return None


lookupStats = {"calls": 0, "flagged": 0}


def scanFunction(client, function, instructions=None):
  findings = []
  if instructions is None:
    instructions = functionDisassembly(client, function, 0x3000)
  for index, instruction in enumerate(instructions):
    if instruction.mnemonic == "call" and instruction.op_str.startswith("0x"):
      target = int(instruction.op_str, 16)
      if target in lookups:
        lookupStats["calls"] += 1
        use = usedBeforeCheck(instructions, index + 1, "eax")
        if use is not None:
          lookupStats["flagged"] += 1
          findings.append(("null lookup", instruction.address, f"{lookups[target]} result used at {use.address:#x}: {use.mnemonic} {use.op_str}"))
      if target in allocators:
        pushes = [previous for previous in instructions[max(0, index - 4):index] if previous.mnemonic == "push"]
        if pushes and not re.match(r"(0x[0-9a-f]+|\d+)$", pushes[-1].op_str):
          findings.append(("sized allocation", instruction.address, f"{allocators[target]}({pushes[-1].op_str})"))
    written = re.match(r"(?:byte|word|dword) ptr \[(e\w\w) \+ (e\w\w)\*[248](?: \+ (?:0x[0-9a-f]+|\d+))?\], ", instruction.op_str)
    if written and instruction.mnemonic in ("mov", "or", "and", "inc", "add", "sub"):
      findings.append(("indexed write", instruction.address, f"{instruction.mnemonic} {instruction.op_str}"))
    computed = re.match(r"lea (e\w\w), \[e\w\w \+ e\w\w\*[248](?: \+ (?:0x[0-9a-f]+|\d+))?\]$", f"{instruction.mnemonic} {instruction.op_str}")
    if computed:
      register = computed.group(1)
      for later in instructions[index + 1:index + 14]:
        if later.mnemonic in ("mov", "or", "and", "add", "sub") and later.op_str.startswith(("dword ptr [" + register + "]", "byte ptr [" + register + "]", "word ptr [" + register + "]")):
          findings.append(("indexed write", later.address, f"{later.mnemonic} {later.op_str} (address from {instruction.op_str})"))
          break
        if re.match(rf"(?:mov|lea|xor|pop) {register},", f"{later.mnemonic} {later.op_str}") or later.mnemonic in ("ret", "call"):
          break
    window = windowGlobal.search(instruction.op_str)
    if window and instruction.mnemonic == "mov" and instruction.op_str.startswith("e"):
      register = instruction.op_str.split(",")[0]
      use = usedBeforeCheck(instructions, index + 1, register)
      if use is not None:
        findings.append(("window pointer", instruction.address, f"[{window.group(1)}] used at {use.address:#x}: {use.mnemonic} {use.op_str}"))
  return findings


def main():
  if len(sys.argv) != 3:
    print(__doc__)
    return 2
  client = ClientImage(sys.argv[1])
  index = ClientIndex(client)
  lines = ["# Robustness Scan (generated)", "",
           "Generated by `robustnessScan.py`. Candidates for a manual read, per handler (the handler and the housing "
           "functions it calls, two levels deep). Do not edit by hand; the reviewed results live in `robustness.md`.", ""]
  total = 0
  for name, roots in handlers:
    functions = reachable(client, roots)
    rows = []
    for function in functions:
      for kind, address, detail in scanFunction(client, function):
        rows.append(f"| {kind} | `{function:#x}` | `{address:#x}` | {detail.replace('|', '/')} |")
    total += len(rows)
    lines += [f"## {name}", "", f"{len(functions)} functions scanned.", ""]
    if rows:
      lines += ["| Kind | Function | Site | Detail |", "|---|---|---|---|"] + rows
    else:
      lines.append("No candidates.")
    lines.append("")
  for name, start, end in inlineHandlers:
    instructions = client.linearDisassembly(start, end)
    rows = [f"| {kind} | inline | `{address:#x}` | {detail.replace('|', '/')} |" for kind, address, detail in scanFunction(client, start, instructions)]
    callees = sorted({int(i.op_str, 16) for i in instructions if i.mnemonic == "call" and i.op_str.startswith("0x") and inHousing(int(i.op_str, 16)) and int(i.op_str, 16) not in lookups})
    for function in reachable(client, callees, 1):
      rows += [f"| {kind} | `{function:#x}` | `{address:#x}` | {detail.replace('|', '/')} |" for kind, address, detail in scanFunction(client, function)]
    total += len(rows)
    lines += [f"## {name}", "", f"Inline range `{start:#x}`–`{end:#x}` and its housing callees.", ""]
    lines += (["| Kind | Function | Site | Detail |", "|---|---|---|---|"] + rows) if rows else ["No candidates."]
    lines.append("")
  lines += [f"Lookup calls seen: {lookupStats['calls']}; used before a null check: {lookupStats['flagged']}.", ""]
  with open(sys.argv[2], "w", encoding="utf-8") as outFile:
    outFile.write("\n".join(lines) + "\n")
  print(f"{len(handlers) + len(inlineHandlers)} handlers, {total} candidates; lookups seen {lookupStats['calls']}, flagged {lookupStats['flagged']}")
  return 0


if __name__ == "__main__":
  sys.exit(main())
