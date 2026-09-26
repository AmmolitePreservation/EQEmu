"""Generates sql/itemHousingTail.sql, the migration that names and decodes the housing tail in `items` (serverPlan.md, D1).

Usage: python3 makeItemHousingTailSql.py > ../sql/itemHousingTail.sql
"""
C = lambda c: f"c.`{c}`"
I = lambda c: f"i.`{c}`"
u = lambda c: f"({C(c)} & 0xFFFFFFFF)"
byteAt = lambda c, k: f"(({C(c)} >> {8 * k}) & 0xFF)"
join = lambda lo, hi: f"(({u(lo)} >> 8) | ({byteAt(hi, 0)} << 24))"
signed = lambda e: f"(CAST({e} AS SIGNED) - IF({e} >= 2147483648, 4294967296, 0))"


def flt(e):
  # MariaDB's bit operators return BIGINT UNSIGNED, so the exponent and mantissa are cast before any signed arithmetic.
  exponent = f"CAST((({e} >> 23) & 0xFF) AS SIGNED)"
  mantissa = f"CAST(({e} & 0x7FFFFF) AS SIGNED)"
  sign = f"IF({e} >= 2147483648, -1, 1)"
  return (f"(CASE WHEN {exponent} = 255 THEN NULL WHEN {exponent} = 0 THEN {sign} * {mantissa} * POW(2, -149) "
          f"ELSE {sign} * (1 + {mantissa} / 8388608) * POW(2, {exponent} - 127) END)")


def plausible(t, r, f, n, s, h, p, o, i):
  return (f"COALESCE(({t} = -1 OR ({t} > 0 AND {t} < 100000)) AND {r} IN (0, 1) AND {f} IN (0, 1) AND {n} <= 100000000 "
          f"AND {s} BETWEEN 0.01 AND 50 AND {h} BETWEEN -1000 AND 1000 AND {p} BETWEEN -1000 AND 1000 AND {o} BETWEEN -1000 AND 1000 "
          f"AND {i} IN (0, 1), FALSE)")


aligned = dict(t=C('UNK233'), r=C('UNK234'), f=C('placeable'), n=u('UNK236'), s=I('hc_a_scale'), h=I('hc_a_heading'), p=I('hc_a_pitch'),
               o=I('hc_a_roll'), i=C('UNK241'))
shifted = dict(t=I('hc_s_trophy'), r=C('placeable'), f=I('hc_s_free'), n=I('hc_s_respawn'), s=I('hc_s_scale'), h=I('hc_s_heading'),
               p=I('hc_s_pitch'), o=I('hc_s_roll'), i=I('hc_s_interactive'))
