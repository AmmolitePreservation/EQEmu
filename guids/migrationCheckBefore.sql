-- TST-6, before the migration: every item and augment of the pre-GUID tables with the place old EQEmu showed it in,
-- written to the tst6 database, which the migration does not touch. A top's key is holder:holder_id:slot; a bag's
-- content or an augment is its parent's key, '/', and its index (a socket's index is the augment slot minus 1).
DROP DATABASE IF EXISTS tst6;
CREATE DATABASE tst6;
CREATE TABLE tst6.expected (
  loc_key VARCHAR(96) NOT NULL PRIMARY KEY,
  source VARCHAR(24) NOT NULL,
  owner_character INT UNSIGNED NULL,
  item_id INT UNSIGNED NOT NULL,
  charges INT NULL,
  color INT UNSIGNED NOT NULL,
  attuned TINYINT NOT NULL,
  custom_data TEXT NOT NULL,
  ornament_icon INT UNSIGNED NOT NULL,
  ornament_idfile INT UNSIGNED NOT NULL,
  ornament_hero_model INT UNSIGNED NOT NULL
);
CREATE TABLE tst6.sockets (socket TINYINT NOT NULL PRIMARY KEY);
INSERT INTO tst6.sockets VALUES (0), (1), (2), (3), (4), (5), (6);

-- inventory: where each row sits (top or under a bag), before augments are split out
CREATE TABLE tst6.inventory_place AS
SELECT i.*, CAST(i.slot_id AS SIGNED) AS s,
  CASE
    WHEN CAST(i.slot_id AS SIGNED) BETWEEN 0 AND 32 THEN CONCAT('inventory:', i.character_id, ':', CAST(i.slot_id AS SIGNED))
    WHEN CAST(i.slot_id AS SIGNED) = 33 THEN CONCAT('cursor:', i.character_id, ':0')
    WHEN CAST(i.slot_id AS SIGNED) BETWEEN 6010 AND 6209 THEN CONCAT('cursor:', i.character_id, ':',
      (SELECT COUNT(*) FROM peq.inventory q WHERE q.character_id = i.character_id AND CAST(q.slot_id AS SIGNED) BETWEEN 6010 AND CAST(i.slot_id AS SIGNED)))
    WHEN CAST(i.slot_id AS SIGNED) BETWEEN 2000 AND 2023 THEN CONCAT('bank:', i.character_id, ':', CAST(i.slot_id AS SIGNED) - 2000)
    WHEN CAST(i.slot_id AS SIGNED) BETWEEN 3000 AND 3007 THEN CONCAT('trade:', i.character_id, ':', CAST(i.slot_id AS SIGNED) - 3000)
    WHEN CAST(i.slot_id AS SIGNED) BETWEEN 4010 AND 6009 THEN CONCAT('inventory:', i.character_id, ':', 23 + (CAST(i.slot_id AS SIGNED) - 4010) DIV 200, '/', (CAST(i.slot_id AS SIGNED) - 4010) MOD 200)
    WHEN CAST(i.slot_id AS SIGNED) BETWEEN 6210 AND 11009 THEN CONCAT('bank:', i.character_id, ':', (CAST(i.slot_id AS SIGNED) - 6210) DIV 200, '/', (CAST(i.slot_id AS SIGNED) - 6210) MOD 200)
    WHEN CAST(i.slot_id AS SIGNED) BETWEEN 11410 AND 13009 THEN CONCAT('trade:', i.character_id, ':', (CAST(i.slot_id AS SIGNED) - 11410) DIV 200, '/', (CAST(i.slot_id AS SIGNED) - 11410) MOD 200)
  END AS loc_key
FROM peq.inventory i;

INSERT INTO tst6.expected
SELECT IF(n.socket = 0, p.loc_key, CONCAT(p.loc_key, '/', n.socket - 1)), 'inventory', p.character_id,
  ELT(n.socket + 1, p.item_id, p.augment_one, p.augment_two, p.augment_three, p.augment_four, p.augment_five, p.augment_six),
  IF(n.socket = 0, p.charges, 0), IF(n.socket = 0, p.color, 0), IF(n.socket = 0, p.instnodrop, 0),
  IF(n.socket = 0, COALESCE(p.custom_data, ''), ''), IF(n.socket = 0, p.ornament_icon, 0), IF(n.socket = 0, p.ornament_idfile, 0),
  IF(n.socket = 0, p.ornament_hero_model, 0)
FROM tst6.inventory_place p JOIN tst6.sockets n
WHERE p.loc_key IS NOT NULL
  AND ELT(n.socket + 1, p.item_id, p.augment_one, p.augment_two, p.augment_three, p.augment_four, p.augment_five, p.augment_six) > 0;

