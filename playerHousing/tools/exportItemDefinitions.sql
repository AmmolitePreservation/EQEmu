-- Read-only export for tools/makeHousingCatalogSql.py (saved as content/itemDefinitions.txt): every item naming a real
-- estate definition, grouped by definition, upkeep id and placeable mask, and EQEmu's alternate currencies with their items.
SET SESSION group_concat_max_len = 100000;
SET @migrated = (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'items' AND COLUMN_NAME = 'real_estate_definition_id');
SET @definition = IF(@migrated, 'real_estate_definition_id', 'UNK227');
SET @upkeep = IF(@migrated, 'real_estate_upkeep_id', 'UNK230');
SET @placeable = IF(@migrated, 'placeable', 'UNK224');
SET @q = CONCAT('SELECT ''items'' AS result, i.', @definition, ' AS definition_id, i.', @upkeep, ' AS upkeep_id, i.', @placeable,
  ' AS placeable, COUNT(*) AS items, MIN(i.id) AS first_item, GROUP_CONCAT(DISTINCT i.Name ORDER BY i.Name SEPARATOR '' | '') AS names',
  ' FROM items i WHERE i.', @definition, ' <> 0 GROUP BY i.', @definition, ', i.', @upkeep, ', i.', @placeable, ' ORDER BY 2, 3, 4');
PREPARE statement FROM @q; EXECUTE statement; DEALLOCATE PREPARE statement;
SELECT 'currencies' AS result, a.id AS alternate_currency_id, a.item_id, i.Name AS item_name FROM alternate_currency a LEFT JOIN items i ON i.id = a.item_id ORDER BY a.id;
