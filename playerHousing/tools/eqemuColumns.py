"""The columns of an EQEmu table, read from its generated base repository, for SQL that copies rows column by column."""
import os, re

here = os.path.dirname(os.path.abspath(__file__))


def columnsOf(table):
  """Every column but the leading id, in table order, from common/repositories/base/base_<table>_repository.h."""
  path = os.path.join(here, '..', '..', 'common', 'repositories', 'base', f'base_{table}_repository.h')
  text = open(path, encoding='utf-8').read()
  body = text[text.index('static std::vector<std::string> Columns()'):]
  columns = re.findall(r'"([a-z_0-9]+)"', body[:body.index('};')])
  if not columns or columns[0] != 'id':
    raise ValueError(f'{table}: the repository does not lead with id: {columns[:3]}')
  return columns[1:]