CREATE TABLE tst6.skipped AS
SELECT 'inventory display slots' AS what, COUNT(*) AS n FROM tst6.inventory_place WHERE loc_key IS NULL;

INSERT INTO tst6.expected
SELECT IF(n.socket = 0, p.loc_key, CONCAT(p.loc_key, '/', n.socket - 1)), 'sharedbank', NULL,
  ELT(n.socket + 1, p.item_id, p.augment_one, p.augment_two, p.augment_three, p.augment_four, p.augment_five, p.augment_six),
  IF(n.socket = 0, p.charges, 0), IF(n.socket = 0, p.color, 0), 0,
  IF(n.socket = 0, COALESCE(p.custom_data, ''), ''), IF(n.socket = 0, p.ornament_icon, 0), IF(n.socket = 0, p.ornament_idfile, 0),
  IF(n.socket = 0, p.ornament_hero_model, 0)
FROM (
  SELECT s.*,
    CASE
      WHEN CAST(s.slot_id AS SIGNED) BETWEEN 2500 AND 2501 THEN CONCAT('shared_bank:', s.account_id, ':', CAST(s.slot_id AS SIGNED) - 2500)
      WHEN CAST(s.slot_id AS SIGNED) BETWEEN 11010 AND 11409 THEN CONCAT('shared_bank:', s.account_id, ':', (CAST(s.slot_id AS SIGNED) - 11010) DIV 200, '/', (CAST(s.slot_id AS SIGNED) - 11010) MOD 200)
    END AS loc_key
  FROM peq.sharedbank s
) p JOIN tst6.sockets n
WHERE ELT(n.socket + 1, p.item_id, p.augment_one, p.augment_two, p.augment_three, p.augment_four, p.augment_five, p.augment_six) > 0;

INSERT INTO tst6.expected
SELECT IF(n.socket = 0, p.loc_key, CONCAT(p.loc_key, '/', n.socket - 1)), 'corpse', p.charid,
  ELT(n.socket + 1, p.item_id, p.aug_1, p.aug_2, p.aug_3, p.aug_4, p.aug_5, p.aug_6),
  IF(n.socket = 0, p.charges, 0), 0, IF(n.socket = 0, p.attuned, 0),
  IF(n.socket = 0, COALESCE(p.custom_data, ''), ''), IF(n.socket = 0, p.ornamenticon, 0), IF(n.socket = 0, p.ornamentidfile, 0),
  IF(n.socket = 0, p.ornament_hero_model, 0)
FROM (
  SELECT ci.*, cc.charid,
    CASE
      WHEN CAST(ci.equip_slot AS SIGNED) BETWEEN 4010 AND 6009 THEN CONCAT('corpse:', ci.corpse_id, ':', 23 + (CAST(ci.equip_slot AS SIGNED) - 4010) DIV 200, '/', (CAST(ci.equip_slot AS SIGNED) - 4010) MOD 200)
      WHEN CAST(ci.equip_slot AS SIGNED) BETWEEN 6010 AND 6209 THEN CONCAT('corpse:', ci.corpse_id, ':33/', CAST(ci.equip_slot AS SIGNED) - 6010)
      ELSE CONCAT('corpse:', ci.corpse_id, ':', CAST(ci.equip_slot AS SIGNED))
    END AS loc_key
  FROM peq.character_corpse_items ci JOIN peq.character_corpses cc ON cc.id = ci.corpse_id
) p JOIN tst6.sockets n
WHERE ELT(n.socket + 1, p.item_id, p.aug_1, p.aug_2, p.aug_3, p.aug_4, p.aug_5, p.aug_6) > 0;

INSERT INTO tst6.expected
SELECT IF(n.socket = 0, CONCAT('parcel:', p.char_id, ':', p.slot_id), CONCAT('parcel:', p.char_id, ':', p.slot_id, '/', n.socket - 1)), 'parcel', p.char_id,
  ELT(n.socket + 1, p.item_id, p.aug_slot_1, p.aug_slot_2, p.aug_slot_3, p.aug_slot_4, p.aug_slot_5, p.aug_slot_6),
  IF(n.socket = 0, p.quantity, 0), 0, 0, '', 0, 0, 0
FROM peq.character_parcels p JOIN tst6.sockets n
WHERE p.item_id <> 99990
  AND ELT(n.socket + 1, p.item_id, p.aug_slot_1, p.aug_slot_2, p.aug_slot_3, p.aug_slot_4, p.aug_slot_5, p.aug_slot_6) > 0;

