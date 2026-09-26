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

#ifndef EQEMU_BASE_GUILD_TROPHY_TRIBUTE_STATE_REPOSITORY_H
#define EQEMU_BASE_GUILD_TROPHY_TRIBUTE_STATE_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseGuildTrophyTributeStateRepository {
public:
	struct GuildTrophyTributeState {
		uint32_t guild_id;
		uint8_t  active;
		uint32_t time_remaining;
		uint32_t edit_lock_character_id;
		time_t   edit_lock_at;
	};

	static std::string PrimaryKey()
	{
		return std::string("guild_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"guild_id",
			"active",
			"time_remaining",
			"edit_lock_character_id",
			"edit_lock_at",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"guild_id",
			"active",
			"time_remaining",
			"edit_lock_character_id",
			"UNIX_TIMESTAMP(edit_lock_at)",
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
		return std::string("guild_trophy_tribute_state");
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

	static GuildTrophyTributeState NewEntity()
	{
		GuildTrophyTributeState e{};

		e.guild_id               = 0;
		e.active                 = 0;
		e.time_remaining         = 0;
		e.edit_lock_character_id = 0;
		e.edit_lock_at           = 0;

		return e;
	}

	static GuildTrophyTributeState GetGuildTrophyTributeState(
		const std::vector<GuildTrophyTributeState> &guild_trophy_tribute_states,
		int guild_trophy_tribute_state_id
	)
	{
		for (auto &guild_trophy_tribute_state : guild_trophy_tribute_states) {
			if (guild_trophy_tribute_state.guild_id == guild_trophy_tribute_state_id) {
				return guild_trophy_tribute_state;
			}
		}

		return NewEntity();
	}

	static GuildTrophyTributeState FindOne(
		Database& db,
		int guild_trophy_tribute_state_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {} = {} LIMIT 1",
				BaseSelect(),
				PrimaryKey(),
				guild_trophy_tribute_state_id
			)
		);

		auto row = results.begin();
		if (results.RowCount() == 1) {
			GuildTrophyTributeState e{};

			e.guild_id               = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.active                 = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.time_remaining         = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.edit_lock_character_id = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.edit_lock_at           = strtoll(row[4] ? row[4] : "-1", nullptr, 10);

			return e;
		}

		return NewEntity();
	}

	static int DeleteOne(
		Database& db,
		int guild_trophy_tribute_state_id
	)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"DELETE FROM {} WHERE {} = {}",
				TableName(),
				PrimaryKey(),
				guild_trophy_tribute_state_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static int UpdateOne(
		Database& db,
		const GuildTrophyTributeState &e
	)
	{
		std::vector<std::string> v;

		auto columns = Columns();

		v.push_back(columns[0] + " = " + std::to_string(e.guild_id));
		v.push_back(columns[1] + " = " + std::to_string(e.active));
		v.push_back(columns[2] + " = " + std::to_string(e.time_remaining));
		v.push_back(columns[3] + " = " + std::to_string(e.edit_lock_character_id));
		v.push_back(columns[4] + " = FROM_UNIXTIME(" + (e.edit_lock_at > 0 ? std::to_string(e.edit_lock_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}",
				TableName(),
				Strings::Implode(", ", v),
				PrimaryKey(),
				e.guild_id
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}

	static GuildTrophyTributeState InsertOne(
		Database& db,
		GuildTrophyTributeState e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.guild_id));
		v.push_back(std::to_string(e.active));
		v.push_back(std::to_string(e.time_remaining));
		v.push_back(std::to_string(e.edit_lock_character_id));
		v.push_back("FROM_UNIXTIME(" + (e.edit_lock_at > 0 ? std::to_string(e.edit_lock_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", v)
			)
		);

		if (results.Success()) {
			e.guild_id = results.LastInsertedID();
			return e;
		}

		e = NewEntity();

		return e;
	}

	static int InsertMany(
		Database& db,
		const std::vector<GuildTrophyTributeState> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.guild_id));
			v.push_back(std::to_string(e.active));
			v.push_back(std::to_string(e.time_remaining));
			v.push_back(std::to_string(e.edit_lock_character_id));
			v.push_back("FROM_UNIXTIME(" + (e.edit_lock_at > 0 ? std::to_string(e.edit_lock_at) : "null") + ")");

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

	static std::vector<GuildTrophyTributeState> All(Database& db)
	{
		std::vector<GuildTrophyTributeState> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{}",
				BaseSelect()
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			GuildTrophyTributeState e{};

			e.guild_id               = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.active                 = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.time_remaining         = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.edit_lock_character_id = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.edit_lock_at           = strtoll(row[4] ? row[4] : "-1", nullptr, 10);

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::vector<GuildTrophyTributeState> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<GuildTrophyTributeState> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			GuildTrophyTributeState e{};

			e.guild_id               = row[0] ? static_cast<uint32_t>(strtoul(row[0], nullptr, 10)) : 0;
			e.active                 = row[1] ? static_cast<uint8_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.time_remaining         = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.edit_lock_character_id = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.edit_lock_at           = strtoll(row[4] ? row[4] : "-1", nullptr, 10);

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
		const GuildTrophyTributeState &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.guild_id));
		v.push_back(std::to_string(e.active));
		v.push_back(std::to_string(e.time_remaining));
		v.push_back(std::to_string(e.edit_lock_character_id));
		v.push_back("FROM_UNIXTIME(" + (e.edit_lock_at > 0 ? std::to_string(e.edit_lock_at) : "null") + ")");

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
		const std::vector<GuildTrophyTributeState> &entries
	)
	{
		std::vector<std::string> insert_chunks;

		for (auto &e: entries) {
			std::vector<std::string> v;

			v.push_back(std::to_string(e.guild_id));
			v.push_back(std::to_string(e.active));
			v.push_back(std::to_string(e.time_remaining));
			v.push_back(std::to_string(e.edit_lock_character_id));
			v.push_back("FROM_UNIXTIME(" + (e.edit_lock_at > 0 ? std::to_string(e.edit_lock_at) : "null") + ")");

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

#endif //EQEMU_BASE_GUILD_TROPHY_TRIBUTE_STATE_REPOSITORY_H
