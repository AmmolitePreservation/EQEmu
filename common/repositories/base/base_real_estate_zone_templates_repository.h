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

#ifndef EQEMU_BASE_REAL_ESTATE_ZONE_TEMPLATES_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_ZONE_TEMPLATES_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateZoneTemplatesRepository {
public:
	struct RealEstateZoneTemplates {
		uint32_t dz_id;
		uint32_t zone_id;
		uint8_t  zone_version;
		float    zone_in_x;
		float    zone_in_y;
		float    zone_in_z;
		float    zone_in_heading;
	};

	static std::string PrimaryKey()
	{
		return std::string("dz_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"dz_id",
			"zone_id",
			"zone_version",
			"zone_in_x",
			"zone_in_y",
			"zone_in_z",
			"zone_in_heading",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"dz_id",
			"zone_id",
			"zone_version",
			"zone_in_x",
			"zone_in_y",
			"zone_in_z",
			"zone_in_heading",
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
		return std::string("real_estate_zone_templates");
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

	static RealEstateZoneTemplates NewEntity()
	{
		RealEstateZoneTemplates e{};

		e.dz_id           = 0;
		e.zone_id         = 0;
		e.zone_version    = 0;
		e.zone_in_x       = 0;
		e.zone_in_y       = 0;
		e.zone_in_z       = 0;
		e.zone_in_heading = 0;

		return e;
	}

	static RealEstateZoneTemplates GetRealEstateZoneTemplates(
		const std::vector<RealEstateZoneTemplates> &real_estate_zone_templatess,
		int real_estate_zone_templates_id
	)
	{
		for (auto &real_estate_zone_templates : real_estate_zone_templatess) {
			if (real_estate_zone_templates.dz_id == real_estate_zone_templates_id) {
				return real_estate_zone_templates;
			}
		}

		return NewEntity();
	}

	static RealEstateZoneTemplates FindOne(
		Database& db,
		int real_estate_zone_templates_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_zone_templates_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateZoneTemplates e{};

			e.dz_id           = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.zone_id         = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.zone_version    = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.zone_in_x       = row[3] ? strtof(row[3], nullptr) : 0;
			e.zone_in_y       = row[4] ? strtof(row[4], nullptr) : 0;
			e.zone_in_z       = row[5] ? strtof(row[5], nullptr) : 0;
			e.zone_in_heading = row[6] ? strtof(row[6], nullptr) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_zone_templates_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_zone_templates_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateZoneTemplates &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.dz_id));
		v.push_back(columns[1] + " = " + std::to_string(e.zone_id));
		v.push_back(columns[2] + " = " + std::to_string(e.zone_version));
		v.push_back(columns[3] + " = " + std::to_string(e.zone_in_x));
		v.push_back(columns[4] + " = " + std::to_string(e.zone_in_y));
		v.push_back(columns[5] + " = " + std::to_string(e.zone_in_z));
		v.push_back(columns[6] + " = " + std::to_string(e.zone_in_heading));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.dz_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstateZoneTemplates InsertOne(
		Database& db,
		RealEstateZoneTemplates e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.dz_id));
		v.push_back(std::to_string(e.zone_id));
		v.push_back(std::to_string(e.zone_version));
		v.push_back(std::to_string(e.zone_in_x));
		v.push_back(std::to_string(e.zone_in_y));
		v.push_back(std::to_string(e.zone_in_z));
		v.push_back(std::to_string(e.zone_in_heading));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.dz_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstateZoneTemplates> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.dz_id));
			v.push_back(std::to_string(e.zone_id));
			v.push_back(std::to_string(e.zone_version));
			v.push_back(std::to_string(e.zone_in_x));
			v.push_back(std::to_string(e.zone_in_y));
			v.push_back(std::to_string(e.zone_in_z));
			v.push_back(std::to_string(e.zone_in_heading));

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

	static std::vector<RealEstateZoneTemplates> All(Database& db)
	{
		std::vector<RealEstateZoneTemplates> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateZoneTemplates e{};

			e.dz_id           = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.zone_id         = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.zone_version    = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.zone_in_x       = row[3] ? strtof(row[3], nullptr) : 0;
			e.zone_in_y       = row[4] ? strtof(row[4], nullptr) : 0;
			e.zone_in_z       = row[5] ? strtof(row[5], nullptr) : 0;
			e.zone_in_heading = row[6] ? strtof(row[6], nullptr) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateZoneTemplates> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateZoneTemplates> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateZoneTemplates e{};

			e.dz_id           = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.zone_id         = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.zone_version    = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.zone_in_x       = row[3] ? strtof(row[3], nullptr) : 0;
			e.zone_in_y       = row[4] ? strtof(row[4], nullptr) : 0;
			e.zone_in_z       = row[5] ? strtof(row[5], nullptr) : 0;
			e.zone_in_heading = row[6] ? strtof(row[6], nullptr) : 0;

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
		const RealEstateZoneTemplates &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.dz_id));
		v.push_back(std::to_string(e.zone_id));
		v.push_back(std::to_string(e.zone_version));
		v.push_back(std::to_string(e.zone_in_x));
		v.push_back(std::to_string(e.zone_in_y));
		v.push_back(std::to_string(e.zone_in_z));
		v.push_back(std::to_string(e.zone_in_heading));

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
		const std::vector<RealEstateZoneTemplates> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.dz_id));
			v.push_back(std::to_string(e.zone_id));
			v.push_back(std::to_string(e.zone_version));
			v.push_back(std::to_string(e.zone_in_x));
			v.push_back(std::to_string(e.zone_in_y));
			v.push_back(std::to_string(e.zone_in_z));
			v.push_back(std::to_string(e.zone_in_heading));

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

#endif //EQEMU_BASE_REAL_ESTATE_ZONE_TEMPLATES_REPOSITORY_H
