-- TST-6, after the migration: the item table's held rows keyed the same way, compared with tst6.expected.
DROP TABLE IF EXISTS tst6.actual;
CREATE TABLE tst6.actual AS
WITH RECURSIVE tree AS (
  SELECT i.guid_first, i.guid_second, i.guid_third, i.item_id, i.charges, i.color, i.attuned, i.custom_data,
    i.ornament_icon, i.ornament_idfile, i.ornament_hero_model,
    CAST(CONCAT(i.holder_type, ':', i.holder_id, ':', i.slot) AS CHAR(96)) AS loc_key
  FROM peq.item_instances i
  WHERE i.parent_guid_first IS NULL AND i.retirement_state IS NULL
  UNION ALL
  SELECT c.guid_first, c.guid_second, c.guid_third, c.item_id, c.charges, c.color, c.attuned, c.custom_data,
    c.ornament_icon, c.ornament_idfile, c.ornament_hero_model,
    CAST(CONCAT(t.loc_key, '/', c.parent_index) AS CHAR(96))
  FROM peq.item_instances c
  JOIN tree t ON c.parent_guid_first = t.guid_first AND c.parent_guid_second = t.guid_second AND c.parent_guid_third = t.guid_third
  WHERE c.retirement_state IS NULL
)
SELECT * FROM tree;
ALTER TABLE tst6.actual ADD PRIMARY KEY (loc_key);

SELECT 'rows' AS what,
  (SELECT COUNT(*) FROM tst6.expected) AS expected,
  (SELECT COUNT(*) FROM tst6.actual) AS actual_held,
  (SELECT COUNT(*) FROM peq.item_instances) AS item_instances_total,
  (SELECT COUNT(*) FROM peq.item_instances WHERE retirement_state IS NOT NULL) AS retired;

SELECT 'missing' AS what, e.source, e.loc_key, e.item_id FROM tst6.expected e LEFT JOIN tst6.actual a ON a.loc_key = e.loc_key WHERE a.loc_key IS NULL;
SELECT 'extra' AS what, a.loc_key, a.item_id FROM tst6.actual a LEFT JOIN tst6.expected e ON e.loc_key = a.loc_key WHERE e.loc_key IS NULL;
SELECT 'differs' AS what, e.source, e.loc_key, e.item_id, a.item_id, e.charges, a.charges, e.color, a.color, e.attuned, a.attuned,
  e.custom_data, a.custom_data, e.ornament_icon, a.ornament_icon, e.ornament_idfile, a.ornament_idfile, e.ornament_hero_model, a.ornament_hero_model
FROM tst6.expected e JOIN tst6.actual a ON a.loc_key = e.loc_key
WHERE e.item_id <> a.item_id OR (e.charges IS NOT NULL AND e.charges <> a.charges) OR e.color <> a.color OR e.attuned <> a.attuned
  OR e.custom_data <> a.custom_data OR e.ornament_icon <> a.ornament_icon OR e.ornament_idfile <> a.ornament_idfile
  OR e.ornament_hero_model <> a.ornament_hero_model;

-- a character's item is issued by that character at its origin world; an item no character holds carries a server stamp
SELECT 'issuer' AS what, e.source,
  SUM(e.owner_character IS NOT NULL AND a.guid_first = e.owner_character AND a.guid_second = cd.origin_world_id) AS by_owner,
  SUM(e.owner_character IS NULL AND a.guid_second > 65535) AS server_stamped,
  SUM(NOT ((e.owner_character IS NOT NULL AND a.guid_first = e.owner_character AND a.guid_second = cd.origin_world_id)
        OR (e.owner_character IS NULL AND a.guid_second > 65535))) AS wrong
FROM tst6.expected e JOIN tst6.actual a ON a.loc_key = e.loc_key
LEFT JOIN peq.character_data cd ON cd.id = e.owner_character
GROUP BY e.source;

SELECT 'counters' AS what, guid_first, COUNT(*) AS items, MIN(guid_third) AS lowest, MAX(guid_third) AS highest,
  MAX(guid_third) - MIN(guid_third) + 1 = COUNT(*) AS consecutive
