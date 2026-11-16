/*	EQEMu: Everquest Server Emulator

	Copyright (C) 2001-2016 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "emu_constants.h"
#include "emu_limits.h"
#include "rulesys.h"

#include <memory>


static bool global_dictionary_init = false;
void EQ::InitializeDynamicLookups() {
	if (global_dictionary_init)
		return;

	constants::InitializeDynamicLookups();
	inventory::InitializeDynamicLookups();
	behavior::InitializeDynamicLookups();
	spells::InitializeDynamicLookups();

	global_dictionary_init = true;
}

static std::unique_ptr<EQ::constants::LookupEntry> constants_dynamic_nongm_lookup_entries[EQ::versions::ClientVersionCount];
static std::unique_ptr<EQ::constants::LookupEntry> constants_dynamic_gm_lookup_entries[EQ::versions::ClientVersionCount];
static const EQ::constants::LookupEntry constants_static_lookup_entries[EQ::versions::ClientVersionCount] =
{
	/*[ClientVersion::Unknown] =*/
	EQ::constants::LookupEntry(
		ClientUnknown::INULL,
		ClientUnknown::INULL
	),
	/*[ClientVersion::RoF2] =*/
	EQ::constants::LookupEntry(
		RoF2::constants::SAY_LINK_BODY_SIZE,
		RoF2::constants::MAX_BAZAAR_TRADERS
	)
};

static bool constants_dictionary_init = false;
void EQ::constants::InitializeDynamicLookups() {
	if (constants_dictionary_init == true)
		return;
	constants_dictionary_init = true;

	// use static references for now
}

const EQ::constants::LookupEntry* EQ::constants::DynamicLookup(versions::ClientVersion client_version, bool gm_flag)
{
	if (gm_flag)
		return DynamicGMLookup(client_version);
	else
		return DynamicNonGMLookup(client_version);
}

const EQ::constants::LookupEntry* EQ::constants::DynamicNonGMLookup(versions::ClientVersion client_version)
{
	client_version = versions::ValidateClientVersion(client_version);
	if (constants_dynamic_nongm_lookup_entries[static_cast<int>(client_version)])
		return constants_dynamic_nongm_lookup_entries[static_cast<int>(client_version)].get();

	return &constants_static_lookup_entries[static_cast<int>(client_version)];
}

const EQ::constants::LookupEntry* EQ::constants::DynamicGMLookup(versions::ClientVersion client_version)
{
	client_version = versions::ValidateClientVersion(client_version);
	if (constants_dynamic_gm_lookup_entries[static_cast<int>(client_version)])
		return constants_dynamic_gm_lookup_entries[static_cast<int>(client_version)].get();

	return &constants_static_lookup_entries[static_cast<int>(client_version)];
}

const EQ::constants::LookupEntry* EQ::constants::StaticLookup(versions::ClientVersion client_version)
{
	return &constants_static_lookup_entries[static_cast<int>(versions::ValidateClientVersion(client_version))];
}

