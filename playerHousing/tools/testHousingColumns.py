"""Tests the sql/*Columns.sql files, which add housing's columns to EQEmu's own tables, against a local MariaDB: each ALTER
runs on the table as EQEmu creates it (Peridot's schema, 2026-10-23), the columns land with their types and defaults, and the
constraints refuse bad rows.

Usage: python testHousingColumns.py   (a local MariaDB with a `peq` database, root without a password)
"""
import os, subprocess

here = os.path.dirname(os.path.abspath(__file__))

eqemuTables = {
  'zone': """CREATE TABLE `zone` (`id` int(10) NOT NULL AUTO_INCREMENT, `short_name` varchar(32) DEFAULT NULL, `shard_at_player_count` int(11) NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1""",
  'doors': """CREATE TABLE `doors` (`id` int(11) NOT NULL AUTO_INCREMENT, `doorid` smallint(4) NOT NULL DEFAULT 0, `zone` varchar(32) NOT NULL DEFAULT '',
    `version` smallint(5) NOT NULL DEFAULT 0, `opentype` smallint(4) NOT NULL DEFAULT 0, `dz_switch_id` int(11) NOT NULL DEFAULT 0, `min_expansion` tinyint(4) NOT NULL DEFAULT -1,
    PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1""",
  'character_instance_safereturns': """CREATE TABLE `character_instance_safereturns` (`id` int(10) unsigned NOT NULL AUTO_INCREMENT, `character_id` int(10) unsigned NOT NULL,
    `instance_zone_id` int(11) NOT NULL DEFAULT 0, `instance_id` int(11) NOT NULL DEFAULT 0, `safe_zone_id` int(11) NOT NULL DEFAULT 0, `safe_x` float NOT NULL DEFAULT 0,
    `safe_y` float NOT NULL DEFAULT 0, `safe_z` float NOT NULL DEFAULT 0, `safe_heading` float NOT NULL DEFAULT 0, PRIMARY KEY (`id`), UNIQUE KEY `character_id` (`character_id`))
    ENGINE=InnoDB DEFAULT CHARSET=latin1""",
  'character_tribute': """CREATE TABLE `character_tribute` (`id` int(11) NOT NULL AUTO_INCREMENT, `character_id` int(11) unsigned NOT NULL DEFAULT 0,
    `tier` tinyint(11) unsigned NOT NULL DEFAULT 0, `tribute` int(11) unsigned NOT NULL DEFAULT 0, PRIMARY KEY (`id`), KEY `idx_character_id` (`character_id`))
    ENGINE=InnoDB DEFAULT CHARSET=latin1""",
  'character_data': """CREATE TABLE `character_data` (`id` int(11) unsigned NOT NULL AUTO_INCREMENT, `tribute_time_remaining` int(11) unsigned NOT NULL DEFAULT 0,
    `career_tribute_points` int(11) unsigned NOT NULL DEFAULT 0, `tribute_points` int(11) unsigned NOT NULL DEFAULT 0, `tribute_active` int(11) unsigned NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1""",
  'guild_members': """CREATE TABLE `guild_members` (`char_id` int(11) NOT NULL DEFAULT 0, `guild_id` mediumint(8) unsigned NOT NULL DEFAULT 0,
    `rank` tinyint(3) unsigned NOT NULL DEFAULT 0, `tribute_enable` tinyint(3) unsigned NOT NULL DEFAULT 0, `total_tribute` int(10) unsigned NOT NULL DEFAULT 0,
    `last_tribute` int(10) unsigned NOT NULL DEFAULT 0, `banker` tinyint(3) unsigned NOT NULL DEFAULT 0, `public_note` text NOT NULL, `alt` tinyint(3) unsigned NOT NULL DEFAULT 0,
    `online` tinyint(3) unsigned NOT NULL DEFAULT 0, PRIMARY KEY (`char_id`)) ENGINE=InnoDB DEFAULT CHARSET=latin1""",
  'guild_tributes': """CREATE TABLE `guild_tributes` (`guild_id` int(11) unsigned NOT NULL DEFAULT 0, `tribute_id_1` int(11) unsigned NOT NULL DEFAULT 0,
    `tribute_id_1_tier` int(11) unsigned NOT NULL DEFAULT 0, `tribute_id_2` int(11) unsigned NOT NULL DEFAULT 0, `tribute_id_2_tier` int(11) unsigned NOT NULL DEFAULT 0,
    `time_remaining` int(11) unsigned NOT NULL DEFAULT 0, `enabled` int(11) unsigned NOT NULL DEFAULT 0, PRIMARY KEY (`guild_id`)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4""",
}
columnFiles = {
  'zone': 'zoneHeaderColumns.sql', 'doors': 'doorColumns.sql', 'character_instance_safereturns': 'safeReturnColumns.sql',
  'character_tribute': 'characterTributeColumns.sql', 'character_data': 'characterDataColumns.sql', 'guild_members': 'guildMembersColumns.sql',
  'guild_tributes': 'guildTributesColumns.sql',
}


