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
counts = {}
for tag in itemPattern.findall(text):
  counts[tag] = counts.get(tag, 0) + 1
found = re.search(r"\*\*Counts\.\*\* fix \d+, liveCheck \d+, peridotTest \d+, question \d+, polish \d+, done \d+; \d+ in all\.", text)
if not found:
  sys.exit("Counts line not found")
text = text[:found.start()] + "**Counts.** fix {}, liveCheck {}, peridotTest {}, question {}, polish {}, done {}; {} in all.".format(
  counts.get("fix", 0), counts.get("liveCheck", 0), counts.get("peridotTest", 0), counts.get("question", 0),
  counts.get("polish", 0), counts.get("done", 0), sum(counts.values())) + text[found.end():]
save(worklistPath, crlf, text)
print(counts)
