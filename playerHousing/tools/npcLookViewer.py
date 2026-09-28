#!/usr/bin/env python3
"""Local viewer for pairing NPC items with captured looks (content/npcItemsToAuthor.md, research/live/master).

Serves a page on 127.0.0.1 and opens it: each worksheet item beside the captured looks most like it, with model images from an
eq-asset-preview clone (assets/npc_models/CTN_<race>_<gender>_<texture>_<helm>.png). Each assignment is written at once to
content/npcLookAssignments.tsv.
Usage: python npcLookViewer.py --assets <eq-asset-preview clone> [--port 8765]
"""
import argparse, csv, json, os, re, threading, time, webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

here = os.path.dirname(os.path.abspath(__file__))
housing = os.path.join(here, '..')
master = os.path.join(housing, 'research', 'live', 'master')
assignmentsPath = os.path.join(housing, 'content', 'npcLookAssignments.tsv')
assignmentColumns = ['itemId', 'itemName', 'decision', 'candidate', 'race', 'gender', 'texture', 'helm', 'height', 'actorDef', 'note', 'updated']
ownerNamed = re.compile(r"`s (familiar|mount|warder|pet)$", re.I)
stopWords = {'a', 'an', 'the', 'of', 'and', 'familiar', 'pet', 'mount', 'house', 'ngh', 'cub', 'small', 'tiny'}
modelImage = re.compile(r'CTN_(\d+)_(\d+)_(\d+)_(\d+)\.png$')


def readTsv(name):
  with open(os.path.join(master, name), encoding='latin-1', newline='') as source:
    return list(csv.DictReader(source, delimiter='\t'))


def words(*texts):
  found = set()
  for text in texts:
    spaced = re.sub(r'([a-z])([A-Z])', r'\1 \2', text or '')
    found.update(w for w in re.split(r'[^a-z0-9]+', spaced.lower()) if len(w) > 2 and w not in stopWords and not w.isdigit())
  return found


def readRaceNames():
  text = open(os.path.join(housing, '..', 'common', 'races.h'), encoding='latin-1').read()
  block = re.search(r'namespace Race\s*\{(.*?)\n\}', text, re.S)
  if not block:
    raise ValueError('common/races.h has no Race namespace')
  names = {}
  for name, raceId in re.findall(r'constexpr uint16\s+(\w+)\s*=\s*(\d+);', block.group(1)):
    names.setdefault(int(raceId), re.sub(r'([a-z])([A-Z0-9])', r'\1 \2', name))
  return names


def readWorksheet():
  items, section, header = [], None, None
  for line in open(os.path.join(housing, 'content', 'npcItemsToAuthor.md'), encoding='utf-8'):
    line = line.rstrip('\n')
    if line.startswith('#'):
      title = line.lstrip('#').strip()
      section, header = (title if title[:2] in ('1.', '2.', '3.') else None), None
      continue
    if not section or not line.startswith('|') or set(line) <= set('|-: '):
      continue
    cells = [cell.strip() for cell in line.strip('|').split('|')]
    if header is None:
      header = cells
      continue
    row = dict(zip(header, cells))
    items.append({'itemId': re.sub(r'\D', '', row['Item']), 'name': row['Item name'], 'section': section,
                  'expected': ' '.join(f'{k}: {v}' for k, v in row.items() if k not in ('Item', 'Item name') and v).replace('`', '')})
  return items


def readAssignments():
  if not os.path.exists(assignmentsPath):
    return {}
  with open(assignmentsPath, encoding='utf-8', newline='') as source:
    return {row['itemId']: row for row in csv.DictReader(source, delimiter='\t')}


def writeAssignments(assignments):
  temporary = assignmentsPath + '.tmp'
  with open(temporary, 'w', encoding='utf-8', newline='') as out:
    writer = csv.DictWriter(out, fieldnames=assignmentColumns, delimiter='\t', lineterminator='\n')
    writer.writeheader()
    for itemId in sorted(assignments, key=int):
      writer.writerow(assignments[itemId])
  os.replace(temporary, assignmentsPath)


