/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2002 EQEMu Development Team (http://eqemu.org)

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
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "../common/global_define.h"
#include "../common/events/player_event_logs.h"
#include "../common/repositories/trader_repository.h"
#include "../common/repositories/character_parcels_repository.h"
#include "../common/item_layer.h"
#include "worldserver.h"
#include "string_ids.h"
#include "client.h"
#include "../common/ruletypes.h"

extern WorldServer worldserver;
extern QueryServ  *QServ;

void Client::SendBulkParcels()
{
	SetEngagedWithParcelMerchant(true);
	LoadParcels();

	if (m_parcels.empty()) {
		return;
	}

	for (auto &p : m_parcels) {
		SendParcelPacket(p.second);
	}

	if (m_parcels.size() >= RuleI(Parcel, ParcelMaxItems) + PARCEL_LIMIT) {
		LogError(
			"Found {} parcels for Character {}.  List truncated to the ParcelMaxItems rule [{}] + PARCEL_LIMIT.",
			m_parcels.size(),
			GetCleanName(),
			RuleI(Parcel, ParcelMaxItems)
		);
		SendParcelStatus();
		return;
	}
}

void Client::SendParcel(Parcel_Struct &parcel_in)
{
	auto results = CharacterParcelsRepository::GetWhere(
		database,
		fmt::format(
			"`char_id` = '{}' AND `slot_id` = '{}' LIMIT 1",
			CharacterID(),
			parcel_in.item_slot
		)
	);

	if (results.empty()) {
		return;
	}

	const auto& r = results.front();

	SendParcelPacket(r);
	m_parcels.emplace(r.slot_id, r);
}

// The parcel's item is its row; money is shown as the money item priced at the amount.
void Client::SendParcelPacket(const CharacterParcelsRepository::CharacterParcels &p)
{
	std::unique_ptr<EQ::ItemInstance> inst;
	if (CharacterParcelsRepository::IsMoney(p)) {
		inst.reset(database.CreateItem(PARCEL_MONEY_ITEM_ID, 1));
		if (inst) {
			inst->SetPrice(p.money);
		}
	}
	else {
		inst.reset(ItemLayer::LoadOne(database, database, CharacterParcelsRepository::GUIDOf(p)));
	}

	if (!inst) {
		return;
	}

	inst->SetMerchantCount(1);
	inst->SetMerchantSlot(p.slot_id);

	ParcelMessaging_Struct pms{};
	pms.packet_type     = ItemPacketParcel;
	pms.player_name     = p.from_name;
	pms.sent_time       = p.sent_date;
	pms.note            = p.note;
	pms.serialized_item = inst->Serialize(p.slot_id);
	pms.slot_id         = p.slot_id;

	std::stringstream           ss;
	cereal::BinaryOutputArchive ar(ss);
	ar(pms);

	uint32 packet_size = ss.str().length();
	std::unique_ptr<EQApplicationPacket> out(new EQApplicationPacket(OP_ItemPacket, packet_size));
	if (out->size != packet_size) {
		LogError(
			"Attempted to send a parcel packet of mismatched size {} with a buffer size of {}.",
			out->Size(),
			packet_size
		);
		return;
	}

	memcpy(out->pBuffer, ss.str().data(), out->size);
	QueuePacket(out.get());
}

void Client::DoParcelCancel()
{
	if (
		m_parcel_platinum ||
		m_parcel_gold ||
		m_parcel_silver ||
		m_parcel_copper
	) {
		m_pp.platinum += m_parcel_platinum;
		m_pp.gold     += m_parcel_gold;
		m_pp.silver   += m_parcel_silver;
		m_pp.copper   += m_parcel_copper;
		m_parcel_platinum = 0;
		m_parcel_gold     = 0;
		m_parcel_silver   = 0;
		m_parcel_copper   = 0;
		SaveCurrency();
		SendMoneyUpdate();
	}
}

void Client::SendParcelStatus()
{
	LoadParcels();

	int32 num_of_parcels = GetParcelCount();
	if (num_of_parcels > 0) {
		int32 num_over_limit = (num_of_parcels - RuleI(Parcel, ParcelMaxItems)) < 0 ? 0 : (num_of_parcels - RuleI(Parcel, ParcelMaxItems));
		if (num_of_parcels == RuleI(Parcel, ParcelMaxItems)) {
			Message(
				Chat::Red,
				fmt::format(
					"You have reached the limit of {} parcels in your mailbox.  You will not be able to send parcels until you retrieve at least 1 parcel. ",
					RuleI(Parcel, ParcelMaxItems)
				).c_str()
			);
		}
		else if (num_over_limit == 1) {
			MessageString(
				Chat::Red,
				PARCEL_STATUS_1,
				std::to_string(num_of_parcels).c_str(),
				std::to_string(RuleI(Parcel, ParcelMaxItems)).c_str()
			);
		}
		else if (num_over_limit > 1) {
			MessageString(
				Chat::Red,
				PARCEL_STATUS_2,
				std::to_string(num_of_parcels).c_str(),
				std::to_string(num_over_limit).c_str(),
				std::to_string(RuleI(Parcel, ParcelMaxItems)).c_str()
			);
		}
		else {
			Message(
				Chat::Yellow,
				fmt::format(
					"You have {} parcels in your mailbox. Please visit a parcel merchant soon.",
					num_of_parcels
				).c_str()
			);
		}
	}
	SendParcelIconStatus();
}


void Client::DoParcelSend(const Parcel_Struct *parcel_in)
{
	if (IsCasting()) {
		StopCasting();
	}

	auto send_to_client = CharacterParcelsRepository::GetParcelCountAndCharacterName(database, parcel_in->send_to);
	auto merchant       = entity_list.GetMob(parcel_in->npc_id);
	if (!merchant) {
		SendParcelAck();
		return;
	}

	if (parcel_in->money_flag && parcel_in->item_slot != INVALID_INDEX) {
		Message(
			Chat::Yellow,
			fmt::format(
				"{} tells you, 'I am confused!  Do you want to send money or an item?'",
				merchant->GetCleanName()
			).c_str()
		);
		DoParcelCancel();
		SendParcelAck();
		return;
	}

	auto num_of_parcels = GetParcelCount();
	if (num_of_parcels >= RuleI(Parcel, ParcelMaxItems)) {
		SendParcelIconStatus();
		Message(
			Chat::Yellow,
			fmt::format(
				"{} tells you, 'Unfortunately, I cannot send your parcel as you are at your parcel limit of {}. Please retrieve a parcel and try again.",
				merchant->GetCleanName(),
				RuleI(Parcel, ParcelMaxItems)
			).c_str()
		);
		DoParcelCancel();
		SendParcelAck();
		return;
	}

	if (send_to_client.at(0).parcel_count >= RuleI(Parcel, ParcelMaxItems)) {
		Message(
			Chat::Yellow,
			fmt::format(
				"{} tells you, 'Unfortunately, {} cannot accept any more parcels at this time. Please try again later.",
				merchant->GetCleanName(),
				send_to_client.at(0).character_name == GetCleanName() ? "you" : send_to_client.at(0).character_name
			).c_str()
		);
		SendParcelAck();
		DoParcelCancel();
		return;
	}

	if (GetParcelTimer()->Check()) {
		SetParcelEnabled(true);
	}

	if (!GetParcelEnabled()) {
		MessageString(Chat::Yellow, PARCEL_DELAY, merchant->GetCleanName());
		DoParcelCancel();
		SendParcelAck();
		return;
	}
	auto next_slot = INVALID_INDEX;
	if (!send_to_client.at(0).character_name.empty()) {
		next_slot = FindNextFreeParcelSlot(send_to_client.at(0).char_id);
		if (next_slot == INVALID_INDEX) {
			Message(
				Chat::Yellow,
				fmt::format(
					"{} tells you, 'Unfortunately, {} cannot accept any more parcels at this time. Please try again later.",
					merchant->GetCleanName(),
					send_to_client.at(0).character_name
				).c_str()
			);
			SendParcelAck();
			DoParcelCancel();
			return;
		}
	}

	switch (parcel_in->money_flag) {
		case PARCEL_SEND_ITEMS: {
			auto inst = GetInv().GetItem(parcel_in->item_slot);
			if (!inst) {
				LogError(
					"Handle_OP_ShopSendParcel Could not find item in inventory slot {} for character {}.",
					parcel_in->item_slot,
					GetCleanName()
				);
				SendParcelAck();
				DoParcelCancel();
				return;
			}

			if (send_to_client.at(0).character_name.empty()) {
				MessageString(
					Chat::Yellow,
					PARCEL_UNKNOWN_NAME,
					merchant->GetCleanName(),
					parcel_in->send_to,
					inst->GetItem()->Name
				);
				SendParcelAck();
				DoParcelCancel();
				return;
			}

			if (!inst->IsDroppable(true)) {
				Message(Chat::Yellow, "Unable to send a parcel that is NO-DROP or contains a NO-DROP item.");
				SendParcelAck();
				DoParcelCancel();
				return;
			}

			CharacterParcelsRepository::CharacterParcels parcel_out{};
			parcel_out.from_name = GetName();
			parcel_out.note      = parcel_in->note;
			parcel_out.sent_date = time(nullptr);
			parcel_out.char_id   = send_to_client.at(0).char_id;
			parcel_out.slot_id   = next_slot;

			// The parcel names the item's row, moved whole or split off the sender's stack, in one transaction.
			const auto   parcel_location = ItemLocation::InHolder(ItemHolderType::Parcel, parcel_out.char_id, static_cast<uint16>(next_slot));
			const bool   partial         = inst->IsStackable() && parcel_in->quantity < static_cast<uint32>(inst->GetCharges());
			const uint32 sent_quantity   = partial ? parcel_in->quantity : static_cast<uint32>(std::max<int16>(inst->GetCharges(), 1));

			database.TransactionBegin();
			bool placed = false;
			if (partial) {
				std::unique_ptr<EQ::ItemInstance> piece(inst->Clone());
				ItemLayer::ClearGUID(*piece);
				piece->SetCharges(parcel_in->quantity);
				placed = ItemLayer::Save(database, *piece, parcel_location, ItemOwner());
				CharacterParcelsRepository::SetGUID(parcel_out, piece->GetGUID());
			}
			else {
				placed = ItemLayer::MoveRow(database, inst->GetGUID(), parcel_location);
				CharacterParcelsRepository::SetGUID(parcel_out, inst->GetGUID());
			}

			auto result = placed ? CharacterParcelsRepository::InsertOne(database, parcel_out) : CharacterParcelsRepository::NewEntity();
			if (!result.id) {
				database.TransactionRollback();
				LogError(
					"Failed to add parcel to database.  From {} to {} item {} quantity {}",
					parcel_out.from_name,
					parcel_out.char_id,
					inst->GetID(),
					sent_quantity
				);
				Message(Chat::Yellow, "Unable to save parcel to the database. Please see an administrator.");
				SendParcelAck();
				DoParcelCancel();
				return;
			}

			database.TransactionCommit();

			if (partial) {
				inst->SetCharges(inst->GetCharges() - parcel_in->quantity);
				SaveInventory(parcel_in->item_slot);
				SendSlotChange(parcel_in->item_slot, inst, false, parcel_in->quantity, true);
			}
			else {
				RemoveItemFromInventory(parcel_in->item_slot, true);
			}

			std::unique_ptr<EQApplicationPacket> outapp(new EQApplicationPacket(OP_ShopSendParcel));
			QueuePacket(outapp.get());

			MessageString(
				Chat::Yellow,
				PARCEL_DELIVERY,
				merchant->GetCleanName(),
				inst->GetItem()->Name,
				send_to_client.at(0).character_name.c_str()
			);

			if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::PARCEL_SEND)) {
				PlayerEvent::ParcelSend e{};
				e.from_player_name = parcel_out.from_name;
				e.to_player_name   = send_to_client.at(0).character_name;
				e.item_id          = inst->GetID();
				e.augment_1_id     = inst->GetAugmentItemID(0);
				e.augment_2_id     = inst->GetAugmentItemID(1);
				e.augment_3_id     = inst->GetAugmentItemID(2);
				e.augment_4_id     = inst->GetAugmentItemID(3);
				e.augment_5_id     = inst->GetAugmentItemID(4);
				e.augment_6_id     = inst->GetAugmentItemID(5);
				e.quantity         = sent_quantity;
				e.sent_date        = parcel_out.sent_date;

				RecordPlayerEventLog(PlayerEvent::PARCEL_SEND, e);

				for (auto const &[index, content] : *inst->GetContents()) {
					if (!content) {
						continue;
					}
					e.from_player_name = parcel_out.from_name;
					e.to_player_name   = send_to_client.at(0).character_name;
					e.item_id          = content->GetID();
					e.augment_1_id     = content->GetAugmentItemID(0);
					e.augment_2_id     = content->GetAugmentItemID(1);
					e.augment_3_id     = content->GetAugmentItemID(2);
					e.augment_4_id     = content->GetAugmentItemID(3);
					e.augment_5_id     = content->GetAugmentItemID(4);
					e.augment_6_id     = content->GetAugmentItemID(5);
					e.quantity         = content->GetCharges() >= 0 ? content->GetCharges() : 1;
					e.sent_date        = parcel_out.sent_date;
					RecordPlayerEventLog(PlayerEvent::PARCEL_SEND, e);
				}
			}

			Parcel_Struct ps{};
			ps.item_slot = parcel_out.slot_id;
			strn0cpy(ps.send_to, send_to_client.at(0).character_name.c_str(), sizeof(ps.send_to));

			SendParcelDeliveryToWorld(ps);

			break;
		}
		case PARCEL_SEND_MONEY: {
			auto item = database.GetItem(PARCEL_MONEY_ITEM_ID);
			if (!item) {
				DoParcelCancel();
				SendParcelAck();
				return;
			}

			std::unique_ptr<EQ::ItemInstance> inst(database.CreateItem(item, 1));
			if (!inst) {
				DoParcelCancel();
				SendParcelAck();
				return;
			}

			if (send_to_client.at(0).character_name.empty()) {
				MessageString(
					Chat::Yellow,
					PARCEL_UNKNOWN_NAME,
					merchant->GetCleanName(),
					parcel_in->send_to,
					"Money"
				);
				DoParcelCancel();
				SendParcelAck();
				return;
			}

			CharacterParcelsRepository::CharacterParcels parcel_out{};
			parcel_out.from_name = GetName();
			parcel_out.note      = parcel_in->note;
			parcel_out.sent_date = time(nullptr);
			parcel_out.money     = parcel_in->quantity;
			parcel_out.char_id   = send_to_client.at(0).char_id;
			parcel_out.slot_id   = next_slot;
			parcel_out.id        = 0;

			auto result = CharacterParcelsRepository::InsertOne(database, parcel_out);
			if (!result.id) {
				LogError(
					"Failed to add parcel to database.  From {} to {} item {} quantity {}",
					parcel_out.from_name,
					send_to_client.at(0).character_name,
					PARCEL_MONEY_ITEM_ID,
					parcel_out.money
				);
				Message(
					Chat::Yellow,
					"Unable to save parcel to the database. Please see an administrator."
				);
				return;
			}

			MessageString(
				Chat::Yellow,
				PARCEL_DELIVERY,
				merchant->GetCleanName(),
				"Money",
				send_to_client.at(0).character_name.c_str()
			);

			if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::PARCEL_SEND)) {
				PlayerEvent::ParcelSend e{};
				e.from_player_name = parcel_out.from_name;
				e.to_player_name   = send_to_client.at(0).character_name;
				e.item_id          = PARCEL_MONEY_ITEM_ID;
				e.quantity         = parcel_out.money;
				e.sent_date        = parcel_out.sent_date;

				RecordPlayerEventLog(PlayerEvent::PARCEL_SEND, e);
			}

			m_parcel_platinum = 0;
			m_parcel_gold     = 0;
			m_parcel_silver   = 0;
			m_parcel_copper   = 0;
			std::unique_ptr<EQApplicationPacket> outapp(new EQApplicationPacket(OP_FinishTrade));
			QueuePacket(outapp.get());

			Parcel_Struct ps{};
			ps.item_slot = parcel_out.slot_id;
			strn0cpy(ps.send_to, send_to_client.at(0).character_name.c_str(), sizeof(ps.send_to));

			SendParcelDeliveryToWorld(ps);

			break;
		}
	}

	SendParcelAck();
	SendParcelIconStatus();
	SetParcelEnabled(false);
	GetParcelTimer()->Enable();
}

