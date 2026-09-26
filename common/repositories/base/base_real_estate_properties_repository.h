/**
 * DO NOT MODIFY THIS FILE
 *
 * This repository was automatically generated and is NOT to be modified directly.
 * Any repository modifications are meant to be made to the repository extending the base.
 * Any modifications to base repositories are to be made by the generator only
 *
 * @generator ./utils/scripts/generators/repository-generator.pl
 * @docs https://docs.eqemu.io/developer/repositories
 */

#ifndef EQEMU_BASE_REAL_ESTATE_PROPERTIES_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_PROPERTIES_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstatePropertiesRepository {
public:
	struct RealEstateProperties {
		uint32_t    id;
		uint32_t    parent_id;
		uint32_t    definition_id;
		uint32_t    guild_id;
		uint32_t    owner_character_id;
		std::string name;
		uint32_t    host_zone_id;
		uint32_t    host_instance_id;
		uint32_t    interior_instance_id;
		uint8_t     contained_in_item;
		uint32_t    upkeep_option_key;
		uint8_t     upkeep_current;
		time_t      next_upkeep_at;
		uint8_t     voting_opt_out;
		uint8_t     archived;
		uint32_t    revision;
		time_t      created_at;
	};

	static std::string PrimaryKey()
	{
		return std::string("id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"id",
			"parent_id",
			"definition_id",
			"guild_id",
			"owner_character_id",
			"name",
			"host_zone_id",
			"host_instance_id",
			"interior_instance_id",
			"contained_in_item",
			"upkeep_option_key",
			"upkeep_current",
			"next_upkeep_at",
			"voting_opt_out",
			"archived",
			"revision",
			"created_at",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"id",
			"parent_id",
			"definition_id",
			"guild_id",
			"owner_character_id",
			"name",
			"host_zone_id",
			"host_instance_id",
			"interior_instance_id",
			"contained_in_item",
			"upkeep_option_key",
			"upkeep_current",
			"UNIX_TIMESTAMP(next_upkeep_at)",
			"voting_opt_out",
			"archived",
			"revision",
			"UNIX_TIMESTAMP(created_at)",
		};
	}

	static std::string ColumnsRaw()
	{
		return std::string(Strings::Implode(", ", Columns()));
	}

	static std::string SelectColumnsRaw()
	{
		return std::string(Strings::Implode(", ", SelectColumns()));
	}

	static std::string TableName()
	{
		return std::string("real_estate_properties");
	}

	static std::string BaseSelect()
	{
		return fmt::format(
			"SELECT {} FROM {}",
			SelectColumnsRaw(),
			TableName()
		);
	}

	static std::string BaseInsert()
	{
		return fmt::format(
			"INSERT INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static RealEstateProperties NewEntity()
	{
		RealEstateProperties e{};

		e.id                   = 0;
		e.parent_id            = 0;
		e.definition_id        = 0;
		e.guild_id             = 0;
		e.owner_character_id   = 0;
		e.name                 = "";
		e.host_zone_id         = 0;
		e.host_instance_id     = 0;
		e.interior_instance_id = 0;
		e.contained_in_item    = 0;
		e.upkeep_option_key    = 0;
		e.upkeep_current       = 1;
		e.next_upkeep_at       = 0;
		e.voting_opt_out       = 0;
		e.archived             = 0;
		e.revision             = 0;
		e.created_at           = std::time(nullptr);

		return e;
	}

	static RealEstateProperties GetRealEstateProperties(
		const std::vector<RealEstateProperties> &real_estate_propertiess,
		int real_estate_properties_id
	)
	{
		for (auto &real_estate_properties : real_estate_propertiess) {
			if (real_estate_properties.id == real_estate_properties_id) {
				return real_estate_properties;
			}
		}

		return NewEntity();
	}

	static RealEstateProperties FindOne(
		Database& db,
		int real_estate_properties_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_properties_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateProperties e{};

			e.id                   = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.parent_id            = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.definition_id        = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.guild_id             = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.owner_character_id   = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.name                 = row[5] ? row[5] : "";
			e.host_zone_id         = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.host_instance_id     = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.interior_instance_id = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.contained_in_item    = row[9] ? static_cast<uint8_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.upkeep_option_key    = row[10] ? static_cast<uint32_t>(strtoul(row[10], nullptr, 10)) : 0;
			e.upkeep_current       = row[11] ? static_cast<uint8_t>(strtoul(row[11], nullptr, 10)) : 1;
			e.next_upkeep_at       = strtoll(row[12] ? row[12] : "-1", nullptr, 10);
			e.voting_opt_out       = row[13] ? static_cast<uint8_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.archived             = row[14] ? static_cast<uint8_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.revision             = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.created_at           = strtoll(row[16] ? row[16] : "-1", nullptr, 10);

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_properties_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_properties_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateProperties &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[1] + " = " + std::to_string(e.parent_id));
		v.push_back(columns[2] + " = " + std::to_string(e.definition_id));
		v.push_back(columns[3] + " = " + std::to_string(e.guild_id));
		v.push_back(columns[4] + " = " + std::to_string(e.owner_character_id));
		v.push_back(columns[5] + " = '" + Strings::Escape(e.name) + "'");
		v.push_back(columns[6] + " = " + std::to_string(e.host_zone_id));
		v.push_back(columns[7] + " = " + std::to_string(e.host_instance_id));
		v.push_back(columns[8] + " = " + std::to_string(e.interior_instance_id));
		v.push_back(columns[9] + " = " + std::to_string(e.contained_in_item));
		v.push_back(columns[10] + " = " + std::to_string(e.upkeep_option_key));
		v.push_back(columns[11] + " = " + std::to_string(e.upkeep_current));
		v.push_back(columns[12] + " = FROM_UNIXTIME(" + (e.next_upkeep_at > 0 ? std::to_string(e.next_upkeep_at) : "null") + ")");
		v.push_back(columns[13] + " = " + std::to_string(e.voting_opt_out));
		v.push_back(columns[14] + " = " + std::to_string(e.archived));
		v.push_back(columns[15] + " = " + std::to_string(e.revision));
		v.push_back(columns[16] + " = FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstateProperties InsertOne(
		Database& db,
		RealEstateProperties e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.parent_id));
		v.push_back(std::to_string(e.definition_id));
		v.push_back(std::to_string(e.guild_id));
		v.push_back(std::to_string(e.owner_character_id));
		v.push_back("'" + Strings::Escape(e.name) + "'");
		v.push_back(std::to_string(e.host_zone_id));
		v.push_back(std::to_string(e.host_instance_id));
		v.push_back(std::to_string(e.interior_instance_id));
		v.push_back(std::to_string(e.contained_in_item));
		v.push_back(std::to_string(e.upkeep_option_key));
		v.push_back(std::to_string(e.upkeep_current));
		v.push_back("FROM_UNIXTIME(" + (e.next_upkeep_at > 0 ? std::to_string(e.next_upkeep_at) : "null") + ")");
		v.push_back(std::to_string(e.voting_opt_out));
		v.push_back(std::to_string(e.archived));
		v.push_back(std::to_string(e.revision));
		v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstateProperties> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.parent_id));
			v.push_back(std::to_string(e.definition_id));
			v.push_back(std::to_string(e.guild_id));
			v.push_back(std::to_string(e.owner_character_id));
			v.push_back("'" + Strings::Escape(e.name) + "'");
			v.push_back(std::to_string(e.host_zone_id));
			v.push_back(std::to_string(e.host_instance_id));
			v.push_back(std::to_string(e.interior_instance_id));
			v.push_back(std::to_string(e.contained_in_item));
			v.push_back(std::to_string(e.upkeep_option_key));
			v.push_back(std::to_string(e.upkeep_current));
			v.push_back("FROM_UNIXTIME(" + (e.next_upkeep_at > 0 ? std::to_string(e.next_upkeep_at) : "null") + ")");
			v.push_back(std::to_string(e.voting_opt_out));
			v.push_back(std::to_string(e.archived));
			v.push_back(std::to_string(e.revision));
			v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseInsert(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static std::vector<RealEstateProperties> All(Database& db)
	{
		std::vector<RealEstateProperties> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateProperties e{};

			e.id                   = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.parent_id            = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.definition_id        = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.guild_id             = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.owner_character_id   = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.name                 = row[5] ? row[5] : "";
			e.host_zone_id         = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.host_instance_id     = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.interior_instance_id = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.contained_in_item    = row[9] ? static_cast<uint8_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.upkeep_option_key    = row[10] ? static_cast<uint32_t>(strtoul(row[10], nullptr, 10)) : 0;
			e.upkeep_current       = row[11] ? static_cast<uint8_t>(strtoul(row[11], nullptr, 10)) : 1;
			e.next_upkeep_at       = strtoll(row[12] ? row[12] : "-1", nullptr, 10);
			e.voting_opt_out       = row[13] ? static_cast<uint8_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.archived             = row[14] ? static_cast<uint8_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.revision             = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.created_at           = strtoll(row[16] ? row[16] : "-1", nullptr, 10);

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateProperties> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateProperties> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateProperties e{};

			e.id                   = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.parent_id            = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.definition_id        = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.guild_id             = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.owner_character_id   = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.name                 = row[5] ? row[5] : "";
			e.host_zone_id         = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.host_instance_id     = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.interior_instance_id = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.contained_in_item    = row[9] ? static_cast<uint8_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.upkeep_option_key    = row[10] ? static_cast<uint32_t>(strtoul(row[10], nullptr, 10)) : 0;
			e.upkeep_current       = row[11] ? static_cast<uint8_t>(strtoul(row[11], nullptr, 10)) : 1;
			e.next_upkeep_at       = strtoll(row[12] ? row[12] : "-1", nullptr, 10);
			e.voting_opt_out       = row[13] ? static_cast<uint8_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.archived             = row[14] ? static_cast<uint8_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.revision             = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.created_at           = strtoll(row[16] ? row[16] : "-1", nullptr, 10);

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static int DeleteWhere(Database& db, const std::string &where_filter)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {}",
				TableName(),
				where_filter
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int Truncate(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"TRUNCATE TABLE {}",
				TableName()
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int64 GetMaxId(Database& db)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COALESCE(MAX({}), 0) FROM {}",
				PrimaryKey(),
				TableName()
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static int64 Count(Database& db, const std::string &where_filter = "")
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COUNT(*) FROM {} {}",
				TableName(),
				(where_filter.empty() ? "" : "WHERE " + where_filter)
			)
		);

		return (results.Success() && results.begin()[0] ? strtoll(results.begin()[0], nullptr, 10) : 0);
	}

	static std::string BaseReplace()
	{
		return fmt::format(
			"REPLACE INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static int ReplaceOne(
		Database& db,
		const RealEstateProperties &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.parent_id));
		v.push_back(std::to_string(e.definition_id));
		v.push_back(std::to_string(e.guild_id));
		v.push_back(std::to_string(e.owner_character_id));
		v.push_back("'" + Strings::Escape(e.name) + "'");
		v.push_back(std::to_string(e.host_zone_id));
		v.push_back(std::to_string(e.host_instance_id));
		v.push_back(std::to_string(e.interior_instance_id));
		v.push_back(std::to_string(e.contained_in_item));
		v.push_back(std::to_string(e.upkeep_option_key));
		v.push_back(std::to_string(e.upkeep_current));
		v.push_back("FROM_UNIXTIME(" + (e.next_upkeep_at > 0 ? std::to_string(e.next_upkeep_at) : "null") + ")");
		v.push_back(std::to_string(e.voting_opt_out));
		v.push_back(std::to_string(e.archived));
		v.push_back(std::to_string(e.revision));
		v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseReplace(),
				Strings::Implode(",", v)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int ReplaceMany(
		Database& db,
		const std::vector<RealEstateProperties> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.parent_id));
			v.push_back(std::to_string(e.definition_id));
			v.push_back(std::to_string(e.guild_id));
			v.push_back(std::to_string(e.owner_character_id));
			v.push_back("'" + Strings::Escape(e.name) + "'");
			v.push_back(std::to_string(e.host_zone_id));
			v.push_back(std::to_string(e.host_instance_id));
			v.push_back(std::to_string(e.interior_instance_id));
			v.push_back(std::to_string(e.contained_in_item));
			v.push_back(std::to_string(e.upkeep_option_key));
			v.push_back(std::to_string(e.upkeep_current));
			v.push_back("FROM_UNIXTIME(" + (e.next_upkeep_at > 0 ? std::to_string(e.next_upkeep_at) : "null") + ")");
			v.push_back(std::to_string(e.voting_opt_out));
			v.push_back(std::to_string(e.archived));
			v.push_back(std::to_string(e.revision));
			v.push_back("FROM_UNIXTIME(" + (e.created_at > 0 ? std::to_string(e.created_at) : "null") + ")");

			insert_chunks.push_back("(" + Strings::Implode(",", v) + ")");
		}

		std::vector<std::string> v;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseReplace(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}
};

#endif //EQEMU_BASE_REAL_ESTATE_PROPERTIES_REPOSITORY_H