static std::unique_ptr<EQ::inventory::LookupEntry> inventory_dynamic_nongm_lookup_entries[EQ::versions::MobVersionCount];
static std::unique_ptr<EQ::inventory::LookupEntry> inventory_dynamic_gm_lookup_entries[EQ::versions::MobVersionCount];
static const EQ::inventory::LookupEntry inventory_static_lookup_entries[EQ::versions::MobVersionCount] =
{
	/*[MobVersion::Unknown] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL,
			ClientUnknown::INULL, ClientUnknown::INULL, ClientUnknown::INULL
		),

		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,

		false,
		false,
		false,
		false
	),
	/*[MobVersion::RoF2] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE,	RoF2::invtype::BANK_SIZE,					RoF2::invtype::SHARED_BANK_SIZE,
			RoF2::invtype::TRADE_SIZE,			RoF2::invtype::WORLD_SIZE,					RoF2::invtype::LIMBO_SIZE,
			RoF2::invtype::TRIBUTE_SIZE,		RoF2::invtype::TROPHY_TRIBUTE_SIZE,			RoF2::invtype::GUILD_TRIBUTE_SIZE,
			RoF2::invtype::MERCHANT_SIZE,		RoF2::invtype::DELETED_SIZE,				RoF2::invtype::CORPSE_SIZE,
			RoF2::invtype::BAZAAR_SIZE,			RoF2::invtype::INSPECT_SIZE,				RoF2::invtype::REAL_ESTATE_SIZE,
			RoF2::invtype::VIEW_MOD_PC_SIZE,	RoF2::invtype::VIEW_MOD_BANK_SIZE,			RoF2::invtype::VIEW_MOD_SHARED_BANK_SIZE,
			RoF2::invtype::VIEW_MOD_LIMBO_SIZE,	RoF2::invtype::ALT_STORAGE_SIZE,			RoF2::invtype::ARCHIVED_SIZE,
			RoF2::invtype::MAIL_SIZE,			RoF2::invtype::GUILD_TROPHY_TRIBUTE_SIZE,	RoF2::invtype::KRONO_SIZE,
			RoF2::invtype::GUILD_BANK_MAIN_SIZE,RoF2::invtype::GUILD_BANK_DEPOSIT_SIZE,     RoF2::invtype::OTHER_SIZE
		),

		RoF2::invslot::EQUIPMENT_BITMASK,
		RoF2::invslot::GENERAL_BITMASK,
		RoF2::invslot::CURSOR_BITMASK,
		RoF2::invslot::POSSESSIONS_BITMASK,
		RoF2::invslot::CORPSE_BITMASK,
		RoF2::invbag::SLOT_COUNT,
		RoF2::invaug::SOCKET_COUNT,

		RoF2::inventory::AllowEmptyBagInBag,
		RoF2::inventory::AllowClickCastFromBag,
		RoF2::inventory::ConcatenateInvTypeLimbo,
		RoF2::inventory::AllowOverLevelEquipment
	),
	/*[MobVersion::NPC] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE,		EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,
			EntityLimits::NPC::invtype::TRADE_SIZE,	EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,
			EntityLimits::NPC::INULL,				EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,
			EntityLimits::NPC::INULL,				EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::NPC::INULL,				EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,
			EntityLimits::NPC::INULL,				EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,
			EntityLimits::NPC::INULL,				EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,
			EntityLimits::NPC::INULL,				EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,
			EntityLimits::NPC::INULL,	EntityLimits::NPC::INULL,EntityLimits::NPC::INULL
		),

		EntityLimits::NPC::INULL,
		EntityLimits::NPC::INULL,
		EntityLimits::NPC::INULL,
		EntityLimits::NPC::INULL,
		EntityLimits::NPC::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		0, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::NPCMerchant] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE, /*InvTypePossessionsSize,*/	EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL,
			EntityLimits::NPCMerchant::invtype::TRADE_SIZE,					EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL,
			EntityLimits::NPCMerchant::INULL,								EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL,
			EntityLimits::NPCMerchant::INULL,								EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::NPCMerchant::INULL,								EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL,
			EntityLimits::NPCMerchant::INULL,								EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL,
			EntityLimits::NPCMerchant::INULL,								EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL,
			EntityLimits::NPCMerchant::INULL,								EntityLimits::NPCMerchant::INULL,	EntityLimits::NPCMerchant::INULL,
			EntityLimits::NPCMerchant::INULL,								EntityLimits::NPCMerchant::INULL,   EntityLimits::NPCMerchant::INULL
		),

		EntityLimits::NPCMerchant::INULL,
		EntityLimits::NPCMerchant::INULL,
		EntityLimits::NPCMerchant::INULL,
		EntityLimits::NPCMerchant::INULL,
		EntityLimits::NPCMerchant::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		0, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::Merc] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE, /*InvTypePossessionsSize,*/	EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL,
			EntityLimits::Merc::invtype::TRADE_SIZE,						EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL,
			EntityLimits::Merc::INULL,										EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL,
			EntityLimits::Merc::INULL,										EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::Merc::INULL,										EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL,
			EntityLimits::Merc::INULL,										EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL,
			EntityLimits::Merc::INULL,										EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL,
			EntityLimits::Merc::INULL,										EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL,
			EntityLimits::Merc::INULL,										EntityLimits::Merc::INULL,	EntityLimits::Merc::INULL
		),

		EntityLimits::Merc::INULL,
		EntityLimits::Merc::INULL,
		EntityLimits::Merc::INULL,
		EntityLimits::Merc::INULL,
		EntityLimits::Merc::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		0, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::Bot] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE, /*InvTypePossessionsSize,*/	EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL,
			EntityLimits::Bot::invtype::TRADE_SIZE,							EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL,
			EntityLimits::Bot::INULL,										EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL,
			EntityLimits::Bot::INULL,										EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::Bot::INULL,										EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL,
			EntityLimits::Bot::INULL,										EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL,
			EntityLimits::Bot::INULL,										EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL,
			EntityLimits::Bot::INULL,										EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL,
			EntityLimits::Bot::INULL,										EntityLimits::Bot::INULL,	EntityLimits::Bot::INULL
		),

		EntityLimits::Bot::invslot::EQUIPMENT_BITMASK,
		EntityLimits::Bot::invslot::GENERAL_BITMASK,
		EntityLimits::Bot::invslot::CURSOR_BITMASK,
		EntityLimits::Bot::invslot::POSSESSIONS_BITMASK,
		EntityLimits::Bot::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		EQ::invaug::SOCKET_COUNT, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::ClientPet] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE, /*InvTypePossessionsSize,*/	EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL,
			EntityLimits::ClientPet::invtype::TRADE_SIZE,					EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL,
			EntityLimits::ClientPet::INULL,									EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL,
			EntityLimits::ClientPet::INULL,									EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::ClientPet::INULL,									EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL,
			EntityLimits::ClientPet::INULL,									EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL,
			EntityLimits::ClientPet::INULL,									EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL,
			EntityLimits::ClientPet::INULL,									EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL,
			EntityLimits::ClientPet::INULL,									EntityLimits::ClientPet::INULL,	EntityLimits::ClientPet::INULL
		),

		EntityLimits::ClientPet::INULL,
		EntityLimits::ClientPet::INULL,
		EntityLimits::ClientPet::INULL,
		EntityLimits::ClientPet::INULL,
		EntityLimits::ClientPet::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		0, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::NPCPet] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE, /*InvTypePossessionsSize,*/	EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL,
			EntityLimits::NPCPet::invtype::TRADE_SIZE,						EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL,
			EntityLimits::NPCPet::INULL,									EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL,
			EntityLimits::NPCPet::INULL,									EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::NPCPet::INULL,									EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL,
			EntityLimits::NPCPet::INULL,									EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL,
			EntityLimits::NPCPet::INULL,									EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL,
			EntityLimits::NPCPet::INULL,									EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL,
			EntityLimits::NPCPet::INULL,									EntityLimits::NPCPet::INULL,	EntityLimits::NPCPet::INULL
		),

		EntityLimits::NPCPet::INULL,
		EntityLimits::NPCPet::INULL,
		EntityLimits::NPCPet::INULL,
		EntityLimits::NPCPet::INULL,
		EntityLimits::NPCPet::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		0, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::MercPet] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE, /*InvTypePossessionsSize,*/	EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL,
			EntityLimits::MercPet::invtype::TRADE_SIZE,						EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL,
			EntityLimits::MercPet::INULL,									EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL,
			EntityLimits::MercPet::INULL,									EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::MercPet::INULL,									EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL,
			EntityLimits::MercPet::INULL,									EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL,
			EntityLimits::MercPet::INULL,									EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL,
			EntityLimits::MercPet::INULL,									EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL,
			EntityLimits::MercPet::INULL,									EntityLimits::MercPet::INULL,	EntityLimits::MercPet::INULL
		),

		EntityLimits::MercPet::INULL,
		EntityLimits::MercPet::INULL,
		EntityLimits::MercPet::INULL,
		EntityLimits::MercPet::INULL,
		EntityLimits::MercPet::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		0, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::BotPet] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			EQ::invtype::POSSESSIONS_SIZE, /*InvTypePossessionsSize,*/	EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL,
			EntityLimits::BotPet::invtype::TRADE_SIZE,						EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL,
			EntityLimits::BotPet::INULL,									EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL,
			EntityLimits::BotPet::INULL,									EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL, /*InvTypeCorpseSize,*/
			EntityLimits::BotPet::INULL,									EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL,
			EntityLimits::BotPet::INULL,									EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL,
			EntityLimits::BotPet::INULL,									EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL,
			EntityLimits::BotPet::INULL,									EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL,
			EntityLimits::BotPet::INULL,									EntityLimits::BotPet::INULL,	EntityLimits::BotPet::INULL
		),

		EntityLimits::BotPet::INULL,
		EntityLimits::BotPet::INULL,
		EntityLimits::BotPet::INULL,
		EntityLimits::BotPet::INULL,
		EntityLimits::BotPet::INULL,
		0, //EQ::inventory::ContainerCount, /*ItemBagSize,*/
		0, //EQ::inventory::SocketCount, /*ItemAugSize,*/

		false,
		false,
		false,
		false
	),
	/*[MobVersion::OfflineRoF2] =*/
	EQ::inventory::LookupEntry(
		EQ::inventory::LookupEntry::InventoryTypeSize_Struct(
			RoF2::INULL,						RoF2::INULL,						RoF2::INULL,
			RoF2::invtype::TRADE_SIZE,			RoF2::INULL,						RoF2::INULL,
			RoF2::INULL,						RoF2::INULL,						RoF2::INULL,
			RoF2::invtype::MERCHANT_SIZE,		RoF2::INULL,						RoF2::INULL,
			RoF2::invtype::BAZAAR_SIZE,			RoF2::invtype::INSPECT_SIZE,		RoF2::INULL,
			RoF2::invtype::VIEW_MOD_PC_SIZE,	RoF2::invtype::VIEW_MOD_BANK_SIZE,	RoF2::invtype::VIEW_MOD_SHARED_BANK_SIZE,
			RoF2::invtype::VIEW_MOD_LIMBO_SIZE,	RoF2::INULL,						RoF2::INULL,
			RoF2::INULL,						RoF2::INULL,						RoF2::INULL,
			RoF2::INULL,						RoF2::INULL,						RoF2::INULL
		),

		RoF2::INULL,
		RoF2::INULL,
		RoF2::INULL,
		RoF2::INULL,
		RoF2::INULL,
		RoF2::invbag::SLOT_COUNT,
		RoF2::invaug::SOCKET_COUNT,

		false,
		false,
		false,
		false
	)
};

