"""Checks every base repository's hand-kept indexes: each field read from row[N] and each UpdateOne write to columns[N] names
the column at N in Columns(). Housing extends EQEmu's tables by hand-editing these generated files; an index left at an old
position silently reads or overwrites another column (character_data's deleted_at, guild_members' online).

Usage: python testRepositoryIndexes.py <repositories/base folder>
"""
import pathlib
import re
import sys

folder = pathlib.Path(sys.argv[1])
columnsPattern = re.compile(r'static std::vector<std::string> Columns\(\)\s*\{\s*return \{(.*?)\};', re.S)
readPattern = re.compile(r'e\.(\w+)\s*=\s*[^;]*?row\[(\d+)\]')
writePattern = re.compile(r'columns\[(\d+)\] \+ " = [^;]*?e\.(\w+)')


def columnField(column):
  # Keywords are kept as backticked columns and trailing-underscore fields (`class` / class_).
  name = column.strip('`')
  return name + '_' if column.startswith('`') else name


problems = []
checked = 0
for path in sorted(folder.glob('base_*_repository.h')):
  text = path.read_text(encoding='utf-8')
  found = columnsPattern.search(text)
  if not found:
    problems.append(f'{path.name}: no Columns() list')
    continue
  fields = [columnField(column) for column in re.findall(r'"([^"]+)"', found.group(1))]
  for field, index in readPattern.findall(text):
    index = int(index)
    if index >= len(fields) or fields[index] != field:
      problems.append(f'{path.name}: {field} is read from row[{index}], which is {fields[index] if index < len(fields) else "past the end"}')
  for index, field in writePattern.findall(text):
    index = int(index)
    if index >= len(fields) or fields[index] != field:
      problems.append(f'{path.name}: {field} is written to columns[{index}], which is {fields[index] if index < len(fields) else "past the end"}')
  checked += 1

if checked == 0:
  raise SystemExit(f'no base repositories under {folder}')
if problems:
  raise SystemExit('\n'.join(problems))
print(f'{checked} repositories: every row read and column write names its own column')
