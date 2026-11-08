#include "../common/features.h"
#include "client.h"

#ifdef EMBPERL_XS_CLASSES

#include "../common/global_define.h"
#include "embperl.h"

bool Perl_Inventory_CanItemFitInContainer(EQ::InventoryProfile* self, EQ::ItemInstance* item_to_check, EQ::ItemInstance* container_to_check)
{
	const EQ::ItemData* item_data = item_to_check->GetItem();
	const EQ::ItemData* container_data = container_to_check->GetItem();
	return self->CanItemFitInContainer(item_data, container_data);
}

bool Perl_Inventory_CheckNoDrop(EQ::InventoryProfile* self, int16_t slot_id)
{
	return self->CheckNoDrop(slot_id);
}

// The profile's owner, whose landing, retire and swap functions write the rows.
static Client *OwnerOf(EQ::InventoryProfile *inventory, const char *action)
{
	auto *c = entity_list.GetClientByInventory(inventory);
	if (!c) {
		LogError("Inventory {} from a script on an inventory no client in this zone owns; nothing changed", action);
	}
	return c;
}

bool Perl_Inventory_DeleteItem(EQ::InventoryProfile* self, int16_t slot_id, uint8_t quantity)
{
	auto *c = OwnerOf(self, "DeleteItem");
	if (!c || !self->GetItem(slot_id)) {
		return false;
	}

	c->DeleteItemInInventory(slot_id, quantity, true, ItemRetirement::Consumed);
	return true;
}

bool Perl_Inventory_DeleteItem(EQ::InventoryProfile* self, int16_t slot_id)
{
	return Perl_Inventory_DeleteItem(self, slot_id, 0);
}
int Perl_Inventory_FindFreeSlot(EQ::InventoryProfile* self, bool is_for_bag, bool try_cursor)
{
	return self->FindFreeSlot(is_for_bag, try_cursor);
}

int Perl_Inventory_FindFreeSlot(EQ::InventoryProfile* self, bool is_for_bag, bool try_cursor, uint8_t min_size)
{
	return self->FindFreeSlot(is_for_bag, try_cursor, min_size);
}

int Perl_Inventory_FindFreeSlot(EQ::InventoryProfile* self, bool is_for_bag, bool try_cursor, uint8_t min_size, bool is_arrow)
{
	return self->FindFreeSlot(is_for_bag, try_cursor, min_size, is_arrow);
}

int Perl_Inventory_GetBagIndex(EQ::InventoryProfile* self, int16_t slot_id)
{
	return self->CalcBagIdx(slot_id);
}

EQ::ItemInstance* Perl_Inventory_GetItem(EQ::InventoryProfile* self, int16_t slot_id)
{
	return self->GetItem(slot_id);
}

int Perl_Inventory_GetMaterialFromSlot(EQ::InventoryProfile* self, int16_t slot_id)
{
	return self->CalcMaterialFromSlot(slot_id);
}

int Perl_Inventory_GetSlotByItemInst(EQ::InventoryProfile* self, EQ::ItemInstance* item)
{
	return self->GetSlotByItemInst(item);
}

int Perl_Inventory_GetSlotFromMaterial(EQ::InventoryProfile* self, uint8_t material)
{
	return self->CalcSlotFromMaterial(material);
}

int Perl_Inventory_GetSlotID(EQ::InventoryProfile* self, int16_t slot_id)
{
	return self->CalcSlotId(slot_id);
}

int Perl_Inventory_GetSlotID(EQ::InventoryProfile* self, int16_t slot_id, uint8_t bag_index)
{
	return self->CalcSlotId(slot_id, bag_index);
}

int Perl_Inventory_HasItem(EQ::InventoryProfile* self, uint32_t item_id)
{
	return self->HasItem(item_id);
}

int Perl_Inventory_HasItem(EQ::InventoryProfile* self, uint32_t item_id, uint8_t quantity)
{
	return self->HasItem(item_id, quantity);
}