FROM peq.item_instances WHERE guid_second <= 65535 GROUP BY guid_first;

SELECT 'server stamps' AS what, COUNT(*) AS n, COUNT(DISTINCT guid_second) AS distinct_counters, MIN(guid_second) AS first_counter,
  MAX(guid_second) - MIN(guid_second) + 1 = COUNT(*) AS consecutive, MIN(guid_third) AS stamp_time, MAX(guid_third) = MIN(guid_third) AS one_time,
  FROM_UNIXTIME(MIN(guid_third)) AS stamped_at
FROM peq.item_instances WHERE guid_second > 65535;

-- side tables
SELECT 'guild_bank' AS what, COUNT(*) AS rows_after,
  (SELECT COUNT(*) FROM tst6.guild_bank_place) AS rows_before,
  SUM(a.loc_key = p.loc_key) AS linked_to_its_item, SUM(g.donator <=> p.donator AND g.permissions = p.permissions AND g.who_for <=> p.who_for) AS details_kept
FROM peq.guild_bank g
LEFT JOIN tst6.guild_bank_place p ON p.id = g.id
LEFT JOIN tst6.actual a ON a.guid_first = g.guid_first AND a.guid_second = g.guid_second AND a.guid_third = g.guid_third;

SELECT 'parcels' AS what, COUNT(*) AS rows_after, (SELECT COUNT(*) FROM tst6.parcels) AS rows_before,
  SUM(b.item_id <> 99990 AND a.loc_key = CONCAT('parcel:', b.char_id, ':', b.slot_id)) AS item_linked,
  SUM(b.item_id = 99990 AND p.money = b.quantity AND p.guid_first IS NULL) AS money_kept,
  SUM(p.char_id = b.char_id AND p.slot_id = b.slot_id) AS place_kept
FROM peq.character_parcels p
JOIN tst6.parcels b ON b.id = p.id
LEFT JOIN tst6.actual a ON a.guid_first = p.guid_first AND a.guid_second = p.guid_second AND a.guid_third = p.guid_third;

SELECT 'evolving' AS what, b.id, b.character_id, b.item_id, b.copies, b.current_amount AS amount_before, e.current_amount AS amount_after,
  a.item_id AS linked_item, a.loc_key AS linked_place, x.owner_character AS linked_owner
FROM tst6.evolving b
LEFT JOIN peq.character_evolving_items e ON e.id = b.id
LEFT JOIN tst6.actual a ON a.guid_first = e.guid_first AND a.guid_second = e.guid_second AND a.guid_third = e.guid_third
LEFT JOIN tst6.expected x ON x.loc_key = a.loc_key;

SELECT 'merchantlist_temp' AS what, COUNT(*) AS rows_after, SUM(guid_first IS NULL) AS without_item,
  (SELECT n FROM tst6.counts WHERE what = 'merchantlist_temp') AS rows_before FROM peq.merchantlist_temp;
SELECT 'inventory_snapshots' AS what, COUNT(*) AS rows_after, (SELECT n FROM tst6.counts WHERE what = 'inventory_snapshots') AS rows_before FROM peq.inventory_snapshots;
SELECT 'object' AS what, COUNT(*) AS rows_after, (SELECT n FROM tst6.counts WHERE what = 'object') AS rows_before FROM peq.object;
SELECT 'trader' AS what, COUNT(*) AS rows_after FROM peq.trader;
SELECT 'old tables left' AS what, COUNT(*) AS n FROM information_schema.tables WHERE table_schema = 'peq'
  AND table_name IN ('inventory', 'sharedbank', 'character_corpse_items', 'character_pet_inventory', 'character_parcels_containers', 'object_contents');
SELECT 'origin worlds' AS what, origin_world_id, COUNT(*) AS characters FROM peq.character_data GROUP BY origin_world_id;
SELECT 'version' AS what, version, bots_version FROM peq.db_version;
