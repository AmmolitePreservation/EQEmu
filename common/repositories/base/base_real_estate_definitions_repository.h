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

#ifndef EQEMU_BASE_REAL_ESTATE_DEFINITIONS_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_DEFINITIONS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateDefinitionsRepository {
public:
	struct RealEstateDefinitions {
		uint32_t    id;
		std::string name;
		std::string name_segment;
		uint8_t     type;
		uint32_t    placement_group_id;
		uint32_t    area_id;
		uint32_t    cost_definition_id;
		uint32_t    upkeep_definition_id;
		uint32_t    max_items;
		uint32_t    max_non_real_estate_items;
		int32_t     max_npc_items;
		float       switch_x;
		float       switch_y;
		float       switch_z;
		float       switch_heading;
		uint32_t    max_children;
		uint32_t    zone_id;
		uint32_t    dz_id;
		uint32_t    icon;
		std::string address;
		uint32_t    exit_zone_id;
		uint8_t     exit_zone_version;
		float       exit_x;
		float       exit_y;
		float       exit_z;
		float       exit_heading;
	};

	static std::string PrimaryKey()
	{
		return std::string("id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"id",
			"name",
			"name_segment",
			"type",
			"placement_group_id",
			"area_id",
			"cost_definition_id",
			"upkeep_definition_id",
			"max_items",
			"max_non_real_estate_items",
			"max_npc_items",
			"switch_x",
			"switch_y",
			"switch_z",
			"switch_heading",
			"max_children",
			"zone_id",
			"dz_id",
			"icon",
			"address",
			"exit_zone_id",
			"exit_zone_version",
			"exit_x",
			"exit_y",
			"exit_z",
			"exit_heading",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"id",
			"name",
			"name_segment",
			"type",
			"placement_group_id",
			"area_id",
			"cost_definition_id",
			"upkeep_definition_id",
			"max_items",
			"max_non_real_estate_items",
			"max_npc_items",
			"switch_x",
			"switch_y",
			"switch_z",
			"switch_heading",
			"max_children",
			"zone_id",
			"dz_id",
			"icon",
			"address",
			"exit_zone_id",
			"exit_zone_version",
			"exit_x",
			"exit_y",
			"exit_z",
			"exit_heading",
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
		return std::string("real_estate_definitions");
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

	static RealEstateDefinitions NewEntity()
	{
		RealEstateDefinitions e{};

		e.id                        = 0;
		e.name                      = "";
		e.name_segment              = "";
		e.type                      = 0;
		e.placement_group_id        = 0;
		e.area_id                   = 0;
		e.cost_definition_id        = 0;
		e.upkeep_definition_id      = 0;
		e.max_items                 = 0;
		e.max_non_real_estate_items = 0;
		e.max_npc_items             = -1;
		e.switch_x                  = 0;
		e.switch_y                  = 0;
		e.switch_z                  = 0;
		e.switch_heading            = 0;
		e.max_children              = 0;
		e.zone_id                   = 0;
		e.dz_id                     = 0;
		e.icon                      = 0;
		e.address                   = "";
		e.exit_zone_id              = 0;
		e.exit_zone_version         = 0;
		e.exit_x                    = 0;
		e.exit_y                    = 0;
		e.exit_z                    = 0;
		e.exit_heading              = 0;

		return e;
	}

	static RealEstateDefinitions GetRealEstateDefinitions(
		const std::vector<RealEstateDefinitions> &real_estate_definitionss,
		int real_estate_definitions_id
	)
	{
		for (auto &real_estate_definitions : real_estate_definitionss) {
			if (real_estate_definitions.id == real_estate_definitions_id) {
				return real_estate_definitions;
			}
		}

		return NewEntity();
	}

	static RealEstateDefinitions FindOne(
		Database& db,
		int real_estate_definitions_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_definitions_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateDefinitions e{};

			e.id                        = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.name                      = row[1] ? row[1] : "";
			e.name_segment              = row[2] ? row[2] : "";
			e.type                      = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.placement_group_id        = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.area_id                   = row[5] ? static_cast<uint32_t>(strtoul(row[5], nullptr, 10)) : 0;
			e.cost_definition_id        = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.upkeep_definition_id      = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.max_items                 = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.max_non_real_estate_items = row[9] ? static_cast<uint32_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.max_npc_items             = row[10] ? static_cast<int32_t>(atoi(row[10])) : -1;
			e.switch_x                  = row[11] ? strtof(row[11], nullptr) : 0;
			e.switch_y                  = row[12] ? strtof(row[12], nullptr) : 0;
			e.switch_z                  = row[13] ? strtof(row[13], nullptr) : 0;
			e.switch_heading            = row[14] ? strtof(row[14], nullptr) : 0;
			e.max_children              = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.zone_id                   = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.dz_id                     = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.icon                      = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 0;
			e.address                   = row[19] ? row[19] : "";
			e.exit_zone_id              = row[20] ? static_cast<uint32_t>(strtoul(row[20], nullptr, 10)) : 0;
			e.exit_zone_version         = row[21] ? static_cast<uint8_t>(strtoul(row[21], nullptr, 10)) : 0;
			e.exit_x                    = row[22] ? strtof(row[22], nullptr) : 0;
			e.exit_y                    = row[23] ? strtof(row[23], nullptr) : 0;
			e.exit_z                    = row[24] ? strtof(row[24], nullptr) : 0;
			e.exit_heading              = row[25] ? strtof(row[25], nullptr) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_definitions_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_definitions_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateDefinitions &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.id));
		v.push_back(columns[1] + " = '" + Strings::Escape(e.name) + "'");
		v.push_back(columns[2] + " = '" + Strings::Escape(e.name_segment) + "'");
		v.push_back(columns[3] + " = " + std::to_string(e.type));
		v.push_back(columns[4] + " = " + std::to_string(e.placement_group_id));
		v.push_back(columns[5] + " = " + std::to_string(e.area_id));
		v.push_back(columns[6] + " = " + std::to_string(e.cost_definition_id));
		v.push_back(columns[7] + " = " + std::to_string(e.upkeep_definition_id));
		v.push_back(columns[8] + " = " + std::to_string(e.max_items));
		v.push_back(columns[9] + " = " + std::to_string(e.max_non_real_estate_items));
		v.push_back(columns[10] + " = " + std::to_string(e.max_npc_items));
		v.push_back(columns[11] + " = " + std::to_string(e.switch_x));
		v.push_back(columns[12] + " = " + std::to_string(e.switch_y));
		v.push_back(columns[13] + " = " + std::to_string(e.switch_z));
		v.push_back(columns[14] + " = " + std::to_string(e.switch_heading));
		v.push_back(columns[15] + " = " + std::to_string(e.max_children));
		v.push_back(columns[16] + " = " + std::to_string(e.zone_id));
		v.push_back(columns[17] + " = " + std::to_string(e.dz_id));
		v.push_back(columns[18] + " = " + std::to_string(e.icon));
		v.push_back(columns[19] + " = '" + Strings::Escape(e.address) + "'");
		v.push_back(columns[20] + " = " + std::to_string(e.exit_zone_id));
		v.push_back(columns[21] + " = " + std::to_string(e.exit_zone_version));
		v.push_back(columns[22] + " = " + std::to_string(e.exit_x));
		v.push_back(columns[23] + " = " + std::to_string(e.exit_y));
		v.push_back(columns[24] + " = " + std::to_string(e.exit_z));
		v.push_back(columns[25] + " = " + std::to_string(e.exit_heading));

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

	static RealEstateDefinitions InsertOne(
		Database& db,
		RealEstateDefinitions e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back("'" + Strings::Escape(e.name) + "'");
		v.push_back("'" + Strings::Escape(e.name_segment) + "'");
		v.push_back(std::to_string(e.type));
		v.push_back(std::to_string(e.placement_group_id));
		v.push_back(std::to_string(e.area_id));
		v.push_back(std::to_string(e.cost_definition_id));
		v.push_back(std::to_string(e.upkeep_definition_id));
		v.push_back(std::to_string(e.max_items));
		v.push_back(std::to_string(e.max_non_real_estate_items));
		v.push_back(std::to_string(e.max_npc_items));
		v.push_back(std::to_string(e.switch_x));
		v.push_back(std::to_string(e.switch_y));
		v.push_back(std::to_string(e.switch_z));
		v.push_back(std::to_string(e.switch_heading));
		v.push_back(std::to_string(e.max_children));
		v.push_back(std::to_string(e.zone_id));
		v.push_back(std::to_string(e.dz_id));
		v.push_back(std::to_string(e.icon));
		v.push_back("'" + Strings::Escape(e.address) + "'");
		v.push_back(std::to_string(e.exit_zone_id));
		v.push_back(std::to_string(e.exit_zone_version));
		v.push_back(std::to_string(e.exit_x));
		v.push_back(std::to_string(e.exit_y));
		v.push_back(std::to_string(e.exit_z));
		v.push_back(std::to_string(e.exit_heading));

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
		const std::vector<RealEstateDefinitions> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back("'" + Strings::Escape(e.name) + "'");
			v.push_back("'" + Strings::Escape(e.name_segment) + "'");
			v.push_back(std::to_string(e.type));
			v.push_back(std::to_string(e.placement_group_id));
			v.push_back(std::to_string(e.area_id));
			v.push_back(std::to_string(e.cost_definition_id));
			v.push_back(std::to_string(e.upkeep_definition_id));
			v.push_back(std::to_string(e.max_items));
			v.push_back(std::to_string(e.max_non_real_estate_items));
			v.push_back(std::to_string(e.max_npc_items));
			v.push_back(std::to_string(e.switch_x));
			v.push_back(std::to_string(e.switch_y));
			v.push_back(std::to_string(e.switch_z));
			v.push_back(std::to_string(e.switch_heading));
			v.push_back(std::to_string(e.max_children));
			v.push_back(std::to_string(e.zone_id));
			v.push_back(std::to_string(e.dz_id));
			v.push_back(std::to_string(e.icon));
			v.push_back("'" + Strings::Escape(e.address) + "'");
			v.push_back(std::to_string(e.exit_zone_id));
			v.push_back(std::to_string(e.exit_zone_version));
			v.push_back(std::to_string(e.exit_x));
			v.push_back(std::to_string(e.exit_y));
			v.push_back(std::to_string(e.exit_z));
			v.push_back(std::to_string(e.exit_heading));

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

	static std::vector<RealEstateDefinitions> All(Database& db)
	{
		std::vector<RealEstateDefinitions> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateDefinitions e{};

			e.id                        = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.name                      = row[1] ? row[1] : "";
			e.name_segment              = row[2] ? row[2] : "";
			e.type                      = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.placement_group_id        = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.area_id                   = row[5] ? static_cast<uint32_t>(strtoul(row[5], nullptr, 10)) : 0;
			e.cost_definition_id        = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.upkeep_definition_id      = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.max_items                 = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.max_non_real_estate_items = row[9] ? static_cast<uint32_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.max_npc_items             = row[10] ? static_cast<int32_t>(atoi(row[10])) : -1;
			e.switch_x                  = row[11] ? strtof(row[11], nullptr) : 0;
			e.switch_y                  = row[12] ? strtof(row[12], nullptr) : 0;
			e.switch_z                  = row[13] ? strtof(row[13], nullptr) : 0;
			e.switch_heading            = row[14] ? strtof(row[14], nullptr) : 0;
			e.max_children              = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.zone_id                   = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.dz_id                     = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.icon                      = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 0;
			e.address                   = row[19] ? row[19] : "";
			e.exit_zone_id              = row[20] ? static_cast<uint32_t>(strtoul(row[20], nullptr, 10)) : 0;
			e.exit_zone_version         = row[21] ? static_cast<uint8_t>(strtoul(row[21], nullptr, 10)) : 0;
			e.exit_x                    = row[22] ? strtof(row[22], nullptr) : 0;
			e.exit_y                    = row[23] ? strtof(row[23], nullptr) : 0;
			e.exit_z                    = row[24] ? strtof(row[24], nullptr) : 0;
			e.exit_heading              = row[25] ? strtof(row[25], nullptr) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateDefinitions> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateDefinitions> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateDefinitions e{};

			e.id                        = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.name                      = row[1] ? row[1] : "";
			e.name_segment              = row[2] ? row[2] : "";
			e.type                      = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.placement_group_id        = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.area_id                   = row[5] ? static_cast<uint32_t>(strtoul(row[5], nullptr, 10)) : 0;
			e.cost_definition_id        = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.upkeep_definition_id      = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.max_items                 = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.max_non_real_estate_items = row[9] ? static_cast<uint32_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.max_npc_items             = row[10] ? static_cast<int32_t>(atoi(row[10])) : -1;
			e.switch_x                  = row[11] ? strtof(row[11], nullptr) : 0;
			e.switch_y                  = row[12] ? strtof(row[12], nullptr) : 0;
			e.switch_z                  = row[13] ? strtof(row[13], nullptr) : 0;
			e.switch_heading            = row[14] ? strtof(row[14], nullptr) : 0;
			e.max_children              = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.zone_id                   = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.dz_id                     = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.icon                      = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 0;
			e.address                   = row[19] ? row[19] : "";
			e.exit_zone_id              = row[20] ? static_cast<uint32_t>(strtoul(row[20], nullptr, 10)) : 0;
			e.exit_zone_version         = row[21] ? static_cast<uint8_t>(strtoul(row[21], nullptr, 10)) : 0;
			e.exit_x                    = row[22] ? strtof(row[22], nullptr) : 0;
			e.exit_y                    = row[23] ? strtof(row[23], nullptr) : 0;
			e.exit_z                    = row[24] ? strtof(row[24], nullptr) : 0;
			e.exit_heading              = row[25] ? strtof(row[25], nullptr) : 0;

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
		const RealEstateDefinitions &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back("'" + Strings::Escape(e.name) + "'");
		v.push_back("'" + Strings::Escape(e.name_segment) + "'");
		v.push_back(std::to_string(e.type));
		v.push_back(std::to_string(e.placement_group_id));
		v.push_back(std::to_string(e.area_id));
		v.push_back(std::to_string(e.cost_definition_id));
		v.push_back(std::to_string(e.upkeep_definition_id));
		v.push_back(std::to_string(e.max_items));
		v.push_back(std::to_string(e.max_non_real_estate_items));
		v.push_back(std::to_string(e.max_npc_items));
		v.push_back(std::to_string(e.switch_x));
		v.push_back(std::to_string(e.switch_y));
		v.push_back(std::to_string(e.switch_z));
		v.push_back(std::to_string(e.switch_heading));
		v.push_back(std::to_string(e.max_children));
		v.push_back(std::to_string(e.zone_id));
		v.push_back(std::to_string(e.dz_id));
		v.push_back(std::to_string(e.icon));
		v.push_back("'" + Strings::Escape(e.address) + "'");
		v.push_back(std::to_string(e.exit_zone_id));
		v.push_back(std::to_string(e.exit_zone_version));
		v.push_back(std::to_string(e.exit_x));
		v.push_back(std::to_string(e.exit_y));
		v.push_back(std::to_string(e.exit_z));
		v.push_back(std::to_string(e.exit_heading));

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
		const std::vector<RealEstateDefinitions> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back("'" + Strings::Escape(e.name) + "'");
			v.push_back("'" + Strings::Escape(e.name_segment) + "'");
			v.push_back(std::to_string(e.type));
			v.push_back(std::to_string(e.placement_group_id));
			v.push_back(std::to_string(e.area_id));
			v.push_back(std::to_string(e.cost_definition_id));
			v.push_back(std::to_string(e.upkeep_definition_id));
			v.push_back(std::to_string(e.max_items));
			v.push_back(std::to_string(e.max_non_real_estate_items));
			v.push_back(std::to_string(e.max_npc_items));
			v.push_back(std::to_string(e.switch_x));
			v.push_back(std::to_string(e.switch_y));
			v.push_back(std::to_string(e.switch_z));
			v.push_back(std::to_string(e.switch_heading));
			v.push_back(std::to_string(e.max_children));
			v.push_back(std::to_string(e.zone_id));
			v.push_back(std::to_string(e.dz_id));
			v.push_back(std::to_string(e.icon));
			v.push_back("'" + Strings::Escape(e.address) + "'");
			v.push_back(std::to_string(e.exit_zone_id));
			v.push_back(std::to_string(e.exit_zone_version));
			v.push_back(std::to_string(e.exit_x));
			v.push_back(std::to_string(e.exit_y));
			v.push_back(std::to_string(e.exit_z));
			v.push_back(std::to_string(e.exit_heading));

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

#endif //EQEMU_BASE_REAL_ESTATE_DEFINITIONS_REPOSITORY_H
