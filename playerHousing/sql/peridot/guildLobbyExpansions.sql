-- Peridot's own, outside the housing migration and deployHousingContent.py (worklist, Owner decisions: Expansion gating): its Planes of
-- Power server (Expansion:CurrentExpansion 4) runs Live's Guild Lobby, version 1 (guildLobby.sql, DOR-17), from Planes of Power on and the
-- 2005 lobby, version 0, before it. Applied by hand after guildLobby.sql; refuses a lobby not split at House of Thule, and a second run.
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
  IF (SELECT COUNT(*) FROM zone WHERE short_name = 'guildlobby' AND version = 0 AND max_expansion = 16) <> 1
      OR (SELECT COUNT(*) FROM zone WHERE short_name = 'guildlobby' AND version = 1 AND min_expansion = 17 AND max_expansion = -1) <> 1 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the Guild Lobby is not split at House of Thule as guildLobby.sql leaves it, or this ran already';
  END IF;
  START TRANSACTION;
  UPDATE zone SET max_expansion = 3 WHERE short_name = 'guildlobby' AND version = 0;
  UPDATE zone SET min_expansion = 4 WHERE short_name = 'guildlobby' AND version = 1;
  COMMIT;
END //
DELIMITER ;
SELECT zoneidnumber, version, min_expansion, max_expansion FROM zone WHERE short_name = 'guildlobby' ORDER BY version;
