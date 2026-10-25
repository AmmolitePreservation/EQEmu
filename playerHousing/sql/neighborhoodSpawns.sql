-- The neighborhood spawns PEQ captured from Live players' placed NPC items (NBH-10): Herbert, Albert, Robert and David, the
-- guardian worgs and their pups, the Icefall leopard, The Punting Gnome and the haze panther (NPC types 712002 to 712010),
-- with their spawn groups, entries, grids and respawn timers. The meeting hall's own NPCs (712000, 712001, 712011 to 712023)
-- stay.
DELIMITER //
BEGIN NOT ATOMIC
  DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
  START TRANSACTION;
  IF NOT EXISTS (SELECT 1 FROM npc_types WHERE id BETWEEN 712002 AND 712010) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the captured neighborhood spawns are already gone: no npc_types 712002 to 712010';
  END IF;
  CREATE TEMPORARY TABLE capturedSpawns AS
    SELECT s.id AS spawn2ID, s.spawngroupID, s.pathgrid FROM spawn2 s JOIN spawnentry e ON e.spawngroupID = s.spawngroupID
    WHERE s.zone = 'neighborhood' AND s.version = 0 AND e.npcID BETWEEN 712002 AND 712010;
  IF (SELECT COUNT(*) FROM capturedSpawns) <> 16 OR (SELECT COUNT(*) FROM npc_types WHERE id BETWEEN 712002 AND 712010) <> 9 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the neighborhood does not hold PEQ''s 16 spawns of the 9 npc_types 712002 to 712010';
  END IF;
  IF EXISTS (SELECT 1 FROM spawnentry WHERE npcID BETWEEN 712002 AND 712010 AND spawngroupID NOT IN (SELECT spawngroupID FROM capturedSpawns))
     OR EXISTS (SELECT 1 FROM spawnentry WHERE spawngroupID IN (SELECT spawngroupID FROM capturedSpawns) AND npcID NOT BETWEEN 712002 AND 712010)
     OR EXISTS (SELECT 1 FROM spawn2 WHERE spawngroupID IN (SELECT spawngroupID FROM capturedSpawns) AND id NOT IN (SELECT spawn2ID FROM capturedSpawns)) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'a spawn group of npc_types 712002 to 712010 is shared with another spawn or NPC';
  END IF;
  IF EXISTS (SELECT 1 FROM spawn2 WHERE zone = 'neighborhood' AND pathgrid IN (SELECT pathgrid FROM capturedSpawns WHERE pathgrid <> 0)
     AND id NOT IN (SELECT spawn2ID FROM capturedSpawns)) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'a grid of npc_types 712002 to 712010 is walked by another neighborhood spawn';
  END IF;
  DELETE FROM grid_entries WHERE zoneid = 712 AND gridid IN (SELECT pathgrid FROM capturedSpawns WHERE pathgrid <> 0);
  DELETE FROM grid WHERE zoneid = 712 AND id IN (SELECT pathgrid FROM capturedSpawns WHERE pathgrid <> 0);
  DELETE FROM respawn_times WHERE id IN (SELECT spawn2ID FROM capturedSpawns);
  DELETE FROM spawnentry WHERE spawngroupID IN (SELECT spawngroupID FROM capturedSpawns);
  DELETE FROM spawn2 WHERE id IN (SELECT spawn2ID FROM capturedSpawns);
  DELETE FROM spawngroup WHERE id IN (SELECT spawngroupID FROM capturedSpawns);
  DELETE FROM npc_types WHERE id BETWEEN 712002 AND 712010;
  DROP TEMPORARY TABLE capturedSpawns;
  COMMIT;
END //
DELIMITER ;
