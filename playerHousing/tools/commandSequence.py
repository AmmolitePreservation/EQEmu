import json
import sys
import time

from claudeLinkClient.linePipe import LinePipeSession

processID = int(sys.argv[1])
session = LinePipeSession(processID)
start = time.monotonic()
try:
  for raw in sys.stdin:
    line = raw.rstrip("\r\n")
    if not line or line.startswith("#"):
      continue
    parts = line.split("|")
    if parts[0] == "sleep":
      time.sleep(float(parts[1]))
      continue
    reply = session.command(parts[0], *parts[1:])
    elapsed = int((time.monotonic() - start) * 1000)
    text = json.dumps(reply)
    print(f"{elapsed:6d} {parts[0]} {text[:300]}")
finally:
  session.close()
