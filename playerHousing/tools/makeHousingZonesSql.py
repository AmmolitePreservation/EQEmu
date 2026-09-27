"""Writes sql/housingZones.sql: zone rows for the 24 housing interior and guild hall zones, which PEQ's zone table lacks.

Ids, short names and titles are the RoF2 client's zone table (robustness.md). Every other column is copied: the expansion
gates from the neighborhood's row, and the rest from the classic guild hall's, an indoor zone; bypass_expansion_check is
set, because EQEmu's zoning check refuses a zone newer than the server's current expansion unless its row bypasses it (a
PoP-era server, housing needs it on every housing zone, the neighborhood included). Empty file_name and map_file_name make EQEmu use each zone's own short name; underworld sits below any
floor. Safe points: the Palatial Guild Hall's (guildhalllrg) from Live and Evantil's Abode's (phinteriortree) chosen by
/loc on Peridot; every other one derived from the client's zone files by housingZoneGeometry.py (its entrance's entry
point). Entering a property from outside puts a player at its zone's safe point, except where entryPoints says otherwise
(no table holds those yet; the property entry handler needs them). Writes the rows (housingZones.sql) and, for rows already
added, their safe points and the bypass, with the neighborhood's bypass (housingZoneSettings.sql).

Usage: python3 makeHousingZonesSql.py
"""
import os, re

here = os.path.dirname(os.path.abspath(__file__))
# (id, short name, title, safe point (x, y, z, heading)): the Palatial's from Live with its heading by #loc on Peridot,
# Evantil's Abode's by /loc and #loc on Peridot, the rest from housingZoneGeometry.py with heading 0 (none known yet)
housingZones = [
  (714, 'phinterior3a1', 'House Interior', (-7.0, 6.2, 3.0, 0.0)),
  (715, 'phinterior1a1', 'House Interior', (-0.4, 5.4, 3.0, 0.0)),
  (716, 'phinterior3a2', 'House Interior', (-7.0, 6.2, 3.0, 0.0)),
  (717, 'phinterior3a3', 'House Interior', (-7.0, 6.2, 3.0, 0.0)),
  (718, 'phinterior1a2', 'House Interior', (-0.4, 5.4, 3.0, 0.0)),
  (719, 'phinterior1a3', 'House Interior', (-0.4, 5.4, 3.0, 0.0)),
  (720, 'phinterior1b1', 'Dragon House Interior', (-12.8, -23.4, 3.0, 0.0)),
  (723, 'phinterior1d1', 'Dragon House Interior', (-0.4, 5.4, 3.0, 0.0)),
  (737, 'guildhalllrg', 'Grand Guild Hall', (0.0, -12.0, 2.2, 384.75)),
  (738, 'guildhallsml', 'Greater Guild Hall', (133.0, -0.2, -13.9, 0.0)),
  (739, 'plhogrinteriors1a1', 'One Bedroom House Interior', (-1.6, 2.9, -1.4, 0.0)),
  (740, 'plhogrinteriors1a2', 'One Bedroom House Interior', (-1.7, 3.9, -2.4, 0.0)),
  (741, 'plhogrinteriors3a1', 'Three Bedroom House Interior', (-0.1, 2.1, -2.1, 0.0)),
  (742, 'plhogrinteriors3a2', 'Three Bedroom House Interior', (0.1, -0.4, -1.6, 0.0)),
  (743, 'plhogrinteriors3b1', 'Three Bedroom House Interior', (1.1, -0.9, -0.6, 0.0)),
  (744, 'plhogrinteriors3b2', 'Three Bedroom House Interior', (1.5, -0.5, -1.1, 0.0)),
  (745, 'plhdkeinteriors1a1', 'One Bedroom House Interior', (0.0, 2.0, 3.0, 0.0)),
  (746, 'plhdkeinteriors1a2', 'One Bedroom House Interior', (0.0, 2.0, 3.0, 0.0)),
  (747, 'plhdkeinteriors1a3', 'One Bedroom House Interior', (0.0, 2.0, 3.0, 0.0)),
  (748, 'plhdkeinteriors3a1', 'Three Bedroom House Interior', (0.0, 2.0, 3.0, 0.0)),
  (749, 'plhdkeinteriors3a2', 'Three Bedroom House Interior', (0.0, 2.0, 3.0, 0.0)),
  (750, 'plhdkeinteriors3a3', 'Three Bedroom House Interior', (0.0, 2.0, 3.0, 0.0)),
  (751, 'guildhall3', 'Modest Guild Hall', (134.6, -0.2, -13.9, 0.0)),
  (766, 'phinteriortree', "Evantil's Abode", (11.25, 79.75, 3.75, 511.25)),
]
# Entry points (x, y, z, heading) that differ from the safe point: the Palatial's from Live, Evantil's Abode's by /loc on
# Peridot; both headings by #loc on Peridot
entryPoints = {
  'guildhalllrg': (71.0, -1.0, 1.56, 384.75),
  'phinteriortree': (0.38, 3.21, 3.75, 511.25),
}
gateColumns = ['min_expansion', 'max_expansion', 'content_flags', 'content_flags_disabled', 'expansion']


