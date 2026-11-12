import re
import sys

path = sys.argv[1]
mode = sys.argv[2]
wanted = set(part for part in sys.argv[3].split(",") if part) if len(sys.argv) > 3 else set()
titleChars = int(sys.argv[4]) if len(sys.argv) > 4 else 160

itemPattern = re.compile(r"^- \*\*([A-Z0-9]+-\d+[a-z]?)\*\* `([^`]+)` \*\*(.+?)\*\*(.*)$")
counts = {}
items = []
with open(path, "r", encoding="utf-8") as handle:
  section = ""
  for number, line in enumerate(handle, 1):
    line = line.rstrip("\n")
    if line.startswith("#"):
      section = line.strip()
      if mode == "outline":
        print("%5d %s" % (number, section))
      continue
    match = itemPattern.match(line)
    if not match:
      if mode == "outline" and line.strip() and not line.startswith("- **"):
        print("%5d   | %s" % (number, line[:titleChars]))
      continue
    itemID, tag, title, rest = match.groups()
    counts[tag] = counts.get(tag, 0) + 1
    items.append((number, section, itemID, tag, title, rest))

if mode == "counts":
  for tag, count in sorted(counts.items(), key=lambda pair: -pair[1]):
    print("%5d %s" % (count, tag))
elif mode == "list":
  lastSection = None
  for number, section, itemID, tag, title, rest in items:
    if wanted and tag not in wanted:
      continue
    if section != lastSection:
      print("\n" + section)
      lastSection = section
    print("%5d %-8s %-12s %s" % (number, itemID, tag, title[:titleChars]))
elif mode == "full":
  for number, section, itemID, tag, title, rest in items:
    if itemID in wanted:
      print("%d [%s] %s `%s` %s%s\n" % (number, section, itemID, tag, title, rest[:titleChars]))
elif mode == "grep":
  pattern = re.compile(sys.argv[3], re.IGNORECASE)
  for number, section, itemID, tag, title, rest in items:
    body = title + rest
    for found in pattern.finditer(body):
      start = max(0, found.start() - titleChars)
      print("%5d %-8s %-10s ...%s..." % (number, itemID, tag, body[start:found.end() + titleChars]))
