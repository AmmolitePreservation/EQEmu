-- The OP_NewZone header fields RoF2 reads that EQEmu's zone table lacks (ZIN-9): the ceiling (max_z), the filter id and
-- whether mana stones are disallowed. The defaults are the values EQEmu's RoF2 encoder sent for every zone before.
ALTER TABLE `zone`
  ADD COLUMN `ceiling` FLOAT NOT NULL DEFAULT 0 AFTER `shard_at_player_count`,
  ADD COLUMN `filter_id` INT(11) NOT NULL DEFAULT 2008 AFTER `ceiling`,
  ADD COLUMN `disallow_mana_stone` TINYINT(4) NOT NULL DEFAULT 1 AFTER `filter_id`;