def zoneColumns():
  path = os.path.join(here, '..', '..', 'common', 'repositories', 'base', 'base_zone_repository.h')
  text = open(path, encoding='utf-8').read()
  body = text[text.index('static std::vector<std::string> Columns()'):]
  columns = re.findall(r'"([a-z_0-9]+)"', body[:body.index('};')])
  needed = ['zoneidnumber', 'version', 'short_name', 'long_name', 'file_name', 'map_file_name', 'note', 'underworld',
            'safe_x', 'safe_y', 'safe_z', 'safe_heading'] + gateColumns
  if columns[0] != 'id' or any(column not in columns for column in needed):
    raise ValueError(f'unexpected zone columns: {columns}')
  return columns[1:]


def insertStatement(columns, zoneId, shortName, longName, safePoint):
  quoted = lambda text: "'" + text.replace("'", "''") + "'"
  values = dict(zoneidnumber=str(zoneId), version='0', short_name=quoted(shortName), long_name=quoted(longName),
                file_name="''", map_file_name="''", note=quoted('Player housing'), underworld='-2000',
                safe_x=repr(safePoint[0]), safe_y=repr(safePoint[1]), safe_z=repr(safePoint[2]), safe_heading=repr(safePoint[3]),
                bypass_expansion_check='1')
  values.update({column: f'gate.`{column}`' for column in gateColumns})
  selected = ', '.join(values.get(column, f'template.`{column}`') for column in columns)
  return (f"  INSERT INTO zone ({', '.join(f'`{column}`' for column in columns)})\n"
          f"    SELECT {selected} FROM zone template JOIN zone gate ON gate.id = gateId WHERE template.id = templateId;\n")


def main():
  columns = zoneColumns()
  unknown = set(entryPoints) - {shortName for _, shortName, _, _ in housingZones}
  if unknown:
    raise ValueError(f'entry points for zones not in housingZones: {sorted(unknown)}')
  points = [(shortName, point) for _, shortName, _, point in housingZones] + list(entryPoints.items())
  badHeadings = [(shortName, point) for shortName, point in points if len(point) != 4 or not 0 <= point[3] < 512]
  if badHeadings:
    raise ValueError(f'points need (x, y, z, heading) with a heading in [0, 512): {badHeadings}')
  ids = ', '.join(str(zoneId) for zoneId, _, _, _ in housingZones)
  names = ', '.join(f"'{shortName}'" for _, shortName, _, _ in housingZones)
  inserts = ''.join(insertStatement(columns, *zone) for zone in housingZones)
  sql = f"""-- Generated by playerHousing/tools/makeHousingZonesSql.py.
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE templateId INT DEFAULT NULL;
  DECLARE gateId INT DEFAULT NULL;
  IF EXISTS (SELECT 1 FROM zone WHERE zoneidnumber IN ({ids}) OR short_name IN ({names})) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'a housing interior or hall zone row already exists';
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
SELECT zoneidnumber, short_name, long_name, min_expansion, max_expansion, underworld FROM zone WHERE zoneidnumber IN ({ids}) ORDER BY zoneidnumber;
"""
  path = os.path.join(here, '..', 'sql', 'housingZones.sql')
  open(path, 'w', encoding='ascii').write(sql)
  updates = ''.join(f"  UPDATE zone SET safe_x = {x!r}, safe_y = {y!r}, safe_z = {z!r}, safe_heading = {heading!r}, bypass_expansion_check = 1 WHERE zoneidnumber = {zoneId} AND version = 0 AND short_name = '{shortName}';\n"
                    for zoneId, shortName, _, (x, y, z, heading) in housingZones)
  updates += "  UPDATE zone SET bypass_expansion_check = 1 WHERE short_name = 'neighborhood';\n"
  update = f"""-- Generated by playerHousing/tools/makeHousingZonesSql.py: safe points and the expansion bypass for the rows
-- housingZones.sql added, and the neighborhood's bypass.
DELIMITER //
BEGIN NOT ATOMIC
  IF (SELECT COUNT(*) FROM zone WHERE version = 0 AND zoneidnumber IN ({ids}) AND short_name IN ({names})) <> {len(housingZones)} THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'not every housing interior and hall zone row is present; run housingZones.sql';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM zone WHERE short_name = 'neighborhood') THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'no neighborhood zone row';
  END IF;
{updates}END //
DELIMITER ;
SELECT zoneidnumber, short_name, safe_x, safe_y, safe_z, safe_heading, expansion, bypass_expansion_check FROM zone WHERE short_name = 'neighborhood' OR (version = 0 AND zoneidnumber IN ({ids})) ORDER BY zoneidnumber;
"""
  updatePath = os.path.join(here, '..', 'sql', 'housingZoneSettings.sql')
  open(updatePath, 'w', encoding='ascii').write(update)
  print(f'wrote {os.path.relpath(path)}: {len(housingZones)} zones, {len(columns)} columns each; {os.path.relpath(updatePath)}: their safe points, the bypass and the neighborhood\'s')


if __name__ == '__main__':
  main()