INSERT INTO tst6.expected
SELECT IF(n.socket = 0, CONCAT('parcel:', p.char_id, ':', p.slot_id, '/', pc.slot_id), CONCAT('parcel:', p.char_id, ':', p.slot_id, '/', pc.slot_id, '/', n.socket - 1)),
  'parcel content', p.char_id,
  ELT(n.socket + 1, pc.item_id, pc.aug_slot_1, pc.aug_slot_2, pc.aug_slot_3, pc.aug_slot_4, pc.aug_slot_5, pc.aug_slot_6),
  IF(n.socket = 0, pc.quantity, 0), 0, 0, '', 0, 0, 0
FROM peq.character_parcels_containers pc JOIN peq.character_parcels p ON p.id = pc.parcels_id JOIN tst6.sockets n
WHERE ELT(n.socket + 1, pc.item_id, pc.aug_slot_1, pc.aug_slot_2, pc.aug_slot_3, pc.aug_slot_4, pc.aug_slot_5, pc.aug_slot_6) > 0;

-- a pet's saved equipment is item numbers only: charges are not compared
INSERT INTO tst6.expected
SELECT CONCAT(IF(i.pet = 0, 'pet', 'suspended_pet'), ':', i.char_id, ':', i.slot), 'pet', i.char_id, i.item_id, NULL, 0, 0, '', 0, 0, 0
FROM peq.character_pet_inventory i WHERE i.item_id > 0;

-- the guild bank shows each area's rows in slot order from 0 (GuildBankManager::Load)
CREATE TABLE tst6.guild_bank_place AS
SELECT g.*, CONCAT(IF(g.area = 1, 'guild_bank_main', 'guild_bank_deposit'), ':', g.guild_id, ':',
  ROW_NUMBER() OVER (PARTITION BY g.guild_id, g.area ORDER BY g.slot, g.id) - 1) AS loc_key
FROM peq.guild_bank g WHERE g.item_id > 0;

INSERT INTO tst6.expected
SELECT IF(n.socket = 0, p.loc_key, CONCAT(p.loc_key, '/', n.socket - 1)), 'guild_bank', NULL,
  ELT(n.socket + 1, p.item_id, p.augment_one_id, p.augment_two_id, p.augment_three_id, p.augment_four_id, p.augment_five_id, p.augment_six_id),
  IF(n.socket = 0, p.quantity, 0), 0, 0, '', 0, 0, 0
FROM tst6.guild_bank_place p JOIN tst6.sockets n
WHERE ELT(n.socket + 1, p.item_id, p.augment_one_id, p.augment_two_id, p.augment_three_id, p.augment_four_id, p.augment_five_id, p.augment_six_id) > 0;

INSERT INTO tst6.expected
SELECT IF(n.socket = 0, CONCAT('world_container:', o.parentid, ':', o.bagidx), CONCAT('world_container:', o.parentid, ':', o.bagidx, '/', n.socket - 1)), 'world_container', NULL,
  ELT(n.socket + 1, o.itemid, o.augslot1, o.augslot2, o.augslot3, o.augslot4, o.augslot5, o.augslot6),
  IF(n.socket = 0, o.charges, 0), 0, 0, '', 0, 0, 0
FROM peq.object_contents o JOIN tst6.sockets n
WHERE ELT(n.socket + 1, o.itemid, o.augslot1, o.augslot2, o.augslot3, o.augslot4, o.augslot5, o.augslot6) > 0;

-- side tables
CREATE TABLE tst6.parcels AS SELECT id, char_id, slot_id, item_id, quantity FROM peq.character_parcels;
CREATE TABLE tst6.evolving AS
SELECT e.id, e.character_id, e.item_id, e.current_amount,
  (SELECT COUNT(*) FROM tst6.expected x WHERE x.owner_character = e.character_id AND x.item_id = e.item_id AND x.source <> 'parcel content') AS copies
FROM peq.character_evolving_items e WHERE e.deleted_at IS NULL;
CREATE TABLE tst6.counts AS
SELECT 'merchantlist_temp' AS what, COUNT(*) AS n FROM peq.merchantlist_temp
UNION ALL SELECT 'inventory_snapshots', COUNT(*) FROM peq.inventory_snapshots
UNION ALL SELECT 'object', COUNT(*) FROM peq.object
UNION ALL SELECT 'deleted evolving records', COUNT(*) FROM peq.character_evolving_items WHERE deleted_at IS NOT NULL;

SELECT source, COUNT(*) AS expected_rows, SUM(loc_key LIKE '%/%') AS children FROM tst6.expected GROUP BY source WITH ROLLUP;
SELECT * FROM tst6.skipped;
SELECT * FROM tst6.counts;
SELECT 'evolving copies' AS what, copies, COUNT(*) AS records FROM tst6.evolving GROUP BY copies;
