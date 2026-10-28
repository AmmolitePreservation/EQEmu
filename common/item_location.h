#ifndef EQEMU_ITEM_LOCATION_H
#define EQEMU_ITEM_LOCATION_H

#include "item_guid.h"
#include <array>
#include <optional>
#include <string_view>

// The holders of item_instances.holder_type, in the ENUM's order. A slot is numbered within its holder; the ground has none.
enum class ItemHolderType : uint8 {
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
	Trade,
	WorldContainer,
};

constexpr std::array<std::string_view, 14> item_holder_type_names = {
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
	"trade",
	"world_container",
};
static_assert(item_holder_type_names.size() == static_cast<size_t>(ItemHolderType::WorldContainer) + 1);

// The reasons of item_instances.retirement_state, in the ENUM's order.
enum class ItemRetirement : uint8 {
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

constexpr std::array<std::string_view, 11> item_retirement_names = {
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
static_assert(item_retirement_names.size() == static_cast<size_t>(ItemRetirement::BotDonated) + 1);

inline std::string_view ItemHolderTypeName(ItemHolderType holder_type)
{
	return item_holder_type_names[static_cast<size_t>(holder_type)];
}

inline std::optional<ItemHolderType> ItemHolderTypeFromName(std::string_view name)
{
	for (size_t i = 0; i < item_holder_type_names.size(); ++i) {
		if (item_holder_type_names[i] == name) {
			return static_cast<ItemHolderType>(i);
		}
	}

	return std::nullopt;
}

inline std::string_view ItemRetirementName(ItemRetirement retirement)
{
	return item_retirement_names[static_cast<size_t>(retirement)];
}

inline std::optional<ItemRetirement> ItemRetirementFromName(std::string_view name)
{
	for (size_t i = 0; i < item_retirement_names.size(); ++i) {
		if (item_retirement_names[i] == name) {
			return static_cast<ItemRetirement>(i);
		}
	}

	return std::nullopt;
}

// Where an item instance is: a top-level item has a holder and, except on the ground, a slot; a bag's content or an
// augment has its parent's GUID and the bag slot or socket instead.
struct ItemLocation {
	static constexpr uint16 in_transit_slot = 65535;

	std::optional<ItemHolderType> holder_type;
	uint32                        holder_id{0};
	std::optional<uint16>         slot;
	std::optional<ItemGUID>       parent_guid;
	uint16                        parent_index{0};

	static ItemLocation InHolder(ItemHolderType holder_type, uint32 holder_id, uint16 slot)
	{
		ItemLocation l;
		l.holder_type = holder_type;
		l.holder_id   = holder_id;
		l.slot        = slot;
		return l;
	}

	static ItemLocation OnGround(uint32 zone_id)
	{
		ItemLocation l;
		l.holder_type = ItemHolderType::Ground;
		l.holder_id   = zone_id;
		return l;
	}

	// A row's place while a swap or a split passes between slots in one game-thread step, never seen outside it.
	static ItemLocation InTransit(uint32 character_id)
	{
		return InHolder(ItemHolderType::Cursor, character_id, in_transit_slot);
	}

	static ItemLocation InParent(const ItemGUID &parent_guid, uint16 parent_index)
	{
		ItemLocation l;
		l.parent_guid  = parent_guid;
		l.parent_index = parent_index;
		return l;
	}

	bool IsInParent() const { return parent_guid.has_value(); }
	bool IsInHolder(ItemHolderType type) const { return holder_type && *holder_type == type; }
};

#endif //EQEMU_ITEM_LOCATION_H