// Three expansions reshape a RoF2 character's inventory: Prophecy of Ro grew the bank from 16 to 24 slots, The Buried
// Sea turned on the power source slot and House of Thule the ninth and tenth general slots. A server without one of
// them keeps the older shape; a GM keeps the power source and general slots but not the bank.
static const int16  bank_size_before_por         = 16;
static const uint64 equipment_bitmask_before_tbs = 0x00000000005FFFFF;
static const uint64 general_bitmask_before_hot   = 0x000000007F800000;

static bool inventory_dictionary_init = false;
void EQ::inventory::InitializeDynamicLookups() {
	if (inventory_dictionary_init == true)
		return;
	inventory_dictionary_init = true;

	const uint32 expansions = RuleI(World, ExpansionSettings);
	const uint32 dynamic_check_mask = (EQ::expansions::bitPoR | EQ::expansions::bitTBS | EQ::expansions::bitHoT);
	if ((dynamic_check_mask & expansions) == dynamic_check_mask)
		return;

	const auto  index = static_cast<int>(versions::MobVersion::RoF2);
	const auto &base  = inventory_static_lookup_entries[index];

	auto nongm = std::make_unique<LookupEntry>(base);
	if (!(expansions & EQ::expansions::bitPoR))
		nongm->InventoryTypeSize.Bank = bank_size_before_por;
	if (!(expansions & EQ::expansions::bitTBS))
		nongm->EquipmentBitmask = equipment_bitmask_before_tbs;
	if (!(expansions & EQ::expansions::bitHoT))
		nongm->GeneralBitmask = general_bitmask_before_hot;
	nongm->PossessionsBitmask = (nongm->EquipmentBitmask | nongm->GeneralBitmask | nongm->CursorBitmask);
	nongm->CorpseBitmask      = (nongm->GeneralBitmask | nongm->CursorBitmask | (nongm->EquipmentBitmask << 34));
	inventory_dynamic_nongm_lookup_entries[index] = std::move(nongm);

	if (!(expansions & EQ::expansions::bitPoR)) {
		auto gm = std::make_unique<LookupEntry>(base);
		gm->InventoryTypeSize.Bank = bank_size_before_por;
		inventory_dynamic_gm_lookup_entries[index] = std::move(gm);
	}
}

