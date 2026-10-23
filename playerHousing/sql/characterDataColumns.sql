-- The personal trophy tribute's active flag and timer (TRO-8), beside EQEmu's tribute_active and tribute_time_remaining.
ALTER TABLE `character_data`
  ADD COLUMN `trophy_tribute_active` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `tribute_active`,
  ADD COLUMN `trophy_tribute_time_remaining` INT(11) UNSIGNED NOT NULL DEFAULT 0 AFTER `trophy_tribute_active`;