void Client::SendParcelAck()
{
	std::unique_ptr<EQApplicationPacket> outapp(new EQApplicationPacket(OP_FinishTrade));
	QueuePacket(outapp.get());

	std::unique_ptr<EQApplicationPacket> outapp2(new EQApplicationPacket(OP_ShopSendParcel, sizeof(Parcel_Struct)));
	auto data = (Parcel_Struct *) outapp2->pBuffer;
	data->item_slot = 0xffffffff;
	data->quantity  = 0xffffffff;
	QueuePacket(outapp2.get());
}

void Client::SendParcelRetrieveAck()
{
	std::unique_ptr<EQApplicationPacket> outapp(new EQApplicationPacket(OP_ShopRetrieveParcel));
	QueuePacket(outapp.get());
}

void Client::SendParcelDeliveryToWorld(const Parcel_Struct &parcel)
{
	std::unique_ptr<ServerPacket> out(new ServerPacket(ServerOP_ParcelDelivery, sizeof(Parcel_Struct)));
	auto                          data = (Parcel_Struct *) out->pBuffer;

	data->item_slot = parcel.item_slot;
	strn0cpy(data->send_to, parcel.send_to, sizeof(data->send_to));

	worldserver.SendPacket(out.get());
}

void Client::DoParcelRetrieve(const ParcelRetrieve_Struct &parcel_in)
{
	auto merchant = entity_list.GetNPCByID(parcel_in.merchant_entity_id);
	if (!merchant) {
		SendParcelRetrieveAck();
		return;
	}

	auto p = std::find_if(
		m_parcels.begin(),
		m_parcels.end(),
		[&](const std::pair<uint32, CharacterParcelsRepository::CharacterParcels> &x) {
			return x.first == parcel_in.parcel_slot_id;
		}
	);
	if (p != m_parcels.end()) {
		uint32 item_id = parcel_in.parcel_item_id;
		if (!item_id) {
			LogError(
				"Attempt to retrieve parcel with erroneous item id for client character id {}.",
				CharacterID()
			);
			SendParcelRetrieveAck();
			return;
		}

		// The parcel's item is its row, with its augments and contents; money has no row.
		std::unique_ptr<EQ::ItemInstance> inst;
		if (CharacterParcelsRepository::IsMoney(p->second)) {
			inst.reset(database.CreateItem(PARCEL_MONEY_ITEM_ID, 1));
		} else {
			inst.reset(ItemLayer::LoadOne(database, database, CharacterParcelsRepository::GUIDOf(p->second)));
		}

		if (!inst || inst->GetID() != item_id) {
			LogError(
				"Parcel slot [{}] of character [{}] holds item [{}], not the requested [{}]",
				parcel_in.parcel_slot_id,
				CharacterID(),
				inst ? inst->GetID() : 0,
				item_id
			);
			SendParcelRetrieveAck();
			return;
		}

		switch (parcel_in.parcel_item_id) {
			case PARCEL_MONEY_ITEM_ID: {
				AddMoneyToPP(p->second.money, true);
				MessageString(
					Chat::Yellow, PARCEL_DELIVERED, merchant->GetCleanName(), "Money", p->second.from_name.c_str()
				);
				break;
			}
			default: {
				// A duplicate lore item among the contents is destroyed or stops the delivery, by rule.
				std::vector<uint8> duplicate_lore_contents;
				for (auto const &[index, content] : *inst->GetContents()) {
					if (content && CheckLoreConflict(content->GetItem())) {
						if (RuleB(Parcel, DeleteOnDuplicate)) {
							MessageString(Chat::Yellow, PARCEL_DUPLICATE_DELETE, content->GetItem()->Name);
							duplicate_lore_contents.push_back(index);
							continue;
						}
						MessageString(Chat::Yellow, DUP_LORE);
						SendParcelRetrieveAck();
						return;
					}
				}
				for (const uint8 index : duplicate_lore_contents) {
					ItemLayer::Retire(database, *inst->GetItem(index), ItemRetirement::Destroyed);
					inst->DeleteItem(index);
				}

				auto const free_id = GetInv().FindFirstFreeSlotThatFitsItemWithStacking(inst.get());
				if (free_id == INVALID_INDEX) {
					SendParcelRetrieveAck();
					MessageString(Chat::White, PARCEL_INV_FULL, merchant->GetCleanName());
					return;
				}

				if (CheckLoreConflict(inst->GetItem())) {
					if (RuleB(Parcel, DeleteOnDuplicate)) {
						MessageString(Chat::Yellow, PARCEL_DUPLICATE_DELETE, inst->GetItem()->Name);
					}
					else {
						MessageString(Chat::Yellow, DUP_LORE);
						SendParcelRetrieveAck();
						return;
					}
				}

				if (AutoPutLootInInventory(*inst.get(), false, true)) {
					MessageString(
						Chat::Yellow,
						PARCEL_DELIVERED_2,
						merchant->GetCleanName(),
						std::to_string(inst->GetCharges()).c_str(),
						inst->GetItem()->Name,
						p->second.from_name.c_str()
					);
				}

				if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::PARCEL_RETRIEVE)) {
					PlayerEvent::ParcelRetrieve e{};
					e.from_player_name = p->second.from_name;
					e.item_id          = inst->GetID();
					e.augment_1_id     = inst->GetAugmentItemID(0);
					e.augment_2_id     = inst->GetAugmentItemID(1);
					e.augment_3_id     = inst->GetAugmentItemID(2);
					e.augment_4_id     = inst->GetAugmentItemID(3);
					e.augment_5_id     = inst->GetAugmentItemID(4);
					e.augment_6_id     = inst->GetAugmentItemID(5);
					e.quantity         = inst->GetCharges();
					e.sent_date        = p->second.sent_date;
					RecordPlayerEventLog(PlayerEvent::PARCEL_RETRIEVE, e);

				}
			}
		}
				DeleteParcel(p->second.id);
		SendParcelDelete(parcel_in);
		m_parcels.erase(p);
	}

	SendParcelRetrieveAck();
	SendParcelIconStatus();
}

