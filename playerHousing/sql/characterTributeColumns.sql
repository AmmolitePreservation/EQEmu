-- Trophy tribute on EQEmu's personal tribute rows (TRO-8): kind 0 is a tribute row in EQEmu's packed order, kind 1 a trophy
-- benefit in one of the client's 10 trophy slots. EQEmu's tribute load and save take kind 0 rows only.
ALTER TABLE `character_tribute`
  ADD COLUMN `kind` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0 AFTER `character_id`,
  ADD COLUMN `slot` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0 AFTER `kind`,
  ADD CONSTRAINT `character_tribute_kind` CHECK (`kind` IN (0, 1)),
  ADD CONSTRAINT `character_tribute_slot` CHECK (`slot` <= 9);