# (new name, final definition, value when captured aligned, value when captured shifted, value for non-placeable items, raw column)
final = [
  ('placeable', 'INT(11) NOT NULL DEFAULT 0', C('UNK224'), C('UNK224'), '0', 'UNK224'),
  ('placeable_ignore_collisions', 'TINYINT(3) UNSIGNED NOT NULL DEFAULT 0', C('UNK225'), C('UNK225'), '0', 'UNK225'),
  ('placement_type', 'INT(11) NOT NULL DEFAULT 0', C('UNK226'), C('UNK226'), '0', 'UNK226'),
  ('real_estate_definition_id', 'INT(11) NOT NULL DEFAULT 0', C('UNK227'), C('UNK227'), '0', 'UNK227'),
  ('placeable_scale_min', 'FLOAT NOT NULL DEFAULT 0', flt(u('UNK228')), flt(u('UNK228')), '0', 'UNK228'),
  ('placeable_scale_max', 'FLOAT NOT NULL DEFAULT 0', flt(u('UNK229')), flt(u('UNK229')), '0', 'UNK229'),
  ('real_estate_upkeep_id', 'INT(11) NOT NULL DEFAULT 0', C('UNK230'), C('UNK230'), '0', 'UNK230'),
  ('max_per_real_estate', 'INT(11) NOT NULL DEFAULT -1', C('UNK231'), C('UNK231'), '-1', 'UNK231'),
  ('housepet_file_name', "VARCHAR(31) NOT NULL DEFAULT ''", "''", "''", "''", 'UNK232'),
  ('trophy_benefit_id', 'INT(11) NOT NULL DEFAULT -1', C('UNK233'), I('hc_s_trophy'), '-1', 'UNK233'),
  ('disable_placement_rotation', 'TINYINT(3) UNSIGNED NOT NULL DEFAULT 0', C('UNK234'), C('placeable'), '0', 'UNK234'),
  ('disable_free_placement', 'TINYINT(3) UNSIGNED NOT NULL DEFAULT 0', C('placeable'), I('hc_s_free'), '0', 'placeable'),
  ('npc_respawn_interval', 'INT(10) UNSIGNED NOT NULL DEFAULT 0', u('UNK236'), I('hc_s_respawn'), '0', 'UNK236'),
  ('placeable_default_scale', 'FLOAT NOT NULL DEFAULT 0', I('hc_a_scale'), I('hc_s_scale'), '0', 'UNK237'),
  ('placeable_default_heading', 'FLOAT NOT NULL DEFAULT 0', I('hc_a_heading'), I('hc_s_heading'), '0', 'UNK238'),
  ('placeable_default_pitch', 'FLOAT NOT NULL DEFAULT 0', I('hc_a_pitch'), I('hc_s_pitch'), '0', 'UNK239'),
  ('placeable_default_roll', 'FLOAT NOT NULL DEFAULT 0', I('hc_a_roll'), I('hc_s_roll'), '0', 'UNK240'),
  ('interactive_object', 'TINYINT(3) UNSIGNED NOT NULL DEFAULT 0', C('UNK241'), I('hc_s_interactive'), '0', 'UNK241'),
]
raw = [f[5] for f in final]
temps = [('hc_s_trophy', 'BIGINT NULL'), ('hc_s_free', 'BIGINT NULL'), ('hc_s_respawn', 'BIGINT NULL'), ('hc_s_scale', 'DOUBLE NULL'),
         ('hc_s_heading', 'DOUBLE NULL'), ('hc_s_pitch', 'DOUBLE NULL'), ('hc_s_roll', 'DOUBLE NULL'), ('hc_s_interactive', 'BIGINT NULL'),
         ('hc_a_scale', 'DOUBLE NULL'), ('hc_a_heading', 'DOUBLE NULL'), ('hc_a_pitch', 'DOUBLE NULL'), ('hc_a_roll', 'DOUBLE NULL'),
         ('hc_layout', 'TINYINT NOT NULL DEFAULT 0')]
# The only non-placeable patterns in the full table (83,732 never captured; 23,977 shifted; 4,658 aligned), UNK225-UNK241 in order.
nonPlaceablePatterns = [[0] * 17, [0, 0, 0, 0, 0, 0, -1, 0, -256, 255, 0, 0, 0, 0, 0, 0, 0], [0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0]]
guards = [('placeable_item_without_one_plausible_layout', "FROM {J} WHERE c.`UNK224` <> 0 AND i.`hc_layout` = 3", 'i.`id`'),
          ('housepet_capture_not_empty', "FROM `items_housing_capture` WHERE `UNK232` <> 0", '`id`'),
          ('non_placeable_item_outside_known_patterns', "FROM `items_housing_capture` WHERE `UNK224` = 0 AND NOT ({match})", '`id`')]
