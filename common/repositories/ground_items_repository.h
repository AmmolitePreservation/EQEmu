#ifndef EQEMU_GROUND_ITEMS_REPOSITORY_H
#define EQEMU_GROUND_ITEMS_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "base/base_ground_items_repository.h"
#include "../item_guid.h"
#include "../item_location.h"

class GroundItemsRepository: public BaseGroundItemsRepository {
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
     * GroundItemsRepository::GetByZoneAndVersion(int zone_id, int zone_version)
     * GroundItemsRepository::GetWhereNeverExpires()
     * GroundItemsRepository::GetWhereXAndY()
     * GroundItemsRepository::DeleteWhereXAndY()
     *
     * Most of the above could be covered by base methods, but if you as a developer
     * find yourself re-using logic for other parts of the code, its best to just make a
     * method that can be re-used easily elsewhere especially if it can use a base repository
     * method and encapsulate filters there
     */

	// The key is the whole GUID; the base methods keyed by guid_first alone are not used.
	static bool Place(Database &db, const ItemGUID &guid, uint32 instance_id, float x, float y, float z, float heading)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"INSERT INTO {} (guid_first, guid_second, guid_third, instance_id, x, y, z, heading) VALUES ({}, {}, {}, {}, {}, {}, {}, {})",
				TableName(),
				guid.first,
				guid.second,
				guid.third,
				instance_id,
				x,
				y,
				z,
				heading
			)
		);

		return results.Success();
	}

	static bool Remove(Database &db, const ItemGUID &guid)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE guid_first = {} AND guid_second = {} AND guid_third = {}",
				TableName(),
				guid.first,
				guid.second,
				guid.third
			)
		);

		return results.Success();
	}

	// A zone that died took its dropped items with it: whatever it left marked ground in this zone and instance decayed.
	static int RetireStranded(Database &db, uint32 zone_id, uint32 instance_id)
	{
		const auto join = fmt::format(
			"{} g JOIN item_instances i ON i.guid_first = g.guid_first AND i.guid_second = g.guid_second AND i.guid_third = g.guid_third",
			TableName()
		);
		const auto where = fmt::format(
			"i.holder_type = '{}' AND i.holder_id = {} AND g.instance_id = {}",
			ItemHolderTypeName(ItemHolderType::Ground),
			zone_id,
			instance_id
		);
		auto retired = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET i.retirement_state = '{}', i.retired_at = NOW() WHERE {} AND i.retirement_state IS NULL",
				join,
				ItemRetirementName(ItemRetirement::Decayed),
				where
			)
		);
		db.QueryDatabase(fmt::format("DELETE g FROM {} WHERE {}", join, where));

		return retired.Success() ? static_cast<int>(retired.RowsAffected()) : -1;
	}

};

#endif //EQEMU_GROUND_ITEMS_REPOSITORY_H
