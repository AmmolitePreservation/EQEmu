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

#ifndef EQEMU_BASE_REAL_ESTATE_COST_ITEMS_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_COST_ITEMS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateCostItemsRepository {
public:
	struct RealEstateCostItems {
		uint32_t cost_id;
		uint32_t option_key;
		uint32_t currency;
		uint32_t sub_key;
		int64_t  amount;
	};

	static std::string PrimaryKey()
	{
		return std::string("cost_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"cost_id",
			"option_key",
			"currency",
			"sub_key",
			"amount",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"cost_id",
			"option_key",
			"currency",
			"sub_key",
			"amount",
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
		return std::string("real_estate_cost_items");
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

	static RealEstateCostItems NewEntity()
	{
		RealEstateCostItems e{};

		e.cost_id    = 0;
		e.option_key = 0;
		e.currency   = 0;
		e.sub_key    = 0;
		e.amount     = 0;

		return e;
	}

	static RealEstateCostItems GetRealEstateCostItems(
		const std::vector<RealEstateCostItems> &real_estate_cost_itemss,
		int real_estate_cost_items_id
	)
	{
		for (auto &real_estate_cost_items : real_estate_cost_itemss) {
			if (real_estate_cost_items.cost_id == real_estate_cost_items_id) {
				return real_estate_cost_items;
			}
		}

		return NewEntity();
	}

	static RealEstateCostItems FindOne(
		Database& db,
		int real_estate_cost_items_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_cost_items_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateCostItems e{};

			e.cost_id    = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.option_key = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.currency   = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.sub_key    = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.amount     = row[4] ? strtoll(row[4], nullptr, 10) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_cost_items_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_cost_items_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateCostItems &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.cost_id));
		v.push_back(columns[1] + " = " + std::to_string(e.option_key));
		v.push_back(columns[2] + " = " + std::to_string(e.currency));
		v.push_back(columns[3] + " = " + std::to_string(e.sub_key));
		v.push_back(columns[4] + " = " + std::to_string(e.amount));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.cost_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstateCostItems InsertOne(
		Database& db,
		RealEstateCostItems e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.cost_id));
		v.push_back(std::to_string(e.option_key));
		v.push_back(std::to_string(e.currency));
		v.push_back(std::to_string(e.sub_key));
		v.push_back(std::to_string(e.amount));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.cost_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstateCostItems> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.cost_id));
			v.push_back(std::to_string(e.option_key));
			v.push_back(std::to_string(e.currency));
			v.push_back(std::to_string(e.sub_key));
			v.push_back(std::to_string(e.amount));

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

	static std::vector<RealEstateCostItems> All(Database& db)
	{
		std::vector<RealEstateCostItems> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateCostItems e{};

			e.cost_id    = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.option_key = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.currency   = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.sub_key    = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.amount     = row[4] ? strtoll(row[4], nullptr, 10) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateCostItems> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateCostItems> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateCostItems e{};

			e.cost_id    = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.option_key = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.currency   = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.sub_key    = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.amount     = row[4] ? strtoll(row[4], nullptr, 10) : 0;

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
		const RealEstateCostItems &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.cost_id));
		v.push_back(std::to_string(e.option_key));
		v.push_back(std::to_string(e.currency));
		v.push_back(std::to_string(e.sub_key));
		v.push_back(std::to_string(e.amount));

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
		const std::vector<RealEstateCostItems> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.cost_id));
			v.push_back(std::to_string(e.option_key));
			v.push_back(std::to_string(e.currency));
			v.push_back(std::to_string(e.sub_key));
			v.push_back(std::to_string(e.amount));

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

#endif //EQEMU_BASE_REAL_ESTATE_COST_ITEMS_REPOSITORY_H
