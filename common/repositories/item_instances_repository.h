#ifndef EQEMU_ITEM_INSTANCES_REPOSITORY_H
#define EQEMU_ITEM_INSTANCES_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "../item_guid.h"
#include <array>
#include <ctime>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>

// Written by hand and on the generator's ignore list: the generator keys a table by one column, cannot write NULL,
// and emits ReplaceOne and DeleteOne, which this table's delete trigger refuses.
class ItemInstancesRepository {
public:
	enum class HolderType : uint8 {
		Inventory,
		Cursor,
		Bank,
		SharedBank,
		Corpse,
		Parcel,
		Pet,
		SuspendedPet,
		Bot,
		GuildBankDeposit,
		GuildBankMain,
		Ground,
	};

	enum class RetirementState : uint8 {
		Destroyed,
		Sold,
		Stacked,
		Consumed,
		Expired,
		Decayed,
		CharacterDeleted,
		RemovedByGM,
		RolledBack,
		PetDonated,
		BotDonated,
	};

	struct ItemInstances {
		ItemGUID                       guid;
		uint32_t                       item_id;
		int16_t                        charges;
		uint32_t                       color;
		bool                           attuned;
		std::string                    custom_data;
		uint32_t                       ornament_icon;
		uint32_t                       ornament_idfile;
		uint32_t                       ornament_hero_model;
		std::optional<HolderType>      holder_type;
		std::optional<uint32_t>        holder_id;
		std::optional<uint16_t>        slot;
		std::optional<ItemGUID>        parent_guid;
		std::optional<uint16_t>        parent_index;
		std::optional<RetirementState> retirement_state;
		std::optional<time_t>          retired_at;
	};

	static constexpr std::array<std::string_view, 12> holder_type_names = {
		"inventory",
		"cursor",
		"bank",
		"shared_bank",
		"corpse",
		"parcel",
		"pet",
		"suspended_pet",
		"bot",
		"guild_bank_deposit",
		"guild_bank_main",
		"ground",
	};
	static_assert(holder_type_names.size() == static_cast<size_t>(HolderType::Ground) + 1);

	static constexpr std::array<std::string_view, 11> retirement_state_names = {
		"destroyed",
		"sold",
		"stacked",
		"consumed",
		"expired",
		"decayed",
		"characterDeleted",
		"removedByGM",
		"rolledBack",
		"petDonated",
		"botDonated",
	};
	static_assert(retirement_state_names.size() == static_cast<size_t>(RetirementState::BotDonated) + 1);

	static constexpr size_t key_column_count = 3;

	static std::string_view HolderTypeName(HolderType holder_type)
	{
		return holder_type_names[static_cast<size_t>(holder_type)];
	}

	static std::optional<HolderType> HolderTypeFromName(std::string_view name)
	{
		for (size_t i = 0; i < holder_type_names.size(); ++i) {
			if (holder_type_names[i] == name) {
				return static_cast<HolderType>(i);
			}
		}

		return std::nullopt;
	}

	static std::string_view RetirementStateName(RetirementState retirement_state)
	{
		return retirement_state_names[static_cast<size_t>(retirement_state)];
	}

	static std::optional<RetirementState> RetirementStateFromName(std::string_view name)
	{
		for (size_t i = 0; i < retirement_state_names.size(); ++i) {
			if (retirement_state_names[i] == name) {
				return static_cast<RetirementState>(i);
			}
		}

		return std::nullopt;
	}

	static std::string TableName()
	{
		return std::string("item_instances");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"guid_first",
			"guid_second",
			"guid_third",
			"item_id",
			"charges",
			"color",
			"attuned",
			"custom_data",
			"ornament_icon",
			"ornament_idfile",
			"ornament_hero_model",
			"holder_type",
			"holder_id",
			"slot",
			"parent_guid_first",
			"parent_guid_second",
			"parent_guid_third",
			"parent_index",
			"retirement_state",
			"retired_at",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"guid_first",
			"guid_second",
			"guid_third",
			"item_id",
			"charges",
			"color",
			"attuned",
			"custom_data",
			"ornament_icon",
			"ornament_idfile",
			"ornament_hero_model",
			"holder_type",
			"holder_id",
			"slot",
			"parent_guid_first",
			"parent_guid_second",
			"parent_guid_third",
			"parent_index",
			"retirement_state",
			"UNIX_TIMESTAMP(retired_at)",
		};
	}

	static std::string BaseSelect()
	{
		return fmt::format(
			"SELECT {} FROM {}",
			Strings::Implode(", ", SelectColumns()),
			TableName()
		);
	}

	static std::string BaseInsert()
	{
		return fmt::format(
			"INSERT INTO {} ({}) ",
			TableName(),
			Strings::Implode(", ", Columns())
		);
	}

	static std::optional<ItemInstances> FindOne(Database &db, const ItemGUID &guid)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE guid_first = {} AND guid_second = {} AND guid_third = {}",
				BaseSelect(),
				guid.first,
				guid.second,
				guid.third
			)
		);

		if (!results.Success() || results.RowCount() != 1) {
			return std::nullopt;
		}

		auto row = results.begin();

		return FromRow(row);
	}

	static std::vector<ItemInstances> GetWhere(Database &db, const std::string &where_filter)
	{
		std::vector<ItemInstances> entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			entries.push_back(FromRow(row));
		}

		return entries;
	}

	// The next number in a character's counter: derived, never stored, and correct because no row is ever deleted.
	static std::optional<uint32_t> NextCounter(Database &db, uint32_t guid_first, uint32_t guid_second)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT COALESCE(MAX(guid_third) + 1, 0) FROM {} WHERE guid_first = {} AND guid_second = {}",
				TableName(),
				guid_first,
				guid_second
			)
		);

		if (!results.Success() || results.RowCount() != 1) {
			return std::nullopt;
		}

		const uint64_t next = strtoull(results.begin()[0], nullptr, 10);
		if (next > std::numeric_limits<uint32_t>::max()) {
			LogError("Item counter for [{}] [{}] is exhausted", guid_first, guid_second);
			return std::nullopt;
		}

		return static_cast<uint32_t>(next);
	}

	static MySQLRequestResult InsertOne(Database &db, const ItemInstances &e)
	{
		return db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseInsert(),
				Strings::Implode(",", Values(e))
			)
		);
	}

	static bool InsertMany(Database &db, const std::vector<ItemInstances> &entries)
	{
		if (entries.empty()) {
			return true;
		}

		std::vector<std::string> insert_chunks;

		for (const auto &e : entries) {
			insert_chunks.push_back("(" + Strings::Implode(",", Values(e)) + ")");
		}

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES {}",
				BaseInsert(),
				Strings::Implode(",", insert_chunks)
			)
		);

		return results.Success();
	}

	static bool UpdateOne(Database &db, const ItemInstances &e)
	{
		const auto columns = Columns();
		const auto values  = Values(e);

		std::vector<std::string> v;

		for (size_t i = key_column_count; i < columns.size(); ++i) {
			v.push_back(columns[i] + " = " + values[i]);
		}

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE guid_first = {} AND guid_second = {} AND guid_third = {}",
				TableName(),
				Strings::Implode(", ", v),
				e.guid.first,
				e.guid.second,
				e.guid.third
			)
		);

		return results.Success();
	}

