#include "../common/evolving_items.h"

#include "../common/events/player_event_logs.h"
#include "../common/global_define.h"

#include "client.h"
#include "string_ids.h"
#include "worldserver.h"

extern WorldServer worldserver;
extern QueryServ*  QServ;
const std::string  SUB_TYPE_DELIMITER = ".";

void Client::DoEvolveItemToggle(const EQApplicationPacket *app)
{
	const auto in = reinterpret_cast<EvolveItemToggle *>(app->pBuffer);
	// The client echoes the item header's two dwords as one number: the serial, and 0 for an item not in a parcel.
	auto *inst = GetInv().GetItemBySerialNumber(static_cast<uint32>(in->unique_id & 0xFFFFFFFF));
	if (!inst || !inst->IsEvolving()) {
		LogWarning("Character [{}] asked to toggle evolving item [{}], which it does not hold", CharacterID(), in->unique_id);
		return;
	}
	LogEvolveItemDetail(
		"Character ID <green>[{}] requested to set evolve item with unique id <yellow>[{}] to status <yellow>[{}]",
		CharacterID(),
		in->unique_id,
		in->activated
	);

	const bool activated = in->activated != 0;
	if (!inst->HasEvolveRecord() || !CharacterEvolvingItemsRepository::SetActivated(database, inst->GetGUID(), activated)) {
		LogEvolveItemDetail(
			"Character ID <green>[{}] toggle evolve item unique id <yellow>[{}] failed", CharacterID(), in->unique_id);
		return;
	}

	inst->SetEvolveActivated(activated);
	SendEvolvingPacket(EvolvingItems::Actions::UPDATE_ITEMS, *inst);
}

void Client::SendEvolvingPacket(const int8 action, const EQ::ItemInstance &inst)
{
	auto       out  = std::make_unique<EQApplicationPacket>(OP_EvolveItem, sizeof(EvolveItemToggle));
	const auto data = reinterpret_cast<EvolveItemToggle *>(out->pBuffer);

	data->action     = action;
	data->unique_id  = inst.GetSerialNumber();
	data->percentage = inst.GetEvolveProgression();
	data->activated  = inst.GetEvolveActivated();

	QueuePacket(out.get());

	LogEvolveItem(
		"Sent evolve item with unique id <yellow>[{}] status <yellow>[{}] percentage <yellow>[{}] to Character ID "
		"<green>[{}]",
		data->unique_id,
		data->activated,
		data->percentage,
		CharacterID()
	);
}

