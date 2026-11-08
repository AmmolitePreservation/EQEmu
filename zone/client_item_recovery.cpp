#include "client.h"
#include "item_recovery.h"
#include "../common/item_layer.h"

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