const EQ::inventory::LookupEntry* EQ::inventory::DynamicLookup(versions::MobVersion mob_version, bool gm_flag)
{
	if (gm_flag)
		return DynamicGMLookup(mob_version);
	else
		return DynamicNonGMLookup(mob_version);
}

const EQ::inventory::LookupEntry* EQ::inventory::DynamicNonGMLookup(versions::MobVersion mob_version)
{
	mob_version = versions::ValidateMobVersion(mob_version);
	if (inventory_dynamic_nongm_lookup_entries[static_cast<int>(mob_version)])
		return inventory_dynamic_nongm_lookup_entries[static_cast<int>(mob_version)].get();

	return &inventory_static_lookup_entries[static_cast<int>(mob_version)];
}

const EQ::inventory::LookupEntry* EQ::inventory::DynamicGMLookup(versions::MobVersion mob_version)
{
	mob_version = versions::ValidateMobVersion(mob_version);
	if (inventory_dynamic_gm_lookup_entries[static_cast<int>(mob_version)])
		return inventory_dynamic_gm_lookup_entries[static_cast<int>(mob_version)].get();

	return &inventory_static_lookup_entries[static_cast<int>(mob_version)];
}

const EQ::inventory::LookupEntry* EQ::inventory::StaticLookup(versions::MobVersion mob_version)
{
	return &inventory_static_lookup_entries[static_cast<int>(versions::ValidateMobVersion(mob_version))];
}

