"""Writes sql/housingZones.sql: zone rows for the 24 housing interior and guild hall zones and the Standard Guild Hall's
interior, guildhall at version 2 (Live's guildhall_int; PEQ uses versions 0 and 1). PEQ already carries the 24 (Peridot does),
so the file adds only the rows a server lacks, each insert guarded by its own NOT EXISTS, and refuses a row that clashes with
one of them: the id and version under another short name, or the short name and version under another id. The rows a server
has keep their columns here and get Live's values from housingZoneSettings.sql.

Ids, short names and titles are the RoF2 client's zone table (robustness.md); version 2 of guildhall keeps its version 0
title. Every other column of an added row is copied: the expansion gates from the neighborhood's row, and the rest from the
classic guild hall's, an indoor zone; bypass_expansion_check is set, because EQEmu's zoning check refuses a zone newer than the
server's current expansion unless its row bypasses it (a PoP-era server, housing needs it on every housing zone, the
neighborhood included). Empty file_name and map_file_name make EQEmu use each zone's own short name.

Header values are Live's, from a livedumps clone's master/fields/zoneHeaders.tsv (ZIN-9): the safe point (EQEmu's x and y),
the floor as underworld, the ceiling, the filter id, the mana stone flag, the zone type, clip planes, fog and no binding. A zone Live never sent keeps its safe point derived
from the client's zone files by housingZoneGeometry.py and an underworld below any floor, and takes the other header values
of Live's house interiors. Every interior and hall is kept running for Live's discard delay once empty (INT-13, INT-14).
Writes the rows (housingZones.sql) and, for rows already added, the same values with the bypass, and the neighborhood's
header values and bypass (housingZoneSettings.sql).

Usage: python makeHousingZonesSql.py --live <livedumps clone>
"""
import argparse, csv, os, re, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from eqemuColumns import columnsOf  # noqa: E402
# (id, version, short name, title or None for the copied row's own, Live's header zone or None, safe point (x, y, z, heading)
# for a zone Live never sent, from housingZoneGeometry.py)
housingZones = [
  (714, 0, 'phinterior3a1', 'House Interior', 'phinterior3a1', None),
  (715, 0, 'phinterior1a1', 'House Interior', 'phinterior1a1', None),
  (716, 0, 'phinterior3a2', 'House Interior', 'phinterior3a2', None),
  (717, 0, 'phinterior3a3', 'House Interior', 'phinterior3a3', None),
  (718, 0, 'phinterior1a2', 'House Interior', 'phinterior1a2', None),
  (719, 0, 'phinterior1a3', 'House Interior', 'phinterior1a3', None),
  (720, 0, 'phinterior1b1', 'Dragon House Interior', None, (-12.8, -23.4, 3.0, 0.0)),
  (723, 0, 'phinterior1d1', 'Dragon House Interior', 'phinterior1d1', None),
  (737, 0, 'guildhalllrg', 'Grand Guild Hall', 'guildhalllrg_int', None),
  (738, 0, 'guildhallsml', 'Greater Guild Hall', 'guildhallsml_int', None),
  (739, 0, 'plhogrinteriors1a1', 'One Bedroom House Interior', None, (-1.6, 2.9, -1.4, 0.0)),
  (740, 0, 'plhogrinteriors1a2', 'One Bedroom House Interior', None, (-1.7, 3.9, -2.4, 0.0)),
  (741, 0, 'plhogrinteriors3a1', 'Three Bedroom House Interior', None, (-0.1, 2.1, -2.1, 0.0)),
  (742, 0, 'plhogrinteriors3a2', 'Three Bedroom House Interior', None, (0.1, -0.4, -1.6, 0.0)),
  (743, 0, 'plhogrinteriors3b1', 'Three Bedroom House Interior', None, (1.1, -0.9, -0.6, 0.0)),
  (744, 0, 'plhogrinteriors3b2', 'Three Bedroom House Interior', None, (1.5, -0.5, -1.1, 0.0)),
  (745, 0, 'plhdkeinteriors1a1', 'One Bedroom House Interior', None, (0.0, 2.0, 3.0, 0.0)),
  (746, 0, 'plhdkeinteriors1a2', 'One Bedroom House Interior', None, (0.0, 2.0, 3.0, 0.0)),
  (747, 0, 'plhdkeinteriors1a3', 'One Bedroom House Interior', None, (0.0, 2.0, 3.0, 0.0)),
  (748, 0, 'plhdkeinteriors3a1', 'Three Bedroom House Interior', None, (0.0, 2.0, 3.0, 0.0)),
  (749, 0, 'plhdkeinteriors3a2', 'Three Bedroom House Interior', None, (0.0, 2.0, 3.0, 0.0)),
  (750, 0, 'plhdkeinteriors3a3', 'Three Bedroom House Interior', None, (0.0, 2.0, 3.0, 0.0)),
  (751, 0, 'guildhall3', 'Modest Guild Hall', 'guildhall3_int', None),
  (766, 0, 'phinteriortree', "Evantil's Abode", 'phinteriortree', None),
  (345, 2, 'guildhall', None, 'guildhall_int', None),
]
houseInteriorHeader = 'phinterior3a1'
liveDesignIds = {737: 2935, 738: 2936, 345: 2937, 751: 2943}
uncapturedUnderworld = -2000.0
discardDelayMilliseconds = 32 * 60 * 1000
gateColumns = ['min_expansion', 'max_expansion', 'content_flags', 'content_flags_disabled', 'expansion']
fogColumns = [('FogRed', ['fog_red', 'fog_red2', 'fog_red3', 'fog_red4']), ('FogGreen', ['fog_green', 'fog_green2', 'fog_green3', 'fog_green4']),
              ('FogBlue', ['fog_blue', 'fog_blue2', 'fog_blue3', 'fog_blue4']), ('FogStart', ['fog_minclip', 'fog_minclip2', 'fog_minclip3', 'fog_minclip4']),
              ('FogEnd', ['fog_maxclip', 'fog_maxclip2', 'fog_maxclip3', 'fog_maxclip4'])]


