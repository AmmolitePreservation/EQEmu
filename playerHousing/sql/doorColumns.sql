-- Live's RealEstateDoorID on EQEmu's doors (DOR-19), as dz_switch_id is: the plot definition a plot border (160) or an
-- apartment entrance opens, 1 on a neighborhood-list door (159), 0 on a front door (161) and on every other door. A door
-- with a non-zero id, or of open type 159-161, is answered by housing; RoF2 gets the id in OP_SpawnDoor.
ALTER TABLE `doors`
  ADD COLUMN `real_estate_door_id` INT(10) UNSIGNED NOT NULL DEFAULT 0 AFTER `dz_switch_id`;
