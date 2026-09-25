"""Inventory every control in the given GUI XML files and join it with the client's
bindings for that control.

For each control the XML defines, report: element type, ScreenID, text, tooltip, checkbox
style, parent piece, and how the client binds it (the window member offset it stores the
control in, and whether the lookup is required or optional). Controls the client looks up
that the XML lacks are reported too.

The XML files are the owner's client files and are not committed; pass their directory.

Usage:
  python guiControlInventory.py <path to eqgame.exe> <XML directory> <output .md> [file pattern ...]

The file patterns default to EQUI_RealEstate*.xml.

Requires: pefile, capstone (via realEstateOpcodeScan)
"""
import collections
import glob
import os
import re
import sys

from realEstateOpcodeScan import ClientImage

getChildItem = 0x85cfd0            # CSidlScreenWnd::GetChildItem(name, flag)
cxStrAssign = 0x805c20             # CXStr assign from char*, used to build the lookup name


def parseXML(path):
  text = open(path, encoding="utf-8", errors="replace").read()
  controls = []
  parentOf = {}
  for match in re.finditer(r"<(\w+)\s+item\s*=\s*\"([^\"]+)\">(.*?)</\1>", text, re.S):
    tag, item, body = match.group(1), match.group(2), match.group(3)
    for piece in re.findall(r"<Pieces>\s*([^<\s]+)\s*</Pieces>", body):
      parentOf[piece] = item
    screenID = re.search(r"<ScreenID>\s*([^<\s]+)\s*</ScreenID>", body)
    textValue = re.search(r"<Text>(.*?)</Text>", body, re.S)
    tooltip = re.search(r"<TooltipReference>(.*?)</TooltipReference>", body, re.S)
    checkbox = re.search(r"<Style_Checkbox>\s*true\s*</Style_Checkbox>", body)
    controls.append({
      "tag": tag,
      "item": item,
      "screenID": screenID.group(1) if screenID else item,
      "text": textValue.group(1).strip() if textValue else "",
      "tooltip": tooltip.group(1).strip() if tooltip else "",
      "checkbox": bool(checkbox),
    })
  for control in controls:
    control["parent"] = parentOf.get(control["item"], "")
  return controls


def containingInstruction(client, reference, value):
  """Start of the instruction whose operand bytes include the immediate `value` at
  `reference` (a code reference points at the immediate, not the instruction start)."""
  for back in range(1, 8):
    start = reference - back
    instruction = next(client.disassembler.disasm(client.image[client.offset(start):client.offset(start) + 16], start), None)
    if instruction is not None and instruction.address + instruction.size > reference and f"{value:#x}" in instruction.op_str:
      return start
  return None


def clientBindings(client, prefixes):
  """{name: [(windowMemberOffset, flagOperand, lookupCallAddress)]} for every GetChildItem
  call whose name starts with one of the control-name prefixes."""
  bindings = collections.defaultdict(list)
  textOffset = client.offset(client.textStart)
  textEndOffset = client.offset(client.textEnd)
  names = {}
  prefixPattern = "|".join(re.escape(prefix) for prefix in sorted(prefixes, key=len, reverse=True))
  for match in re.finditer(rf"(?:{prefixPattern})_\w+(?=\x00)".encode(), client.image):
    # The name must not be the tail of a longer identifier. Requiring a NUL before it is too
    # strict: string literals can follow non-string data (REPW_Eviction_Label follows a float).
    previous = client.image[match.start() - 1] if match.start() > 0 else 0
    if not (chr(previous).isalnum() or previous == ord("_")):
      names[client.imageBase + match.start()] = match.group(0).decode()
  for nameAddress, name in names.items():
    for reference in client.codeReferences(nameAddress):
      instructionStart = containingInstruction(client, reference, nameAddress)
      if instructionStart is None:
        continue
      window = client.linearDisassembly(instructionStart, instructionStart + 0x60)
      for index, instruction in enumerate(window):
        if instruction.mnemonic == "call" and instruction.op_str == f"{getChildItem:#x}":
          pushes = [i for i in window[:index] if i.mnemonic == "push"]
          flagOperand = pushes[-2].op_str if len(pushes) >= 2 else "?"
          offset = None
          for following in window[index + 1:index + 4]:
            store = re.match(r"dword ptr \[e\w\w \+ (0x[0-9a-f]+)\], eax$", following.op_str)
            if following.mnemonic == "mov" and store:
              offset = int(store.group(1), 16)
              break
          bindings[name].append((offset, flagOperand, instruction.address))
          break
  return bindings


