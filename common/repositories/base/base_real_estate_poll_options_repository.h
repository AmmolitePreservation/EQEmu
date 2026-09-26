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

#ifndef EQEMU_BASE_REAL_ESTATE_POLL_OPTIONS_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_POLL_OPTIONS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstatePollOptionsRepository {
public:
	struct RealEstatePollOptions {
		uint32_t poll_id;
		uint32_t option_key;
		uint8_t  position;
		uint32_t unknown_value;
	};

	static std::string PrimaryKey()
	{
		return std::string("poll_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"poll_id",
			"option_key",
			"position",
			"unknown_value",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"poll_id",
			"option_key",
			"position",
			"unknown_value",
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
		return std::string("real_estate_poll_options");
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

	static RealEstatePollOptions NewEntity()
	{
		RealEstatePollOptions e{};

		e.poll_id       = 0;
		e.option_key    = 0;
		e.position      = 0;
		e.unknown_value = 0;

		return e;
	}

	static RealEstatePollOptions GetRealEstatePollOptions(
		const std::vector<RealEstatePollOptions> &real_estate_poll_optionss,
		int real_estate_poll_options_id
	)
	{
		for (auto &real_estate_poll_options : real_estate_poll_optionss) {
			if (real_estate_poll_options.poll_id == real_estate_poll_options_id) {
				return real_estate_poll_options;
			}
		}

		return NewEntity();
	}

	static RealEstatePollOptions FindOne(
		Database& db,
		int real_estate_poll_options_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_poll_options_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstatePollOptions e{};

			e.poll_id       = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.option_key    = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.position      = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.unknown_value = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_poll_options_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_poll_options_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstatePollOptions &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.poll_id));
		v.push_back(columns[1] + " = " + std::to_string(e.option_key));
		v.push_back(columns[2] + " = " + std::to_string(e.position));
		v.push_back(columns[3] + " = " + std::to_string(e.unknown_value));

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.poll_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static RealEstatePollOptions InsertOne(
		Database& db,
		RealEstatePollOptions e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.poll_id));
		v.push_back(std::to_string(e.option_key));
		v.push_back(std::to_string(e.position));
		v.push_back(std::to_string(e.unknown_value));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.poll_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<RealEstatePollOptions> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.poll_id));
			v.push_back(std::to_string(e.option_key));
			v.push_back(std::to_string(e.position));
			v.push_back(std::to_string(e.unknown_value));

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

	static std::vector<RealEstatePollOptions> All(Database& db)
	{
		std::vector<RealEstatePollOptions> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstatePollOptions e{};

			e.poll_id       = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.option_key    = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.position      = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.unknown_value = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstatePollOptions> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstatePollOptions> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstatePollOptions e{};

			e.poll_id       = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.option_key    = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.position      = row[2] ? static_cast<uint8_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.unknown_value = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;

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
		const RealEstatePollOptions &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.poll_id));
		v.push_back(std::to_string(e.option_key));
		v.push_back(std::to_string(e.position));
		v.push_back(std::to_string(e.unknown_value));

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
		const std::vector<RealEstatePollOptions> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.poll_id));
			v.push_back(std::to_string(e.option_key));
			v.push_back(std::to_string(e.position));
			v.push_back(std::to_string(e.unknown_value));

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

#endif //EQEMU_BASE_REAL_ESTATE_POLL_OPTIONS_REPOSITORY_H