private:
	template<typename T>
	static std::string NullableValue(const std::optional<T> &value)
	{
		return value ? std::to_string(*value) : std::string("NULL");
	}

	static std::vector<std::string> Values(const ItemInstances &e)
	{
		return {
			std::to_string(e.guid.first),
			std::to_string(e.guid.second),
			std::to_string(e.guid.third),
			std::to_string(e.item_id),
			std::to_string(e.charges),
			std::to_string(e.color),
			std::to_string(e.attuned ? 1 : 0),
			"'" + Strings::Escape(e.custom_data) + "'",
			std::to_string(e.ornament_icon),
			std::to_string(e.ornament_idfile),
			std::to_string(e.ornament_hero_model),
			e.holder_type ? fmt::format("'{}'", HolderTypeName(*e.holder_type)) : std::string("NULL"),
			NullableValue(e.holder_id),
			NullableValue(e.slot),
			e.parent_guid ? std::to_string(e.parent_guid->first) : std::string("NULL"),
			e.parent_guid ? std::to_string(e.parent_guid->second) : std::string("NULL"),
			e.parent_guid ? std::to_string(e.parent_guid->third) : std::string("NULL"),
			NullableValue(e.parent_index),
			e.retirement_state ? fmt::format("'{}'", RetirementStateName(*e.retirement_state)) : std::string("NULL"),
			e.retired_at ? fmt::format("FROM_UNIXTIME({})", *e.retired_at) : std::string("NULL"),
		};
	}

	static ItemInstances FromRow(MySQLRequestRow &row)
	{
		ItemInstances e{};

		e.guid = {
			static_cast<uint32_t>(strtoul(row[0], nullptr, 10)),
			static_cast<uint32_t>(strtoul(row[1], nullptr, 10)),
			static_cast<uint32_t>(strtoul(row[2], nullptr, 10)),
		};

		e.item_id             = static_cast<uint32_t>(strtoul(row[3], nullptr, 10));
		e.charges             = static_cast<int16_t>(atoi(row[4]));
		e.color               = static_cast<uint32_t>(strtoul(row[5], nullptr, 10));
		e.attuned             = atoi(row[6]) != 0;
		e.custom_data         = row[7];
		e.ornament_icon       = static_cast<uint32_t>(strtoul(row[8], nullptr, 10));
		e.ornament_idfile     = static_cast<uint32_t>(strtoul(row[9], nullptr, 10));
		e.ornament_hero_model = static_cast<uint32_t>(strtoul(row[10], nullptr, 10));

		if (row[11]) {
			e.holder_type = HolderTypeFromName(row[11]);
			if (!e.holder_type) {
				throw std::runtime_error(fmt::format("item_instances holder_type [{}] is unknown", row[11]));
			}
		}

		if (row[12]) {
			e.holder_id = static_cast<uint32_t>(strtoul(row[12], nullptr, 10));
		}

		if (row[13]) {
			e.slot = static_cast<uint16_t>(strtoul(row[13], nullptr, 10));
		}

		if (row[14]) {
			e.parent_guid = ItemGUID{
				static_cast<uint32_t>(strtoul(row[14], nullptr, 10)),
				static_cast<uint32_t>(strtoul(row[15], nullptr, 10)),
				static_cast<uint32_t>(strtoul(row[16], nullptr, 10)),
			};
		}

		if (row[17]) {
			e.parent_index = static_cast<uint16_t>(strtoul(row[17], nullptr, 10));
		}

		if (row[18]) {
			e.retirement_state = RetirementStateFromName(row[18]);
			if (!e.retirement_state) {
				throw std::runtime_error(fmt::format("item_instances retirement_state [{}] is unknown", row[18]));
			}
		}

		if (row[19]) {
			e.retired_at = static_cast<time_t>(strtoll(row[19], nullptr, 10));
		}

		return e;
	}
};

#endif //EQEMU_ITEM_INSTANCES_REPOSITORY_H
