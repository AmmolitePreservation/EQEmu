-- A property holds its items, placed and stored, as item_instances rows of the real_estate holder: the holder id is the
-- property's id and there is no slot, as on the ground. The location check is replaced to allow it.
ALTER TABLE `item_instances`
  MODIFY COLUMN `holder_type` ENUM('inventory', 'cursor', 'bank', 'shared_bank', 'corpse', 'parcel', 'pet', 'suspended_pet', 'bot', 'guild_bank_deposit', 'guild_bank_main', 'ground', 'trade', 'world_container', 'real_estate') NULL DEFAULT NULL,
  DROP CONSTRAINT IF EXISTS `item_instances_location`;
ALTER TABLE `item_instances` ADD CONSTRAINT `item_instances_location` CHECK (
  (
    `holder_type` IS NOT NULL AND `holder_id` IS NOT NULL AND (`slot` IS NULL) = (`holder_type` IN ('ground', 'real_estate')) AND
    `parent_guid_first` IS NULL AND `parent_guid_second` IS NULL AND `parent_guid_third` IS NULL AND `parent_index` IS NULL
  ) OR (
    `holder_type` IS NULL AND `holder_id` IS NULL AND `slot` IS NULL AND
    `parent_guid_first` IS NOT NULL AND `parent_guid_second` IS NOT NULL AND `parent_guid_third` IS NOT NULL AND `parent_index` IS NOT NULL
  )
);