def main():
  if len(sys.argv) < 4:
    print(__doc__)
    return 2
  exePath, xmlDirectory, outputPath = sys.argv[1:4]
  patterns = sys.argv[4:] or ["EQUI_RealEstate*.xml"]
  xmlFiles = sorted({path for pattern in patterns for path in glob.glob(os.path.join(xmlDirectory, pattern))})
  if not xmlFiles:
    raise RuntimeError(f"No files matching {patterns} in {xmlDirectory}")
  parsed = [(xmlPath, parseXML(xmlPath)) for xmlPath in xmlFiles]
  windowPrefixes = tuple(sorted({control["screenID"].split("_")[0] for _, controls in parsed
                                 for control in controls if "_" in control["screenID"]}))
  client = ClientImage(exePath)
  bindings = clientBindings(client, windowPrefixes)

  lines = ["# GUI Control Inventory (generated)", "",
           f"Generated by `guiControlInventory.py` from the owner's {', '.join(f'`{pattern}`' for pattern in patterns)} files and the RoF2 client. "
           "Do not edit by hand; the per-control analysis lives in `windows/`.", "",
           "**Client binding** is the window member offset where the client stores the control, "
           "from its `GetChildItem` (`0x85cfd0`) call. **Lookup** is that call's flag: `1` means "
           "required; any other operand is shown as-is (a register holding 0 means optional). "
           "**not bound** means the client never looks the control up by name, which is normal "
           "for static labels, layout boxes, and templates.", ""]
  xmlNames = set()
  for xmlPath, controls in parsed:
    lines += [f"## {os.path.basename(xmlPath)}", "",
              "| Type | ScreenID | Parent | Text / tooltip | Client binding | Lookup |",
              "|---|---|---|---|---|---|"]
    for control in controls:
      name = control["screenID"]
      xmlNames.add(name)
      if control["tag"] == "Screen" and not name.startswith(windowPrefixes):
        continue
      kind = control["tag"] + (" (checkbox)" if control["checkbox"] else "")
      label = " / ".join(x for x in (control["text"], control["tooltip"]) if x).replace("|", "\\|")
      bound = bindings.get(name)
      binding = ", ".join(f"`+{o:#x}`" if o is not None else "(not stored)" for o, _, _ in bound) if bound else "not bound"
      lookup = ", ".join(sorted({f for _, f, _ in bound})) if bound else ""
      lines.append(f"| {kind} | `{name}` | {control['parent']} | {label} | {binding} | {lookup} |")
    lines.append("")
  missing = sorted(n for n in bindings if n not in xmlNames)
  lines += ["## Looked up by the client but absent from the XML", ""]
  for name in missing:
    detail = ", ".join(f"`+{o:#x}` flag `{f}` at `{a:#x}`" for o, f, a in bindings[name] if o is not None)
    lines.append(f"- `{name}` {detail}")
  if not missing:
    lines.append("- none")
  with open(outputPath, "w", encoding="utf-8") as outFile:
    outFile.write("\n".join(lines) + "\n")
  total = sum(1 for line in lines if line.startswith("| ") and not line.startswith("| Type") and not line.startswith("|---"))
  print(f"{len(xmlFiles)} XML files, {total} controls listed, {len(bindings)} client-bound names, {len(missing)} missing from XML")
  return 0


if __name__ == "__main__":
  sys.exit(main())
