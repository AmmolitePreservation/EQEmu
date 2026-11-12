# source commandSequence.sh: runs a list of link commands in one pipe session of a client.
# sq <pid> reads lines 'command|argument|...' or 'sleep|seconds' from standard input.
toolsFolder="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
sq(){ ( cd /c/Onyx/git/github/AmmolitePreservation/claudelink/mcp && MSYS_NO_PATHCONV=1 .venv/Scripts/python.exe "$(cygpath -w "$toolsFolder/commandSequence.py")" "$1" ); }
# confirmPress <pid> <window> <button>: press a button, show the confirmation's text, answer Yes, read the Items window counts
confirmPress(){ sed "s/@W@/$2/; s/@B@/$3/" "$toolsFolder/confirmPress.txt" | sq $1 2>&1 | sed 's/"address": "[0-9a-fx]*", //; s/, "className": ""//'; }
# placeRow <pid> <list row> <nudge y> <nudge x> <nudge z>: place a stored row through the Items window
placeRow(){ sed "s/@ROW@/$2/g; s/@NY@/$3/; s/@NX@/$4/; s/@NZ@/$5/" "$toolsFolder/placeRow.txt" | sq $1 2>&1 | cut -c1-330; }
