#include "housingItems.h"

#include "housingStore.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/patches/rof2.h"
#include "../zonedb.h"

#include <limits>

namespace housing {
  std::vector<PlacedItem> itemsOf(const std::set<uint32> &propertyIds) {
    if (propertyIds.empty()) {
      return {};
    }
    return RealEstateItemsRepository::GetWhere(database, fmt::format("property_id IN ({}) ORDER BY property_id, id", idList(propertyIds)));
  }

  // EQEmu serials are 16 decimal digits, so a leading hex letter keeps a row's GUID apart from every serial.
  std::string itemGuid(const PlacedItem &row) {
    return fmt::format("a{:015x}", row.id);
  }

  std::unique_ptr<EQ::ItemInstance> itemInstance(const PlacedItem &row) {
    const EQ::ItemData *item = database.GetItem(row.item_id);
    if (!item) {
      return nullptr;
    }
    std::unique_ptr<EQ::ItemInstance> instance(database.CreateBaseItem(item, row.charges));
    if (!instance) {
      return nullptr;
    }
    if (!row.custom_data.empty()) {
      instance->SetCustomDataString(row.custom_data);
    }
    instance->SetOrnamentIcon(row.ornament_icon);
    instance->SetOrnamentationIDFile(row.ornament_idfile);
    instance->SetOrnamentHeroModel(item->HerosForgeModel);
    if (row.instnodrop) {
      instance->SetAttuned(true);
    }
    if (row.color > 0) {
      instance->SetColor(row.color);
    }
    if (row.charges == std::numeric_limits<int16>::max()) {
      instance->SetCharges(-1);
    }
    else if (row.charges == 0 && instance->IsStackable()) {
      instance->SetCharges(1);
    }
    else {
      instance->SetCharges(row.charges);
    }
    if (item->IsClassCommon()) {
      const uint32 augments[EQ::invaug::SOCKET_COUNT] = {
        row.augment_one, row.augment_two, row.augment_three, row.augment_four, row.augment_five, row.augment_six
      };
      for (int socket = EQ::invaug::SOCKET_BEGIN; socket <= EQ::invaug::SOCKET_END; socket++) {
        if (augments[socket]) {
          instance->PutAugment(&database, socket, augments[socket]);
        }
      }
    }
    return instance;
  }

  bool writeItemEntry(WireWriter &message, const PlacedItem &row, const std::map<uint32, CharacterIdentity> &owners) {
    auto instance = itemInstance(row);
    if (!instance) {
      LogError("Housing: property [{}] record [{}] holds item [{}], which does not exist; not sent", row.property_id, row.id, row.item_id);
      return false;
    }
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
    const std::string serialized = RoF2::SerializeItemForHousing(instance.get(), itemGuid(row), EQ::invslot::SLOT_INVALID, ItemPacketViewLink);
    message.bytes(serialized.data(), serialized.size());
    return true;
  }

  WireWriter itemList(uint32 propertyId, const std::vector<PlacedItem> &rows, const std::map<uint32, CharacterIdentity> &owners) {
    WireWriter entries;
    uint32 count = 0;
    for (const auto &row : rows) {
      if (row.property_id != propertyId) {
        continue;
      }
      if (writeItemEntry(entries, row, owners)) {
        count++;
      }
    }
    WireWriter message;
    message.u32(propertyId).u32(count).bytes(entries.content().data(), entries.content().size());
    return message;
  }
}
