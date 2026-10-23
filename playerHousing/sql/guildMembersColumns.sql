-- A member's guild trophy tribute opt-in (TRO-8), beside EQEmu's guild tribute opt-in; Live keeps the two apart.
ALTER TABLE `guild_members`
  ADD COLUMN `trophy_tribute_enable` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0 AFTER `tribute_enable`;
