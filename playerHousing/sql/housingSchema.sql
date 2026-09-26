-- Player housing schema (serverPlan.md, chunk 3). Every column exists for a message or rule; anything derivable is derived
-- when sent (owner names, children, poll states, region matrices, the character's property lists, trophy flags).
-- Content tables: definitions, costs, regions, NPC items, neighborhood names, poll labels. The rest is player state.

CREATE TABLE IF NOT EXISTS `real_estate_definitions` (
  `id` INT(10) UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `type` TINYINT(3) UNSIGNED NOT NULL,
  `category` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `region_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `purchase_cost_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_cost_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `max_items` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `max_non_real_estate_items` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `max_pet_items` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `layout_origin_x` FLOAT NOT NULL DEFAULT 0,
  `layout_origin_y` FLOAT NOT NULL DEFAULT 0,
  `layout_origin_z` FLOAT NOT NULL DEFAULT 0,
  `layout_heading` FLOAT NOT NULL DEFAULT 0,
  `instance_zone_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `instance_zone_version` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `unknown_30` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `unknown_34` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `unknown_38` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `unknown_44` INT(10) UNSIGNED NOT NULL DEFAULT 336,
  `unknown_string` VARCHAR(600) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`),
  CONSTRAINT `real_estate_definition_type` CHECK (`type` BETWEEN 1 AND 8),
  CONSTRAINT `real_estate_definition_item_caps` CHECK (`max_non_real_estate_items` <= `max_items` AND `max_pet_items` <= `max_items`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_definition_children` (
  `parent_id` INT(10) UNSIGNED NOT NULL,
  `child_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`parent_id`, `child_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_costs` (
  `id` INT(10) UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The client uses an option key as the index into its cost element's options (0x7fb970), so each cost's keys are 0, 1, 2, ...
-- in order; the housing module refuses to load a cost whose keys have gaps.
CREATE TABLE IF NOT EXISTS `real_estate_cost_options` (
  `cost_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`cost_id`, `option_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_cost_items` (
  `cost_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  `currency` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `sub_key` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` BIGINT(20) NOT NULL DEFAULT 0,
  PRIMARY KEY (`cost_id`, `option_key`, `currency`, `sub_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_regions` (
  `id` INT(10) UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `x` FLOAT NOT NULL DEFAULT 0,
  `y` FLOAT NOT NULL DEFAULT 0,
  `z` FLOAT NOT NULL DEFAULT 0,
  `size_x` FLOAT NOT NULL DEFAULT 0,
  `size_y` FLOAT NOT NULL DEFAULT 0,
  `size_z` FLOAT NOT NULL DEFAULT 0,
  `heading` FLOAT NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  CONSTRAINT `real_estate_region_size` CHECK (`size_x` > 0 AND `size_y` > 0 AND `size_z` > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Names for the neighborhoods the server creates as others fill; a name is in use while a neighborhood property has it.
CREATE TABLE IF NOT EXISTS `real_estate_neighborhood_names` (
  `name` VARCHAR(64) NOT NULL,
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_npc_items` (
  `item_id` INT(11) UNSIGNED NOT NULL,
  `npc_type_id` INT(11) UNSIGNED NOT NULL,
  PRIMARY KEY (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_properties` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `parent_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `definition_id` INT(10) UNSIGNED NOT NULL,
  `guild_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `owner_character_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `host_zone_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `host_instance_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `interior_instance_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `contained_in_item_guid` BIGINT(20) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_option_key` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_current` TINYINT(3) UNSIGNED NOT NULL DEFAULT 1,
  `next_upkeep_at` DATETIME NULL DEFAULT NULL,
  `voting_opt_out` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `archived` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `revision` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  KEY `parent_id` (`parent_id`),
  KEY `owner_character_id` (`owner_character_id`),
  KEY `guild_id` (`guild_id`),
  KEY `host` (`host_zone_id`, `host_instance_id`),
  KEY `contained_in_item_guid` (`contained_in_item_guid`),
  KEY `next_upkeep_at` (`next_upkeep_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Upkeep is charged hourly, 1/24 of the daily cost at a time, so balances are fractional (the client keeps them as f64);
-- coin is in copper.
CREATE TABLE IF NOT EXISTS `real_estate_property_balances` (
  `property_id` INT(10) UNSIGNED NOT NULL,
  `currency` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` DECIMAL(24,6) NOT NULL DEFAULT 0,
  PRIMARY KEY (`property_id`, `currency`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_items` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `property_id` INT(10) UNSIGNED NOT NULL,
  `owner_character_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `placed` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `heading` FLOAT NOT NULL DEFAULT 0,
  `pitch` FLOAT NOT NULL DEFAULT 0,
  `roll` FLOAT NOT NULL DEFAULT 0,
  `scale` FLOAT NOT NULL DEFAULT 1,
  `x` FLOAT NOT NULL DEFAULT 0,
  `y` FLOAT NOT NULL DEFAULT 0,
  `z` FLOAT NOT NULL DEFAULT 0,
  `item_id` INT(11) UNSIGNED NOT NULL,
  `charges` SMALLINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `color` INT(11) UNSIGNED NOT NULL DEFAULT 0,
  `augment_one` MEDIUMINT(7) UNSIGNED NOT NULL DEFAULT 0,
  `augment_two` MEDIUMINT(7) UNSIGNED NOT NULL DEFAULT 0,
  `augment_three` MEDIUMINT(7) UNSIGNED NOT NULL DEFAULT 0,
  `augment_four` MEDIUMINT(7) UNSIGNED NOT NULL DEFAULT 0,
  `augment_five` MEDIUMINT(7) UNSIGNED NOT NULL DEFAULT 0,
  `augment_six` MEDIUMINT(7) UNSIGNED NOT NULL DEFAULT 0,
  `instnodrop` TINYINT(1) UNSIGNED NOT NULL DEFAULT 0,
  `custom_data` TEXT NULL,
  `ornament_icon` INT(11) UNSIGNED NOT NULL DEFAULT 0,
  `ornament_idfile` INT(11) UNSIGNED NOT NULL DEFAULT 0,
  `ornament_hero_model` INT(11) NOT NULL DEFAULT 0,
  `guid` BIGINT(20) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `property_id` (`property_id`),
  KEY `owner_character_id` (`owner_character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_access_groups` (
  `property_id` INT(10) UNSIGNED NOT NULL,
  `group_number` TINYINT(3) UNSIGNED NOT NULL,
  `permissions` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`property_id`, `group_number`),
  CONSTRAINT `real_estate_access_group_number` CHECK (`group_number` <= 6)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_access_names` (
  `property_id` INT(10) UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL,
  `group_number` TINYINT(3) UNSIGNED NOT NULL,
  PRIMARY KEY (`property_id`, `name`),
  CONSTRAINT `real_estate_access_name_group` CHECK (`group_number` <= 6)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_transfer_offers` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `property_id` INT(10) UNSIGNED NOT NULL,
  `from_character_id` INT(10) UNSIGNED NOT NULL,
  `to_character_id` INT(10) UNSIGNED NOT NULL,
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `expires_at` DATETIME NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `property_id` (`property_id`),
  KEY `to_character_id` (`to_character_id`),
  KEY `expires_at` (`expires_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_polls` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `kind` TINYINT(3) UNSIGNED NOT NULL,
  `question` VARCHAR(512) NOT NULL DEFAULT '',
  `polling_location` VARCHAR(128) NOT NULL DEFAULT '',
  `selection_type` TINYINT(3) UNSIGNED NOT NULL,
  `max_selections` INT(10) UNSIGNED NOT NULL DEFAULT 1,
  `opens_at` DATETIME NOT NULL,
  `closes_at` DATETIME NOT NULL,
  `published` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `archived` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `results_unpublished` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `prompt_players` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  CONSTRAINT `real_estate_poll_kind` CHECK (`kind` IN (1, 2)),
  CONSTRAINT `real_estate_poll_selection_type` CHECK (`selection_type` IN (1, 2, 3)),
  CONSTRAINT `real_estate_poll_dates` CHECK (`opens_at` < `closes_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_poll_options` (
  `poll_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  `position` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `unknown_value` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`poll_id`, `option_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_poll_labels` (
  `id` INT(10) UNSIGNED NOT NULL,
  `text` VARCHAR(256) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_poll_votes` (
  `poll_id` INT(10) UNSIGNED NOT NULL,
  `character_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  `voted_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`poll_id`, `character_id`, `option_key`),
  KEY `character_id` (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The property "Translocate Primary Anchor" (spell effect 438) sends a character to; the guild hall anchor is derived (the
-- guild's hall).
CREATE TABLE IF NOT EXISTS `character_real_estate_anchors` (
  `character_id` INT(10) UNSIGNED NOT NULL,
  `primary_property_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_trophy_tribute` (
  `character_id` INT(10) UNSIGNED NOT NULL,
  `slot` TINYINT(3) UNSIGNED NOT NULL,
  `tribute_id` INT(10) UNSIGNED NOT NULL,
  `tier` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `slot`),
  CONSTRAINT `character_trophy_tribute_slot` CHECK (`slot` <= 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `character_trophy_tribute_state` (
  `character_id` INT(10) UNSIGNED NOT NULL,
  `active` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `time_remaining` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `guild_opt_in` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `guild_trophy_tribute` (
  `guild_id` INT(10) UNSIGNED NOT NULL,
  `slot` TINYINT(3) UNSIGNED NOT NULL,
  `tribute_id` INT(10) UNSIGNED NOT NULL,
  `tier` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guild_id`, `slot`),
  CONSTRAINT `guild_trophy_tribute_slot` CHECK (`slot` <= 3)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `guild_trophy_tribute_state` (
  `guild_id` INT(10) UNSIGNED NOT NULL,
  `active` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `time_remaining` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `edit_lock_character_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `edit_lock_at` DATETIME NULL DEFAULT NULL,
  PRIMARY KEY (`guild_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