int Perl_Inventory_HasItem(EQ::InventoryProfile* self, uint32_t item_id, uint8_t quantity, uint8_t where_to_look)
{
	return self->HasItem(item_id, quantity, where_to_look);
}

int Perl_Inventory_HasItemByLoreGroup(EQ::InventoryProfile* self, uint32_t loregroup)
{
	return self->HasItemByLoreGroup(loregroup);
}

int Perl_Inventory_HasItemByLoreGroup(EQ::InventoryProfile* self, uint32_t loregroup, uint8_t where_to_look)
{
	return self->HasItemByLoreGroup(loregroup, where_to_look);
}

int Perl_Inventory_HasItemByUse(EQ::InventoryProfile* self, uint8_t item_use, uint8_t quantity)
{
	return self->HasItemByUse(item_use, quantity);
}

int Perl_Inventory_HasItemByUse(EQ::InventoryProfile* self, uint8_t item_use, uint8_t quantity, uint8_t where_to_look)
{
	return self->HasItemByUse(item_use, quantity, where_to_look);
}

bool Perl_Inventory_HasSpaceForItem(EQ::InventoryProfile* self, EQ::ItemInstance* item_to_check, uint8_t quantity)
{
	const EQ::ItemData* item_data = item_to_check->GetItem();
	return self->HasSpaceForItem(item_data, quantity);
}

bool Perl_Inventory_SupportsContainers(EQ::InventoryProfile* self, int16_t slot_id)
{
	return self->SupportsContainers(slot_id);
}

bool Perl_Inventory_SwapItem(EQ::InventoryProfile* self, int16_t source_slot_id, int16_t destination_slot_id)
{
	auto *c = OwnerOf(self, "SwapItem");
	return c && c->SwapSlots(source_slot_id, destination_slot_id);
}

int Perl_Inventory_PushCursor(EQ::InventoryProfile* self, EQ::ItemInstance* item)
{
	auto *c = OwnerOf(self, "PushCursor");
	if (!item || !c) {
		return INVALID_INDEX;
	}

	return c->PushItemOnCursor(*item, true) ? EQ::invslot::slotCursor : INVALID_INDEX;
}

int Perl_Inventory_PutItem(EQ::InventoryProfile* self, int16_t slot_id, EQ::ItemInstance* item)
{
	auto *c = OwnerOf(self, "PutItem");
	if (!item || !c) {
		return INVALID_INDEX;
	}
	if (self->GetItem(slot_id)) {
		LogError("Inventory PutItem from a script into slot [{}] of character [{}], which holds an item; nothing changed", slot_id, c->CharacterID());
		return INVALID_INDEX;
	}

	return c->PutItemInInventory(slot_id, *item, true) ? slot_id : INVALID_INDEX;
}
bool Perl_Inventory_HasAugmentEquippedByID(EQ::InventoryProfile* self, uint32_t item_id)
{
	return self->HasAugmentEquippedByID(item_id);
}

uint32 Perl_Inventory_CountAugmentEquippedByID(EQ::InventoryProfile* self, uint32_t item_id)
{
	return self->CountAugmentEquippedByID(item_id);
}

bool Perl_Inventory_HasItemEquippedByID(EQ::InventoryProfile* self, uint32_t item_id)
{
	return self->HasItemEquippedByID(item_id);
}

uint32 Perl_Inventory_CountItemEquippedByID(EQ::InventoryProfile* self, uint32_t item_id)
{
	return self->CountItemEquippedByID(item_id);
}

perl::array Perl_Inventory_GetAugmentIDsBySlotID(EQ::InventoryProfile* self, int16 slot_id)
{
	perl::array result;
	auto augments = self->GetAugmentIDsBySlotID(slot_id);

	for (int i = 0; i < augments.size(); ++i) {
		result.push_back(augments[i]);
	}

	return result;
}

