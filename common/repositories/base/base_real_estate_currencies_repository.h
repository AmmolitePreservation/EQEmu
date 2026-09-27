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

#ifndef EQEMU_BASE_REAL_ESTATE_CURRENCIES_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_CURRENCIES_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstateCurrenciesRepository {
public:
	struct RealEstateCurrencies {
		uint32_t alternate_currency_id;
		uint32_t sub_key;
		float    platinum_per_unit;
		uint8_t  plot_purchase;
		uint8_t  plot_upkeep;
		uint8_t  item_upkeep;
	};

	static std::string PrimaryKey()
	{
		return std::string("alternate_currency_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"alternate_currency_id",
			"sub_key",
			"platinum_per_unit",
			"plot_purchase",
			"plot_upkeep",
			"item_upkeep",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"alternate_currency_id",
			"sub_key",
			"platinum_per_unit",
			"plot_purchase",
			"plot_upkeep",
			"item_upkeep",
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
		return std::string("real_estate_currencies");
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

	static RealEstateCurrencies NewEntity()
	{
		RealEstateCurrencies e{};

		e.alternate_currency_id = 0;
		e.sub_key               = 0;
		e.platinum_per_unit     = 0;
		e.plot_purchase         = 0;
		e.plot_upkeep           = 0;
		e.item_upkeep           = 0;

		return e;
	}

	static RealEstateCurrencies GetRealEstateCurrencies(
		const std::vector<RealEstateCurrencies> &real_estate_currenciess,
		int real_estate_currencies_id
	)
	{
		for (auto &real_estate_currencies : real_estate_currenciess) {
			if (real_estate_currencies.alternate_currency_id == real_estate_currencies_id) {
				return real_estate_currencies;
			}
		}

		return NewEntity();
	}

	static RealEstateCurrencies FindOne(
		Database& db,
		int real_estate_currencies_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_currencies_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstateCurrencies e{};

			e.alternate_currency_id = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.sub_key               = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.platinum_per_unit     = row[2] ? strtof(row[2], nullptr) : 0;
			e.plot_purchase         = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.plot_upkeep           = row[4] ? static_cast<uint8_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.item_upkeep           = row[5] ? static_cast<uint8_t>(strtoul(row[5], nullptr, 10)) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_currencies_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_currencies_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstateCurrencies &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.alternate_currency_id));
		v.push_back(columns[1] + " = " + std::to_string(e.sub_key));
		v.push_back(columns[2] + " = " + std::to_string(e.platinum_per_unit));
		v.push_back(columns[3] + " = " + std::to_string(e.plot_purchase));
		v.push_back(columns[4] + " = " + std::to_string(e.plot_upkeep));
		v.push_back(columns[5] + " = " + std::to_string(e.item_upkeep));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.alternate_currency_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstateCurrencies InsertOne(
		Database& db,
		RealEstateCurrencies e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.alternate_currency_id));
		v.push_back(std::to_string(e.sub_key));
		v.push_back(std::to_string(e.platinum_per_unit));
		v.push_back(std::to_string(e.plot_purchase));
		v.push_back(std::to_string(e.plot_upkeep));
		v.push_back(std::to_string(e.item_upkeep));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.alternate_currency_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstateCurrencies> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.alternate_currency_id));
			v.push_back(std::to_string(e.sub_key));
			v.push_back(std::to_string(e.platinum_per_unit));
			v.push_back(std::to_string(e.plot_purchase));
			v.push_back(std::to_string(e.plot_upkeep));
			v.push_back(std::to_string(e.item_upkeep));

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

	static std::vector<RealEstateCurrencies> All(Database& db)
	{
		std::vector<RealEstateCurrencies> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateCurrencies e{};

			e.alternate_currency_id = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.sub_key               = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.platinum_per_unit     = row[2] ? strtof(row[2], nullptr) : 0;
			e.plot_purchase         = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.plot_upkeep           = row[4] ? static_cast<uint8_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.item_upkeep           = row[5] ? static_cast<uint8_t>(strtoul(row[5], nullptr, 10)) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstateCurrencies> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstateCurrencies> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstateCurrencies e{};

			e.alternate_currency_id = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.sub_key               = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.platinum_per_unit     = row[2] ? strtof(row[2], nullptr) : 0;
			e.plot_purchase         = row[3] ? static_cast<uint8_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.plot_upkeep           = row[4] ? static_cast<uint8_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.item_upkeep           = row[5] ? static_cast<uint8_t>(strtoul(row[5], nullptr, 10)) : 0;

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
		const RealEstateCurrencies &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.alternate_currency_id));
		v.push_back(std::to_string(e.sub_key));
		v.push_back(std::to_string(e.platinum_per_unit));
		v.push_back(std::to_string(e.plot_purchase));
		v.push_back(std::to_string(e.plot_upkeep));
		v.push_back(std::to_string(e.item_upkeep));

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
		const std::vector<RealEstateCurrencies> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.alternate_currency_id));
			v.push_back(std::to_string(e.sub_key));
			v.push_back(std::to_string(e.platinum_per_unit));
			v.push_back(std::to_string(e.plot_purchase));
			v.push_back(std::to_string(e.plot_upkeep));
			v.push_back(std::to_string(e.item_upkeep));

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

#endif //EQEMU_BASE_REAL_ESTATE_CURRENCIES_REPOSITORY_H