void Client::ProcessEvolvingItem(const uint64 exp, const Mob *mob)
{
	std::vector<int16> queue{};

	for (auto &[key, inst]: GetInv().GetWorn()) {
		LogEvolveItemDetail(
			"CharacterID <green>[{}] found equipped item ID <yellow>[{}]", CharacterID(), inst->GetID());
		if (!inst->IsEvolving() || !inst->GetEvolveActivated()) {
			LogEvolveItemDetail(
				"CharacterID <green>[{}], item ID <yellow>[{}] not an evolving item.", CharacterID(), inst->GetID()
			);
			continue;
		}

		if (inst->GetTimers().contains("evolve") && !inst->GetTimers().at("evolve").Check(false)) {
			LogEvolveItemDetail(
				"CharacterID <green>[{}], item ID <yellow>[{}] timer not yet expired. <red>[{}] secs remaining.",
				CharacterID(),
				inst->GetID(),
				inst->GetTimers().at("evolve").GetRemainingTime() / 1000);
			continue;
		}

		if (!EvolvingItemsManager::Instance()->GetEvolvingItemsCache().contains(inst->GetID())) {
			LogEvolveItem(
				"Character ID {} has an evolving item that is not found in the db. Please check your "
				"items_evolving_details table for item id {}",
				CharacterID(),
				inst->GetID()
			);
			continue;
		}

		auto const type     = EvolvingItemsManager::Instance()->GetEvolvingItemsCache().at(inst->GetID()).type;
		auto const sub_type = EvolvingItemsManager::Instance()->GetEvolvingItemsCache().at(inst->GetID()).sub_type;

		LogEvolveItemDetail(
			"CharacterID <green>[{}] item id <green>[{}] type {} sub_type {} is Evolving.  Continue processing...",
			CharacterID(),
			inst->GetID(),
			type,
			sub_type
		);

		auto sub_types = Strings::Split(sub_type, SUB_TYPE_DELIMITER);
		auto has_sub_type = [&](uint32_t type) {
			return Strings::Contains(sub_types, std::to_string(type));
		};

		switch (type) {
			case EvolvingItems::Types::AMOUNT_OF_EXP: {
				LogEvolveItemDetail("Type <green>[{}] Processing sub_type", type);

				// Determine the evolve amount based on sub_type conditions
				int evolve_amount = 0;

				if (has_sub_type(EvolvingItems::SubTypes::ALL_EXP) ||
					(has_sub_type(EvolvingItems::SubTypes::GROUP_EXP) && IsGrouped())) {
					evolve_amount = exp * RuleR(EvolvingItems, PercentOfGroupExperience) / 100;
				}
				else if (has_sub_type(EvolvingItems::SubTypes::ALL_EXP) ||
						 (has_sub_type(EvolvingItems::SubTypes::RAID_EXP) && IsRaidGrouped())) {
					evolve_amount = exp * RuleR(EvolvingItems, PercentOfRaidExperience) / 100;
				}
				else if (has_sub_type(EvolvingItems::SubTypes::ALL_EXP) ||
						 has_sub_type(EvolvingItems::SubTypes::SOLO_EXP)) {
					evolve_amount = exp * RuleR(EvolvingItems, PercentOfSoloExperience) / 100;
				}

				inst->SetEvolveAddToCurrentAmount(evolve_amount);
				inst->CalculateEvolveProgression();

				if (!CharacterEvolvingItemsRepository::SetCurrentAmount(database, inst->GetGUID(), inst->GetEvolveCurrentAmount())) {
					break;
				}
				SendEvolvingPacket(EvolvingItems::Actions::UPDATE_ITEMS, *inst);
				LogEvolveItem(
					"Processing Complete for item id <green>[{1}] Type 1 Amount of EXP - SubType <yellow>[{0}] - "
					"Assigned <yellow>[{2}] of exp to <green>[{1}]",
					sub_type,
					inst->GetID(),
					exp * 0.001
				);

				if (inst->GetEvolveProgression() >= 100) {
					queue.push_back(key);
				}

				break;
			}
			case EvolvingItems::Types::SPECIFIC_MOB_RACE: {
				LogEvolveItemDetail("Type <green>[{}] Processing sub type", type);
				if (mob && has_sub_type(mob->GetRace())) {
					LogEvolveItemDetail("Sub_Type <green>[{}] Processing Item", sub_type);
					inst->SetEvolveAddToCurrentAmount(1);
					inst->CalculateEvolveProgression();

					if (!CharacterEvolvingItemsRepository::SetCurrentAmount(database, inst->GetGUID(), inst->GetEvolveCurrentAmount())) {
						break;
					}
					SendEvolvingPacket(EvolvingItems::Actions::UPDATE_ITEMS, *inst);
					LogEvolveItem(
						"Processing Complete for item id <green>[{1}] Type 3 Specific Mob Race - SubType "
						"<yellow>[{0}] "
						"- Increased count by 1 for <green>[{1}]",
						sub_type,
						inst->GetID()
					);
				}

				if (inst->GetEvolveProgression() >= 100) {
					queue.push_back(key);
				}

				break;
			}
			case EvolvingItems::Types::SPECIFIC_ZONE_ID: {
				LogEvolveItemDetail("Type <green>[{}] Processing sub type", type);
				if (mob && has_sub_type(mob->GetZoneID())) {
					LogEvolveItemDetail("Sub_Type <green>[{}] Processing Item", sub_type);
					inst->SetEvolveAddToCurrentAmount(1);
					inst->CalculateEvolveProgression();

					if (!CharacterEvolvingItemsRepository::SetCurrentAmount(database, inst->GetGUID(), inst->GetEvolveCurrentAmount())) {
						break;
					}
					SendEvolvingPacket(EvolvingItems::Actions::UPDATE_ITEMS, *inst);
					LogEvolveItem(
						"Processing Complete for item id <green>[{1}] Type 4 Specific Zone ID - SubType "
						"<yellow>[{0}] "
						"- Increased count by 1 for <green>[{1}]",
						sub_type,
						inst->GetID()
					);
				}

				if (inst->GetEvolveProgression() >= 100) {
					queue.push_back(key);
				}

				break;
			}
			case EvolvingItems::Types::NUMBER_OF_KILLS: {
				LogEvolveItemDetail("Type <green>[{}] Processing sub type", type);
				if (mob) {
					if (mob->GetLevel() >= Strings::ToUnsignedInt(sub_types.front()) ||
						Strings::ToUnsignedInt(sub_types.front()) == 0
					) {
						LogEvolveItemDetail("Sub_Type <green>[{}] Processing Item", sub_type);
						inst->SetEvolveAddToCurrentAmount(1);
						inst->CalculateEvolveProgression();

						if (!CharacterEvolvingItemsRepository::SetCurrentAmount(database, inst->GetGUID(), inst->GetEvolveCurrentAmount())) {
							break;
						}
						SendEvolvingPacket(EvolvingItems::Actions::UPDATE_ITEMS, *inst);
						LogEvolveItem(
							"Processing Complete for item id <green>[{1}] Type 4 Specific Zone ID - SubType "
							"<yellow>[{0}] "
							"- Increased count by 1 for <green>[{1}]",
							sub_type,
							inst->GetID()
						);
					}
				}

				if (inst->GetEvolveProgression() >= 100) {
					queue.push_back(key);
				}

				break;
			}
			default: {
			}
		}
	}

	if (!queue.empty()) {
		for (const int16 slot_id : queue) {
			DoEvolveCheckProgression(slot_id);
		}
	}
}