static std::unique_ptr<EQ::behavior::LookupEntry> behavior_dynamic_nongm_lookup_entries[EQ::versions::MobVersionCount];
static std::unique_ptr<EQ::behavior::LookupEntry> behavior_dynamic_gm_lookup_entries[EQ::versions::MobVersionCount];
static const EQ::behavior::LookupEntry behavior_static_lookup_entries[EQ::versions::MobVersionCount] =
{
	/*[MobVersion::Unknown] =*/
	EQ::behavior::LookupEntry(
		true
	),
	/*[MobVersion::RoF2] =*/
	EQ::behavior::LookupEntry(
		RoF2::behavior::CoinHasWeight
	),
	/*[MobVersion::NPC] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::NPCMerchant] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::Merc] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::Bot] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::ClientPet] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::NPCPet] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::MercPet] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::BotPet] =*/
	EQ::behavior::LookupEntry(
		EQ::behavior::CoinHasWeight
	),
	/*[MobVersion::OfflineRoF2] =*/
	EQ::behavior::LookupEntry(
		RoF2::behavior::CoinHasWeight
	)
};

static bool behavior_dictionary_init = false;
void EQ::behavior::InitializeDynamicLookups() {
	if (behavior_dictionary_init == true)
		return;
	behavior_dictionary_init = true;

	// use static references for now
}

const EQ::behavior::LookupEntry* EQ::behavior::DynamicLookup(versions::MobVersion mob_version, bool gm_flag)
{
	if (gm_flag)
		return DynamicGMLookup(mob_version);
	else
		return DynamicNonGMLookup(mob_version);
}