bool Client::DeleteParcel(uint32 parcel_id)
{
	auto result = CharacterParcelsRepository::DeleteOne(database, parcel_id);
	if (!result) {
		LogError("Error deleting parcel id {} from the database.", parcel_id);
		return false;
	}

	SetParcelCount(GetParcelCount() - 1);

	return true;
}

void Client::LoadParcels()
{
	m_parcels.clear();
	auto results = CharacterParcelsRepository::GetWhere(database, fmt::format("char_id = '{}'", CharacterID()));

	for (auto const &p: results) {
		m_parcels.emplace(p.slot_id, p);
	}

	SetParcelCount(m_parcels.size());
}

void Client::SendParcelDelete(const ParcelRetrieve_Struct &parcel_in)
{
	std::unique_ptr<EQApplicationPacket> outapp(new EQApplicationPacket(OP_ShopDeleteParcel, sizeof(ParcelRetrieve_Struct)));
	auto data   = (ParcelRetrieve_Struct *) outapp->pBuffer;

	data->merchant_entity_id = parcel_in.merchant_entity_id;
	data->player_entity_id   = parcel_in.player_entity_id;
	data->parcel_slot_id     = parcel_in.parcel_slot_id;
	data->parcel_item_id     = parcel_in.parcel_item_id;

	QueuePacket(outapp.get());
}