void Client::DoEvolveItemDisplayFinalResult(const EQApplicationPacket *app)
{
	const auto in        = reinterpret_cast<EvolveItemToggle *>(app->pBuffer);

	const uint32 item_id = static_cast<uint32>(in->unique_id & 0xFFFFFFFF);
	if (item_id == 0) {
		LogEvolveItem("Error - Item ID of final evolve item is blank.");
		return;
	}

	std::unique_ptr<EQ::ItemInstance> const inst(database.CreateItem(item_id));
	if (!inst) {
		return;
	}

	LogEvolveItemDetail(
		"Character ID <green>[{}] requested to view final evolve item id <yellow>[{}] for evolve item id <yellow>[{}]",
		CharacterID(),
		item_id,
		EvolvingItemsManager::Instance()->GetFirstItemInLoreGroupByItemID(item_id)
	);

	inst->SetEvolveProgression(100);

	LogEvolveItemDetail(
		"Sending final result for item id <yellow>[{}] to Character ID <green>[{}]", item_id, CharacterID()
	);
	SendItemPacket(0, inst.get(), ItemPacketViewLink);
}

// Makes the evolving item in a slot another item of its line where it is. The row keeps its GUID and its record, whose
// amount becomes new_amount, and an augment stays unless the new item's socket refuses it. The caller holds the
// transaction, undoes this with UndoEvolveInPlace when the act fails, and sends the item once it has committed.
bool Client::EvolveItemInPlace(const int16 slot_id, const uint32 new_item_id, const uint64 new_amount, std::vector<CharacterParcelsRepository::CharacterParcels> &parcels)
{
	const auto *inst = m_inv.GetItem(slot_id);
	const std::unique_ptr<EQ::ItemInstance> new_inst(database.CreateItem(new_item_id));
	if (!inst || !inst->HasEvolveRecord() || !new_inst) {
		return false;
	}

	if (!CharacterEvolvingItemsRepository::SetCurrentAmount(database, inst->GetGUID(), new_amount)) {
		return false;
	}

	ItemLayer::CarryGUID(*inst, *new_inst);
	new_inst->GetEvolvingDetails() = inst->GetEvolvingDetails();
	new_inst->SetEvolveCurrentAmount(new_amount);
	new_inst->CalculateEvolveProgression();
	new_inst->SetEvolveFinalItemID(EvolvingItemsManager::Instance()->GetFinalItemID(*new_inst));

	const bool to_parcels = RuleB(Parcel, EnableParcelMerchants) && !RuleB(EvolvingItems, DestroyAugmentsOnEvolve);
	for (uint8 socket = EQ::invaug::SOCKET_BEGIN; socket <= EQ::invaug::SOCKET_END; socket++) {
		const auto *augment = inst->GetAugment(socket);
		if (!augment) {
			continue;
		}

		if (new_inst->IsAugmentSlotAvailable(augment->GetAugmentType(), socket)) {
			new_inst->PutAugment(socket, *augment);
			continue;
		}

		const int32 parcel_slot = to_parcels ? FindNextFreeParcelSlotUsingMemory() : INVALID_INDEX;
		if (parcel_slot == INVALID_INDEX) {
			if (!ItemLayer::RetireRow(database, augment->GetGUID(), ItemRetirement::Destroyed)) {
				return false;
			}
			continue;
		}

		CharacterParcelsRepository::CharacterParcels p{};
		p.char_id   = CharacterID();
		p.from_name = "Evolving Item Sub-System";
		p.note      = fmt::format("System automatically removed from {} which recently evolved.", inst->GetItem()->Name);
		p.slot_id   = parcel_slot;
		p.sent_date = time(nullptr);
		CharacterParcelsRepository::SetGUID(p, augment->GetGUID());

		if (!ItemLayer::MoveRow(database, augment->GetGUID(), ItemLocation::InHolder(ItemHolderType::Parcel, CharacterID(), static_cast<uint16>(parcel_slot)))) {
			return false;
		}

		p.id = CharacterParcelsRepository::InsertOne(database, p).id;
		if (!p.id) {
			return false;
		}

		if (PlayerEventLogs::Instance()->IsEventEnabled(PlayerEvent::PARCEL_SEND)) {
			PlayerEvent::ParcelSend e{};
			e.from_player_name = p.from_name;
			e.to_player_name   = GetCleanName();
			e.item_id          = augment->GetID();
			e.quantity         = 1;
			e.sent_date        = p.sent_date;
			RecordPlayerEventLog(PlayerEvent::PARCEL_SEND, e);
		}

		parcels.push_back(p);
		m_parcels.emplace(p.slot_id, p);
	}

	m_inv.PutItem(slot_id, *new_inst);

	return database.SaveInventory(this, slot_id);
}

