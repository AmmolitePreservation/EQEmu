#ifndef EQEMU_TRADER_REPOSITORY_H
#define EQEMU_TRADER_REPOSITORY_H

#include "../../common/shareddb.h"
#include "../strings.h"
#include "base/base_trader_repository.h"
#include "../item_guid.h"
#include "items_repository.h"
#include "../../common/item_data.h"
#include "../../common/races.h"
#include "../cereal/include/cereal/archives/binary.hpp"
#include "../cereal/include/cereal/types/string.hpp"

class TraderRepository : public BaseTraderRepository {
public:
	static constexpr uint32 TRADER_CONVERT_ID = 4000000000;

	struct DistinctTraders_Struct {
		uint32      trader_id;
		uint32      zone_id;
		uint32      zone_instance_id;
		uint32      entity_id;
		std::string trader_name;
	};

	struct BulkTraders_Struct {
		uint32                              count{0};
		uint32                              name_length{0};
		std::vector<DistinctTraders_Struct> traders{};
	};

	struct BazaarTraderSearch_Struct {
		Trader      trader;
		std::string trader_name;
		uint32      item_id;
		int32       charges;
	};

	static ItemGUID GUIDOf(const Trader &e)
	{
		return ItemGUID{e.guid_first, e.guid_second, e.guid_third};
	}

	static void SetGUID(Trader &e, const ItemGUID &guid)
	{
		e.guid_first  = guid.first;
		e.guid_second = guid.second;
		e.guid_third  = guid.third;
	}

	struct WelcomeData_Struct {
		uint32 count_of_traders;
		uint32 count_of_items;
	};

	static std::vector<BazaarSearchResultsFromDB_Struct>
	GetBazaarSearchResults(
		SharedDatabase &db,
		BazaarSearchCriteria_Struct search,
		uint32 char_zone_id,
		int32 char_zone_instance_id
	);

	static BulkTraders_Struct GetDistinctTraders(
		Database &db,
		uint32 char_zone_instance_id,
		uint32 max_results = std::numeric_limits<uint32>::max()
		)
	{
		BulkTraders_Struct                  all_entries{};
		std::vector<DistinctTraders_Struct> distinct_traders;
		MySQLRequestResult                  results;

		if (RuleB(Bazaar, UseAlternateBazaarSearch)) {
			results = db.QueryDatabase(fmt::format(
				"SELECT DISTINCT(t.char_id), t.char_zone_id, t.char_zone_instance_id, t.char_entity_id, c.name "
				"FROM trader AS t "
				"JOIN character_data AS c ON t.char_id = c.id "
				"WHERE t.char_zone_instance_id = {} "
				"ORDER BY t.char_zone_instance_id ASC "
				"LIMIT {}",
				char_zone_instance_id,
				max_results)
			);
		}
		else {
			results = db.QueryDatabase(fmt::format(
				"SELECT DISTINCT(t.char_id), t.char_zone_id, t.char_zone_instance_id, t.char_entity_id, c.name "
				"FROM trader AS t "
				"JOIN character_data AS c ON t.char_id = c.id "
				"ORDER BY t.char_zone_instance_id ASC "
				"LIMIT {}",
				max_results)
			);
		}

		distinct_traders.reserve(results.RowCount());

		for (auto row: results) {
			DistinctTraders_Struct e{};

			e.trader_id        = Strings::ToInt(row[0]);
			e.zone_id          = Strings::ToInt(row[1]);
			e.zone_instance_id = Strings::ToInt(row[2]);
			e.entity_id        = Strings::ToInt(row[3]);
			e.trader_name      = row[4] ? row[4] : "";
			all_entries.name_length += e.trader_name.length() + 1;

			all_entries.traders.push_back(e);
		}
		all_entries.count = results.RowCount();
		return all_entries;
	}

	static WelcomeData_Struct GetWelcomeData(Database &db)
	{
		WelcomeData_Struct e{};

		auto results = db.QueryDatabase("SELECT COUNT(DISTINCT char_id), count(char_id) FROM trader;");

		if (!results.RowCount()) {
			return e;
		}

		auto r = results.begin();
		e.count_of_traders = Strings::ToInt(r[0]);
		e.count_of_items   = Strings::ToInt(r[1]);
		return e;
	}

	// Every listing of this item the trader has up, or only those with this many charges when charges is not zero.
	static std::string ItemFilter(uint32 char_id, uint32 item_id, int32 charges)
	{
		return fmt::format(
			"t.char_id = {} AND i.item_id = {}{}",
			char_id,
			item_id,
			charges ? fmt::format(" AND i.charges = {}", charges) : ""
		);
	}

	static std::string ItemJoin()
	{
		return fmt::format(
			"{} t JOIN item_instances i ON i.guid_first = t.guid_first AND i.guid_second = t.guid_second AND i.guid_third = t.guid_third",
			TableName()
		);
	}

