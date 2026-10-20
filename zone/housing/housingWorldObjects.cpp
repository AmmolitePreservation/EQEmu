#include "housingWorldObjects.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingInteriors.h"
#include "housingItemTail.h"
#include "housingNpcItems.h"

#include "../../common/eqemu_logsys.h"
#include "../client.h"
#include "../entity.h"
#include "../zonedb.h"

#include <stdexcept>

extern EntityList entity_list;

namespace housing {
  namespace {
    constexpr size_t referenceGuidLength = 0x14;

    bool isRoF2(Client *client) {
      return client->ClientVersion() == EQ::versions::ClientVersion::RoF2;
    }

    const EQ::ItemHousingTail &tailOf(uint32 itemId) {
      const EQ::ItemHousingTail *tail = itemHousingTail(itemId);
      if (!tail) {
        throw std::runtime_error(fmt::format("item [{}] is not placeable", itemId));
      }
      return *tail;
    }

    void sendToZone(EmuOpcode opcode, const WireWriter &message) {
      for (const auto &[id, client] : entity_list.GetClientList()) {
        if (isRoF2(client) && session(client)) {
          sendMessage(client, opcode, message);
        }
      }
    }
  }

  MountTilts mountTilts(const EQ::ItemInstance &instance) {
    const EQ::ItemData *item = instance.GetItem();
    const EQ::ItemHousingTail &tail = tailOf(item->ID);
    std::vector<uint32> augmentSlots;
    if (item->IsClassCommon()) {
      for (int socket = EQ::invaug::SOCKET_BEGIN; socket <= EQ::invaug::SOCKET_END; socket++) {
        const EQ::ItemInstance *augment = instance.GetAugment(socket);
        if (augment && augment->GetItem()) {
          augmentSlots.push_back(augment->GetItem()->Slots);
        }
      }
    }
    return heldItemTilts(tail.placeableDefaultPitch, tail.placeableDefaultRoll, effectiveSlots(item->Slots, item->IsClassCommon(), augmentSlots));
  }

  PlacementObject placementObject(const EQ::ItemInstance &instance, const std::string &guid, uint32 propertyId, uint32 recordId, const ObjectTransform &transform) {
    const EQ::ItemData *item = instance.GetItem();
    const EQ::ItemHousingTail &tail = tailOf(item->ID);
    PlacementObject object{};
    const NpcItem *npcItem = isNpcItemId(item->ID) ? catalog().npcItem(item->ID) : nullptr;
    if (isNpcItemId(item->ID) && !npcItem) {
      throw std::runtime_error(fmt::format("NPC item [{}] has no real_estate_npc_items row", item->ID));
    }
    // RoF2 spawns an NPC item's placement preview from the race its first three characters name (0x504b60); Live's IT63 names none.
    object.model = npcItem ? npcItem->raceTag : worldObjectModel(item->IDFile, instance.GetOrnamentationIDFile());
    object.npcItem = npcItem != nullptr;
    object.npcSize = npcItem ? content_db.LoadNPCTypesData(npcItem->npcTypeId)->size : 0.0f;
    object.itemId = item->ID;
    object.guid = guid;
    object.propertyId = propertyId;
    object.recordId = recordId;
    object.transform = transform;
    object.ignoreCollisions = npcItem || tail.placeableIgnoreCollisions;
    object.disablePlacementRotation = tail.disablePlacementRotation;
    object.disableFreePlacement = tail.disableFreePlacement;
    object.placementType = tail.placementType;
    object.scaleMinimum = tail.placeableScaleMin;
    object.scaleMaximum = tail.placeableScaleMax;
    object.scaleDefault = tail.placeableDefaultScale;
    object.headingOffset = tail.placeableDefaultHeading;
    object.tilts = mountTilts(instance);
    object.lightType = static_cast<uint32>(static_cast<uint8>(item->Light));
    return object;
  }

  ObjectTransform rowTransform(const PlacedItem &row) {
    return ObjectTransform{row.scale, row.heading, row.pitch, row.roll, row.x, row.y, row.z};
  }