def buildData():
  raceNames = readRaceNames()
  npcItems = {row['itemId']: row for row in readTsv('npcItems.tsv')}
  items = {row['itemId']: row for row in readTsv('items.tsv')}
  first = lambda value: value.split(' | ')[0] if value else ''
  candidates = []
  for look in readTsv('npcLooks.tsv'):
    candidates.append({'key': 'look:' + look['npcName'], 'label': look['displayedName'], 'npcName': look['npcName'], 'kind': 'placed NPC item',
                       'race': first(look['race']), 'gender': first(look['gender']), 'height': first(look['height']), 'level': look['level'],
                       'actorDef': look['actorDef'], 'textureType': look['textureType'], 'material': look['material'], 'headType': look['headType'],
                       'seen': look['zones']})
  for spawn in readTsv('spawns.tsv'):
    if ownerNamed.search(spawn['displayedName']):
      candidates.append({'key': f"spawn:{spawn['zone']}/{spawn['name']}", 'label': spawn['displayedName'], 'npcName': spawn['name'], 'kind': 'summoned',
                         'race': first(spawn['race']), 'gender': first(spawn['gender']), 'height': first(spawn['height']), 'level': spawn['level'],
                         'actorDef': spawn['actorDef'], 'textureType': spawn['textureType'], 'material': spawn['material'],
                         'headType': spawn['headType'], 'seen': spawn['zone']})
  linked = {}
  for itemId, row in items.items():
    for npcName in filter(None, row.get('npcNames', '').split(' | ')):
      linked.setdefault(npcName, []).append(f"{itemId} {row['name']}")
  for candidate in candidates:
    race = int(candidate['race']) if candidate['race'] else -1
    candidate['raceName'] = raceNames.get(race, f'race {race} (not in EQEmu)')
    candidate['linkedItems'] = linked.get(candidate['npcName'], [])
    candidate['words'] = sorted(words(candidate['label'], candidate['npcName'], candidate['raceName']))
  worksheet = readWorksheet()
  for item in worksheet:
    known = npcItems.get(item['itemId'], {})
    item['housepet'] = known.get('housepetFileName') or items.get(item['itemId'], {}).get('housepetFileName', '')
    item['lookSource'] = known.get('lookSource', '')
    item['words'] = sorted(words(item['name'], item['housepet'], item['expected']))
  return {'items': worksheet, 'candidates': candidates, 'raceNames': {str(k): v for k, v in raceNames.items()}}


class Viewer(BaseHTTPRequestHandler):
  data = None
  models = {}
  assetsRoot = ''
  lock = threading.Lock()

  def log_message(self, *arguments):
    pass

  def reply(self, status, body, contentType):
    self.send_response(status)
    self.send_header('Content-Type', contentType)
    self.send_header('Content-Length', str(len(body)))
    self.end_headers()
    self.wfile.write(body)

  def do_GET(self):
    url = urlparse(self.path)
    query = {key: values[0] for key, values in parse_qs(url.query).items()}
    if url.path == '/':
      self.reply(200, page.encode('utf-8'), 'text/html; charset=utf-8')
    elif url.path == '/data':
      with self.lock:
        body = dict(self.data, assignments=readAssignments())
      self.reply(200, json.dumps(body).encode('utf-8'), 'application/json')
    elif url.path == '/variants':
      race, gender = query.get('race', ''), query.get('gender', '')
      variants = sorted(v for v in self.models.get(race, []) if v[0] == gender) or sorted(self.models.get(race, []))
      self.reply(200, json.dumps(variants).encode('utf-8'), 'application/json')
    elif url.path == '/image':
      name = f"CTN_{query.get('race')}_{query.get('gender')}_{query.get('texture')}_{query.get('helm')}.png"
      path = os.path.join(self.assetsRoot, name)
      if not modelImage.match(name) or not os.path.exists(path):
        self.reply(404, b'no image', 'text/plain')
        return
      self.reply(200, open(path, 'rb').read(), 'image/png')
    else:
      self.reply(404, b'not found', 'text/plain')

  def do_POST(self):
    body = json.loads(self.rfile.read(int(self.headers['Content-Length'])) or b'{}')
    itemId = str(body.get('itemId', ''))
    item = next((i for i in self.data['items'] if i['itemId'] == itemId), None)
    if item is None:
      self.reply(400, b'unknown item', 'text/plain')
      return
    with self.lock:
      assignments = readAssignments()
      if self.path == '/clear':
        assignments.pop(itemId, None)
      elif self.path == '/assign':
        if body.get('decision') not in ('look', 'needsCapture'):
          self.reply(400, b'decision must be look or needsCapture', 'text/plain')
          return
        row = {column: str(body.get(column, '')) for column in assignmentColumns}
        row.update(itemId=itemId, itemName=item['name'], updated=time.strftime('%Y-%m-%d %H:%M'))
        assignments[itemId] = row
      else:
        self.reply(404, b'not found', 'text/plain')
        return
      writeAssignments(assignments)
    self.reply(200, b'{}', 'application/json')


