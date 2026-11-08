#include "client.h"
#include "entity.h"
#include "item_recovery.h"
#include "string_ids.h"
#include "../common/classes.h"
#include "../common/item_layer.h"
#include "../common/misc_functions.h"

#include <memory>
// Recovery listings are numbered past every merchant slot (stock and temporary items stay within MERCHANT_SIZE),
// so a request naming one cannot be read as a purchase.
static constexpr uint32 RECOVERY_SLOT_BASE = 10000;

void Client::SendRecoverableItems()
{
	m_recovery_listing.clear();

	for (auto &entry : ItemLayer::LoadRecoverable(database, database, CharacterID(), ItemRecovery::States())) {
		m_recovery_listing.push_back(entry.guid);
		const uint32 slot = RECOVERY_SLOT_BASE + static_cast<uint32>(m_recovery_listing.size());

		entry.inst->SetMerchantSlot(slot);
		entry.inst->SetMerchantCount(1);
		entry.inst->SetPrice(ItemRecovery::Currency() ? 1 : 0);
		SendItemPacket(static_cast<int16>(slot), entry.inst, ItemPacketRecovery);
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

	if (in->listing_slot <= RECOVERY_SLOT_BASE || in->listing_slot - RECOVERY_SLOT_BASE > m_recovery_listing.size()) {
		LogError("Character [{}] asked to recover listing slot [{}], not one of the [{}] it was sent", CharacterID(), in->listing_slot, m_recovery_listing.size());
		return;
	}

	ItemGUID &guid   = m_recovery_listing[in->listing_slot - RECOVERY_SLOT_BASE - 1];
	auto      listed = ItemLayer::LoadRecoverable(database, database, CharacterID(), ItemRecovery::States(), guid);
	if (listed.size() != 1) {
		LogError("Character [{}] asked to recover [{}], which is no longer listed", CharacterID(), ItemGUIDText(guid));
		SendRecoveryListingRemoved(in->npc_id, in->listing_slot);
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

	const uint32 currency = ItemRecovery::Currency();
	if (currency && GetAlternateCurrencyValue(currency) < 1) {
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
	if (currency && !RemoveAlternateCurrencyValue(currency, 1)) {
		LogError("Character [{}] could not pay currency [{}] to recover [{}]; nothing changed", CharacterID(), currency, ItemGUIDText(guid));
		return;
	}
	PutLootInInventory(slot, *inst);
	recovery.Commit();

	LogInventory("Character [{}] recovered [{}] item [{}] into slot [{}]", CharacterID(), ItemGUIDText(guid), inst->GetID(), slot);
	guid = ItemGUID{};
	SendRecoveryListingRemoved(in->npc_id, in->listing_slot);
}

void Client::SendRecoveryListingRemoved(uint32 merchant_id, uint32 listing_slot)
{
	auto  outapp = new EQApplicationPacket(OP_ShopDelItem, sizeof(Merchant_DelItem_Struct));
	auto *del    = reinterpret_cast<Merchant_DelItem_Struct *>(outapp->pBuffer);
	del->npcid    = merchant_id;
	del->playerid = GetID();
	del->itemslot = listing_slot;
	QueuePacket(outapp);
	safe_delete(outapp);
}