int32 Client::FindNextFreeParcelSlot(uint32 char_id)
{
	auto results = CharacterParcelsRepository::GetWhere(
		database,
		fmt::format("char_id = '{}' ORDER BY slot_id ASC", char_id)
	);

	if (results.empty()) {
		return PARCEL_BEGIN_SLOT;
	}

	for (uint32 i = PARCEL_BEGIN_SLOT; i <= RuleI(Parcel, ParcelMaxItems); i++) {
		auto it = std::find_if(results.cbegin(), results.cend(), [&](const auto &x) { return x.slot_id == i; });
		if (it == results.end()) {
			return i;
		}
	}

	return INVALID_INDEX;
}

void Client::SendParcelIconStatus()
{
	std::unique_ptr<EQApplicationPacket> outapp(new EQApplicationPacket(OP_ShopParcelIcon, sizeof(ParcelIcon_Struct)));
	auto data   = (ParcelIcon_Struct *) outapp->pBuffer;

	auto const num_of_parcels = GetParcelCount();

	data->status = IconOn;
	if (num_of_parcels == 0) {
		data->status = IconOff;
	}
	else if (num_of_parcels > RuleI(Parcel, ParcelMaxItems)) {
		data->status = Overlimit;
	}

	QueuePacket(outapp.get());
}

void Client::AddParcel(CharacterParcelsRepository::CharacterParcels &parcel)
{
	auto result = CharacterParcelsRepository::InsertOne(database, parcel);
	if (!result.id) {
		LogError(
			"Failed to add parcel to database.  From {} to id {} slot {}",
			parcel.from_name,
			parcel.char_id,
			parcel.slot_id
		);
		Message(
			Chat::Yellow,
			"Unable to send parcel at this time.  Please try again later."
		);
		SendParcelAck();
	}
}

int32 Client::FindNextFreeParcelSlotUsingMemory()
{
	auto const results = GetParcels();

	if (results.empty()) {
		return PARCEL_BEGIN_SLOT;
	}

	for (uint32 i = PARCEL_BEGIN_SLOT; i <= RuleI(Parcel, ParcelMaxItems); i++) {
		if (!results.contains(i)) {
			return i;
		}
	}

	return INVALID_INDEX;
}
