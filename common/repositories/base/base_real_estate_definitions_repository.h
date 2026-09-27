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
		uint8_t     type;
		uint32_t    category;
		uint32_t    region_id;
		uint64_t    price;
		uint32_t    max_items;
		uint32_t    max_non_real_estate_items;
		uint32_t    max_pet_items;
		float       layout_origin_x;
		float       layout_origin_y;
		float       layout_origin_z;
		float       layout_heading;
		uint32_t    zone_id;
		uint8_t     zone_version;
		uint32_t    max_houses;
		uint32_t    unknown_34;
		uint32_t    unknown_38;
		uint32_t    unknown_44;
		std::string unknown_string;
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
			"type",
			"category",
			"region_id",
			"price",
			"max_items",
			"max_non_real_estate_items",
			"max_pet_items",
			"layout_origin_x",
			"layout_origin_y",
			"layout_origin_z",
			"layout_heading",
			"zone_id",
			"zone_version",
			"max_houses",
			"unknown_34",
			"unknown_38",
			"unknown_44",
			"unknown_string",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"id",
			"name",
			"type",
			"category",
			"region_id",
			"price",
			"max_items",
			"max_non_real_estate_items",
			"max_pet_items",
			"layout_origin_x",
			"layout_origin_y",
			"layout_origin_z",
			"layout_heading",
			"zone_id",
			"zone_version",
			"max_houses",
			"unknown_34",
			"unknown_38",
			"unknown_44",
			"unknown_string",
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
		e.type                      = 0;
		e.category                  = 0;
		e.region_id                 = 0;
		e.price                     = 0;
		e.max_items                 = 0;
		e.max_non_real_estate_items = 0;
		e.max_pet_items             = 0;
		e.layout_origin_x           = 0;
		e.layout_origin_y           = 0;
		e.layout_origin_z           = 0;
		e.layout_heading            = 0;
		e.zone_id                   = 0;
		e.zone_version              = 0;
		e.max_houses                = 0;
		e.unknown_34                = 0;
		e.unknown_38                = 0;
		e.unknown_44                = 336;
		e.unknown_string            = "";

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
			e.type                      = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.category                  = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.region_id                 = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.price                     = row[5] ? strtoull(row[5], nullptr, 10) : 0;
			e.max_items                 = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.max_non_real_estate_items = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.max_pet_items             = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.layout_origin_x           = row[9] ? strtof(row[9], nullptr) : 0;
			e.layout_origin_y           = row[10] ? strtof(row[10], nullptr) : 0;
			e.layout_origin_z           = row[11] ? strtof(row[11], nullptr) : 0;
			e.layout_heading            = row[12] ? strtof(row[12], nullptr) : 0;
			e.zone_id                   = row[13] ? static_cast<uint32_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.zone_version              = row[14] ? static_cast<uint8_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.max_houses                = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.unknown_34                = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.unknown_38                = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.unknown_44                = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 336;
			e.unknown_string            = row[19] ? row[19] : "";

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
		v.push_back(columns[2] + " = " + std::to_string(e.type));
		v.push_back(columns[3] + " = " + std::to_string(e.category));
		v.push_back(columns[4] + " = " + std::to_string(e.region_id));
		v.push_back(columns[5] + " = " + std::to_string(e.price));
		v.push_back(columns[6] + " = " + std::to_string(e.max_items));
		v.push_back(columns[7] + " = " + std::to_string(e.max_non_real_estate_items));
		v.push_back(columns[8] + " = " + std::to_string(e.max_pet_items));
		v.push_back(columns[9] + " = " + std::to_string(e.layout_origin_x));
		v.push_back(columns[10] + " = " + std::to_string(e.layout_origin_y));
		v.push_back(columns[11] + " = " + std::to_string(e.layout_origin_z));
		v.push_back(columns[12] + " = " + std::to_string(e.layout_heading));
		v.push_back(columns[13] + " = " + std::to_string(e.zone_id));
		v.push_back(columns[14] + " = " + std::to_string(e.zone_version));
		v.push_back(columns[15] + " = " + std::to_string(e.max_houses));
		v.push_back(columns[16] + " = " + std::to_string(e.unknown_34));
		v.push_back(columns[17] + " = " + std::to_string(e.unknown_38));
		v.push_back(columns[18] + " = " + std::to_string(e.unknown_44));
		v.push_back(columns[19] + " = '" + Strings::Escape(e.unknown_string) + "'");

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
		v.push_back(std::to_string(e.type));
		v.push_back(std::to_string(e.category));
		v.push_back(std::to_string(e.region_id));
		v.push_back(std::to_string(e.price));
		v.push_back(std::to_string(e.max_items));
		v.push_back(std::to_string(e.max_non_real_estate_items));
		v.push_back(std::to_string(e.max_pet_items));
		v.push_back(std::to_string(e.layout_origin_x));
		v.push_back(std::to_string(e.layout_origin_y));
		v.push_back(std::to_string(e.layout_origin_z));
		v.push_back(std::to_string(e.layout_heading));
		v.push_back(std::to_string(e.zone_id));
		v.push_back(std::to_string(e.zone_version));
		v.push_back(std::to_string(e.max_houses));
		v.push_back(std::to_string(e.unknown_34));
		v.push_back(std::to_string(e.unknown_38));
		v.push_back(std::to_string(e.unknown_44));
		v.push_back("'" + Strings::Escape(e.unknown_string) + "'");

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
			v.push_back(std::to_string(e.type));
			v.push_back(std::to_string(e.category));
			v.push_back(std::to_string(e.region_id));
			v.push_back(std::to_string(e.price));
			v.push_back(std::to_string(e.max_items));
			v.push_back(std::to_string(e.max_non_real_estate_items));
			v.push_back(std::to_string(e.max_pet_items));
			v.push_back(std::to_string(e.layout_origin_x));
			v.push_back(std::to_string(e.layout_origin_y));
			v.push_back(std::to_string(e.layout_origin_z));
			v.push_back(std::to_string(e.layout_heading));
			v.push_back(std::to_string(e.zone_id));
			v.push_back(std::to_string(e.zone_version));
			v.push_back(std::to_string(e.max_houses));
			v.push_back(std::to_string(e.unknown_34));
			v.push_back(std::to_string(e.unknown_38));
			v.push_back(std::to_string(e.unknown_44));
			v.push_back("'" + Strings::Escape(e.unknown_string) + "'");

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
			e.type                      = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.category                  = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.region_id                 = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.price                     = row[5] ? strtoull(row[5], nullptr, 10) : 0;
			e.max_items                 = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.max_non_real_estate_items = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.max_pet_items             = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.layout_origin_x           = row[9] ? strtof(row[9], nullptr) : 0;
			e.layout_origin_y           = row[10] ? strtof(row[10], nullptr) : 0;
			e.layout_origin_z           = row[11] ? strtof(row[11], nullptr) : 0;
			e.layout_heading            = row[12] ? strtof(row[12], nullptr) : 0;
			e.zone_id                   = row[13] ? static_cast<uint32_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.zone_version              = row[14] ? static_cast<uint8_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.max_houses                = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.unknown_34                = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.unknown_38                = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.unknown_44                = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 336;
			e.unknown_string            = row[19] ? row[19] : "";

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
			e.type                      = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.category                  = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.region_id                 = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.price                     = row[5] ? strtoull(row[5], nullptr, 10) : 0;
			e.max_items                 = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.max_non_real_estate_items = row[7] ? static_cast<uint32_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.max_pet_items             = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.layout_origin_x           = row[9] ? strtof(row[9], nullptr) : 0;
			e.layout_origin_y           = row[10] ? strtof(row[10], nullptr) : 0;
			e.layout_origin_z           = row[11] ? strtof(row[11], nullptr) : 0;
			e.layout_heading            = row[12] ? strtof(row[12], nullptr) : 0;
			e.zone_id                   = row[13] ? static_cast<uint32_t>(strtoul(row[13], nullptr, 10)) : 0;
			e.zone_version              = row[14] ? static_cast<uint8_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.max_houses                = row[15] ? static_cast<uint32_t>(strtoul(row[15], nullptr, 10)) : 0;
			e.unknown_34                = row[16] ? static_cast<uint32_t>(strtoul(row[16], nullptr, 10)) : 0;
			e.unknown_38                = row[17] ? static_cast<uint32_t>(strtoul(row[17], nullptr, 10)) : 0;
			e.unknown_44                = row[18] ? static_cast<uint32_t>(strtoul(row[18], nullptr, 10)) : 336;
			e.unknown_string            = row[19] ? row[19] : "";

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
		v.push_back(std::to_string(e.type));
		v.push_back(std::to_string(e.category));
		v.push_back(std::to_string(e.region_id));
		v.push_back(std::to_string(e.price));
		v.push_back(std::to_string(e.max_items));
		v.push_back(std::to_string(e.max_non_real_estate_items));
		v.push_back(std::to_string(e.max_pet_items));
		v.push_back(std::to_string(e.layout_origin_x));
		v.push_back(std::to_string(e.layout_origin_y));
		v.push_back(std::to_string(e.layout_origin_z));
		v.push_back(std::to_string(e.layout_heading));
		v.push_back(std::to_string(e.zone_id));
		v.push_back(std::to_string(e.zone_version));
		v.push_back(std::to_string(e.max_houses));
		v.push_back(std::to_string(e.unknown_34));
		v.push_back(std::to_string(e.unknown_38));
		v.push_back(std::to_string(e.unknown_44));
		v.push_back("'" + Strings::Escape(e.unknown_string) + "'");

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
			v.push_back(std::to_string(e.type));
			v.push_back(std::to_string(e.category));
			v.push_back(std::to_string(e.region_id));
			v.push_back(std::to_string(e.price));
			v.push_back(std::to_string(e.max_items));
			v.push_back(std::to_string(e.max_non_real_estate_items));
			v.push_back(std::to_string(e.max_pet_items));
			v.push_back(std::to_string(e.layout_origin_x));
			v.push_back(std::to_string(e.layout_origin_y));
			v.push_back(std::to_string(e.layout_origin_z));
			v.push_back(std::to_string(e.layout_heading));
			v.push_back(std::to_string(e.zone_id));
			v.push_back(std::to_string(e.zone_version));
			v.push_back(std::to_string(e.max_houses));
			v.push_back(std::to_string(e.unknown_34));
			v.push_back(std::to_string(e.unknown_38));
			v.push_back(std::to_string(e.unknown_44));
			v.push_back("'" + Strings::Escape(e.unknown_string) + "'");

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
