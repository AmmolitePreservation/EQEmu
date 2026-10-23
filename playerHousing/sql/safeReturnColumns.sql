-- The property whose copy a character was saved in (INT-32), on EQEmu's record of where a character goes when the instance
-- it logged out in is gone. A zone-in into a housing copy writes it; world's login makes a new copy of the property when
-- the row's instance no longer exists, and EQEmu's safe return applies otherwise.
ALTER TABLE `character_instance_safereturns`
  ADD COLUMN `property_id` INT(10) UNSIGNED NOT NULL DEFAULT 0 AFTER `safe_heading`;
