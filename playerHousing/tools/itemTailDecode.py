"""Decodes the housing tail captured in the items table (UNK223-UNK241 and placeable) and validates it.

Input: a tab-separated export of `SELECT id, Name, UNK223 ... UNK234, placeable, UNK236 ... UNK241 FROM items WHERE UNK224<>0`
(the second result set of itemTailFull.sql; the first lists the column types, all int(11)). Every row is decoded twice,
as captured aligned and as captured shifted by one byte (the capture missed bInteractiveObject, which follows the
housepet string on the wire); exactly one decoding
must be plausible. serverPlan.md, D1, records the results; the migration does the same byte math in SQL.

Usage: python3 itemTailDecode.py itemTailFull.txt
"""
import collections, struct, sys
rows=[]
with open(sys.argv[1]) as f:
  lines=f.read().split('\n')
start=[i for i,l in enumerate(lines) if l.startswith('id\tName')][0]
cols=lines[start].split('\t')
for l in lines[start+1:]:
  if not l.strip(): continue
  p=l.split('\t'); r=dict(zip(cols,p))
  for c in cols[2:]: r[c]=int(r[c])
  r['id']=int(r['id']); rows.append(r)

u32=lambda v: v & 0xffffffff
def i32(v): v&=0xffffffff; return v-(1<<32) if v>=1<<31 else v
def f32(bits): return struct.unpack('<f',struct.pack('<I',u32(bits)))[0]

def aligned(r):
  return dict(trophy=i32(r['UNK233']), rotation=r['UNK234'], free=r['placeable'], respawn=u32(r['UNK236']),
    scale=f32(r['UNK237']), heading=f32(r['UNK238']), pitch=f32(r['UNK239']), roll=f32(r['UNK240']), interactive=r['UNK241'])

def byte(v,k): return (u32(v)>>(8*k))&0xff
def join(lo, hi): return (u32(lo)>>8) | (byte(hi,0)<<24)   # the 3 high bytes of lo, then the low byte of hi

def shifted(r):
  # On the wire, bInteractiveObject follows the housepet string (client reader 0x7f6bf0); this capture didn't know it,
  # so from there on each column holds the next field's bytes one position early, and UNK233's low byte is the flag.
  return dict(trophy=i32(join(r['UNK233'], r['UNK234'])), rotation=r['placeable'], free=byte(r['UNK236'],0),
    respawn=join(r['UNK236'], r['UNK237']), scale=f32(join(r['UNK237'], r['UNK238'])), heading=f32(join(r['UNK238'], r['UNK239'])),
    pitch=f32(join(r['UNK239'], r['UNK240'])), roll=f32(join(r['UNK240'], r['UNK241'])), interactive=byte(r['UNK233'],0))

def problems(d, r, layout):
  bad=[]
  if not (d['trophy']==-1 or 0<d['trophy']<100000): bad.append(f"trophy {d['trophy']}")
  if d['rotation'] not in (0,1): bad.append(f"rotation {d['rotation']}")
  if d['free'] not in (0,1): bad.append(f"free {d['free']}")
  if not (0.01<=d['scale']<=50): bad.append(f"scale {d['scale']:.4g}")
  for k in ('heading','pitch','roll'):
    if not (d[k]==d[k] and -1000<=d[k]<=1000): bad.append(f"{k} {d[k]:.4g}")
  if d['respawn']>100000000: bad.append(f"respawn {d['respawn']}")
  if d['interactive'] not in (0,1): bad.append(f"interactive {d['interactive']}")
  return bad

stats=collections.Counter(); ambiguous=[]; neither=[]; chosen={}
for r in rows:
  a,s=aligned(r),shifted(r)
  pa,ps=problems(a,r,'aligned'),problems(s,r,'shifted')
  if not pa and not ps: ambiguous.append((r,a,s)); stats['both plausible']+=1; chosen[r['id']]=('ambiguous',a,s)
  elif not pa: stats['aligned']+=1; chosen[r['id']]=('aligned',a,None)
  elif not ps: stats['shifted']+=1; chosen[r['id']]=('shifted',s,None)
  else: neither.append((r,pa,ps)); stats['neither']+=1
print('rows',len(rows),dict(stats))
print('--- ambiguous examples'); [print(' ',r['id'],r['Name'][:32],'| aligned',{k:(round(v,3) if isinstance(v,float) else v) for k,v in a.items()},'| shifted',{k:(round(v,3) if isinstance(v,float) else v) for k,v in s.items()}) for r,a,s in ambiguous[:4]]
print('--- neither examples'); [print(' ',r['id'],r['Name'][:32],'| aligned:',pa,'| shifted:',ps) for r,pa,ps in neither[:8]]

print('\n=== decoded tail fields (both layouts)')
dist=collections.defaultdict(collections.Counter)
byLayout=collections.defaultdict(collections.Counter)
for r in rows:
  layout,d,_=chosen[r['id']]
  for k in ('trophy','rotation','free','respawn','interactive'): dist[k][d[k]]+=1
  for k in ('scale','heading','pitch','roll'): dist[k][round(d[k],4)]+=1
  byLayout[layout][(d['rotation'],d['free'])]+=1
for k in ('trophy','rotation','free','interactive','respawn','scale','heading','pitch','roll'):
  items=dist[k].most_common(8); print(f'{k:12s} {len(dist[k]):3d} distinct:', items)
print('rotation/free by layout:', {k:dict(v) for k,v in byLayout.items()})

print('\n=== already-aligned columns')
bitmask=collections.Counter(r['UNK224'] for r in rows); print('placeable bitmask:', sorted(bitmask.items()), '| outside 0x8F:', [b for b in bitmask if b & ~0x8f and b!=-1])
for c,label in (('UNK223','subclass'),('UNK225','ignoreCollisions'),('UNK226','placementType'),('UNK227','realEstateDefinitionId'),('UNK230','upkeepId'),('UNK231','maxPerRealEstate'),('UNK232','housepet column')):
  v=collections.Counter(r[c] for r in rows); print(f'{label:24s} {len(v):3d} distinct:', sorted(v.items())[:24])
for c,label in (('UNK228','scaleMin'),('UNK229','scaleMax')):
  v=collections.Counter(round(f32(r[c]),4) for r in rows); print(f'{label:24s}', sorted(v.items()))

print('\n=== bitmask -1 row'); [print(' ',r['id'],r['Name'],{c:r[c] for c in cols[2:]}) for r in rows if r['UNK224']==-1]
print('=== trophy items (decoded id -> name)')
trophies=[(chosen[r['id']][1]['trophy'],r['id'],r['Name'],chosen[r['id']][0]) for r in rows if chosen[r['id']][1]['trophy']!=-1]
for t in sorted(trophies)[:14]: print('  ',t)
print('  ... layouts of trophy rows:', collections.Counter(t[3] for t in trophies))
