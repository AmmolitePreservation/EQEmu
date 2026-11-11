#ifndef EQEMU_REAL_ESTATE_PROPERTIES_REPOSITORY_H
#define EQEMU_REAL_ESTATE_PROPERTIES_REPOSITORY_H

#include "../database.h"
#include "../item_guid.h"
#include "../strings.h"
#include <ctime>
#include <optional>
#include <stdexcept>

// Written by hand and on the generator's ignore list: the generator cannot read the nullable container GUID. It only
// reads; housing writes properties with its own statements.
class RealEstatePropertiesRepository {
public:
	struct RealEstateProperties {
		uint32_t                id;
		uint32_t                parent_id;
		uint32_t                definition_id;
		uint32_t                guild_id;
		uint32_t                owner_character_id;
		std::string             name;
		// The item that carries the property: a house's or a crate's own item.
		std::optional<ItemGUID> container_guid;
		uint32_t                upkeep_currency;
		uint32_t                upkeep_expired_time;
		// Zero while the column is NULL.
		time_t                  upkeep_settled_at;
		time_t                  next_upkeep_at;
		uint8_t                 voting_opt_out;
		uint8_t                 archived;
		uint32_t                revision;
	};

	static std::vector<RealEstateProperties> GetWhere(Database &db, const std::string &where_filter)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT id, parent_id, definition_id, guild_id, owner_character_id, name, container_guid_first, "
				"container_guid_second, container_guid_third, upkeep_currency, upkeep_expired_time, "
				"UNIX_TIMESTAMP(upkeep_settled_at), UNIX_TIMESTAMP(next_upkeep_at), voting_opt_out, archived, revision "
				"FROM real_estate_properties WHERE {}",
				where_filter
			)
		);
		if (!results.Success()) {
			throw std::runtime_error(fmt::format("real_estate_properties read failed: {}", results.ErrorMessage()));
		}

		std::vector<RealEstateProperties> all_entries;
		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateProperties e{};

			e.id                 = static_cast<uint32_t>(strtoul(row[0], nullptr, 10));
			e.parent_id          = static_cast<uint32_t>(strtoul(row[1], nullptr, 10));
			e.definition_id      = static_cast<uint32_t>(strtoul(row[2], nullptr, 10));
			e.guild_id           = static_cast<uint32_t>(strtoul(row[3], nullptr, 10));
			e.owner_character_id = static_cast<uint32_t>(strtoul(row[4], nullptr, 10));
			e.name               = row[5];
			if (row[6]) {
				e.container_guid = ItemGUID{
					static_cast<uint32_t>(strtoul(row[6], nullptr, 10)),
					static_cast<uint32_t>(strtoul(row[7], nullptr, 10)),
					static_cast<uint32_t>(strtoul(row[8], nullptr, 10)),
				};
			}
			e.upkeep_currency     = static_cast<uint32_t>(strtoul(row[9], nullptr, 10));
			e.upkeep_expired_time = static_cast<uint32_t>(strtoul(row[10], nullptr, 10));
			e.upkeep_settled_at   = row[11] ? static_cast<time_t>(strtoll(row[11], nullptr, 10)) : 0;
			e.next_upkeep_at      = row[12] ? static_cast<time_t>(strtoll(row[12], nullptr, 10)) : 0;
			e.voting_opt_out      = static_cast<uint8_t>(strtoul(row[13], nullptr, 10));
			e.archived            = static_cast<uint8_t>(strtoul(row[14], nullptr, 10));
			e.revision            = static_cast<uint32_t>(strtoul(row[15], nullptr, 10));

			all_entries.push_back(e);
		}

		return all_entries;
	}
};

#endif //EQEMU_REAL_ESTATE_PROPERTIES_REPOSITORY_H
