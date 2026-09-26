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

#ifndef EQEMU_BASE_REAL_ESTATE_REGIONS_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_REGIONS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateRegionsRepository {
public:
	struct RealEstateRegions {
		uint32_t    id;
		std::string name;
		float       x;
		float       y;
		float       z;
		float       size_x;
		float       size_y;
		float       size_z;
		float       heading;
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
			"x",
			"y",
			"z",
			"size_x",
			"size_y",
			"size_z",
			"heading",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"id",
			"name",
			"x",
			"y",
			"z",
			"size_x",
			"size_y",
			"size_z",
			"heading",
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
		return std::string("real_estate_regions");
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

	static RealEstateRegions NewEntity()
	{
		RealEstateRegions e{};

		e.id      = 0;
		e.name    = "";
		e.x       = 0;
		e.y       = 0;
		e.z       = 0;
		e.size_x  = 0;
		e.size_y  = 0;
		e.size_z  = 0;
		e.heading = 0;

		return e;
	}

	static RealEstateRegions GetRealEstateRegions(
		const std::vector<RealEstateRegions> &real_estate_regionss,
		int real_estate_regions_id
	)
	{
		for (auto &real_estate_regions : real_estate_regionss) {
			if (real_estate_regions.id == real_estate_regions_id) {
				return real_estate_regions;
			}
		}

		return NewEntity();
	}

	static RealEstateRegions FindOne(
		Database& db,
		int real_estate_regions_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_regions_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateRegions e{};

			e.id      = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.name    = row[1] ? row[1] : "";
			e.x       = row[2] ? strtof(row[2], nullptr) : 0;
			e.y       = row[3] ? strtof(row[3], nullptr) : 0;
			e.z       = row[4] ? strtof(row[4], nullptr) : 0;
			e.size_x  = row[5] ? strtof(row[5], nullptr) : 0;
			e.size_y  = row[6] ? strtof(row[6], nullptr) : 0;
			e.size_z  = row[7] ? strtof(row[7], nullptr) : 0;
			e.heading = row[8] ? strtof(row[8], nullptr) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_regions_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_regions_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateRegions &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.id));
		v.push_back(columns[1] + " = '" + Strings::Escape(e.name) + "'");
		v.push_back(columns[2] + " = " + std::to_string(e.x));
		v.push_back(columns[3] + " = " + std::to_string(e.y));
		v.push_back(columns[4] + " = " + std::to_string(e.z));
		v.push_back(columns[5] + " = " + std::to_string(e.size_x));
		v.push_back(columns[6] + " = " + std::to_string(e.size_y));
		v.push_back(columns[7] + " = " + std::to_string(e.size_z));
		v.push_back(columns[8] + " = " + std::to_string(e.heading));

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

	static RealEstateRegions InsertOne(
		Database& db,
		RealEstateRegions e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back("'" + Strings::Escape(e.name) + "'");
		v.push_back(std::to_string(e.x));
		v.push_back(std::to_string(e.y));
		v.push_back(std::to_string(e.z));
		v.push_back(std::to_string(e.size_x));
		v.push_back(std::to_string(e.size_y));
		v.push_back(std::to_string(e.size_z));
		v.push_back(std::to_string(e.heading));

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
		const std::vector<RealEstateRegions> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back("'" + Strings::Escape(e.name) + "'");
			v.push_back(std::to_string(e.x));
			v.push_back(std::to_string(e.y));
			v.push_back(std::to_string(e.z));
			v.push_back(std::to_string(e.size_x));
			v.push_back(std::to_string(e.size_y));
			v.push_back(std::to_string(e.size_z));
			v.push_back(std::to_string(e.heading));

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

	static std::vector<RealEstateRegions> All(Database& db)
	{
		std::vector<RealEstateRegions> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateRegions e{};

			e.id      = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.name    = row[1] ? row[1] : "";
			e.x       = row[2] ? strtof(row[2], nullptr) : 0;
			e.y       = row[3] ? strtof(row[3], nullptr) : 0;
			e.z       = row[4] ? strtof(row[4], nullptr) : 0;
			e.size_x  = row[5] ? strtof(row[5], nullptr) : 0;
			e.size_y  = row[6] ? strtof(row[6], nullptr) : 0;
			e.size_z  = row[7] ? strtof(row[7], nullptr) : 0;
			e.heading = row[8] ? strtof(row[8], nullptr) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateRegions> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateRegions> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateRegions e{};

			e.id      = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.name    = row[1] ? row[1] : "";
			e.x       = row[2] ? strtof(row[2], nullptr) : 0;
			e.y       = row[3] ? strtof(row[3], nullptr) : 0;
			e.z       = row[4] ? strtof(row[4], nullptr) : 0;
			e.size_x  = row[5] ? strtof(row[5], nullptr) : 0;
			e.size_y  = row[6] ? strtof(row[6], nullptr) : 0;
			e.size_z  = row[7] ? strtof(row[7], nullptr) : 0;
			e.heading = row[8] ? strtof(row[8], nullptr) : 0;

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
		const RealEstateRegions &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back("'" + Strings::Escape(e.name) + "'");
		v.push_back(std::to_string(e.x));
		v.push_back(std::to_string(e.y));
		v.push_back(std::to_string(e.z));
		v.push_back(std::to_string(e.size_x));
		v.push_back(std::to_string(e.size_y));
		v.push_back(std::to_string(e.size_z));
		v.push_back(std::to_string(e.heading));

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
		const std::vector<RealEstateRegions> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back("'" + Strings::Escape(e.name) + "'");
			v.push_back(std::to_string(e.x));
			v.push_back(std::to_string(e.y));
			v.push_back(std::to_string(e.z));
			v.push_back(std::to_string(e.size_x));
			v.push_back(std::to_string(e.size_y));
			v.push_back(std::to_string(e.size_z));
			v.push_back(std::to_string(e.heading));

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

#endif //EQEMU_BASE_REAL_ESTATE_REGIONS_REPOSITORY_H
