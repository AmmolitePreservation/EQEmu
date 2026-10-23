-- Player housing schema (serverPlan.md, chunk 3). Every column exists for a message or rule; anything derivable is derived
-- when sent (owner names, children, composed property names, poll states, region matrices, the character's property lists,
-- trophy flags, anchor targets). Trophy tribute lives on EQEmu's tribute tables (sql/*Columns.sql, TRO-8).
-- Content tables: definitions, costs, regions, zone templates, neighborhood words, NPC items and auras. The rest is player
-- state.

-- Columns take the names of Live's definition fields. cost_definition_id and upkeep_definition_id name real_estate_costs
-- (plots buy with one of five price tiers and pay that tier's upkeep; houses pay their own upkeep); placed items pay their
-- own upkeep ids. max_children is the client's +0x30: the houses and guild halls a yard holds. max_npc_items is -1 for no
-- cap. A property's name is composed when sent: its parent's composed name, then the plot's address or the definition's
-- name_segment. switch_x and switch_y are EQEmu x and y (the wire and Live's dumps put y first). A plot's exit point is
-- where its house's or hall's front door leaves to.
CREATE TABLE IF NOT EXISTS `real_estate_definitions` (
  `id` INT(10) UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `name_segment` VARCHAR(64) NOT NULL DEFAULT '',
  `type` TINYINT(3) UNSIGNED NOT NULL,
  `placement_group_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `area_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `cost_definition_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_definition_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `max_items` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `max_non_real_estate_items` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `max_npc_items` INT(11) NOT NULL DEFAULT -1,
  `switch_x` FLOAT NOT NULL DEFAULT 0,
  `switch_y` FLOAT NOT NULL DEFAULT 0,
  `switch_z` FLOAT NOT NULL DEFAULT 0,
  `switch_heading` FLOAT NOT NULL DEFAULT 0,
  `max_children` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `zone_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `dz_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `icon` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `address` VARCHAR(600) NOT NULL DEFAULT '',
  `exit_zone_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `exit_zone_version` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `exit_x` FLOAT NOT NULL DEFAULT 0,
  `exit_y` FLOAT NOT NULL DEFAULT 0,
  `exit_z` FLOAT NOT NULL DEFAULT 0,
  `exit_heading` FLOAT NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  CONSTRAINT `real_estate_definition_type` CHECK (`type` BETWEEN 1 AND 8),
  CONSTRAINT `real_estate_definition_item_caps` CHECK (`max_non_real_estate_items` <= `max_items` AND (`max_npc_items` = -1 OR `max_npc_items` BETWEEN 0 AND `max_items`)),
  CONSTRAINT `real_estate_definition_area` CHECK (`area_id` = 0 OR `type` IN (4, 8)),
  CONSTRAINT `real_estate_definition_name_segment` CHECK (`name_segment` = '' OR `type` NOT IN (1, 4, 5, 8)),
  CONSTRAINT `real_estate_definition_exit` CHECK ((`exit_zone_id` <> 0) = (`type` IN (4, 8)))
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

-- A cost's options are alternative ways to pay it, each gated by expansion like other EQEmu content (Live accepts Doubloons,
-- Orux and Phosphenes from TBS, Faycitum from SoF and Chronobines from SoD beside coin). option_key orders them; the client
-- uses an option's position as its index (0x7fb970), so the housing module numbers the enabled options 0, 1, 2, ... when sent.
CREATE TABLE IF NOT EXISTS `real_estate_cost_options` (
  `cost_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  `min_expansion` TINYINT(4) NOT NULL DEFAULT -1,
  `max_expansion` TINYINT(4) NOT NULL DEFAULT -1,
  `content_flags` VARCHAR(100) NULL DEFAULT NULL,
  `content_flags_disabled` VARCHAR(100) NULL DEFAULT NULL,
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

-- RoF2 keys its placement permissions by region id and falls back to the current property's id in the same map (0x4b2bf0),
-- so region ids stay below 100000, where property ids start.
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
  CONSTRAINT `real_estate_region_size` CHECK (`size_x` > 0 AND `size_y` > 0 AND `size_z` > 0),
  CONSTRAINT `real_estate_region_below_property_ids` CHECK (`id` < 100000)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The zone a definition's dz_id starts a copy of, and where entering puts a player. A copy is a plain instance, shared by
-- everyone inside, discarded some time after it empties.
CREATE TABLE IF NOT EXISTS `real_estate_zone_templates` (
  `dz_id` INT(10) UNSIGNED NOT NULL,
  `zone_id` INT(10) UNSIGNED NOT NULL,
  `zone_version` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `zone_in_x` FLOAT NOT NULL,
  `zone_in_y` FLOAT NOT NULL,
  `zone_in_z` FLOAT NOT NULL,
  `zone_in_heading` FLOAT NOT NULL,
  PRIMARY KEY (`dz_id`),
  CONSTRAINT `real_estate_zone_template_zone` CHECK (`dz_id` <> 0 AND `zone_id` <> 0),
  CONSTRAINT `real_estate_zone_template_heading` CHECK (`zone_in_heading` >= 0 AND `zone_in_heading` < 512)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The doors housing answers, keyed as the client keys them (a door id is a byte). real_estate_door_id is Live's RealEstateDoorID,
-- which RoF2 keeps but never reads: the plot definition a plot border (type 160) or an apartment entrance opens, 1 on a
-- neighborhood-list door (type 159), 0 on a front door (type 161).
-- An item that creates a guild neighborhood of a definition when used (Live's Guild Writ of the Sunrise makes Sunrise Hills).
-- Words the server combines into names for the neighborhoods it creates as others fill, as Live does ("Maple Bridge Point",
-- "Walnut Lake", "Circle Hills"): position 0 an optional tree, 1 an optional middle word, 2 the place word, and at least one of
-- the first two. A name is in use while a neighborhood property has it.
CREATE TABLE IF NOT EXISTS `real_estate_neighborhood_words` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `position` TINYINT(3) UNSIGNED NOT NULL,
  `word` VARCHAR(32) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `position_word` (`position`, `word`),
  CONSTRAINT `neighborhood_word_position` CHECK (`position` <= 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- An NPC item spawns its NPC at the placed transform; a familiar that rides a mount also spawns the mount, unbound (Emperor
-- Ganak's). race_tag is the three-character tag RoF2 spawns the placement preview from (0x504b60), since Live's IT63 names no
-- race there. A training dummy's menu offers Live's dummy actions; the action rows are a pet's (Live's b codes and order).
CREATE TABLE IF NOT EXISTS `real_estate_npc_items` (
  `item_id` INT(11) UNSIGNED NOT NULL,
  `npc_type_id` INT(11) UNSIGNED NOT NULL,
  `mount_npc_type_id` INT(11) UNSIGNED NOT NULL DEFAULT 0,
  `race_tag` CHAR(3) NOT NULL,
  `training_dummy` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`item_id`),
  CONSTRAINT `real_estate_npc_item_race_tag` CHECK (CHAR_LENGTH(`race_tag`) = 3),
  CONSTRAINT `real_estate_npc_item_dummy` CHECK (`training_dummy` IN (0, 1))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_npc_item_actions` (
  `item_id` INT(11) UNSIGNED NOT NULL,
  `action_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`item_id`, `action_id`),
  CONSTRAINT `real_estate_npc_item_action_item` FOREIGN KEY (`item_id`) REFERENCES `real_estate_npc_items` (`item_id`) ON DELETE CASCADE,
  CONSTRAINT `real_estate_npc_item_action_id` CHECK (`action_id` IN (16175, 16176, 16177, 16178, 16179, 16394, 16492))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- A placed item that keeps an aura about it (the Guild Regeneration Pool): the spell, the circle's radius from the item's
-- position, and the lines the server prints on walking in and out (Live's spell strings are empty).
CREATE TABLE IF NOT EXISTS `real_estate_auras` (
  `item_id` INT(11) UNSIGNED NOT NULL,
  `aura_type` INT(10) NOT NULL,
  PRIMARY KEY (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The keys a guild hall's Teleportation Assistant aligns its Device to: the destination's text and its landing. A guild
-- standard key goes to the guild's planted standard instead of a fixed point.
-- name holds only a neighborhood's generated name. Where a property is is derived, never stored: its root's definition names the
-- zone (dz_id), a zone root runs at instance 0 and any other root in the copy real_estate_instances maps to it. upkeep_currency is 0 (coin) or an alternate currency id: the record's
-- option key is derived from it when sent, since option positions move as the expansion enables options. Upkeep runs on a
-- clock per property: a charge at next_upkeep_at takes the daily cost prorated over the seconds since upkeep_settled_at;
-- upkeep_expired_time is the second a charge found the escrow empty (0 while paid). Ids start above every region id.
CREATE TABLE IF NOT EXISTS `real_estate_properties` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `parent_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `definition_id` INT(10) UNSIGNED NOT NULL,
  `guild_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `owner_character_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `contained_in_item` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_currency` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_expired_time` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_settled_at` DATETIME NULL DEFAULT NULL,
  `next_upkeep_at` DATETIME NULL DEFAULT NULL,
  `voting_opt_out` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `archived` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `revision` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`),
  KEY `parent_id` (`parent_id`),
  KEY `owner_character_id` (`owner_character_id`),
  KEY `guild_id` (`guild_id`),
  KEY `next_upkeep_at` (`next_upkeep_at`)
) ENGINE=InnoDB AUTO_INCREMENT=100000 DEFAULT CHARSET=utf8mb4;

-- A running copy of a property's template zone (an interior or a neighborhood): one per property, removed with its instance
-- when the copy is discarded.
CREATE TABLE IF NOT EXISTS `real_estate_instances` (
  `instance_id` INT(10) UNSIGNED NOT NULL,
  `property_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`instance_id`),
  UNIQUE KEY `property_id` (`property_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The instance id the next copy takes: copies count up as Live's do, skipping ids in use and wrapping past the last.
CREATE TABLE IF NOT EXISTS `real_estate_instance_counter` (
  `id` TINYINT(3) UNSIGNED NOT NULL,
  `next_instance_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `real_estate_instance_counter_single` CHECK (`id` = 1)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- A character saved inside a copy that was discarded, and the property it was a copy of: the next login enters a new copy.
-- Charges are prorated by the second, so balances are fractional (the client keeps them as f64); coin is in copper.
CREATE TABLE IF NOT EXISTS `real_estate_property_balances` (
  `property_id` INT(10) UNSIGNED NOT NULL,
  `currency` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` DECIMAL(24,6) NOT NULL DEFAULT 0,
  PRIMARY KEY (`property_id`, `currency`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- An item stored without ever being placed has Live's all-zero transform, so scale has no default. upkeep_expired_at is the
-- second an empty escrow stamped the item.
CREATE TABLE IF NOT EXISTS `real_estate_items` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `property_id` INT(10) UNSIGNED NOT NULL,
  `owner_character_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `placed` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `upkeep_expired_at` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `heading` FLOAT NOT NULL DEFAULT 0,
  `pitch` FLOAT NOT NULL DEFAULT 0,
  `roll` FLOAT NOT NULL DEFAULT 0,
  `scale` FLOAT NOT NULL,
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
  PRIMARY KEY (`id`),
  KEY `property_id` (`property_id`),
  KEY `owner_character_id` (`owner_character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The look the Augmentater gave one placed copy of an NPC item, kept until RESET or until the item is picked up (Live drops
-- it then), so a row exists only once the Augmentater was used on that copy.
CREATE TABLE IF NOT EXISTS `real_estate_item_looks` (
  `real_estate_item_id` INT(10) UNSIGNED NOT NULL,
  `race` SMALLINT(5) UNSIGNED NOT NULL,
  `gender` TINYINT(3) UNSIGNED NOT NULL,
  `face` TINYINT(3) UNSIGNED NOT NULL,
  `hair_style` TINYINT(3) UNSIGNED NOT NULL,
  `beard` TINYINT(3) UNSIGNED NOT NULL,
  `hair_color` TINYINT(3) UNSIGNED NOT NULL,
  `body_material` TINYINT(3) UNSIGNED NOT NULL,
  `body_tint` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`real_estate_item_id`),
  CONSTRAINT `real_estate_item_look_item` FOREIGN KEY (`real_estate_item_id`) REFERENCES `real_estate_items` (`id`) ON DELETE CASCADE,
  CONSTRAINT `real_estate_item_look_race` CHECK (`race` > 0),
  CONSTRAINT `real_estate_item_look_gender` CHECK (`gender` <= 2)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- A placed aura item whose owner deactivated its aura; a row only while it is off, gone with the placed row.
CREATE TABLE IF NOT EXISTS `real_estate_inactive_auras` (
  `real_estate_item_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`real_estate_item_id`),
  CONSTRAINT `real_estate_inactive_aura_item` FOREIGN KEY (`real_estate_item_id`) REFERENCES `real_estate_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- The key a guild hall's Teleportation Device was last aligned to; it lasts until the next key (Live).
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
  CONSTRAINT `real_estate_access_name_group` CHECK (`group_number` BETWEEN 1 AND 5)
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
  PRIMARY KEY (`poll_id`, `option_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_poll_labels` (
  `id` INT(10) UNSIGNED NOT NULL,
  `text` VARCHAR(256) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS `real_estate_poll_votes` (
  `poll_id` INT(10) UNSIGNED NOT NULL,
  `account_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`poll_id`, `account_id`, `option_key`),
  KEY `account_id` (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