	static int UpdatePriceByItem(Database &db, uint32 char_id, uint32 item_id, int32 charges, uint32 new_price)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET t.item_cost = {}, t.listing_date = NOW() WHERE {}",
				ItemJoin(),
				new_price,
				ItemFilter(char_id, item_id, charges)
			)
		);

		return results.Success() ? static_cast<int>(results.RowsAffected()) : -1;
	}

	static int DeleteByItem(Database &db, uint32 char_id, uint32 item_id)
	{
		auto results = db.QueryDatabase(
			fmt::format("DELETE t FROM {} WHERE {}", ItemJoin(), ItemFilter(char_id, item_id, 0))
		);

		return results.Success() ? static_cast<int>(results.RowsAffected()) : -1;
	}

	static Trader GetItemBySerialNumber(Database &db, uint32 serial_number, uint32 trader_id)
	{
		Trader     e{};
		const auto trader_item = GetWhere(
			db,
			fmt::format("`char_id` = '{}' AND `item_sn` = '{}' LIMIT 1", trader_id, serial_number)
		);

		if (trader_item.empty()) {
			return e;
		}

		return trader_item.at(0);
	}

	static Trader GetItemBySerialNumber(Database &db, std::string serial_number, uint32 trader_id)
	{
		Trader     e{};
		auto       sn          = Strings::ToUnsignedBigInt(serial_number);
		const auto trader_item = GetWhere(
			db,
			fmt::format("`char_id` = '{}' AND `item_sn` = '{}' LIMIT 1", trader_id, sn)
		);

		if (trader_item.empty()) {
			return e;
		}

		return trader_item.at(0);
	}

	static int UpdateActiveTransaction(Database &db, uint32 id, bool status)
	{
		auto e = FindOne(db, id);
		if (!e.id) {
			return 0;
		}

		e.active_transaction = status == true ? 1 : 0;
		e.listing_date       = time(nullptr);

		return UpdateOne(db, e);
	}

	static int DeleteMany(Database &db, const std::vector<Trader> &entries)
	{
		std::vector<std::string> delete_ids;

		for (auto const &e: entries) {
			delete_ids.push_back(std::to_string(e.id));
		}

		if (delete_ids.empty()) {
			return 0;
		}

		return DeleteWhere(db, fmt::format("`id` IN({})", Strings::Implode(",", delete_ids)));
	}

	static DistinctTraders_Struct GetTraderByInstanceAndSerialnumber(
		Database &db,
		uint32 instance_id,
		const char *serial_number
		)
	{
		DistinctTraders_Struct trader{};

		auto query = fmt::format(
			"SELECT t.id, t.char_id, c.name "
			"FROM trader AS t "
			"JOIN character_data AS c ON c.id = t.char_id "
			"WHERE t.char_zone_id = 151 AND t.char_zone_instance_id = {} AND t.item_sn = {} LIMIT 1",
			instance_id,
			serial_number
		);

		auto results = db.QueryDatabase(query);

		if (results.RowCount() == 0) {
			return trader;
		}

		auto        row    = results.begin();
		std::string name   = row[2];
		trader.trader_id   = Strings::ToUnsignedInt(row[1]);
		trader.trader_name = row[2] ? row[2] : "";

		return trader;
	}

	static std::vector<BazaarTraderSearch_Struct> GetBazaarTraderDetails(
		Database &db,
		std::string &search_criteria_trader
		)
	{
		std::vector<BazaarTraderSearch_Struct> all_entries{};

		auto query = fmt::format(
			"SELECT trader.id, trader.char_id, trader.guid_first, trader.guid_second, trader.guid_third, trader.item_sn, "
			"trader.item_cost, trader.slot_id, trader.char_entity_id, trader.char_zone_id, trader.char_zone_instance_id, "
			"trader.active_transaction, UNIX_TIMESTAMP(trader.listing_date), c.`name`, i.item_id, i.charges "
			"FROM `trader` INNER JOIN character_data AS c ON trader.char_id = c.id "
			"INNER JOIN item_instances AS i ON i.guid_first = trader.guid_first AND i.guid_second = trader.guid_second "
			"AND i.guid_third = trader.guid_third "
			"WHERE {} ORDER BY trader.char_id ASC",
			search_criteria_trader
		);
		auto results = db.QueryDatabase(query);
		if (results.RowCount() == 0) {
			return all_entries;
		}

		all_entries.reserve(results.RowCount());
		for (auto row = results.begin(); row != results.end(); ++row) {
			BazaarTraderSearch_Struct e{};
			e.trader.id                    = row[0] ? strtoull(row[0], nullptr, 10) : 0;
			e.trader.char_id               = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.trader.guid_first            = row[2] ? static_cast<uint32_t>(strtoul(row[2], nullptr, 10)) : 0;
			e.trader.guid_second           = row[3] ? static_cast<uint32_t>(strtoul(row[3], nullptr, 10)) : 0;
			e.trader.guid_third            = row[4] ? static_cast<uint32_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.trader.item_sn               = row[5] ? static_cast<uint32_t>(strtoul(row[5], nullptr, 10)) : 0;
			e.trader.item_cost             = row[6] ? static_cast<uint32_t>(strtoul(row[6], nullptr, 10)) : 0;
			e.trader.slot_id               = row[7] ? static_cast<uint8_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.trader.char_entity_id        = row[8] ? static_cast<uint32_t>(strtoul(row[8], nullptr, 10)) : 0;
			e.trader.char_zone_id          = row[9] ? static_cast<uint32_t>(strtoul(row[9], nullptr, 10)) : 0;
			e.trader.char_zone_instance_id = row[10] ? static_cast<int32_t>(atoi(row[10])) : 0;
			e.trader.active_transaction    = row[11] ? static_cast<uint8_t>(strtoul(row[11], nullptr, 10)) : 0;
			e.trader.listing_date          = row[12] ? strtoll(row[12], nullptr, 10) : 0;
			e.trader_name                  = row[13] ? row[13] : std::string("");
			e.item_id                      = row[14] ? static_cast<uint32_t>(strtoul(row[14], nullptr, 10)) : 0;
			e.charges                      = row[15] ? static_cast<int32_t>(atoi(row[15])) : 0;
			all_entries.push_back(e);
		}

		return all_entries;
	}
};

#endif //EQEMU_TRADER_REPOSITORY_H