// Puts back in memory what EvolveItemInPlace changed there; the caller's transaction undoes the rows.
void Client::UndoEvolveInPlace(const int16 slot_id, const EQ::ItemInstance &old_inst, const std::vector<CharacterParcelsRepository::CharacterParcels> &parcels)
{
	m_inv.PutItem(slot_id, old_inst);
	for (const auto &p : parcels) {
		m_parcels.erase(p.slot_id);
	}
}

void Client::SendEvolvedItem(const int16 slot_id)
{
	SendItemPacket(slot_id, m_inv.GetItem(slot_id), ItemPacketTrade);
	if (slot_id <= EQ::invslot::EQUIPMENT_END) {
		SendWearChange(SlotConvert2(slot_id));
	}
}

bool Client::DoEvolveCheckProgression(const int16 slot_id)
{
	const auto *inst = m_inv.GetItem(slot_id);
	if (!inst || !inst->IsEvolving()) {
		return false;
	}

	if (inst->GetEvolveProgression() < 100 || inst->GetEvolveLvl() == inst->GetMaxEvolveLvl()) {
		return false;
	}

	const auto new_item_id = EvolvingItemsManager::Instance()->GetNextEvolveItemID(*inst);
	if (!new_item_id) {
		return false;
	}

	const std::unique_ptr<EQ::ItemInstance> old_inst(inst->Clone());
	const auto carried = EvolvingItemsManager::AmountPastRequirement(inst->GetEvolveCurrentAmount(), inst->GetID());

	// The item's next level, its record's amount and any augment that leaves it are one evolve.
	std::vector<CharacterParcelsRepository::CharacterParcels> parcels;
	DBcore::Transaction evolve(database);
	if (!EvolveItemInPlace(slot_id, new_item_id, carried, parcels) || !evolve.Commit()) {
		UndoEvolveInPlace(slot_id, *old_inst, parcels);
		LogError("Evolving item [{}] of character [{}] did not become [{}]; it stays as it was", old_inst->GetID(), CharacterID(), new_item_id);
		return false;
	}

	const auto *new_inst = m_inv.GetItem(slot_id);
	CheckItemDiscoverability(new_item_id);
	SendEvolvedItem(slot_id);
	CalcBonuses();
	if (!parcels.empty()) {
		SendParcelStatus();
		SendParcelIconStatus();
	}

	PlayerEvent::EvolveItem e{};
	EvolvingItemsManager::Instance()->LoadPlayerEvent(*old_inst, e);
	e.status = "Evolved Item due to obtaining progression - Old Evolve Item.";
	RecordPlayerEventLog(PlayerEvent::EVOLVE_ITEM, e);

	EvolvingItemsManager::Instance()->LoadPlayerEvent(*new_inst, e);
	e.status = "Evolved Item due to obtaining progression - New Evolve Item in its place.";
	RecordPlayerEventLog(PlayerEvent::EVOLVE_ITEM, e);

	MessageString(Chat::Yellow, EVOLVE_ITEM_EVOLVED, old_inst->GetItem()->Name);

	LogEvolveItem(
		"Evolved item id <red>[{}] into item id <green>[{}] for Character ID <green>[{}]",
		old_inst->GetID(),
		new_item_id,
		CharacterID());

	return true;
}