J = "`items` i JOIN `items_housing_capture` c ON c.`id` = i.`id`"
match = " OR ".join("(" + " AND ".join(f"`{c}` = {v}" for c, v in zip(raw[1:], p)) + ")" for p in nonPlaceablePatterns)
out = [
  "-- Names and decodes the housing tail captured in `items` (serverPlan.md, D1). Runs while `UNK224` exists: every statement",
  "-- before the final ALTER can be repeated, and the final ALTER is atomic. Decoding reads only `items_housing_capture`, the",
  "-- untouched capture. A guard stops the migration before any change if the data differs from what was validated; its",
  "-- constraint's name says which.",
  "CREATE TABLE IF NOT EXISTS `items_housing_capture` (PRIMARY KEY (`id`)) AS SELECT `id`, " + ", ".join(f"`{c}`" for c in raw) + " FROM `items`;",
  "ALTER TABLE `items` " + ", ".join(f"ADD COLUMN IF NOT EXISTS `{c}` {t}" for c, t in temps) + ", "
  + ", ".join(f"ADD COLUMN IF NOT EXISTS `hd_{c}` {t}" for c, t, *_ in final) + ";",
  f"UPDATE {J} SET " + ", ".join([
    f"i.`hc_s_trophy` = {signed(join('UNK233', 'UNK234'))}", f"i.`hc_s_free` = {byteAt('UNK236', 0)}",
    f"i.`hc_s_respawn` = {join('UNK236', 'UNK237')}", f"i.`hc_s_scale` = {flt(join('UNK237', 'UNK238'))}",
    f"i.`hc_s_heading` = {flt(join('UNK238', 'UNK239'))}", f"i.`hc_s_pitch` = {flt(join('UNK239', 'UNK240'))}",
    f"i.`hc_s_roll` = {flt(join('UNK240', 'UNK241'))}", f"i.`hc_s_interactive` = {byteAt('UNK233', 0)}",
    f"i.`hc_a_scale` = {flt(u('UNK237'))}", f"i.`hc_a_heading` = {flt(u('UNK238'))}", f"i.`hc_a_pitch` = {flt(u('UNK239'))}",
    f"i.`hc_a_roll` = {flt(u('UNK240'))}"]) + " WHERE c.`UNK224` <> 0;",
  f"UPDATE {J} SET i.`hc_layout` = CASE WHEN {plausible(**aligned)} AND NOT {plausible(**shifted)} THEN 1 "
  f"WHEN {plausible(**shifted)} AND NOT {plausible(**aligned)} THEN 2 ELSE 3 END WHERE c.`UNK224` <> 0;",
  "CREATE TEMPORARY TABLE IF NOT EXISTS `hc_guard` (`id` INT NOT NULL, `problem` VARCHAR(48) NOT NULL, "
  + ", ".join(f"CONSTRAINT `{name}` CHECK (`problem` <> '{name}')" for name, *_ in guards) + ");",
]
for name, source, idColumn in guards:
  out.append(f"INSERT INTO `hc_guard` SELECT {idColumn}, '{name}' " + source.format(J=J, match=match) + ";")
out += [
  "DROP TEMPORARY TABLE `hc_guard`;",
  f"UPDATE {J} SET " + ", ".join(f"i.`hd_{c}` = CASE WHEN c.`UNK224` = 0 THEN {n} WHEN i.`hc_layout` = 1 THEN {a} ELSE {s} END"
                                 for c, t, a, s, n, _ in final) + ";",
  "ALTER TABLE `items` CHANGE COLUMN IF EXISTS `placeable` `disable_free_placement` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0;",
  "ALTER TABLE `items` " + ", ".join(f"MODIFY COLUMN `{r}` {t}" for c, t, a, s, n, r in final if r != 'placeable') + ";",
  "UPDATE `items` SET `disable_free_placement` = `hd_disable_free_placement`, "
  + ", ".join(f"`{r}` = `hd_{c}`" for c, t, a, s, n, r in final if r != 'placeable') + ";",
  "ALTER TABLE `items` " + ", ".join(f"CHANGE COLUMN `{r}` `{c}` {t}" for c, t, a, s, n, r in final if r != 'placeable') + ", "
  + ", ".join(f"DROP COLUMN `{c}`" for c, _ in temps) + ", " + ", ".join(f"DROP COLUMN `hd_{c}`" for c, *_ in final) + ";",
]
print("\n".join(out))