const EQ::behavior::LookupEntry* EQ::behavior::DynamicNonGMLookup(versions::MobVersion mob_version)
{
	mob_version = versions::ValidateMobVersion(mob_version);
	if (behavior_dynamic_nongm_lookup_entries[static_cast<int>(mob_version)])
		return behavior_dynamic_nongm_lookup_entries[static_cast<int>(mob_version)].get();

	return &behavior_static_lookup_entries[static_cast<int>(mob_version)];
}

const EQ::behavior::LookupEntry* EQ::behavior::DynamicGMLookup(versions::MobVersion mob_version)
{
	mob_version = versions::ValidateMobVersion(mob_version);
	if (behavior_dynamic_gm_lookup_entries[static_cast<int>(mob_version)])
		return behavior_dynamic_gm_lookup_entries[static_cast<int>(mob_version)].get();

	return &behavior_static_lookup_entries[static_cast<int>(mob_version)];
}

const EQ::behavior::LookupEntry* EQ::behavior::StaticLookup(versions::MobVersion mob_version)
{
	return &behavior_static_lookup_entries[static_cast<int>(versions::ValidateMobVersion(mob_version))];
}

static std::unique_ptr<EQ::spells::LookupEntry> spells_dynamic_nongm_lookup_entries[EQ::versions::ClientVersionCount];
static std::unique_ptr<EQ::spells::LookupEntry> spells_dynamic_gm_lookup_entries[EQ::versions::ClientVersionCount];
static const EQ::spells::LookupEntry spells_static_lookup_entries[EQ::versions::ClientVersionCount] =
{
	/*[ClientVersion::Unknown] =*/
	EQ::spells::LookupEntry(
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL,
		ClientUnknown::INULL
	),
	/*[ClientVersion::RoF2] =*/
	EQ::spells::LookupEntry(
		RoF2::spells::SPELL_ID_MAX,
		RoF2::spells::SPELLBOOK_SIZE,
		EQ::spells::SPELL_GEM_COUNT,
		RoF2::spells::LONG_BUFFS,
		RoF2::spells::SHORT_BUFFS,
		RoF2::spells::DISC_BUFFS,
		RoF2::spells::TOTAL_BUFFS,
		RoF2::spells::NPC_BUFFS,
		RoF2::spells::PET_BUFFS,
		RoF2::spells::MERC_BUFFS
	)
};

static bool spells_dictionary_init = false;
void EQ::spells::InitializeDynamicLookups() {
	if (spells_dictionary_init == true)
		return;
	spells_dictionary_init = true;

	// use static references for now
}

const EQ::spells::LookupEntry* EQ::spells::DynamicLookup(versions::ClientVersion client_version, bool gm_flag)
{
	if (gm_flag)
		return DynamicGMLookup(client_version);
	else
		return DynamicNonGMLookup(client_version);
}

const EQ::spells::LookupEntry* EQ::spells::DynamicNonGMLookup(versions::ClientVersion client_version)
{
	client_version = versions::ValidateClientVersion(client_version);
	if (spells_dynamic_nongm_lookup_entries[static_cast<int>(client_version)])
		return spells_dynamic_nongm_lookup_entries[static_cast<int>(client_version)].get();

	return &spells_static_lookup_entries[static_cast<int>(client_version)];
}

const EQ::spells::LookupEntry* EQ::spells::DynamicGMLookup(versions::ClientVersion client_version)
{
	client_version = versions::ValidateClientVersion(client_version);
	if (spells_dynamic_gm_lookup_entries[static_cast<int>(client_version)])
		return spells_dynamic_gm_lookup_entries[static_cast<int>(client_version)].get();

	return &spells_static_lookup_entries[static_cast<int>(client_version)];
}

const EQ::spells::LookupEntry* EQ::spells::StaticLookup(versions::ClientVersion client_version)
{
	return &spells_static_lookup_entries[static_cast<int>(versions::ValidateClientVersion(client_version))];
}