page = r'''<!doctype html><html><head><meta charset="utf-8"><title>NPC look viewer</title><style>
body{font:13px system-ui,sans-serif;margin:0;display:flex;height:100vh;background:#1c1d21;color:#ddd}
#items{width:330px;overflow:auto;border-right:1px solid #333;padding:8px}#detail{flex:1;overflow:auto;padding:12px}
.item{padding:5px 6px;border-radius:4px;cursor:pointer;margin:1px 0}.item:hover{background:#2a2c33}.item.on{background:#34405a}
.badge{font-size:11px;padding:1px 5px;border-radius:3px;margin-left:4px}.complete{background:#2d5a36}.assigned{background:#2d4f7a}
.needsCapture{background:#6a5a22}.missing{background:#5a2d2d}.grid{display:flex;flex-wrap:wrap;gap:8px}
.card{width:200px;background:#26282e;border-radius:6px;padding:6px}.card img{width:188px;height:188px;object-fit:contain;background:#111}
.noimg{width:188px;height:188px;display:flex;align-items:center;justify-content:center;background:#111;color:#777;text-align:center}
button{background:#3a4a6a;color:#eee;border:0;border-radius:4px;padding:3px 8px;cursor:pointer;margin:2px}
input{background:#26282e;color:#eee;border:1px solid #444;border-radius:4px;padding:4px;width:95%}h3{margin:14px 0 6px}.dim{color:#999}
.warn{color:#e0b050}
#filters label{display:block;margin:1px 0}#filters b{display:block;margin-top:6px}
</style></head><body><div id="items"><div id="filters"></div>
<input id="itemFilter" placeholder="filter items"><div id="itemList"></div></div><div id="detail">Pick an item.</div><script>
let data, current = null, sections = [];
const statuses = ['missing', 'needsCapture', 'assigned', 'complete'];
let filters = JSON.parse(localStorage.getItem('npcLookViewerFilters') || 'null') || {sections: {}, statuses: {missing: true, needsCapture: true, assigned: false, complete: false}};
function setFilter(group, key, on) { filters[group][key] = on; localStorage.setItem('npcLookViewerFilters', JSON.stringify(filters)); renderFilters(); renderItems(); }
function renderFilters() {
  sections = [...new Set(data.items.map(i => i.section))];
  sections.forEach(s => { if (!(s in filters.sections)) filters.sections[s] = true; });
  const count = (test) => data.items.filter(test).length;
  const box = (group, key, label, index) => `<label><input type="checkbox" style="width:auto" ${filters[group][key] ? 'checked' : ''}
    onchange="setFilter('${group}', ${group === 'sections' ? `sections[${index}]` : `'${key}'`}, this.checked)"> ${esc(label)}</label>`;
  document.getElementById('filters').innerHTML = '<b>Sections</b>' + sections.map((s, index) => box('sections', s,
      `${s.split(' (')[0]} (${count(i => i.section === s && status(i) === 'missing')} missing of ${count(i => i.section === s)})`, index)).join('')
    + '<b>Status</b>' + statuses.map(st => box('statuses', st, `${st} (${count(i => status(i) === st)})`)).join('');
}
const esc = s => String(s ?? '').replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
function status(item) {
  const a = data.assignments[item.itemId];
  if (item.lookSource) return 'complete';
  return a ? a.decision === 'look' ? 'assigned' : 'needsCapture' : 'missing';
}
async function load() { data = await (await fetch('/data')).json(); renderFilters(); renderItems(); if (current) show(current); }
function renderItems() {
  const filter = document.getElementById('itemFilter').value.toLowerCase();
  document.getElementById('itemList').innerHTML = data.items.filter(i => filters.sections[i.section] && filters.statuses[status(i)]
    && (i.name + ' ' + i.itemId).toLowerCase().includes(filter)).map(i => `<div class="item ${current === i.itemId ? 'on' : ''}"
    onclick="show('${i.itemId}')">${esc(i.itemId)} ${esc(i.name)}<span class="badge ${status(i)}">${status(i)}</span>
    <div class="dim">${esc(i.section.split(' (')[0])}</div></div>`).join('');
}
function score(item, c) { return c.words.filter(w => item.words.includes(w)).length * 10 + (c.npcName.toLowerCase() === item.housepet.toLowerCase() ? 100 : 0); }
function card(c, index) {
  return `<div class="card"><div id="img${index}" class="noimg">loading</div><div id="var${index}" class="dim"></div>
    <b>${esc(c.label)}</b><div>${esc(c.raceName)} (${esc(c.race)}), gender ${esc(c.gender)}, height ${esc(c.height)}</div>
    <div class="dim">${esc(c.kind)} in ${esc(c.seen)}; ${esc(c.actorDef)}; texture ${esc(c.textureType)} material ${esc(c.material)} head ${esc(c.headType)}</div>
    ${c.linkedItems.length ? `<div class="dim">linked: ${esc(c.linkedItems.join(', '))}</div>` : ''}
    <button onclick="assign(${index})">Assign</button> <button onclick="step(${index},-1)">&lt;</button><button onclick="step(${index},1)">&gt;</button></div>`;
}
let shown = [], variants = {}, variantIndex = {};
async function show(itemId) {
  current = itemId; renderItems();
  const item = data.items.find(i => i.itemId === itemId), a = data.assignments[itemId];
  const query = document.getElementById('candidateFilter')?.value || '';
  shown = data.candidates.map(c => [score(item, c), c]).filter(([s, c]) => query ? (c.label + ' ' + c.npcName + ' ' + c.raceName).toLowerCase().includes(query.toLowerCase()) : s > 0)
    .sort((x, y) => y[0] - x[0]).slice(0, 60).map(([, c]) => c);
  document.getElementById('detail').innerHTML = `<h2>${esc(item.itemId)} ${esc(item.name)}</h2>
    <div>model name <b>${esc(item.housepet || 'unknown')}</b>; ${esc(item.expected)}</div>
    <div>status <span class="badge ${status(item)}">${status(item)}</span> ${a ? `${esc(a.decision)} ${esc(a.candidate)} race ${esc(a.race)} texture ${esc(a.texture)} helm ${esc(a.helm)} (${esc(a.updated)})` : ''}</div>
    <button onclick="needsCapture()">Needs capture</button><button onclick="clearItem()">Clear</button>
    <h3>${query ? 'Matching' : 'Suggested'} looks (${shown.length})</h3><input id="candidateFilter" placeholder="search every look by name or race, Enter to apply" value="${esc(query)}"
    onkeydown="if(event.key==='Enter')show(current)"><div class="grid">${shown.map(card).join('')}</div>`;
  shown.forEach(async (c, index) => {
    const list = variants[index] = await (await fetch(`/variants?race=${c.race}&gender=${c.gender}`)).json();
    const texture = captured(c.textureType), head = captured(c.headType);
    let pick = list.findIndex(v => v[1] === texture && v[2] === head);
    if (pick < 0) pick = list.findIndex(v => v[1] === texture);
    variantIndex[index] = Math.max(pick, 0);
    paint(index);
  });
}
const captured = value => String(value ?? '').split(' | ')[0];
function paint(index) {
  const list = variants[index] || [], box = document.getElementById('img' + index), label = document.getElementById('var' + index);
  if (!list.length) { box.outerHTML = `<div id="img${index}" class="noimg">no image for this race</div>`; label.textContent = ''; return; }
  const [gender, texture, helm] = list[variantIndex[index]], wanted = captured(shown[index].textureType);
  box.outerHTML = `<img id="img${index}" src="/image?race=${shown[index].race}&gender=${gender}&texture=${texture}&helm=${helm}">`;
  const hasWanted = list.some(v => v[1] === wanted);
  label.innerHTML = `image ${variantIndex[index] + 1} of ${list.length}: gender ${gender}, texture ${texture}, helm ${helm}`
    + (hasWanted ? '' : ` <span class="warn">no render for captured texture ${esc(wanted)}</span>`);
}
function step(index, delta) { const n = (variants[index] || []).length; if (n) { variantIndex[index] = (variantIndex[index] + delta + n) % n; paint(index); } }
async function post(path, body) { const r = await fetch(path, {method: 'POST', body: JSON.stringify(body)}); if (!r.ok) alert(await r.text()); await load(); }
function assign(index) {
  const c = shown[index], list = variants[index] || [], v = list[variantIndex[index]] || [c.gender, '', ''];
  post('/assign', {itemId: current, decision: 'look', candidate: c.key, race: c.race, gender: v[0], texture: v[1], helm: v[2], height: c.height, actorDef: c.actorDef});
}
function needsCapture() { post('/assign', {itemId: current, decision: 'needsCapture'}); }
function clearItem() { post('/clear', {itemId: current}); }
document.getElementById('itemFilter').oninput = renderItems; load();
</script></body></html>'''


def main():
  parser = argparse.ArgumentParser(description='Pair NPC items with captured looks.')
  parser.add_argument('--assets', required=True, help='an eq-asset-preview clone')
  parser.add_argument('--port', type=int, default=8765)
  arguments = parser.parse_args()
  Viewer.assetsRoot = os.path.join(arguments.assets, 'assets', 'npc_models')
  if not os.path.isdir(Viewer.assetsRoot):
    raise SystemExit(f'{Viewer.assetsRoot} does not exist: --assets must be an eq-asset-preview clone')
  for name in os.listdir(Viewer.assetsRoot):
    match = modelImage.match(name)
    if match:
      race, gender, texture, helm = match.groups()
      Viewer.models.setdefault(race, []).append((gender, texture, helm))
  Viewer.data = buildData()
  server = ThreadingHTTPServer(('127.0.0.1', arguments.port), Viewer)
  address = f'http://127.0.0.1:{arguments.port}/'
  print(f"{len(Viewer.data['items'])} items, {len(Viewer.data['candidates'])} looks, {sum(map(len, Viewer.models.values()))} model images; serving {address} (Ctrl+C stops)")
  webbrowser.open(address)
  server.serve_forever()


if __name__ == '__main__':
  main()
