#ifndef EQEMU_INVENTORY_SNAPSHOTS_REPOSITORY_H
#define EQEMU_INVENTORY_SNAPSHOTS_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "base/base_inventory_snapshots_repository.h"
#include "item_instances_repository.h"

class InventorySnapshotsRepository: public BaseInventorySnapshotsRepository {
public:

    /**
     * This file was auto generated and can be modified and extended upon
     *
     * Base repository methods are automatically
     * generated in the "base" version of this repository. The base repository
     * is immutable and to be left untouched, while methods in this class
     * are used as extension methods for more specific persistence-layer
     * accessors or mutators.
     *
     * Base Methods (Subject to be expanded upon in time)
     *
     * Note: Not all tables are designed appropriately to fit functionality with all base methods
     *
     * InsertOne
     * UpdateOne
     * DeleteOne
     * FindOne
     * GetWhere(std::string where_filter)
     * DeleteWhere(std::string where_filter)
     * InsertMany
     * All
     *
     * Example custom methods in a repository
     *
     * InventorySnapshotsRepository::GetByZoneAndVersion(int zone_id, int zone_version)
     * InventorySnapshotsRepository::GetWhereNeverExpires()
     * InventorySnapshotsRepository::GetWhereXAndY()
     * InventorySnapshotsRepository::DeleteWhereXAndY()
     *
     * Most of the above could be covered by base methods, but if you as a developer
     * find yourself re-using logic for other parts of the code, its best to just make a
     * method that can be re-used easily elsewhere especially if it can use a base repository
     * method and encapsulate filters there
     */

	// The holders a snapshot covers: those EQEmu's inventory rows held for the character.
	static std::string HolderFilter(uint32 character_id)
	{
		return fmt::format("holder_type IN ('inventory', 'cursor', 'bank', 'trade') AND holder_id = {}", character_id);
	}

	// A snapshot row copies an item row's values and location; it holds no retirement, since only items in play are taken.
	static std::vector<std::string> ItemColumns()
	{
		std::vector<std::string> columns;
		for (const auto &c : ItemInstancesRepository::Columns()) {
			if (c != "retirement_state" && c != "retired_at") {
				columns.push_back(c);
			}
		}

		return columns;
	}

	static bool Capture(Database &db, uint32 character_id, uint32 time_index)
	{
		const auto columns = Strings::Implode(", ", ItemColumns());

		auto results = db.QueryDatabase(
			fmt::format(
				"INSERT INTO {table} (time_index, charid, {columns}) "
				"WITH RECURSIVE held AS ("
				"SELECT {columns} FROM {items} WHERE retirement_state IS NULL AND ({filter}) "
				"UNION ALL "
				"SELECT {child_columns} FROM {items} AS child JOIN held ON child.parent_guid_first = held.guid_first "
				"AND child.parent_guid_second = held.guid_second AND child.parent_guid_third = held.guid_third "
				"WHERE child.retirement_state IS NULL"
				") SELECT {time_index}, {character_id}, {columns} FROM held",
				fmt::arg("table", TableName()),
				fmt::arg("columns", columns),
				fmt::arg("items", ItemInstancesRepository::TableName()),
				fmt::arg("filter", HolderFilter(character_id)),
				fmt::arg("child_columns", "child." + Strings::Implode(", child.", ItemColumns())),
				fmt::arg("time_index", time_index),
				fmt::arg("character_id", character_id)
			)
		);

		return results.Success();
	}

	static std::vector<ItemInstancesRepository::ItemInstances> GetItemRows(Database &db, uint32 character_id, uint32 time_index)
	{
		return ItemInstancesRepository::Collect(
			db.QueryDatabase(
				fmt::format(
					"SELECT {}, NULL, NULL FROM {} WHERE charid = {} AND time_index = {}",
					Strings::Implode(", ", ItemColumns()),
					TableName(),
					character_id,
					time_index
				)
			)
		);
	}

	static int64 CountInventorySnapshots(Database& db)
	{
		const std::string& query = "SELECT COUNT(*) FROM (SELECT * FROM `inventory_snapshots` a GROUP BY `charid`, `time_index`) b";

		auto results = db.QueryDatabase(query);

		if (!results.Success() || !results.RowCount()) {
			return -1;
		}

		auto row = results.begin();

		const int64 count = Strings::ToBigInt(row[0]);

		if (count > std::numeric_limits<int>::max()) {
			return -2;
		}

		if (count < 0) {
			return -3;
		}

		return count;
	}
};

#endif //EQEMU_INVENTORY_SNAPSHOTS_REPOSITORY_H