void Client::SendEvolveXPTransferWindow()
{
	auto       out  = std::make_unique<EQApplicationPacket>(OP_EvolveItem, sizeof(EvolveItemToggle));
	const auto data = reinterpret_cast<EvolveItemToggle *>(out->pBuffer);

	data->action    = 1;

	QueuePacket(out.get());
}

void Client::SendEvolveXPWindowDetails(const EQApplicationPacket *app)
{
	const auto in        = reinterpret_cast<EvolveXPWindowReceive *>(app->pBuffer);
	const auto inst_from = GetInv().GetItemBySerialNumber(static_cast<uint32>(in->item1_unique_id & 0xFFFFFFFF));
	const auto inst_to   = GetInv().GetItemBySerialNumber(static_cast<uint32>(in->item2_unique_id & 0xFFFFFFFF));
	if (!inst_from || !inst_to || !inst_from->IsEvolving() || !inst_to->IsEvolving()) {
		return;
	}
	const auto results = EvolvingItemsManager::Instance()->DetermineTransferResults(*inst_from, *inst_to);

	if (!results.item_from_id || !results.item_to_id) {
		SendEvolveTransferResults(*inst_from, *inst_to, *inst_from, *inst_to, 0, 0);
		return;
	}

	std::unique_ptr<EQ::ItemInstance> const inst_from_new(database.CreateItem(results.item_from_id));
	std::unique_ptr<EQ::ItemInstance> const inst_to_new(database.CreateItem(results.item_to_id));
	if (!inst_from_new || !inst_to_new) {
		SendEvolveTransferResults(*inst_from, *inst_to, *inst_from, *inst_to, 0, 0);
		return;
	}

	inst_from_new->SetEvolveCurrentAmount(results.item_from_current_amount);
	inst_from_new->CalculateEvolveProgression();
	inst_to_new->SetEvolveCurrentAmount(results.item_to_current_amount);
	inst_to_new->CalculateEvolveProgression();

	SendEvolveTransferResults(
		*inst_from, *inst_to, *inst_from_new, *inst_to_new, results.compatibility, results.max_transfer_level);
}