def zoneColumns():
  columns = columnsOf('zone')
  needed = ['zoneidnumber', 'version', 'short_name', 'long_name', 'file_name', 'map_file_name', 'note', 'underworld', 'safe_x', 'safe_y', 'safe_z',
            'safe_heading', 'ztype', 'minclip', 'maxclip', 'fog_density', 'canbind', 'shutdowndelay', 'bypass_expansion_check', 'ceiling',
            'filter_id', 'disallow_mana_stone'] + gateColumns
  needed += [column for _, names in fogColumns for column in names]
  if any(column not in columns for column in needed):
    raise ValueError(f'unexpected zone columns: {columns}')
  return columns


def number(value):
  parsed = float(value)
  return repr(int(parsed)) if parsed == int(parsed) else repr(parsed)


def headerValues(header, geometry):
  if header['bNoBind'] != '1':
    raise ValueError(f"{header['zone']}: Live lets players bind there, which no housing zone does")
  values = dict(ztype=str(int(header['ZoneType'])), minclip=number(header['MinClip']), maxclip=number(header['MaxClip']),
                fog_density=number(header['FogDensity']), canbind='0', ceiling=number(header['Ceiling']), filter_id=str(int(header['FilterID'])),
                disallow_mana_stone=str(int(header['bDisallowManaStone'])))
  for field, names in fogColumns:
    parts = header[field].split(',')
    if len(parts) != len(names):
      raise ValueError(f"{header['zone']}: {field} {header[field]!r} is not {len(names)} values")
    values.update(zip(names, (number(part) for part in parts)))
  if geometry is None:
    values.update(safe_x=number(header['SafeXLoc']), safe_y=number(header['SafeYLoc']), safe_z=number(header['SafeZLoc']),
                  safe_heading=number(header['SafeHeading']), underworld=number(header['Floor']))
  else:
    values.update(safe_x=number(geometry[0]), safe_y=number(geometry[1]), safe_z=number(geometry[2]), safe_heading=number(geometry[3]),
                  underworld=number(uncapturedUnderworld))
  if not 0 <= float(values['safe_heading']) < 512:
    raise ValueError(f"{header['zone']}: heading {values['safe_heading']} is outside [0, 512)")
  return values


def insertStatement(columns, zoneId, version, shortName, longName, values):
  quoted = lambda text: "'" + text.replace("'", "''") + "'"
  row = dict(zoneidnumber=str(zoneId), version=str(version), short_name=quoted(shortName), file_name="''", map_file_name="''",
             note=quoted('Player housing'), bypass_expansion_check='1', **values)
  if longName is not None:
    row['long_name'] = quoted(longName)
  row.update({column: f'gate.`{column}`' for column in gateColumns})
  selected = ', '.join(row.get(column, f'template.`{column}`') for column in columns)
  return (f"  INSERT INTO zone ({', '.join(f'`{column}`' for column in columns)})\n"
          f"    SELECT {selected} FROM zone template JOIN zone gate ON gate.id = gateId WHERE template.id = templateId\n"
          f"      AND NOT EXISTS (SELECT 1 FROM zone existing WHERE existing.zoneidnumber = {zoneId} AND existing.version = {version} AND existing.short_name = {quoted(shortName)});\n")


def clashCondition(zoneId, version, shortName):
  return (f"(zoneidnumber = {zoneId} AND version = {version} AND short_name <> '{shortName}') OR "
          f"(short_name = '{shortName}' AND version = {version} AND zoneidnumber <> {zoneId})")


def updateStatement(values, condition):
  return f"  UPDATE zone SET {', '.join(f'{column} = {value}' for column, value in values.items())} WHERE {condition};\n"


