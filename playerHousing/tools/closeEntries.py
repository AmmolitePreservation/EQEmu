import json
import pathlib
import re
import sys

from docEditHelpers import insertAfterEntry, load, save

worklistPath = pathlib.Path(sys.argv[1])
changes = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))

crlf, text = load(worklistPath)
for change in changes:
  if "insertAfter" in change:
    text = insertAfterEntry(text, change["insertAfter"], change["text"])
    continue
  key = change["key"]
  oldTag = change["oldTag"]
  newTag = change["newTag"]
  found = re.search(rf"^- \*\*{key}\*\* `{oldTag}` .*$", text, re.M)
  if not found:
    sys.exit(f"{key} entry not found with tag {oldTag}")
  line = found.group(0).replace(f"**{key}** `{oldTag}`", f"**{key}** `{newTag}`", 1) + change["addition"]
  text = text[:found.start()] + line + text[found.end():]

itemPattern = re.compile(r"^- \*\*[A-Z0-9]+-\d+[a-z]?\*\* `([^`]+)` ", re.M)
tags = ("fix", "liveCheck", "clientAnalysis", "peridotTest", "done")
counts = dict.fromkeys(tags, 0)
for tag in itemPattern.findall(text):
  if tag not in counts:
    sys.exit(f"an entry carries the unknown tag {tag}")
  counts[tag] += 1
questions = len(re.findall(r"^### Q\d+[a-z]?\. ", text, re.M))
found = re.search(r"\*\*Counts\.\*\* fix \d+, liveCheck \d+, clientAnalysis \d+, peridotTest \d+, question \d+, done \d+; \d+ in all\.", text)
if not found:
  sys.exit("Counts line not found")
text = text[:found.start()] + "**Counts.** fix {}, liveCheck {}, clientAnalysis {}, peridotTest {}, question {}, done {}; {} in all.".format(
  counts["fix"], counts["liveCheck"], counts["clientAnalysis"], counts["peridotTest"], questions, counts["done"],
  sum(counts.values()) + questions) + text[found.end():]
save(worklistPath, crlf, text)
print(counts)