void Client::DoEvolveTransferXP(const EQApplicationPacket *app)
{
	const auto in        = reinterpret_cast<EvolveXPWindowReceive *>(app->pBuffer);
	const auto inst_from = GetInv().GetItemBySerialNumber(static_cast<uint32>(in->item1_unique_id & 0xFFFFFFFF));
	const auto inst_to   = GetInv().GetItemBySerialNumber(static_cast<uint32>(in->item2_unique_id & 0xFFFFFFFF));
	if (!inst_from || !inst_to || !inst_from->IsEvolving() || !inst_to->IsEvolving()) {
		Message(Chat::Red, "Transfer Failed.  Incompatible Items.");
		LogEvolveItem("Transfer Failed for Character ID <green>[{}]", CharacterID());
		return;
	}

	const auto results = EvolvingItemsManager::Instance()->DetermineTransferResults(*inst_from, *inst_to);

	if (!results.item_from_id || !results.item_to_id) {
		Message(Chat::Red, "Transfer Failed.  Incompatible Items.");
		LogEvolveItem("Transfer Failed for Character ID <green>[{}]", CharacterID());
		return;
	}

	const int16 slot_from = SlotOfSerialNumber(inst_from->GetSerialNumber());
	const int16 slot_to   = SlotOfSerialNumber(inst_to->GetSerialNumber());
	if (slot_from == EQ::invslot::SLOT_INVALID || slot_to == EQ::invslot::SLOT_INVALID ||
		slot_from == EQ::invslot::slotCursor || slot_to == EQ::invslot::slotCursor) {
		Message(Chat::Red, "Transfer Failed.  Both items must be in your inventory, not on your cursor.");
		LogEvolveItem("Transfer Failed for Character ID <green>[{}]", CharacterID());
		return;
	}

	const std::unique_ptr<EQ::ItemInstance> old_from(inst_from->Clone());
	const std::unique_ptr<EQ::ItemInstance> old_to(inst_to->Clone());

	// Both items' new levels and amounts are one transfer.
	std::vector<CharacterParcelsRepository::CharacterParcels> parcels;
	DBcore::Transaction transfer(database);
	if (!EvolveItemInPlace(slot_from, results.item_from_id, results.item_from_current_amount, parcels) ||
		!EvolveItemInPlace(slot_to, results.item_to_id, results.item_to_current_amount, parcels) ||
		!transfer.Commit()) {
		UndoEvolveInPlace(slot_from, *old_from, parcels);
		UndoEvolveInPlace(slot_to, *old_to, parcels);
		Message(Chat::Red, "Transfer Failed.");
		LogError("Evolve transfer from item [{}] to item [{}] of character [{}] failed; both stay as they were", old_from->GetID(), old_to->GetID(), CharacterID());
		return;
	}

	const auto *new_from = m_inv.GetItem(slot_from);
	const auto *new_to   = m_inv.GetItem(slot_to);
	SendEvolvedItem(slot_from);
	SendEvolvedItem(slot_to);
	CalcBonuses();
	if (!parcels.empty()) {
		SendParcelStatus();
		SendParcelIconStatus();
	}

	PlayerEvent::EvolveItem e{};
	EvolvingItemsManager::Instance()->LoadPlayerEvent(*old_from, e);
	e.status = "Transfer XP - Original FROM Evolve Item.";
	RecordPlayerEventLog(PlayerEvent::EVOLVE_ITEM, e);

	EvolvingItemsManager::Instance()->LoadPlayerEvent(*new_from, e);
	e.status = "Transfer XP - Updated FROM item in its place.";
	RecordPlayerEventLog(PlayerEvent::EVOLVE_ITEM, e);

	EvolvingItemsManager::Instance()->LoadPlayerEvent(*old_to, e);
	e.status = "Transfer XP - Original TO Evolve Item.";
	RecordPlayerEventLog(PlayerEvent::EVOLVE_ITEM, e);

	EvolvingItemsManager::Instance()->LoadPlayerEvent(*new_to, e);
	e.status = "Transfer XP - Updated TO Evolve item in its place.";
	RecordPlayerEventLog(PlayerEvent::EVOLVE_ITEM, e);

	LogEvolveItem(
		"Evolve Transfer XP resulted in evolved item id <red>[{}] into item id <green>[{}] for Character ID "
		"<green>[{}]",
		old_to->GetID(),
		new_to->GetID(),
		CharacterID()
	);
}

void Client::SendEvolveTransferResults(
	const EQ::ItemInstance &inst_from,
	const EQ::ItemInstance &inst_to,
	const EQ::ItemInstance &inst_from_new,
	const EQ::ItemInstance &inst_to_new,
	const uint32            compatibility,
	const uint32            max_transfer_level)
{
	std::stringstream           ss;
	cereal::BinaryOutputArchive ar(ss);

	EvolveXPWindowSend e{};
	e.action             = EvolvingItems::Actions::TRANSFER_WINDOW_DETAILS;
	e.compatibility      = compatibility;
	e.item1_unique_id    = inst_from.GetSerialNumber();
	e.item2_unique_id    = inst_to.GetSerialNumber();
	e.max_transfer_level = max_transfer_level;
	e.item1_present      = 1;
	e.item2_present      = 1;
	e.serialize_item_1   = inst_from_new.Serialize(0);
	e.serialize_item_2   = inst_to_new.Serialize(0);

	{
		ar(e);
	}

	uint32 packet_size = sizeof(EvolveItemMessaging) + ss.str().length();

	std::unique_ptr<EQApplicationPacket> out(new EQApplicationPacket(OP_EvolveItem, packet_size));
	const auto                           data = reinterpret_cast<EvolveItemMessaging *>(out->pBuffer);

	data->action                              = EvolvingItems::Actions::TRANSFER_WINDOW_DETAILS;
	memcpy(data->serialized_data, ss.str().data(), ss.str().length());

	QueuePacket(out.get());

	ss.str("");
	ss.clear();
}