def main():
  parser = argparse.ArgumentParser(description='Write sql/housingZones.sql and sql/housingZoneSettings.sql.')
  parser.add_argument('--live', required=True, help='a livedumps clone')
  live = parser.parse_args().live
  headers = {row['zone']: row for row in csv.DictReader(open(os.path.join(live, 'master', 'fields', 'zoneHeaders.tsv'), encoding='latin-1'), delimiter='\t')}
  columns = zoneColumns()
  if len({(zoneId, version) for zoneId, version, *_ in housingZones}) != len(housingZones):
    raise ValueError('a housing zone row repeats')
  rows = []
  for zoneId, version, shortName, longName, liveZone, geometry in housingZones:
    if (liveZone is None) == (geometry is None):
      raise ValueError(f'{shortName}: needs either Live\'s header or a derived safe point')
    header = headers[liveZone or houseInteriorHeader]
    if liveZone is not None and int(header['ZoneID']) != liveDesignIds.get(zoneId, zoneId):
      raise ValueError(f"{shortName}: Live's {liveZone} header is zone {header['ZoneID']}")
    values = headerValues(header, geometry)
    values['shutdowndelay'] = str(discardDelayMilliseconds)
    rows.append((zoneId, version, shortName, longName, values))
  neighborhoodValues = headerValues(headers['neighborhood'], None)

  rowCondition = lambda zoneId, version, shortName: f"zoneidnumber = {zoneId} AND version = {version} AND short_name = '{shortName}'"
  anyRow = ' OR '.join(f'({rowCondition(zoneId, version, shortName)})' for zoneId, version, shortName, *_ in rows)
  clashes = ' OR '.join(clashCondition(zoneId, version, shortName) for zoneId, version, shortName, *_ in rows)
  inserts = ''.join(insertStatement(columns, *row) for row in rows)
  sql = f"""-- Generated by playerHousing/tools/makeHousingZonesSql.py: the housing interior and hall zone rows a server lacks (PEQ has all but
-- guildhall version 2); the rows it has are left to housingZoneSettings.sql.
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE templateId INT DEFAULT NULL;
  DECLARE gateId INT DEFAULT NULL;
  IF EXISTS (SELECT 1 FROM zone WHERE {clashes}) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'a zone row clashes with a housing interior or hall row: its id and version under another short name, or its short name and version under another id';
  END IF;
  SELECT id INTO templateId FROM zone WHERE short_name = 'guildhall' AND version = 0 ORDER BY id LIMIT 1;
  IF templateId IS NULL THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'no guildhall zone row to copy';
  END IF;
  SELECT id INTO gateId FROM zone WHERE short_name = 'neighborhood' AND version = 0 ORDER BY id LIMIT 1;
  IF gateId IS NULL THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'no neighborhood zone row for the expansion gates';
  END IF;
{inserts}END //
DELIMITER ;
SELECT zoneidnumber, version, short_name, long_name, min_expansion, max_expansion, underworld, ceiling, filter_id, disallow_mana_stone, ztype, canbind, shutdowndelay FROM zone WHERE {anyRow} ORDER BY zoneidnumber, version;
"""
  path = os.path.join(here, '..', 'sql', 'housingZones.sql')
  open(path, 'w', encoding='ascii').write(sql)
  updates = ''.join(updateStatement(dict(values, bypass_expansion_check='1'), rowCondition(zoneId, version, shortName)) for zoneId, version, shortName, _, values in rows)
  updates += updateStatement(dict(neighborhoodValues, bypass_expansion_check='1'), "short_name = 'neighborhood' AND version = 0")
  update = f"""-- Generated by playerHousing/tools/makeHousingZonesSql.py: Live's header values, the discard delay and the expansion bypass
-- for the rows housingZones.sql added, and the neighborhood's header values and bypass.
DELIMITER //
BEGIN NOT ATOMIC
  IF (SELECT COUNT(*) FROM zone WHERE {anyRow}) <> {len(rows)} THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'not every housing interior and hall zone row is present; run housingZones.sql';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM zone WHERE short_name = 'neighborhood' AND version = 0) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'no neighborhood zone row';
  END IF;
{updates}END //
DELIMITER ;
SELECT zoneidnumber, version, short_name, safe_x, safe_y, safe_z, safe_heading, underworld, ceiling, filter_id, disallow_mana_stone, ztype, maxclip, canbind, shutdowndelay, expansion, bypass_expansion_check FROM zone WHERE (short_name = 'neighborhood' AND version = 0) OR {anyRow} ORDER BY zoneidnumber, version;
"""
  updatePath = os.path.join(here, '..', 'sql', 'housingZoneSettings.sql')
  open(updatePath, 'w', encoding='ascii').write(update)
  print(f"wrote {os.path.relpath(path)}: {len(rows)} zone rows, {len(columns)} columns each; {os.path.relpath(updatePath)}: their header values, the bypass and the neighborhood's")


if __name__ == '__main__':
  main()
