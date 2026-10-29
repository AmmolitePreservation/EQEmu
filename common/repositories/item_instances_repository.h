#ifndef EQEMU_ITEM_INSTANCES_REPOSITORY_H
#define EQEMU_ITEM_INSTANCES_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "../item_location.h"
#include <ctime>
#include <limits>
#include <optional>
#include <stdexcept>

// Written by hand and on the generator's ignore list: the generator keys a table by one column, cannot write NULL,
// and emits ReplaceOne and DeleteOne, which this table's delete trigger refuses.
class ItemInstancesRepository {
public:
	struct ItemInstances {
		ItemGUID                      guid;
		uint32_t                      item_id;
		int16_t                       charges;
		uint32_t                      color;
		bool                          attuned;
		std::string                   custom_data;
		uint32_t                      ornament_icon;
		uint32_t                      ornament_idfile;
		uint32_t                      ornament_hero_model;
		ItemLocation                  location;
		std::optional<ItemRetirement> retirement_state;
		std::optional<time_t>         retired_at;
	};

	static constexpr size_t key_column_count       = 3;
	static constexpr size_t location_column_offset = 11;

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
		auto columns = Columns();
		columns.back() = "UNIX_TIMESTAMP(retired_at)";
		return columns;
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

	static std::string KeyFilter(const ItemGUID &guid)
	{
		return fmt::format(
			"guid_first = {} AND guid_second = {} AND guid_third = {}",
			guid.first,
			guid.second,
			guid.third
		);
	}

	static std::optional<ItemInstances> FindOne(Database &db, const ItemGUID &guid)
	{
		auto results = db.QueryDatabase(fmt::format("{} WHERE {}", BaseSelect(), KeyFilter(guid)));

		if (!results.Success() || results.RowCount() != 1) {
			return std::nullopt;
		}

		auto row = results.begin();

		return FromRow(row);
	}

	static std::vector<ItemInstances> GetWhere(Database &db, const std::string &where_filter)
	{
		return Collect(db.QueryDatabase(fmt::format("{} WHERE {}", BaseSelect(), where_filter)));
	}

	// The rows in play whose holder matches the filter, then every row inside them (bag contents, augments, the
	// augments of bag contents), parents before children.
	static std::vector<ItemInstances> GetHeld(Database &db, const std::string &holder_filter)
	{
		const auto select_columns = Strings::Implode(", ", SelectColumns());
		const auto plain_columns  = Strings::Implode(", ", Columns());

		return Collect(
			db.QueryDatabase(
				fmt::format(
					"WITH RECURSIVE held AS ("
					"SELECT {columns}, 0 AS depth FROM {table} WHERE retirement_state IS NULL AND ({filter}) "
					"UNION ALL "
					"SELECT {child_columns}, held.depth + 1 FROM {table} AS child JOIN held "
					"ON child.parent_guid_first = held.guid_first AND child.parent_guid_second = held.guid_second "
					"AND child.parent_guid_third = held.guid_third WHERE child.retirement_state IS NULL"
					") SELECT {select_columns} FROM held ORDER BY depth, guid_first, guid_second, guid_third",
					fmt::arg("columns", plain_columns),
					fmt::arg("child_columns", Strings::Replace(plain_columns, ", ", ", child.").insert(0, "child.")),
					fmt::arg("table", TableName()),
					fmt::arg("filter", holder_filter),
					fmt::arg("select_columns", select_columns)
				)
			)
		);
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
		return db.QueryDatabase(fmt::format("{} VALUES ({})", BaseInsert(), Strings::Implode(",", Values(e))));
	}

	static MySQLRequestResult InsertMany(Database &db, const std::vector<ItemInstances> &entries)
	{
		std::vector<std::string> insert_chunks;

		for (const auto &e : entries) {
			insert_chunks.push_back("(" + Strings::Implode(",", Values(e)) + ")");
		}

		return db.QueryDatabase(fmt::format("{} VALUES {}", BaseInsert(), Strings::Implode(",", insert_chunks)));
	}

	static bool UpdateLocation(Database &db, const ItemGUID &guid, const ItemLocation &location)
	{
		const auto columns = Columns();
		const auto values  = LocationValues(location);

		std::vector<std::string> v;

		for (size_t i = 0; i < values.size(); ++i) {
			v.push_back(columns[location_column_offset + i] + " = " + values[i]);
		}

		auto results = db.QueryDatabase(
			fmt::format("UPDATE {} SET {} WHERE {}", TableName(), Strings::Implode(", ", v), KeyFilter(guid))
		);

		return results.Success();
	}

