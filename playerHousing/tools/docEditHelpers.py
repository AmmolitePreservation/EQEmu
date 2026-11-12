import pathlib
import re
import sys


def load(path):
  raw = path.read_bytes()
  return b"\r\n" in raw, raw.decode("utf-8").replace("\r\n", "\n")


def save(path, crlf, text):
  path.write_bytes((text.replace("\n", "\r\n") if crlf else text).encode("utf-8"))
  print(f"edited {path.name}")


def replaceOnce(text, old, new, label):
  if text.count(old) != 1:
    sys.exit(f"{label}: expected one match, got {text.count(old)} for:\n{old[:200]}")
  return text.replace(old, new)


def replaceEntry(text, key, entry):
  found = re.search(rf"^- \*\*{key}\*\* `\w+` .*$", text, re.M)
  if not found:
    sys.exit(f"{key} entry not found")
  return text[:found.start()] + entry + text[found.end():]


def insertAfterEntry(text, key, addition):
  found = re.search(rf"^- \*\*{key}\*\* `\w+` .*\n(?:  <sub>.*</sub>\n)?", text, re.M)
  if not found:
    sys.exit(f"{key} entry not found")
  return text[:found.end()] + addition + text[found.end():]


def replaceSpan(text, startMarker, endMarker, new, label):
  if text.count(startMarker) != 1:
    sys.exit(f"{label}: expected one start, got {text.count(startMarker)} for:\n{startMarker[:200]}")
  start = text.index(startMarker)
  end = text.find(endMarker, start + len(startMarker))
  if end < 0:
    sys.exit(f"{label}: end marker not found:\n{endMarker[:200]}")
  return text[:start] + new + text[end:]