  void writePlacementObject(WireWriter &message, const PlacementObject &object) {
    message.cString(object.model)
      .u8(object.npcItem)
      .f32(object.npcSize)
      .u32(object.itemId)
      .cString(object.guid)
      .u32(object.propertyId)
      .u32(object.recordId)
      .f32(object.transform.scale)
      .f32(object.transform.heading)
      .f32(object.transform.pitch)
      .f32(object.transform.roll)
      .f32(object.transform.y)
      .f32(object.transform.x)
      .f32(object.transform.z)
      .u8(object.ignoreCollisions)
      .u8(object.disablePlacementRotation)
      .u8(object.disableFreePlacement)
      .u32(object.placementType)
      .f32(object.scaleMinimum)
      .f32(object.scaleMaximum)
      .f32(object.scaleDefault)
      .f32(object.headingOffset)
      .f32(object.tilts.pitch)
      .f32(object.tilts.roll)
      .u32(object.lightType);
  }

  bool standsOutdoors(const Property &property) {
    switch (typeOf(property)) {
      case DefinitionType::regularZone:
      case DefinitionType::plot:
      case DefinitionType::neighborhood:
      case DefinitionType::town:
      case DefinitionType::guildPlot:
        return true;
      default:
        return false;
    }
  }

  bool shownHere(const Property &property, const Session &state) {
    return property.id == state.currentPropertyId || (standsOutdoors(property) && state.hostedIds.count(property.id));
  }

  std::set<uint32> propertiesShownInZone() {
    std::set<uint32> shown;
    const auto root = hostedRoot();
    for (const auto &property : hostedProperties()) {
      if (standsOutdoors(property) || (root && property.id == root->id)) {
        shown.insert(property.id);
      }
    }
    return shown;
  }

  WireWriter itemReference(const std::string &guid, const ItemLocation &location, uint32 propertyId, uint32 recordId) {
    WireWriter message;
    message.fixedString(guid, referenceGuidLength)
      .u32(location.type)
      .u16(static_cast<uint16>(location.slot))
      .u16(static_cast<uint16>(location.subIndex))
      .u16(static_cast<uint16>(location.augIndex))
      .u16(0xFFFF)
      .u32(propertyId)
      .u32(recordId)
      .u32(0);
    return message;
  }

  void sendWorldObjects(Client *client, const Session &state) {
    std::set<uint32> shown;
    for (const auto &[id, property] : state.properties) {
      if (shownHere(property, state)) {
        shown.insert(id);
      }
    }
    size_t sent = 0;
    for (const auto &row : itemsOf(shown)) {
      if (!row.placed || isNpcItemId(row.item_id)) {
        continue;
      }
      auto instance = itemInstance(row);
      if (!instance) {
        LogError("Housing: property [{}] record [{}] holds item [{}], which does not exist; no world object", row.property_id, row.id, row.item_id);
        continue;
      }
      WireWriter message;
      writePlacementObject(message, placementObject(*instance, itemGuid(row), row.property_id, row.id, rowTransform(row)));
      sendMessage(client, OP_RealEstateWorldObject, message);
      sent++;
    }
    LogHousing("Sent [{}] world objects to [{}]", sent, client->GetCleanName());
  }

  void broadcastWorldObject(const PlacedItem &row) {
    auto instance = itemInstance(row);
    if (!instance) {
      throw std::runtime_error(fmt::format("record [{}] holds item [{}], which does not exist", row.id, row.item_id));
    }
    WireWriter message;
    writePlacementObject(message, placementObject(*instance, itemGuid(row), row.property_id, row.id, rowTransform(row)));
    sendToZone(OP_RealEstateWorldObject, message);
  }

  void broadcastWorldObjectRemove(const PlacedItem &row) {
    sendToZone(OP_RealEstateWorldObjectRemove, itemReference(itemGuid(row), ItemLocation{}, row.property_id, row.id));
  }

  void sendPlacedItemMoved(Client *client, const PlacedItem &row, const ItemLocation &location) {
    sendMessage(client, OP_RealEstateWorldObjectMoved, itemReference(itemGuid(row), location, row.property_id, row.id));
  }
}