	// Moves every row in play that a holder filter matches to another holder type, keeping holder id and slot.
	static int UpdateHolderType(Database &db, const std::string &holder_filter, ItemHolderType holder_type)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET holder_type = '{}' WHERE retirement_state IS NULL AND ({})",
				TableName(),
				ItemHolderTypeName(holder_type),
				holder_filter
			)
		);

		return results.Success() ? static_cast<int>(results.RowsAffected()) : -1;
	}

	// Retires one row in play; the rows changed, so a retired or unknown row answers 0.
	static int Retire(Database &db, const ItemGUID &guid, ItemRetirement reason)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET retirement_state = '{}', retired_at = NOW() WHERE {} AND retirement_state IS NULL",
				TableName(),
				ItemRetirementName(reason),
				KeyFilter(guid)
			)
		);

		return results.Success() ? static_cast<int>(results.RowsAffected()) : -1;
	}

	static int RetireKeys(Database &db, const std::vector<ItemGUID> &guids, ItemRetirement reason)
	{
		std::vector<std::string> keys;
		for (const auto &guid : guids) {
			keys.push_back(fmt::format("({}, {}, {})", guid.first, guid.second, guid.third));
		}

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET retirement_state = '{}', retired_at = NOW() WHERE (guid_first, guid_second, guid_third) IN ({}) AND retirement_state IS NULL",
				TableName(),
				ItemRetirementName(reason),
				Strings::Implode(", ", keys)
			)
		);

		return results.Success() ? static_cast<int>(results.RowsAffected()) : -1;
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
			fmt::format("UPDATE {} SET {} WHERE {}", TableName(), Strings::Implode(", ", v), KeyFilter(e.guid))
		);

		return results.Success();
	}

private:
	template<typename T>
	static std::string NullableValue(const std::optional<T> &value)
	{
		return value ? std::to_string(*value) : std::string("NULL");
	}

	static std::vector<std::string> LocationValues(const ItemLocation &l)
	{
		return {
			l.holder_type ? fmt::format("'{}'", ItemHolderTypeName(*l.holder_type)) : std::string("NULL"),
			l.holder_type ? std::to_string(l.holder_id) : std::string("NULL"),
			NullableValue(l.slot),
			l.parent_guid ? std::to_string(l.parent_guid->first) : std::string("NULL"),
			l.parent_guid ? std::to_string(l.parent_guid->second) : std::string("NULL"),
			l.parent_guid ? std::to_string(l.parent_guid->third) : std::string("NULL"),
			l.parent_guid ? std::to_string(l.parent_index) : std::string("NULL"),
		};
	}

	static std::vector<std::string> Values(const ItemInstances &e)
	{
		std::vector<std::string> v = {
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
		};

		for (const auto &value : LocationValues(e.location)) {
			v.push_back(value);
		}

		v.push_back(e.retirement_state ? fmt::format("'{}'", ItemRetirementName(*e.retirement_state)) : std::string("NULL"));
		v.push_back(e.retired_at ? fmt::format("FROM_UNIXTIME({})", *e.retired_at) : std::string("NULL"));

		return v;
	}

	static std::vector<ItemInstances> Collect(MySQLRequestResult results)
	{
		std::vector<ItemInstances> entries;

		if (!results.Success()) {
			throw std::runtime_error(fmt::format("item_instances query failed: {}", results.ErrorMessage()));
		}

		entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			entries.push_back(FromRow(row));
		}

		return entries;
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
			e.location.holder_type = ItemHolderTypeFromName(row[11]);
			if (!e.location.holder_type) {
				throw std::runtime_error(fmt::format("item_instances holder_type [{}] is unknown", row[11]));
			}
			e.location.holder_id = static_cast<uint32_t>(strtoul(row[12], nullptr, 10));
			if (row[13]) {
				e.location.slot = static_cast<uint16_t>(strtoul(row[13], nullptr, 10));
			}
		}

		if (row[14]) {
			e.location.parent_guid = ItemGUID{
				static_cast<uint32_t>(strtoul(row[14], nullptr, 10)),
				static_cast<uint32_t>(strtoul(row[15], nullptr, 10)),
				static_cast<uint32_t>(strtoul(row[16], nullptr, 10)),
			};
			e.location.parent_index = static_cast<uint16_t>(strtoul(row[17], nullptr, 10));
		}

		if (row[18]) {
			e.retirement_state = ItemRetirementFromName(row[18]);
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
