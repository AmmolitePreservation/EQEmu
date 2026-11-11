#include "housingItems.h"

#include "housingStore.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/item_layer.h"
#include "../../common/patches/rof2.h"
#include "../zonedb.h"

namespace housing {
  namespace {
    bool writeItemEntry(WireWriter &message, const PlacedItem &row, const EQ::ItemInstance &instance, const std::map<uint32, CharacterIdentity> &owners) {
      const CharacterIdentity *owner = nullptr;
      if (row.owner_character_id) {
        auto found = owners.find(row.owner_character_id);
        if (found == owners.end()) {
          LogError("Housing: property [{}] record [{}] was placed by character [{}], which does not exist; not sent", row.property_id, row.id, row.owner_character_id);
          return false;
        }
        owner = &found->second;
      }
      message.u32(row.property_id)
        .u32(row.id)
        .u8(row.placed)
        .u32(row.upkeep_expired_at)
        .f32(row.heading)
        .f32(row.pitch)
        .f32(row.roll)
        .f32(row.scale)
        .f32(row.y)
        .f32(row.x)
        .f32(row.z)
        .lengthString(owner ? owner->name : "")
        .lengthString(owner ? std::to_string(owner->accountId) : "");
      const std::string serialized = RoF2::SerializeItemForHousing(&instance, EQ::invslot::SLOT_INVALID, ItemPacketViewLink);
      message.bytes(serialized.data(), serialized.size());
      return true;
    }
  }

  std::vector<PlacedItem> itemsOf(const std::set<uint32> &propertyIds) {
    if (propertyIds.empty()) {
      return {};
    }
    return RealEstateItemsRepository::GetWhere(database, fmt::format("property_id IN ({}) ORDER BY property_id, id", idList(propertyIds)));
  }

  HeldInstances itemInstancesOf(const std::set<uint32> &propertyIds) {
    HeldInstances instances;
    if (propertyIds.empty()) {
      return instances;
    }
    for (const auto &held : ItemLayer::Load(database, database, fmt::format("holder_type = 'real_estate' AND holder_id IN ({})", idList(propertyIds)))) {
      instances[ItemGUIDText(held.inst->GetGUID())].reset(held.inst);
    }
    return instances;
  }

  std::unique_ptr<EQ::ItemInstance> itemInstance(const PlacedItem &row) {
    return std::unique_ptr<EQ::ItemInstance>(ItemLayer::LoadOne(database, database, row.guid));
  }

  WireWriter itemList(uint32 propertyId, const std::vector<PlacedItem> &rows, const HeldInstances &instances, const std::map<uint32, CharacterIdentity> &owners) {
    WireWriter entries;
    uint32 count = 0;
    for (const auto &row : rows) {
      if (row.property_id != propertyId) {
        continue;
      }
      auto instance = instances.find(ItemGUIDText(row.guid));
      if (instance == instances.end()) {
        LogError("Housing: property [{}] record [{}] holds item [{}], which did not load; not sent", row.property_id, row.id, row.item_id);
        continue;
      }
      if (writeItemEntry(entries, row, *instance->second, owners)) {
        count++;
      }
    }
    WireWriter message;
    message.u32(propertyId).u32(count).bytes(entries.content().data(), entries.content().size());
    return message;
  }

  std::optional<PlacedItem> placedItem(uint32 propertyId, uint32 recordId) {
    auto rows = RealEstateItemsRepository::GetWhere(database, fmt::format("id = {} AND property_id = {}", recordId, propertyId));
    if (rows.empty()) {
      return std::nullopt;
    }
    return rows.front();
  }
}