void perl_register_inventory()
{
	perl::interpreter perl(PERL_GET_THX);

	auto package = perl.new_class<EQ::InventoryProfile>("Inventory");
	package.add("CanItemFitInContainer", &Perl_Inventory_CanItemFitInContainer);
	package.add("CountAugmentEquippedByID", &Perl_Inventory_CountAugmentEquippedByID);
	package.add("CountItemEquippedByID", &Perl_Inventory_CountItemEquippedByID);
	package.add("CheckNoDrop", &Perl_Inventory_CheckNoDrop);
	package.add("DeleteItem", (bool(*)(EQ::InventoryProfile*, int16_t))&Perl_Inventory_DeleteItem);
	package.add("DeleteItem", (bool(*)(EQ::InventoryProfile*, int16_t, uint8_t))&Perl_Inventory_DeleteItem);
	package.add("FindFreeSlot", (int(*)(EQ::InventoryProfile*, bool, bool))&Perl_Inventory_FindFreeSlot);
	package.add("FindFreeSlot", (int(*)(EQ::InventoryProfile*, bool, bool, uint8_t))&Perl_Inventory_FindFreeSlot);
	package.add("FindFreeSlot", (int(*)(EQ::InventoryProfile*, bool, bool, uint8_t, bool))&Perl_Inventory_FindFreeSlot);
	package.add("GetAugmentIDsBySlotID", &Perl_Inventory_GetAugmentIDsBySlotID);
	package.add("GetBagIndex", &Perl_Inventory_GetBagIndex);
	package.add("GetItem", &Perl_Inventory_GetItem);
	package.add("GetMaterialFromSlot", &Perl_Inventory_GetMaterialFromSlot);
	package.add("GetSlotByItemInst", &Perl_Inventory_GetSlotByItemInst);
	package.add("GetSlotFromMaterial", &Perl_Inventory_GetSlotFromMaterial);
	package.add("GetSlotID", (int(*)(EQ::InventoryProfile*, int16_t))&Perl_Inventory_GetSlotID);
	package.add("GetSlotID", (int(*)(EQ::InventoryProfile*, int16_t, uint8_t))&Perl_Inventory_GetSlotID);
	package.add("HasAugmentEquippedByID", &Perl_Inventory_HasAugmentEquippedByID);
	package.add("HasItem", (int(*)(EQ::InventoryProfile*, uint32_t))&Perl_Inventory_HasItem);
	package.add("HasItem", (int(*)(EQ::InventoryProfile*, uint32_t, uint8_t))&Perl_Inventory_HasItem);
	package.add("HasItem", (int(*)(EQ::InventoryProfile*, uint32_t, uint8_t, uint8_t))&Perl_Inventory_HasItem);
	package.add("HasItemByLoreGroup", (int(*)(EQ::InventoryProfile*, uint32_t))&Perl_Inventory_HasItemByLoreGroup);
	package.add("HasItemByLoreGroup", (int(*)(EQ::InventoryProfile*, uint32_t, uint8_t))&Perl_Inventory_HasItemByLoreGroup);
	package.add("HasItemByUse", (int(*)(EQ::InventoryProfile*, uint8_t, uint8_t))&Perl_Inventory_HasItemByUse);
	package.add("HasItemByUse", (int(*)(EQ::InventoryProfile*, uint8_t, uint8_t, uint8_t))&Perl_Inventory_HasItemByUse);
	package.add("HasItemEquippedByID", &Perl_Inventory_HasItemEquippedByID);
	package.add("HasSpaceForItem", &Perl_Inventory_HasSpaceForItem);
	package.add("PushCursor", &Perl_Inventory_PushCursor);
	package.add("PutItem", &Perl_Inventory_PutItem);
	package.add("SupportsContainers", &Perl_Inventory_SupportsContainers);
	package.add("SwapItem", &Perl_Inventory_SwapItem);
}

#endif //EMBPERL_XS_CLASSES
