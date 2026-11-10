#ifndef EQEMU_GUILD_BANK_REPOSITORY_H
#define EQEMU_GUILD_BANK_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "base/base_guild_bank_repository.h"
#include "../item_guid.h"
#include "../item_location.h"

class GuildBankRepository: public BaseGuildBankRepository {
public:
	// A guild bank entry as the zone holds it: the side row with the item row's place and item. Area 1 is the main
	// area, as GuildBankMainArea in guild_mgr.h.
	struct GuildBankItem {
		uint32      id;
		ItemGUID    guid;
		uint8       area;
		uint32      slot;
		uint32      item_id;
		int32       quantity;
		std::string donator;
		uint8       permissions;
		std::string who_for;
	};

	static std::string HolderFilter(uint32 guild_id)
	{
		return fmt::format(
			"i.holder_type IN ('{}', '{}') AND i.holder_id = {} AND i.retirement_state IS NULL",
			ItemHolderTypeName(ItemHolderType::GuildBankDeposit),
			ItemHolderTypeName(ItemHolderType::GuildBankMain),
			guild_id
		);
	}

	static std::vector<GuildBankItem> GetGuildBankItems(Database &db, uint32 guild_id)
	{
		std::vector<GuildBankItem> all_entries;
		auto results = db.QueryDatabase(
			fmt::format(
				"SELECT b.id, b.guid_first, b.guid_second, b.guid_third, i.holder_type, i.slot, i.item_id, i.charges, "
				"b.donator, b.permissions, b.who_for FROM {} b JOIN item_instances i ON i.guid_first = b.guid_first "
				"AND i.guid_second = b.guid_second AND i.guid_third = b.guid_third WHERE {} ORDER BY i.holder_type, i.slot",
				TableName(),
				HolderFilter(guild_id)
			)
		);
		all_entries.reserve(results.RowCount());
		for (auto row : results) {
			GuildBankItem e{};
			e.id          = Strings::ToUnsignedInt(row[0]);
			e.guid        = ItemGUID{Strings::ToUnsignedInt(row[1]), Strings::ToUnsignedInt(row[2]), Strings::ToUnsignedInt(row[3])};
			e.area        = std::string_view(row[4]) == ItemHolderTypeName(ItemHolderType::GuildBankMain) ? 1 : 0;
			e.slot        = Strings::ToUnsignedInt(row[5]);
			e.item_id     = Strings::ToUnsignedInt(row[6]);
			e.quantity    = Strings::ToInt(row[7]);
			e.donator     = row[8] ? row[8] : "";
			e.permissions = static_cast<uint8>(Strings::ToUnsignedInt(row[9]));
			e.who_for     = row[10] ? row[10] : "";
			all_entries.push_back(e);
		}

		return all_entries;
	}

	static uint32 InsertItem(Database &db, const GuildBankItem &e)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"INSERT INTO {} (guid_first, guid_second, guid_third, donator, permissions, who_for) VALUES ({}, {}, {}, '{}', {}, '{}')",
				TableName(),
				e.guid.first,
				e.guid.second,
				e.guid.third,
				Strings::Escape(e.donator),
				e.permissions,
				Strings::Escape(e.who_for)
			)
		);

		return results.Success() ? static_cast<uint32>(results.LastInsertedID()) : 0;
	}

	static bool UpdatePermissions(Database &db, uint32 id, uint8 permissions, const std::string &who_for)
	{
		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET permissions = {}, who_for = '{}' WHERE id = {}",
				TableName(),
				permissions,
				Strings::Escape(who_for),
				id
			)
		);

		return results.Success();
	}

	static int CountGuildItems(Database &db, uint32 guild_id)
	{
		auto results = db.QueryDatabase(fmt::format("SELECT COUNT(*) FROM item_instances i WHERE {}", HolderFilter(guild_id)));
		if (!results.Success() || !results.RowCount()) {
			return -1;
		}

		return Strings::ToInt(results.begin()[0]);
	}
};

#endif //EQEMU_GUILD_BANK_REPOSITORY_H
