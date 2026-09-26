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

#ifndef EQEMU_BASE_REAL_ESTATE_ACCESS_GROUPS_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_ACCESS_GROUPS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateAccessGroupsRepository {
public:
	struct RealEstateAccessGroups {
		uint32_t property_id;
		uint8_t  group_number;
		uint32_t permissions;
	};

	static std::string PrimaryKey()
	{
		return std::string("property_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"property_id",
			"group_number",
			"permissions",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"property_id",
			"group_number",
			"permissions",
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
		return std::string("real_estate_access_groups");
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

	static RealEstateAccessGroups NewEntity()
	{
		RealEstateAccessGroups e{};

		e.property_id  = 0;
		e.group_number = 0;
		e.permissions  = 0;

		return e;
	}

	static RealEstateAccessGroups GetRealEstateAccessGroups(
		const std::vector<RealEstateAccessGroups> &real_estate_access_groupss,
		int real_estate_access_groups_id
	)
	{
		for (auto &real_estate_access_groups : real_estate_access_groupss) {
			if (real_estate_access_groups.property_id == real_estate_access_groups_id) {
				return real_estate_access_groups;
			}
		}

		return NewEntity();
	}

	static RealEstateAccessGroups FindOne(
		Database& db,
		int real_estate_access_groups_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_access_groups_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateAccessGroups e{};

			e.property_id  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.group_number = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.permissions  = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_access_groups_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_access_groups_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateAccessGroups &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.property_id));
		v.push_back(columns[1] + " = " + std::to_string(e.group_number));
		v.push_back(columns[2] + " = " + std::to_string(e.permissions));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.property_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstateAccessGroups InsertOne(
		Database& db,
		RealEstateAccessGroups e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.property_id));
		v.push_back(std::to_string(e.group_number));
		v.push_back(std::to_string(e.permissions));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.property_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstateAccessGroups> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.property_id));
			v.push_back(std::to_string(e.group_number));
			v.push_back(std::to_string(e.permissions));

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

	static std::vector<RealEstateAccessGroups> All(Database& db)
	{
		std::vector<RealEstateAccessGroups> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateAccessGroups e{};

			e.property_id  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.group_number = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.permissions  = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateAccessGroups> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateAccessGroups> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateAccessGroups e{};

			e.property_id  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.group_number = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.permissions  = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;

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
		const RealEstateAccessGroups &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.property_id));
		v.push_back(std::to_string(e.group_number));
		v.push_back(std::to_string(e.permissions));

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
		const std::vector<RealEstateAccessGroups> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.property_id));
			v.push_back(std::to_string(e.group_number));
			v.push_back(std::to_string(e.permissions));

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

#endif //EQEMU_BASE_REAL_ESTATE_ACCESS_GROUPS_REPOSITORY_H
