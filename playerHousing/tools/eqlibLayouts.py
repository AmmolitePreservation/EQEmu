"""Decodes raw client memory with the offset-annotated layouts in eqlib's headers (/*0x5d8*/ char HousepetFileName[32];)."""
import re
import struct

primitiveFormats = {
  'bool': '?', 'char': 'b', 'signed char': 'b', 'int8_t': 'b', 'unsigned char': 'B', 'uint8_t': 'B', 'BYTE': 'B',
  'short': 'h', 'SHORT': 'h', 'int16_t': 'h', 'unsigned short': 'H', 'uint16_t': 'H', 'WORD': 'H',
  'int': 'i', 'int32_t': 'i', 'long': 'i', 'LONG': 'i', 'unsigned int': 'I', 'uint32_t': 'I', 'DWORD': 'I', 'unsigned long': 'I', 'UINT': 'I',
  'int64_t': 'q', '__int64': 'q', 'long long': 'q', 'uint64_t': 'Q', 'unsigned __int64': 'Q', 'unsigned long long': 'Q',
  'eqtime_t': 'q', 'time_t': 'q', 'float': 'f', 'double': 'd',
}
typeDefinition = re.compile(r'\b(struct|class)\s+(?:\[\[offsetcomments\]\]\s+)?(\w+)\s*(?::\s*([^{;]+?))?\s*\{')
enumDefinition = re.compile(r'\benum\s+(?:class\s+)?(\w+)\s*(?::\s*([\w: ]+?))?\s*\{')
aliasDefinition = re.compile(r'\busing\s+(\w+)\s*=\s*([\w: <>*]+?)\s*;')
constantDefinition = re.compile(r'(?:\bconstexpr\s+(?:unsigned\s+)?\w+\s+(\w+)\s*=\s*([^;]+);|^\s*#define\s+(\w+)\s+([^\n/]+))', re.MULTILINE)
annotatedMember = re.compile(r'^\s*/\*(0x[0-9a-fA-F]+)\*/\s*(.+?)\s*;')
sizeMarker = re.compile(r'^\s*/\*(0x[0-9a-fA-F]+)\*/\s*$')
declaration = re.compile(r'^(?P<type>.+?[\s*&])(?P<name>\w+)\s*(?P<arrays>(?:\[[^\]]*\]\s*)*)$')


def blockEnd(text, openIndex):
  depth = 0
  for index in range(openIndex, len(text)):
    if text[index] == '{':
      depth += 1
    elif text[index] == '}':
      depth -= 1
      if depth == 0:
        return index
  raise ValueError('unbalanced braces')


def memberLines(body):
  # Keeps the lines of anonymous unions and structs; drops nested named types and function bodies, whose annotations belong elsewhere.
  kept = []
  index = 0
  while index < len(body):
    brace = body.find('{', index)
    if brace < 0:
      kept.append(body[index:])
      break
    kept.append(body[index:brace])
    lead = re.split(r'[;{}]', body[:brace])[-1]
    end = blockEnd(body, brace)
    anonymous = re.fullmatch(r'\s*(?:(?://[^\n]*)?\s*)*(union|struct)\s*(?://[^\n]*)?\s*', lead) is not None
    if anonymous:
      kept.append(memberLines(body[brace + 1:end]))
    index = end + 1
  return ''.join(kept)