def sql(text, expectError=False):
  r = subprocess.run(['mariadb', '-uroot', '-N', 'peq'], input=text, capture_output=True, text=True)
  if (r.returncode != 0) != expectError:
    raise RuntimeError(f"unexpected result ({r.returncode}): {r.stderr[:400]}")
  return r.stderr if expectError else r.stdout.strip()


def check(label, query, expected):
  got = sql(query)
  assert got == expected, f'{label}: expected {expected!r}, got {got!r}'
  print(f'{label}: {got!r}')


def columns(table):
  return sql(f"SELECT GROUP_CONCAT(CONCAT(COLUMN_NAME, ' ', COLUMN_TYPE, ' ', COLUMN_DEFAULT) ORDER BY ORDINAL_POSITION SEPARATOR '; ') FROM information_schema.COLUMNS "
             f"WHERE TABLE_SCHEMA = 'peq' AND TABLE_NAME = '{table}'")


sql(''.join(f"DROP TABLE IF EXISTS `{table}`;" for table in eqemuTables))
for table, create in eqemuTables.items():
  sql(create + ';')
  sql(open(os.path.join(here, '..', 'sql', columnFiles[table])).read())
  print(f'{table}: {columnFiles[table]} applied')

check('the zone header columns', columns('zone'),
      'id int(10) NULL; short_name varchar(32) NULL; shard_at_player_count int(11) 0; ceiling float 0; filter_id int(11) 2008; disallow_mana_stone tinyint(4) 1')
check("the door's RealEstateDoorID follows dz_switch_id", columns('doors'),
      'id int(11) NULL; doorid smallint(4) 0; zone varchar(32) \'\'; version smallint(5) 0; opentype smallint(4) 0; dz_switch_id int(11) 0; real_estate_door_id int(10) unsigned 0; min_expansion tinyint(4) -1')
check("the safe return's property is its last column", columns('character_instance_safereturns').split('; ')[-1], 'property_id int(10) unsigned 0')
check('the tribute row kind and slot follow the character', columns('character_tribute'),
      'id int(11) NULL; character_id int(11) unsigned 0; kind tinyint(3) unsigned 0; slot tinyint(3) unsigned 0; tier tinyint(11) unsigned 0; tribute int(11) unsigned 0')
check('the trophy flag and timer follow tribute_active', columns('character_data').split('; ')[-2:],
      ['trophy_tribute_active int(11) unsigned 0', 'trophy_tribute_time_remaining int(11) unsigned 0'])
check('the trophy opt-in follows tribute_enable', columns('guild_members').split('; ')[3:5], ['tribute_enable tinyint(3) unsigned 0', 'trophy_tribute_enable tinyint(3) unsigned 0'])
check('the four guild trophy slots, their timer and flag follow enabled', columns('guild_tributes').split('; ')[7:],
      ['trophy_id_1 int(11) unsigned 0', 'trophy_id_1_tier int(11) unsigned 0', 'trophy_id_2 int(11) unsigned 0', 'trophy_id_2_tier int(11) unsigned 0',
       'trophy_id_3 int(11) unsigned 0', 'trophy_id_3_tier int(11) unsigned 0', 'trophy_id_4 int(11) unsigned 0', 'trophy_id_4_tier int(11) unsigned 0',
       'trophy_time_remaining int(11) unsigned 0', 'trophy_enabled int(11) unsigned 0'])

check("EQEmu's tribute rows take kind 0 and slot 0 by default", "INSERT INTO character_tribute (character_id, tier, tribute) VALUES (7, 1, 100); SELECT CONCAT(kind, ':', slot) FROM character_tribute", '0:0')
check('a trophy row in slot 9', "INSERT INTO character_tribute (character_id, kind, slot, tier, tribute) VALUES (7, 1, 9, 0, 200); SELECT COUNT(*) FROM character_tribute WHERE kind = 1", '1')
check('a door with no RealEstateDoorID reads 0', "INSERT INTO doors (doorid, zone) VALUES (1, 'qeynos'); SELECT real_estate_door_id FROM doors", '0')
for label, bad, reason in [('tribute row kind 2', "INSERT INTO character_tribute (character_id, kind) VALUES (7, 2)", 'character_tribute_kind'),
                           ('trophy slot above 9', "INSERT INTO character_tribute (character_id, kind, slot) VALUES (7, 1, 10)", 'character_tribute_slot')]:
  error = sql(bad, expectError=True)
  assert reason in error, f'{label}: refused for another reason: {error[:300]}'
  print(f'rejected: {label} ({reason})')

sql(''.join(f"DROP TABLE `{table}`;" for table in eqemuTables))
print('all column checks passed')
