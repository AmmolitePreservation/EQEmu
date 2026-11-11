#ifndef EQEMU_REAL_ESTATE_ITEMS_REPOSITORY_H
#define EQEMU_REAL_ESTATE_ITEMS_REPOSITORY_H

#include "../database.h"
#include "../item_guid.h"
#include "../strings.h"
#include <stdexcept>

// Written by hand and on the generator's ignore list: a row is the housing side of an item a property holds
// (item_instances.holder_type real_estate), and is only read joined to that item.
class RealEstateItemsRepository {
public:
	struct RealEstateItems {
		uint32_t id;
		ItemGUID guid;
		uint32_t owner_character_id;
		uint8_t  placed;
		uint32_t upkeep_expired_at;
		float    heading;
		float    pitch;
		float    roll;
		float    scale;
		float    x;
		float    y;
		float    z;
		// Read from the item's own row and from the property the item carries; never written from here.
		uint32_t property_id;
		uint32_t item_id;
		int16_t  charges;
		uint8_t  attuned;
		uint32_t carried_property_id;
	};

	// The rows whose item is in play on a property; the filter names the struct's fields.
	static std::vector<RealEstateItems> GetWhere(Database &db, const std::string &where_filter)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT * FROM ("
				"SELECT r.id, r.guid_first, r.guid_second, r.guid_third, r.owner_character_id, r.placed, "
				"r.upkeep_expired_at, r.heading, r.pitch, r.roll, r.scale, r.x, r.y, r.z, "
				"i.holder_id AS property_id, i.item_id, i.charges, i.attuned, COALESCE(c.id, 0) AS carried_property_id "
				"FROM real_estate_items AS r "
				"JOIN item_instances AS i ON i.guid_first = r.guid_first AND i.guid_second = r.guid_second "
				"AND i.guid_third = r.guid_third AND i.holder_type = 'real_estate' AND i.retirement_state IS NULL "
				"LEFT JOIN real_estate_properties AS c ON c.container_guid_first = r.guid_first "
				"AND c.container_guid_second = r.guid_second AND c.container_guid_third = r.guid_third"
				") AS placed_items WHERE {}",
				where_filter
			)
		);
		if (!results.Success()) {
			throw std::runtime_error(fmt::format("real_estate_items read failed: {}", results.ErrorMessage()));
		}

		std::vector<RealEstateItems> all_entries;
		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateItems e{};

			e.id   = static_cast<uint32_t>(strtoul(row[0], nullptr, 10));
			e.guid = {
				static_cast<uint32_t>(strtoul(row[1], nullptr, 10)),
				static_cast<uint32_t>(strtoul(row[2], nullptr, 10)),
				static_cast<uint32_t>(strtoul(row[3], nullptr, 10)),
			};
			e.owner_character_id  = static_cast<uint32_t>(strtoul(row[4], nullptr, 10));
			e.placed              = static_cast<uint8_t>(strtoul(row[5], nullptr, 10));
			e.upkeep_expired_at   = static_cast<uint32_t>(strtoul(row[6], nullptr, 10));
			e.heading             = strtof(row[7], nullptr);
			e.pitch               = strtof(row[8], nullptr);
			e.roll                = strtof(row[9], nullptr);
			e.scale               = strtof(row[10], nullptr);
			e.x                   = strtof(row[11], nullptr);
			e.y                   = strtof(row[12], nullptr);
			e.z                   = strtof(row[13], nullptr);
			e.property_id         = static_cast<uint32_t>(strtoul(row[14], nullptr, 10));
			e.item_id             = static_cast<uint32_t>(strtoul(row[15], nullptr, 10));
			e.charges             = static_cast<int16_t>(atoi(row[16]));
			e.attuned             = static_cast<uint8_t>(strtoul(row[17], nullptr, 10));
			e.carried_property_id = static_cast<uint32_t>(strtoul(row[18], nullptr, 10));

			all_entries.push_back(e);
		}

		return all_entries;
	}

	// Adds the housing side of an item its property already holds, and returns the row's new id.
	static uint32_t InsertOne(Database &db, const RealEstateItems &e)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"INSERT INTO real_estate_items (guid_first, guid_second, guid_third, owner_character_id, placed, "
				"upkeep_expired_at, heading, pitch, roll, scale, x, y, z) "
				"VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
				e.guid.first,
				e.guid.second,
				e.guid.third,
				e.owner_character_id,
				e.placed,
				e.upkeep_expired_at,
				e.heading,
				e.pitch,
				e.roll,
				e.scale,
				e.x,
				e.y,
				e.z
			)
		);
		if (!results.Success() || !results.LastInsertedID()) {
			throw std::runtime_error(fmt::format("real_estate_items insert failed: {}", results.ErrorMessage()));
		}

		return static_cast<uint32_t>(results.LastInsertedID());
	}
};

#endif //EQEMU_REAL_ESTATE_ITEMS_REPOSITORY_H
