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

#ifndef EQEMU_BASE_REAL_ESTATE_DEFINITION_CHILDREN_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_DEFINITION_CHILDREN_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateDefinitionChildrenRepository {
public:
	struct RealEstateDefinitionChildren {
		uint32_t parent_id;
		uint32_t child_id;
	};

	static std::string PrimaryKey()
	{
		return std::string("parent_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"parent_id",
			"child_id",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"parent_id",
			"child_id",
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
		return std::string("real_estate_definition_children");
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

	static RealEstateDefinitionChildren NewEntity()
	{
		RealEstateDefinitionChildren e{};

		e.parent_id = 0;
		e.child_id  = 0;

		return e;
	}

	static RealEstateDefinitionChildren GetRealEstateDefinitionChildren(
		const std::vector<RealEstateDefinitionChildren> &real_estate_definition_childrens,
		int real_estate_definition_children_id
	)
	{
		for (auto &real_estate_definition_children : real_estate_definition_childrens) {
			if (real_estate_definition_children.parent_id == real_estate_definition_children_id) {
				return real_estate_definition_children;
			}
		}

		return NewEntity();
	}

	static RealEstateDefinitionChildren FindOne(
		Database& db,
		int real_estate_definition_children_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_definition_children_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateDefinitionChildren e{};

			e.parent_id = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.child_id  = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_definition_children_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_definition_children_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateDefinitionChildren &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.parent_id));
		v.push_back(columns[1] + " = " + std::to_string(e.child_id));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.parent_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstateDefinitionChildren InsertOne(
		Database& db,
		RealEstateDefinitionChildren e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.parent_id));
		v.push_back(std::to_string(e.child_id));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.parent_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstateDefinitionChildren> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.parent_id));
			v.push_back(std::to_string(e.child_id));

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

	static std::vector<RealEstateDefinitionChildren> All(Database& db)
	{
		std::vector<RealEstateDefinitionChildren> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateDefinitionChildren e{};

			e.parent_id = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.child_id  = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateDefinitionChildren> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateDefinitionChildren> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateDefinitionChildren e{};

			e.parent_id = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.child_id  = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;

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
		const RealEstateDefinitionChildren &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.parent_id));
		v.push_back(std::to_string(e.child_id));

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
		const std::vector<RealEstateDefinitionChildren> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.parent_id));
			v.push_back(std::to_string(e.child_id));

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

#endif //EQEMU_BASE_REAL_ESTATE_DEFINITION_CHILDREN_REPOSITORY_H
