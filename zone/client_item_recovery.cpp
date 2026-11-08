#include "client.h"
#include "entity.h"
#include "item_recovery.h"
#include "string_ids.h"
#include "../common/classes.h"
#include "../common/item_layer.h"
#include "../common/misc_functions.h"

#include <memory>
// A listed item is keyed by its session serial, which is also the unique id the client echoes from the item
// header and the number the recovered item keeps when it lands.
void Client::SendRecoverableItems()
{
	m_recovery_listing.clear();

	for (auto &entry : ItemLayer::LoadRecoverable(database, database, CharacterID(), ItemRecovery::States())) {
		const uint32 serial = static_cast<uint32>(entry.inst->GetSerialNumber());
		m_recovery_listing.emplace(serial, entry.guid);

		entry.inst->SetMerchantSlot(serial);
		entry.inst->SetMerchantCount(1);
		entry.inst->SetPrice(1);
		SendItemPacket(static_cast<int16>(serial), entry.inst, ItemPacketRecovery);
		safe_delete(entry.inst);
	}
	LogInventory("Character [{}] was sent [{}] recoverable items", CharacterID(), m_recovery_listing.size());
}

void Client::Handle_OP_ShopRecoverItem(const EQApplicationPacket *app)
{
	VERIFY_PACKET_LENGTH(OP_ShopRecoverItem, app, MerchantRecover_Struct);
	const auto *in = reinterpret_cast<const MerchantRecover_Struct *>(app->pBuffer);

	Mob *merchant = entity_list.GetMob(in->npc_id);
	if (
		!merchant ||
		!merchant->IsNPC() ||
		merchant->GetClass() != Class::Merchant ||
		DistanceSquared(m_Position, merchant->GetPosition()) > USE_NPC_RANGE2
	) {
		SendMerchantEnd();
		return;
	}

	const auto listing = m_recovery_listing.find(static_cast<uint32>(in->unique_id));
	if (listing == m_recovery_listing.end()) {
		LogError("Character [{}] asked to recover unique id [{}], not one of the [{}] it was sent", CharacterID(), in->unique_id, m_recovery_listing.size());
		return;
	}

	const ItemGUID guid   = listing->second;
	auto           listed = ItemLayer::LoadRecoverable(database, database, CharacterID(), ItemRecovery::States(), guid);
	if (listed.size() != 1) {
		LogError("Character [{}] asked to recover [{}], which is no longer listed", CharacterID(), ItemGUIDText(guid));
		m_recovery_listing.erase(listing);
		AcknowledgeRecovery(*in, 0);
		SendRecoveryListingRemoved(in->npc_id, in->unique_id);
		return;
	}
	std::unique_ptr<EQ::ItemInstance> inst(listed.front().inst);
	if (CheckLoreConflict(inst->GetItem())) {
		MessageString(Chat::Yellow, DUP_LORE);
		return;
	}
	for (const auto &[index, content] : *inst->GetContents()) {
		if (content && CheckLoreConflict(content->GetItem())) {
			MessageString(Chat::Yellow, DUP_LORE);
			return;
		}
	}

	if (GetAlternateCurrencyValue(ItemRecovery::currency) < 1) {
		MessageString(Chat::White, RECOVER_CANNOT_AFFORD, merchant->GetCleanName(), inst->GetItem()->Name);
		return;
	}

	const int16 slot = m_inv.FindFreeSlot(inst->IsClassBag(), true, inst->GetItem()->Size, inst->GetItem()->ItemType == EQ::item::ItemTypeArrow);
	if (slot == INVALID_INDEX) {
		Message(Chat::White, fmt::format("{} tells you, 'Your inventory appears full. Make room to recover the {}.'", merchant->GetCleanName(), inst->GetItem()->Name).c_str());
		return;
	}

	// The currency and the item's return are one exchange. Saving the item where it lands writes its rows whole,
	// so the retirement clears with the location.
	DBcore::Transaction recovery(database);
	if (!RemoveAlternateCurrencyValue(ItemRecovery::currency, 1)) {
		LogError("Character [{}] could not pay currency [{}] to recover [{}]; nothing changed", CharacterID(), ItemRecovery::currency, ItemGUIDText(guid));
		return;
	}
	PutLootInInventory(slot, *inst);
	recovery.Commit();

	LogInventory("Character [{}] recovered [{}] item [{}] into slot [{}]", CharacterID(), ItemGUIDText(guid), inst->GetID(), slot);
	m_recovery_listing.erase(listing);
	AcknowledgeRecovery(*in, 1);
	SendRecoveryListingRemoved(in->npc_id, in->unique_id);
}

void Client::SendRecoveryListingRemoved(uint32 merchant_id, uint64 unique_id)
{
	auto  outapp = new EQApplicationPacket(OP_ShopDelRecoveryItem, sizeof(MerchantDelRecoveryItem_Struct));
	auto *out    = reinterpret_cast<MerchantDelRecoveryItem_Struct *>(outapp->pBuffer);
	out->npc_id    = merchant_id;
	out->player_id = GetID();
	out->unique_id = unique_id;
	QueuePacket(outapp);
	safe_delete(outapp);
}
// The request echoed back, as a purchase is, naming the currency handed over.
void Client::AcknowledgeRecovery(const MerchantRecover_Struct &request, uint64 currency_paid)
{
	auto  outapp = new EQApplicationPacket(OP_ShopRecoverItem, sizeof(MerchantRecover_Struct));
	auto *out    = reinterpret_cast<MerchantRecover_Struct *>(outapp->pBuffer);
	*out               = request;
	out->unknown020    = 0;
	out->currency_paid = currency_paid;
	QueuePacket(outapp);
	safe_delete(outapp);
}
