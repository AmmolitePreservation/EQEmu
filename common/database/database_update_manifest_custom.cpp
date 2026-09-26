#include "database_update.h"

std::vector<ManifestEntry> manifest_entries_custom = {
	ManifestEntry{
		.version = 1,
		.description = "2026_09_26_items_housing_tail",
		.check = "SHOW COLUMNS FROM `items` LIKE 'UNK224'",
		.condition = "not_empty",
		.match = "",
		.sql =
			R"(CREATE TABLE IF NOT EXISTS `items_housing_capture` (PRIMARY KEY (`id`)) AS SELECT `id`, `UNK224`, `UNK225`, `UNK226`, `UNK227`, `UNK228`, `UNK229`, `UNK230`, `UNK231`, `UNK232`, `UNK233`, `UNK234`, `placeable`, `UNK236`, `UNK237`, `UNK238`, `UNK239`, `UNK240`, `UNK241` FROM `items`;
)"
			R"(ALTER TABLE `items` ADD COLUMN IF NOT EXISTS `hc_s_trophy` BIGINT NULL, ADD COLUMN IF NOT EXISTS `hc_s_free` BIGINT NULL, ADD COLUMN IF NOT EXISTS `hc_s_respawn` BIGINT NULL, ADD COLUMN IF NOT EXISTS `hc_s_scale` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_s_heading` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_s_pitch` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_s_roll` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_s_interactive` BIGINT NULL, ADD COLUMN IF NOT EXISTS `hc_a_scale` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_a_heading` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_a_pitch` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_a_roll` DOUBLE NULL, ADD COLUMN IF NOT EXISTS `hc_layout` TINYINT NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable` INT(11) NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable_ignore_collisions` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placement_type` INT(11) NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_real_estate_definition_id` INT(11) NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable_scale_min` FLOAT NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable_scale_max` FLOAT NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_real_estate_upkeep_id` INT(11) NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_max_per_real_estate` INT(11) NOT NULL DEFAULT -1, ADD COLUMN IF NOT EXISTS `hd_housepet_file_name` VARCHAR(31) NOT NULL DEFAULT '', ADD COLUMN IF NOT EXISTS `hd_trophy_benefit_id` INT(11) NOT NULL DEFAULT -1, ADD COLUMN IF NOT EXISTS `hd_disable_placement_rotation` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_disable_free_placement` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_npc_respawn_interval` INT(10) UNSIGNED NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable_default_scale` FLOAT NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable_default_heading` FLOAT NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable_default_pitch` FLOAT NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_placeable_default_roll` FLOAT NOT NULL DEFAULT 0, ADD COLUMN IF NOT EXISTS `hd_interactive_object` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0;
)"
			R"(UPDATE `items` i JOIN `items_housing_capture` c ON c.`id` = i.`id` SET i.`hc_s_trophy` = (CAST((((c.`UNK233` & 0xFFFFFFFF) >> 8) | (((c.`UNK234` >> 0) & 0xFF) << 24)) AS SIGNED) - IF((((c.`UNK233` & 0xFFFFFFFF) >> 8) | (((c.`UNK234` >> 0) & 0xFF) << 24)) >= 2147483648, 4294967296, 0)), i.`hc_s_free` = ((c.`UNK236` >> 0) & 0xFF), i.`hc_s_respawn` = (((c.`UNK236` & 0xFFFFFFFF) >> 8) | (((c.`UNK237` >> 0) & 0xFF) << 24)), i.`hc_s_scale` = (CASE WHEN CAST((((((c.`UNK237` & 0xFFFFFFFF) >> 8) | (((c.`UNK238` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((((c.`UNK237` & 0xFFFFFFFF) >> 8) | (((c.`UNK238` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((((c.`UNK237` & 0xFFFFFFFF) >> 8) | (((c.`UNK238` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * CAST(((((c.`UNK237` & 0xFFFFFFFF) >> 8) | (((c.`UNK238` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((((c.`UNK237` & 0xFFFFFFFF) >> 8) | (((c.`UNK238` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * (1 + CAST(((((c.`UNK237` & 0xFFFFFFFF) >> 8) | (((c.`UNK238` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((((c.`UNK237` & 0xFFFFFFFF) >> 8) | (((c.`UNK238` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) - 127) END), i.`hc_s_heading` = (CASE WHEN CAST((((((c.`UNK238` & 0xFFFFFFFF) >> 8) | (((c.`UNK239` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((((c.`UNK238` & 0xFFFFFFFF) >> 8) | (((c.`UNK239` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((((c.`UNK238` & 0xFFFFFFFF) >> 8) | (((c.`UNK239` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * CAST(((((c.`UNK238` & 0xFFFFFFFF) >> 8) | (((c.`UNK239` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((((c.`UNK238` & 0xFFFFFFFF) >> 8) | (((c.`UNK239` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * (1 + CAST(((((c.`UNK238` & 0xFFFFFFFF) >> 8) | (((c.`UNK239` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((((c.`UNK238` & 0xFFFFFFFF) >> 8) | (((c.`UNK239` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) - 127) END), i.`hc_s_pitch` = (CASE WHEN CAST((((((c.`UNK239` & 0xFFFFFFFF) >> 8) | (((c.`UNK240` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((((c.`UNK239` & 0xFFFFFFFF) >> 8) | (((c.`UNK240` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((((c.`UNK239` & 0xFFFFFFFF) >> 8) | (((c.`UNK240` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * CAST(((((c.`UNK239` & 0xFFFFFFFF) >> 8) | (((c.`UNK240` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((((c.`UNK239` & 0xFFFFFFFF) >> 8) | (((c.`UNK240` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * (1 + CAST(((((c.`UNK239` & 0xFFFFFFFF) >> 8) | (((c.`UNK240` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((((c.`UNK239` & 0xFFFFFFFF) >> 8) | (((c.`UNK240` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) - 127) END), i.`hc_s_roll` = (CASE WHEN CAST((((((c.`UNK240` & 0xFFFFFFFF) >> 8) | (((c.`UNK241` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((((c.`UNK240` & 0xFFFFFFFF) >> 8) | (((c.`UNK241` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((((c.`UNK240` & 0xFFFFFFFF) >> 8) | (((c.`UNK241` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * CAST(((((c.`UNK240` & 0xFFFFFFFF) >> 8) | (((c.`UNK241` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((((c.`UNK240` & 0xFFFFFFFF) >> 8) | (((c.`UNK241` >> 0) & 0xFF) << 24)) >= 2147483648, -1, 1) * (1 + CAST(((((c.`UNK240` & 0xFFFFFFFF) >> 8) | (((c.`UNK241` >> 0) & 0xFF) << 24)) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((((c.`UNK240` & 0xFFFFFFFF) >> 8) | (((c.`UNK241` >> 0) & 0xFF) << 24)) >> 23) & 0xFF) AS SIGNED) - 127) END), i.`hc_s_interactive` = ((c.`UNK233` >> 0) & 0xFF), i.`hc_a_scale` = (CASE WHEN CAST((((c.`UNK237` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK237` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK237` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK237` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK237` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK237` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK237` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END), i.`hc_a_heading` = (CASE WHEN CAST((((c.`UNK238` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK238` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK238` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK238` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK238` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK238` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK238` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END), i.`hc_a_pitch` = (CASE WHEN CAST((((c.`UNK239` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK239` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK239` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK239` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK239` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK239` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK239` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END), i.`hc_a_roll` = (CASE WHEN CAST((((c.`UNK240` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK240` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK240` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK240` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK240` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK240` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK240` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END) WHERE c.`UNK224` <> 0;
)"
			R"(UPDATE `items` i JOIN `items_housing_capture` c ON c.`id` = i.`id` SET i.`hc_layout` = CASE WHEN COALESCE((c.`UNK233` = -1 OR (c.`UNK233` > 0 AND c.`UNK233` < 100000)) AND c.`UNK234` IN (0, 1) AND c.`placeable` IN (0, 1) AND (c.`UNK236` & 0xFFFFFFFF) <= 100000000 AND i.`hc_a_scale` BETWEEN 0.01 AND 50 AND i.`hc_a_heading` BETWEEN -1000 AND 1000 AND i.`hc_a_pitch` BETWEEN -1000 AND 1000 AND i.`hc_a_roll` BETWEEN -1000 AND 1000 AND c.`UNK241` IN (0, 1), FALSE) AND NOT COALESCE((i.`hc_s_trophy` = -1 OR (i.`hc_s_trophy` > 0 AND i.`hc_s_trophy` < 100000)) AND c.`placeable` IN (0, 1) AND i.`hc_s_free` IN (0, 1) AND i.`hc_s_respawn` <= 100000000 AND i.`hc_s_scale` BETWEEN 0.01 AND 50 AND i.`hc_s_heading` BETWEEN -1000 AND 1000 AND i.`hc_s_pitch` BETWEEN -1000 AND 1000 AND i.`hc_s_roll` BETWEEN -1000 AND 1000 AND i.`hc_s_interactive` IN (0, 1), FALSE) THEN 1 WHEN COALESCE((i.`hc_s_trophy` = -1 OR (i.`hc_s_trophy` > 0 AND i.`hc_s_trophy` < 100000)) AND c.`placeable` IN (0, 1) AND i.`hc_s_free` IN (0, 1) AND i.`hc_s_respawn` <= 100000000 AND i.`hc_s_scale` BETWEEN 0.01 AND 50 AND i.`hc_s_heading` BETWEEN -1000 AND 1000 AND i.`hc_s_pitch` BETWEEN -1000 AND 1000 AND i.`hc_s_roll` BETWEEN -1000 AND 1000 AND i.`hc_s_interactive` IN (0, 1), FALSE) AND NOT COALESCE((c.`UNK233` = -1 OR (c.`UNK233` > 0 AND c.`UNK233` < 100000)) AND c.`UNK234` IN (0, 1) AND c.`placeable` IN (0, 1) AND (c.`UNK236` & 0xFFFFFFFF) <= 100000000 AND i.`hc_a_scale` BETWEEN 0.01 AND 50 AND i.`hc_a_heading` BETWEEN -1000 AND 1000 AND i.`hc_a_pitch` BETWEEN -1000 AND 1000 AND i.`hc_a_roll` BETWEEN -1000 AND 1000 AND c.`UNK241` IN (0, 1), FALSE) THEN 2 ELSE 3 END WHERE c.`UNK224` <> 0;
)"
			R"(CREATE TEMPORARY TABLE IF NOT EXISTS `hc_guard` (`id` INT NOT NULL, `problem` VARCHAR(48) NOT NULL, CONSTRAINT `placeable_item_without_one_plausible_layout` CHECK (`problem` <> 'placeable_item_without_one_plausible_layout'), CONSTRAINT `housepet_capture_not_empty` CHECK (`problem` <> 'housepet_capture_not_empty'), CONSTRAINT `non_placeable_item_outside_known_patterns` CHECK (`problem` <> 'non_placeable_item_outside_known_patterns'));
)"
			R"(INSERT INTO `hc_guard` SELECT i.`id`, 'placeable_item_without_one_plausible_layout' FROM `items` i JOIN `items_housing_capture` c ON c.`id` = i.`id` WHERE c.`UNK224` <> 0 AND i.`hc_layout` = 3;
)"
			R"(INSERT INTO `hc_guard` SELECT `id`, 'housepet_capture_not_empty' FROM `items_housing_capture` WHERE `UNK232` <> 0;
)"
			R"(INSERT INTO `hc_guard` SELECT `id`, 'non_placeable_item_outside_known_patterns' FROM `items_housing_capture` WHERE `UNK224` = 0 AND NOT ((`UNK225` = 0 AND `UNK226` = 0 AND `UNK227` = 0 AND `UNK228` = 0 AND `UNK229` = 0 AND `UNK230` = 0 AND `UNK231` = 0 AND `UNK232` = 0 AND `UNK233` = 0 AND `UNK234` = 0 AND `placeable` = 0 AND `UNK236` = 0 AND `UNK237` = 0 AND `UNK238` = 0 AND `UNK239` = 0 AND `UNK240` = 0 AND `UNK241` = 0) OR (`UNK225` = 0 AND `UNK226` = 0 AND `UNK227` = 0 AND `UNK228` = 0 AND `UNK229` = 0 AND `UNK230` = 0 AND `UNK231` = -1 AND `UNK232` = 0 AND `UNK233` = -256 AND `UNK234` = 255 AND `placeable` = 0 AND `UNK236` = 0 AND `UNK237` = 0 AND `UNK238` = 0 AND `UNK239` = 0 AND `UNK240` = 0 AND `UNK241` = 0) OR (`UNK225` = 0 AND `UNK226` = 0 AND `UNK227` = 0 AND `UNK228` = 0 AND `UNK229` = 0 AND `UNK230` = 0 AND `UNK231` = -1 AND `UNK232` = 0 AND `UNK233` = -1 AND `UNK234` = 0 AND `placeable` = 0 AND `UNK236` = 0 AND `UNK237` = 0 AND `UNK238` = 0 AND `UNK239` = 0 AND `UNK240` = 0 AND `UNK241` = 0));
)"
			R"(DROP TEMPORARY TABLE `hc_guard`;
)"
			R"(UPDATE `items` i JOIN `items_housing_capture` c ON c.`id` = i.`id` SET i.`hd_placeable` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`UNK224` ELSE c.`UNK224` END, i.`hd_placeable_ignore_collisions` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`UNK225` ELSE c.`UNK225` END, i.`hd_placement_type` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`UNK226` ELSE c.`UNK226` END, i.`hd_real_estate_definition_id` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`UNK227` ELSE c.`UNK227` END, i.`hd_placeable_scale_min` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN (CASE WHEN CAST((((c.`UNK228` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK228` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK228` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK228` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK228` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK228` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK228` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END) ELSE (CASE WHEN CAST((((c.`UNK228` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK228` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK228` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK228` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK228` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK228` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK228` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END) END, i.`hd_placeable_scale_max` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN (CASE WHEN CAST((((c.`UNK229` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK229` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK229` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK229` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK229` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK229` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK229` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END) ELSE (CASE WHEN CAST((((c.`UNK229` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 255 THEN NULL WHEN CAST((((c.`UNK229` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) = 0 THEN IF((c.`UNK229` & 0xFFFFFFFF) >= 2147483648, -1, 1) * CAST(((c.`UNK229` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) * POW(2, -149) ELSE IF((c.`UNK229` & 0xFFFFFFFF) >= 2147483648, -1, 1) * (1 + CAST(((c.`UNK229` & 0xFFFFFFFF) & 0x7FFFFF) AS SIGNED) / 8388608) * POW(2, CAST((((c.`UNK229` & 0xFFFFFFFF) >> 23) & 0xFF) AS SIGNED) - 127) END) END, i.`hd_real_estate_upkeep_id` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`UNK230` ELSE c.`UNK230` END, i.`hd_max_per_real_estate` = CASE WHEN c.`UNK224` = 0 THEN -1 WHEN i.`hc_layout` = 1 THEN c.`UNK231` ELSE c.`UNK231` END, i.`hd_housepet_file_name` = CASE WHEN c.`UNK224` = 0 THEN '' WHEN i.`hc_layout` = 1 THEN '' ELSE '' END, i.`hd_trophy_benefit_id` = CASE WHEN c.`UNK224` = 0 THEN -1 WHEN i.`hc_layout` = 1 THEN c.`UNK233` ELSE i.`hc_s_trophy` END, i.`hd_disable_placement_rotation` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`UNK234` ELSE c.`placeable` END, i.`hd_disable_free_placement` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`placeable` ELSE i.`hc_s_free` END, i.`hd_npc_respawn_interval` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN (c.`UNK236` & 0xFFFFFFFF) ELSE i.`hc_s_respawn` END, i.`hd_placeable_default_scale` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN i.`hc_a_scale` ELSE i.`hc_s_scale` END, i.`hd_placeable_default_heading` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN i.`hc_a_heading` ELSE i.`hc_s_heading` END, i.`hd_placeable_default_pitch` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN i.`hc_a_pitch` ELSE i.`hc_s_pitch` END, i.`hd_placeable_default_roll` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN i.`hc_a_roll` ELSE i.`hc_s_roll` END, i.`hd_interactive_object` = CASE WHEN c.`UNK224` = 0 THEN 0 WHEN i.`hc_layout` = 1 THEN c.`UNK241` ELSE i.`hc_s_interactive` END;
)"
			R"(ALTER TABLE `items` CHANGE COLUMN IF EXISTS `placeable` `disable_free_placement` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0;
)"
			R"(ALTER TABLE `items` MODIFY COLUMN `UNK224` INT(11) NOT NULL DEFAULT 0, MODIFY COLUMN `UNK225` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, MODIFY COLUMN `UNK226` INT(11) NOT NULL DEFAULT 0, MODIFY COLUMN `UNK227` INT(11) NOT NULL DEFAULT 0, MODIFY COLUMN `UNK228` FLOAT NOT NULL DEFAULT 0, MODIFY COLUMN `UNK229` FLOAT NOT NULL DEFAULT 0, MODIFY COLUMN `UNK230` INT(11) NOT NULL DEFAULT 0, MODIFY COLUMN `UNK231` INT(11) NOT NULL DEFAULT -1, MODIFY COLUMN `UNK232` VARCHAR(31) NOT NULL DEFAULT '', MODIFY COLUMN `UNK233` INT(11) NOT NULL DEFAULT -1, MODIFY COLUMN `UNK234` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, MODIFY COLUMN `UNK236` INT(10) UNSIGNED NOT NULL DEFAULT 0, MODIFY COLUMN `UNK237` FLOAT NOT NULL DEFAULT 0, MODIFY COLUMN `UNK238` FLOAT NOT NULL DEFAULT 0, MODIFY COLUMN `UNK239` FLOAT NOT NULL DEFAULT 0, MODIFY COLUMN `UNK240` FLOAT NOT NULL DEFAULT 0, MODIFY COLUMN `UNK241` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0;
)"
			R"(UPDATE `items` SET `disable_free_placement` = `hd_disable_free_placement`, `UNK224` = `hd_placeable`, `UNK225` = `hd_placeable_ignore_collisions`, `UNK226` = `hd_placement_type`, `UNK227` = `hd_real_estate_definition_id`, `UNK228` = `hd_placeable_scale_min`, `UNK229` = `hd_placeable_scale_max`, `UNK230` = `hd_real_estate_upkeep_id`, `UNK231` = `hd_max_per_real_estate`, `UNK232` = `hd_housepet_file_name`, `UNK233` = `hd_trophy_benefit_id`, `UNK234` = `hd_disable_placement_rotation`, `UNK236` = `hd_npc_respawn_interval`, `UNK237` = `hd_placeable_default_scale`, `UNK238` = `hd_placeable_default_heading`, `UNK239` = `hd_placeable_default_pitch`, `UNK240` = `hd_placeable_default_roll`, `UNK241` = `hd_interactive_object`;
)"
			R"(ALTER TABLE `items` CHANGE COLUMN `UNK224` `placeable` INT(11) NOT NULL DEFAULT 0, CHANGE COLUMN `UNK225` `placeable_ignore_collisions` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, CHANGE COLUMN `UNK226` `placement_type` INT(11) NOT NULL DEFAULT 0, CHANGE COLUMN `UNK227` `real_estate_definition_id` INT(11) NOT NULL DEFAULT 0, CHANGE COLUMN `UNK228` `placeable_scale_min` FLOAT NOT NULL DEFAULT 0, CHANGE COLUMN `UNK229` `placeable_scale_max` FLOAT NOT NULL DEFAULT 0, CHANGE COLUMN `UNK230` `real_estate_upkeep_id` INT(11) NOT NULL DEFAULT 0, CHANGE COLUMN `UNK231` `max_per_real_estate` INT(11) NOT NULL DEFAULT -1, CHANGE COLUMN `UNK232` `housepet_file_name` VARCHAR(31) NOT NULL DEFAULT '', CHANGE COLUMN `UNK233` `trophy_benefit_id` INT(11) NOT NULL DEFAULT -1, CHANGE COLUMN `UNK234` `disable_placement_rotation` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, CHANGE COLUMN `UNK236` `npc_respawn_interval` INT(10) UNSIGNED NOT NULL DEFAULT 0, CHANGE COLUMN `UNK237` `placeable_default_scale` FLOAT NOT NULL DEFAULT 0, CHANGE COLUMN `UNK238` `placeable_default_heading` FLOAT NOT NULL DEFAULT 0, CHANGE COLUMN `UNK239` `placeable_default_pitch` FLOAT NOT NULL DEFAULT 0, CHANGE COLUMN `UNK240` `placeable_default_roll` FLOAT NOT NULL DEFAULT 0, CHANGE COLUMN `UNK241` `interactive_object` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0, DROP COLUMN `hc_s_trophy`, DROP COLUMN `hc_s_free`, DROP COLUMN `hc_s_respawn`, DROP COLUMN `hc_s_scale`, DROP COLUMN `hc_s_heading`, DROP COLUMN `hc_s_pitch`, DROP COLUMN `hc_s_roll`, DROP COLUMN `hc_s_interactive`, DROP COLUMN `hc_a_scale`, DROP COLUMN `hc_a_heading`, DROP COLUMN `hc_a_pitch`, DROP COLUMN `hc_a_roll`, DROP COLUMN `hc_layout`, DROP COLUMN `hd_placeable`, DROP COLUMN `hd_placeable_ignore_collisions`, DROP COLUMN `hd_placement_type`, DROP COLUMN `hd_real_estate_definition_id`, DROP COLUMN `hd_placeable_scale_min`, DROP COLUMN `hd_placeable_scale_max`, DROP COLUMN `hd_real_estate_upkeep_id`, DROP COLUMN `hd_max_per_real_estate`, DROP COLUMN `hd_housepet_file_name`, DROP COLUMN `hd_trophy_benefit_id`, DROP COLUMN `hd_disable_placement_rotation`, DROP COLUMN `hd_disable_free_placement`, DROP COLUMN `hd_npc_respawn_interval`, DROP COLUMN `hd_placeable_default_scale`, DROP COLUMN `hd_placeable_default_heading`, DROP COLUMN `hd_placeable_default_pitch`, DROP COLUMN `hd_placeable_default_roll`, DROP COLUMN `hd_interactive_object`;;
)",
		.content_schema_update = true,
	},
	ManifestEntry{
		.version = 2,
		.description = "2026_09_26_real_estate_catalog",
		.check = "SHOW TABLES LIKE 'real_estate_npc_items'",
		.condition = "empty",
		.match = "",
		.sql =
			R"(CREATE TABLE IF NOT EXISTS `real_estate_definitions` (
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
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_definition_children` (
  `parent_id` INT(10) UNSIGNED NOT NULL,
  `child_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`parent_id`, `child_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_costs` (
  `id` INT(10) UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_cost_options` (
  `cost_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`cost_id`, `option_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_cost_items` (
  `cost_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  `currency` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `sub_key` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` BIGINT(20) NOT NULL DEFAULT 0,
  PRIMARY KEY (`cost_id`, `option_key`, `currency`, `sub_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_regions` (
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
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_neighborhood_names` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(64) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_npc_items` (
  `item_id` INT(11) UNSIGNED NOT NULL,
  `npc_type_id` INT(11) UNSIGNED NOT NULL,
  PRIMARY KEY (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)",
		.content_schema_update = true,
	},
	ManifestEntry{
		.version = 3,
		.description = "2026_09_26_real_estate_state",
		.check = "SHOW TABLES LIKE 'guild_trophy_tribute_state'",
		.condition = "empty",
		.match = "",
		.sql =
			R"(CREATE TABLE IF NOT EXISTS `real_estate_properties` (
  `id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
  `parent_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `definition_id` INT(10) UNSIGNED NOT NULL,
  `guild_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `owner_character_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `name` VARCHAR(64) NOT NULL DEFAULT '',
  `host_zone_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `host_instance_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `interior_instance_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `contained_in_item` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
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
  KEY `next_upkeep_at` (`next_upkeep_at`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_property_balances` (
  `property_id` INT(10) UNSIGNED NOT NULL,
  `currency` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` DECIMAL(24,6) NOT NULL DEFAULT 0,
  PRIMARY KEY (`property_id`, `currency`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_items` (
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
  PRIMARY KEY (`id`),
  KEY `property_id` (`property_id`),
  KEY `owner_character_id` (`owner_character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_access_groups` (
  `property_id` INT(10) UNSIGNED NOT NULL,
  `group_number` TINYINT(3) UNSIGNED NOT NULL,
  `permissions` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`property_id`, `group_number`),
  CONSTRAINT `real_estate_access_group_number` CHECK (`group_number` <= 6)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_access_names` (
  `property_id` INT(10) UNSIGNED NOT NULL,
  `name` VARCHAR(64) NOT NULL,
  `group_number` TINYINT(3) UNSIGNED NOT NULL,
  PRIMARY KEY (`property_id`, `name`),
  CONSTRAINT `real_estate_access_name_group` CHECK (`group_number` <= 6)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_transfer_offers` (
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
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_polls` (
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
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_poll_options` (
  `poll_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  `position` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `unknown_value` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`poll_id`, `option_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_poll_labels` (
  `id` INT(10) UNSIGNED NOT NULL,
  `text` VARCHAR(256) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `real_estate_poll_votes` (
  `poll_id` INT(10) UNSIGNED NOT NULL,
  `character_id` INT(10) UNSIGNED NOT NULL,
  `option_key` INT(10) UNSIGNED NOT NULL,
  `voted_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`poll_id`, `character_id`, `option_key`),
  KEY `character_id` (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `character_real_estate_anchors` (
  `character_id` INT(10) UNSIGNED NOT NULL,
  `primary_property_id` INT(10) UNSIGNED NOT NULL,
  PRIMARY KEY (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `character_trophy_tribute` (
  `character_id` INT(10) UNSIGNED NOT NULL,
  `slot` TINYINT(3) UNSIGNED NOT NULL,
  `tribute_id` INT(10) UNSIGNED NOT NULL,
  `tier` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`, `slot`),
  CONSTRAINT `character_trophy_tribute_slot` CHECK (`slot` <= 9)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `character_trophy_tribute_state` (
  `character_id` INT(10) UNSIGNED NOT NULL,
  `active` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `time_remaining` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `guild_opt_in` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`character_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `guild_trophy_tribute` (
  `guild_id` INT(10) UNSIGNED NOT NULL,
  `slot` TINYINT(3) UNSIGNED NOT NULL,
  `tribute_id` INT(10) UNSIGNED NOT NULL,
  `tier` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`guild_id`, `slot`),
  CONSTRAINT `guild_trophy_tribute_slot` CHECK (`slot` <= 3)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)"
			R"(CREATE TABLE IF NOT EXISTS `guild_trophy_tribute_state` (
  `guild_id` INT(10) UNSIGNED NOT NULL,
  `active` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
  `time_remaining` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `edit_lock_character_id` INT(10) UNSIGNED NOT NULL DEFAULT 0,
  `edit_lock_at` DATETIME NULL DEFAULT NULL,
  PRIMARY KEY (`guild_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)",
		.content_schema_update = false,
	},
// Used for testing
//	ManifestEntry{
//		.version = 9229,
//		.description = "new_database_check_test",
//		.check = "SHOW TABLES LIKE 'new_table'",
//		.condition = "empty",
//		.match = "",
//		.sql = R"(
//CREATE TABLE `new_table`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//CREATE TABLE `new_table1`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//CREATE TABLE `new_table2`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//CREATE TABLE `new_table3`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//)",
//	}

};

// see struct definitions for what each field does
// struct ManifestEntry {
// 	int         version{};     // database version of the migration
// 	std::string description{}; // description of the migration ex: "add_new_table" or "add_index_to_table"
// 	std::string check{};       // query that checks against the condition
// 	std::string condition{};   // condition or "match_type" - Possible values [contains|match|missing|empty|not_empty]
// 	std::string match{};       // match field that is not always used, but works in conjunction with "condition" values [missing|match|contains]
// 	std::string sql{};         // the SQL DDL that gets ran when the condition is true
// };
