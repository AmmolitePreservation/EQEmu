-- The neighborhood spawns PEQ captured from Live players' placed NPC items (NBH-10): Herbert, Albert, Robert and David, the
-- guardian worgs and their pups, the Icefall leopard, The Punting Gnome and the haze panther (NPC types 712002 to 712010),
-- with their spawn groups, entries, grids and respawn timers. The meeting hall's own NPCs (712000, 712001, 712011 to 712023)
-- stay, and three of them get the height Live's captures settle them at (NPC-13): PEQ holds Garik Nogflop and Ragthar where
-- a capture caught them just spawned, above the ground.
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
  IF (SELECT COUNT(*) FROM spawn2 s JOIN spawnentry e ON e.spawngroupID = s.spawngroupID WHERE s.zone = 'neighborhood' AND s.version = 0
      AND ((e.npcID = 712020 AND s.x = 2043.5 AND s.y = -2792.75) OR (e.npcID = 712018 AND s.x = 2052.625 AND s.y = -2788.25)
        OR (e.npcID = 712011 AND s.x = 2021.75 AND s.y = -2824.125))) <> 3 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'the neighborhood does not hold PEQ''s one spawn each of Garik Nogflop, Ragthar and Varlad Rahnta';
  END IF;
  UPDATE spawn2 s JOIN spawnentry e ON e.spawngroupID = s.spawngroupID
    SET s.z = CASE e.npcID WHEN 712020 THEN 4.6 WHEN 712018 THEN 4.9 ELSE 3.5 END
    WHERE s.zone = 'neighborhood' AND s.version = 0 AND e.npcID IN (712011, 712018, 712020);
  COMMIT;
END //
DELIMITER ;
