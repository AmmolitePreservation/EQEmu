-- The guild's four trophy tribute slots, timer and active flag (TRO-8), beside EQEmu's two tribute slots; an empty slot is
-- 0, as EQEmu's own tribute slots are, and the client's -1 is made when sent.
ALTER TABLE `guild_tributes`
  ADD COLUMN `trophy_id_1` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `enabled`,
  ADD COLUMN `trophy_id_1_tier` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_1`,
  ADD COLUMN `trophy_id_2` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_1_tier`,
  ADD COLUMN `trophy_id_2_tier` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_2`,
  ADD COLUMN `trophy_id_3` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_2_tier`,
  ADD COLUMN `trophy_id_3_tier` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_3`,
  ADD COLUMN `trophy_id_4` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_3_tier`,
  ADD COLUMN `trophy_id_4_tier` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_4`,
  ADD COLUMN `trophy_time_remaining` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_id_4_tier`,
  ADD COLUMN `trophy_enabled` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_time_remaining`;
