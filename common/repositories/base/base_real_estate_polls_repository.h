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

#ifndef EQEMU_BASE_REAL_ESTATE_POLLS_REPOSITORY_H
#define EQEMU_BASE_REAL_ESTATE_POLLS_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseRealEstatePollsRepository {
public:
	struct RealEstatePolls {
		uint32_t    id;
		uint8_t     kind;
		std::string question;
		std::string polling_location;
		uint8_t     selection_type;
		uint32_t    max_selections;
		time_t      opens_at;
		time_t      closes_at;
		uint8_t     published;
		uint8_t     archived;
		uint8_t     results_unpublished;
		uint8_t     prompt_players;
	};

	static std::string PrimaryKey()
	{
		return std::string("id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"id",
			"kind",
			"question",
			"polling_location",
			"selection_type",
			"max_selections",
			"opens_at",
			"closes_at",
			"published",
			"archived",
			"results_unpublished",
			"prompt_players",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"id",
			"kind",
			"question",
			"polling_location",
			"selection_type",
			"max_selections",
			"UNIX_TIMESTAMP(opens_at)",
			"UNIX_TIMESTAMP(closes_at)",
			"published",
			"archived",
			"results_unpublished",
			"prompt_players",
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
		return std::string("real_estate_polls");
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

	static RealEstatePolls NewEntity()
	{
		RealEstatePolls e{};

		e.id                  = 0;
		e.kind                = 0;
		e.question            = "";
		e.polling_location    = "";
		e.selection_type      = 0;
		e.max_selections      = 1;
		e.opens_at            = 0;
		e.closes_at           = 0;
		e.published           = 0;
		e.archived            = 0;
		e.results_unpublished = 0;
		e.prompt_players      = 0;

		return e;
	}

	static RealEstatePolls GetRealEstatePolls(
		const std::vector<RealEstatePolls> &real_estate_pollss,
		int real_estate_polls_id
	)
	{
		for (auto &real_estate_polls : real_estate_pollss) {
			if (real_estate_polls.id == real_estate_polls_id) {
				return real_estate_polls;
			}
		}

		return NewEntity();
	}

	static RealEstatePolls FindOne(
		Database& db,
		int real_estate_polls_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				real_estate_polls_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			RealEstatePolls e{};

			e.id                  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.kind                = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.question            = row[2] ? row[2] : "";
			e.polling_location    = row[3] ? row[3] : "";
			e.selection_type      = row[4] ? static_cast<uint8_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.max_selections      = row[5] ? static_cast<uint32_t>(strtoul(row[5], nullptr, 10)) : 1;
			e.opens_at            = strtoll(row[6] ? row[6] : "-1", nullptr, 10);
			e.closes_at           = strtoll(row[7] ? row[7] : "-1", nullptr, 10);
			e.published           = row[8] ? static_cast<uint8_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.archived            = row[9] ? static_cast<uint8_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.results_unpublished = row[10] ? static_cast<uint8_t>(strtoul(row[10], nullptr, 10)) : 0;
			e.prompt_players      = row[11] ? static_cast<uint8_t>(strtoul(row[11], nullptr, 10)) : 0;

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int real_estate_polls_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				real_estate_polls_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const RealEstatePolls &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[1] + " = " + std::to_string(e.kind));
		v.push_back(columns[2] + " = '" + Strings::Escape(e.question) + "'");
		v.push_back(columns[3] + " = '" + Strings::Escape(e.polling_location) + "'");
		v.push_back(columns[4] + " = " + std::to_string(e.selection_type));
		v.push_back(columns[5] + " = " + std::to_string(e.max_selections));
		v.push_back(columns[6] + " = FROM_UNIXTIME(" + (e.opens_at > 0 ? std::to_string(e.opens_at) : "null") + ")");
		v.push_back(columns[7] + " = FROM_UNIXTIME(" + (e.closes_at > 0 ? std::to_string(e.closes_at) : "null") + ")");
		v.push_back(columns[8] + " = " + std::to_string(e.published));
		v.push_back(columns[9] + " = " + std::to_string(e.archived));
		v.push_back(columns[10] + " = " + std::to_string(e.results_unpublished));
		v.push_back(columns[11] + " = " + std::to_string(e.prompt_players));

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

	static RealEstatePolls InsertOne(
		Database& db,
		RealEstatePolls e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.kind));
		v.push_back("'" + Strings::Escape(e.question) + "'");
		v.push_back("'" + Strings::Escape(e.polling_location) + "'");
		v.push_back(std::to_string(e.selection_type));
		v.push_back(std::to_string(e.max_selections));
		v.push_back("FROM_UNIXTIME(" + (e.opens_at > 0 ? std::to_string(e.opens_at) : "null") + ")");
		v.push_back("FROM_UNIXTIME(" + (e.closes_at > 0 ? std::to_string(e.closes_at) : "null") + ")");
		v.push_back(std::to_string(e.published));
		v.push_back(std::to_string(e.archived));
		v.push_back(std::to_string(e.results_unpublished));
		v.push_back(std::to_string(e.prompt_players));

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
		const std::vector<RealEstatePolls> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.kind));
			v.push_back("'" + Strings::Escape(e.question) + "'");
			v.push_back("'" + Strings::Escape(e.polling_location) + "'");
			v.push_back(std::to_string(e.selection_type));
			v.push_back(std::to_string(e.max_selections));
			v.push_back("FROM_UNIXTIME(" + (e.opens_at > 0 ? std::to_string(e.opens_at) : "null") + ")");
			v.push_back("FROM_UNIXTIME(" + (e.closes_at > 0 ? std::to_string(e.closes_at) : "null") + ")");
			v.push_back(std::to_string(e.published));
			v.push_back(std::to_string(e.archived));
			v.push_back(std::to_string(e.results_unpublished));
			v.push_back(std::to_string(e.prompt_players));

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

	static std::vector<RealEstatePolls> All(Database& db)
	{
		std::vector<RealEstatePolls> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstatePolls e{};

			e.id                  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.kind                = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.question            = row[2] ? row[2] : "";
			e.polling_location    = row[3] ? row[3] : "";
			e.selection_type      = row[4] ? static_cast<uint8_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.max_selections      = row[5] ? static_cast<uint32_t>(strtoul(row[5], nullptr, 10)) : 1;
			e.opens_at            = strtoll(row[6] ? row[6] : "-1", nullptr, 10);
			e.closes_at           = strtoll(row[7] ? row[7] : "-1", nullptr, 10);
			e.published           = row[8] ? static_cast<uint8_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.archived            = row[9] ? static_cast<uint8_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.results_unpublished = row[10] ? static_cast<uint8_t>(strtoul(row[10], nullptr, 10)) : 0;
			e.prompt_players      = row[11] ? static_cast<uint8_t>(strtoul(row[11], nullptr, 10)) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<RealEstatePolls> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<RealEstatePolls> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			RealEstatePolls e{};

			e.id                  = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.kind                = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.question            = row[2] ? row[2] : "";
			e.polling_location    = row[3] ? row[3] : "";
			e.selection_type      = row[4] ? static_cast<uint8_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.max_selections      = row[5] ? static_cast<uint32_t>(strtoul(row[5], nullptr, 10)) : 1;
			e.opens_at            = strtoll(row[6] ? row[6] : "-1", nullptr, 10);
			e.closes_at           = strtoll(row[7] ? row[7] : "-1", nullptr, 10);
			e.published           = row[8] ? static_cast<uint8_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.archived            = row[9] ? static_cast<uint8_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.results_unpublished = row[10] ? static_cast<uint8_t>(strtoul(row[10], nullptr, 10)) : 0;
			e.prompt_players      = row[11] ? static_cast<uint8_t>(strtoul(row[11], nullptr, 10)) : 0;

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
		const RealEstatePolls &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.id));
		v.push_back(std::to_string(e.kind));
		v.push_back("'" + Strings::Escape(e.question) + "'");
		v.push_back("'" + Strings::Escape(e.polling_location) + "'");
		v.push_back(std::to_string(e.selection_type));
		v.push_back(std::to_string(e.max_selections));
		v.push_back("FROM_UNIXTIME(" + (e.opens_at > 0 ? std::to_string(e.opens_at) : "null") + ")");
		v.push_back("FROM_UNIXTIME(" + (e.closes_at > 0 ? std::to_string(e.closes_at) : "null") + ")");
		v.push_back(std::to_string(e.published));
		v.push_back(std::to_string(e.archived));
		v.push_back(std::to_string(e.results_unpublished));
		v.push_back(std::to_string(e.prompt_players));

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
		const std::vector<RealEstatePolls> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.id));
			v.push_back(std::to_string(e.kind));
			v.push_back("'" + Strings::Escape(e.question) + "'");
			v.push_back("'" + Strings::Escape(e.polling_location) + "'");
			v.push_back(std::to_string(e.selection_type));
			v.push_back(std::to_string(e.max_selections));
			v.push_back("FROM_UNIXTIME(" + (e.opens_at > 0 ? std::to_string(e.opens_at) : "null") + ")");
			v.push_back("FROM_UNIXTIME(" + (e.closes_at > 0 ? std::to_string(e.closes_at) : "null") + ")");
			v.push_back(std::to_string(e.published));
			v.push_back(std::to_string(e.archived));
			v.push_back(std::to_string(e.results_unpublished));
			v.push_back(std::to_string(e.prompt_players));

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

#endif //EQEMU_BASE_REAL_ESTATE_POLLS_REPOSITORY_H
