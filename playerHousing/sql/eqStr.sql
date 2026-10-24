-- The client's eqstr_us.txt as a table (PAT-8), beside EQEmu's db_str: import_client_files fills it from import/eqstr_us.txt (RoF2's),
-- content alters the strings Peridot changes, and export_client_files writes export/eqstr_us.txt whole, header and count included.
CREATE TABLE IF NOT EXISTS `eq_str` (
  `id` INT(11) NOT NULL,
  `value` TEXT NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