class Layouts:
  def __init__(self, headerDirectory):
    self.types = {}
    self.enums = {}
    self.aliases = {}
    self.constants = {}
    for path in sorted(headerDirectory.rglob('*.h')):
      text = path.read_text(encoding='latin-1')
      text = re.sub(r'/\*(?!0x[0-9a-fA-F]+\*/)[\s\S]*?\*/', '', text)
      for match in enumDefinition.finditer(text):
        self.enums[match.group(1)] = primitiveFormats.get((match.group(2) or 'int').strip().split('::')[-1], 'i')
      for match in aliasDefinition.finditer(text):
        self.aliases.setdefault(match.group(1), match.group(2).strip())
      for match in constantDefinition.finditer(text):
        name, value = (match.group(1), match.group(2)) if match.group(1) else (match.group(3), match.group(4))
        self.constants.setdefault(name, value.strip())
      for match in typeDefinition.finditer(text):
        name = match.group(2)
        end = blockEnd(text, match.end() - 1)
        members = []
        size = None
        for line in memberLines(text[match.end():end]).split('\n'):
          line = line.split('//')[0]
          marker = sizeMarker.match(line)
          if marker:
            size = int(marker.group(1), 16)
            continue
          member = annotatedMember.match(line)
          if member:
            members.append((int(member.group(1), 16), member.group(2)))
        if not members and name in self.types:
          continue
        bases = [base.strip().split()[-1] for base in (match.group(3) or '').split(',') if base.strip()]
        self.types[name] = {'members': members, 'size': size, 'bases': bases}

  def resolve(self, typeName):
    seen = set()
    while typeName in self.aliases and typeName not in seen and typeName not in self.types:
      seen.add(typeName)
      typeName = self.aliases[typeName]
    return typeName

  def fields(self, typeName):
    layout = self.types[typeName]
    members = []
    for base in layout['bases']:
      if base in self.types:
        members.extend(self.fields(base))
    members.extend(layout['members'])
    return sorted(members, key=lambda member: member[0])

  def size(self, typeName):
    typeName = self.resolve(typeName)
    if typeName in primitiveFormats:
      return struct.calcsize(primitiveFormats[typeName])
    if typeName in self.enums:
      return struct.calcsize(self.enums[typeName])
    layout = self.types.get(typeName)
    return layout['size'] if layout else None

  def dimension(self, text, depth=0):
    text = text.strip()
    if re.fullmatch(r'0[xX][0-9a-fA-F]+|\d+', text):
      return int(text, 0)
    if text in self.constants and depth < 8:
      return self.dimension(self.constants[text], depth + 1)
    product = re.fullmatch(r'(.+?)\s*\*\s*(.+)', text)
    if product:
      left, right = self.dimension(product.group(1), depth + 1), self.dimension(product.group(2), depth + 1)
      return left * right if left and right else None
    return None

  def decode(self, typeName, raw, offset=0, prefix='', limit=None):
    values = {}
    members = self.fields(typeName)
    end = limit if limit is not None else len(raw)
    for index, (memberOffset, text) in enumerate(members):
      spanEnd = next((later for later, _ in members[index + 1:] if later > memberOffset), end - offset)
      self.decodeMember(values, text, raw, offset + memberOffset, offset + spanEnd, prefix)
    return values

  def decodeMember(self, values, text, raw, start, end, prefix):
    text = re.sub(r'\{[^}]*\}', '', text.split('=')[0])
    parsed = declaration.match(re.sub(r'\b(const|volatile|mutable|static)\b\s*', '', text).strip())
    if not parsed or ':' in parsed.group('name'):
      values[f'{prefix}?{start:#x}'] = raw[start:end].hex()
      return
    typeText = parsed.group('type').strip()
    name = prefix + parsed.group('name')
    if '*' in typeText or '&' in typeText:
      return
    typeName = self.resolve(' '.join(typeText.split()))
    span = raw[start:end]
    elementSize = self.size(typeName)
    count = 1
    if parsed.group('arrays'):
      dimensions = [self.dimension(dimension) for dimension in re.findall(r'\[([^\]]*)\]', parsed.group('arrays'))]
      if not elementSize:
        values[name] = span.hex()
        return
      if all(dimensions):
        count = 1
        for dimension in dimensions:
          count *= dimension
        span = raw[start:start + count * elementSize]
      else:
        count = len(span) // elementSize
    if typeName in ('char', 'signed char') and parsed.group('arrays'):
      values[name] = span.split(b'\0')[0].decode('latin-1')
      return
    fmt = primitiveFormats.get(typeName) or self.enums.get(typeName)
    if fmt:
      items = [struct.unpack_from('<' + fmt, raw, start + index * struct.calcsize(fmt))[0] for index in range(count)
        if start + (index + 1) * struct.calcsize(fmt) <= len(raw)]
      rendered = [('%g' % item) if isinstance(item, float) else str(int(item)) for item in items]
      values[name] = ','.join(rendered)
      return
    if typeName in self.types and self.types[typeName]['members'] and elementSize:
      for index in range(count):
        label = f'{name}[{index}].' if parsed.group('arrays') else f'{name}.'
        values.update(self.decode(typeName, raw, start + index * elementSize, label, start + (index + 1) * elementSize))
      return
    values[name] = span.hex()
