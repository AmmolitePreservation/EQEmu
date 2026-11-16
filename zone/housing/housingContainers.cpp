#include "housingContainers.h"

#include "housing.h"

#include "../../common/eq_packet_structs.h"
#include "../../common/eqemu_logsys.h"
#include "../../common/item_layer.h"
#include "../../common/repositories/item_instances_repository.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../entity.h"
#include "../object.h"
#include "../zonedb.h"

#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>

extern EntityList entity_list;

namespace housing {
  namespace {
    constexpr uint8 worldLocationType = 4;

    struct OpenContainer {
      std::unique_ptr<Object> object;
      uint32 recordId;
      ItemGUID guid;
    };

    // At most one per player: the client refuses a second world container while one is open (0x4c19a0).
    std::map<Client *, OpenContainer> open;

    bool isStorageBag(uint8 bagType) {
      switch (bagType) {
        case EQ::item::BagTypeSmallBag:
        case EQ::item::BagTypeLargeBag:
        case EQ::item::BagTypeQuiver:
        case EQ::item::BagTypeBeltPouch:
        case EQ::item::BagTypeWristPouch:
        case EQ::item::BagTypeBackPack:
        case EQ::item::BagTypeSmallChest:
        case EQ::item::BagTypeLargeChest:
        case EQ::item::BagTypeBandolier:
        case EQ::item::BagTypeUnknown1:
        case EQ::item::BagTypeTradersSatchel:
        case EQ::item::BagTypeTradeskillBag:
        case EQ::item::BagTypeCollectibleBag:
          return true;
        default:
          return false;
      }
    }

    void forget(std::map<Client *, OpenContainer>::iterator entry) {
      Client *client = entry->first;
      Object *object = entry->second.object.get();
      if (client->GetTradeskillObject() == object) {
        object->Close();
      }
      object->ClearUser();
      LogHousing("Closed [{}]'s housing container, record [{}]", client->GetCleanName(), entry->second.recordId);
      open.erase(entry);
    }
  }

  bool isTradeskillContainer(uint32 itemId) {
    const EQ::ItemData *item = database.GetItem(itemId);
    return item && item->IsClassBag() && !isStorageBag(item->BagType);
  }

  void queueOpen(Client *client, const PlacedItem &row, const EQ::ItemData *item) {
    ClickObjectActionHousing_Struct housing{};
    housing.action.player_id = client->GetID();
    housing.action.drop_id = 0;
    housing.action.open = 1;
    housing.action.type = item->BagType;
    housing.action.icon = item->Icon;
    strn0cpy(housing.action.object_name, item->Name, sizeof(housing.action.object_name));
    housing.property_id = static_cast<int32>(row.property_id);
    housing.record_id = static_cast<int32>(row.id);
    EQApplicationPacket packet(OP_ClickObjectAction, sizeof(housing));
    memcpy(packet.pBuffer, &housing, sizeof(housing));
    client->QueuePacket(&packet);
  }

  // A transient Object holds the container item while it is open, so combines run through EQEmu's own code. What is put in is
  // saved as the placed item's contents, so a crash leaves it there for the next open (serverPlan.md, housing tradeskill containers).
  void openContainer(Client *client, const PlacedItem &row) {
    auto instance = itemInstance(row);
    if (!instance || !isTradeskillContainer(row.item_id)) {
      throw std::runtime_error(fmt::format("record [{}] holds item [{}], which is not a tradeskill container", row.id, row.item_id));
    }
    const EQ::ItemData *item = instance->GetItem();

    // An entry the client closed itself, which processContainers has not cleared yet.
    auto found = open.find(client);
    if (found != open.end() && client->GetTradeskillObject() != found->second.object.get()) {
      forget(found);
    }
    // The client refuses a second world container itself (0x4c19a0, eqstr 1443) and no server message closes the open
    // one, so the open goes out for the client to answer and nothing here changes.
    if (client->GetTradeskillObject()) {
      LogHousing("[{}] already has a world container open; record [{}] stays closed", client->GetCleanName(), row.id);
      queueOpen(client, row, item);
      return;
    }

    Object_Struct data{};
    data.x = row.x;
    data.y = row.y;
    data.z = row.z;
    data.heading = row.heading;
    data.size = row.scale;
    auto object = std::make_unique<Object>(0, item->BagType, item->Icon, data, instance.get(), true);
    object->SetTransient(true);
    object->SetUser(client);
    client->SetTradeskillObject(object.get());

    queueOpen(client, row, item);
    object->SendContents(client);

    open[client] = OpenContainer{std::move(object), row.id, row.guid};
    LogHousing("[{}] opened housing container record [{}] (item [{}])", client->GetCleanName(), row.id, row.item_id);
  }

  void handleContainerOpened(Client *client, const EQApplicationPacket *app) {
    auto found = open.find(client);
    if (found == open.end()) {
      return;
    }
    uint32 locationType;
    memcpy(&locationType, app->pBuffer, sizeof(locationType));
    if ((locationType & 0xFFFF) != worldLocationType) {
      return;
    }
    LogHousing("[{}]'s client opened housing container record [{}]", client->GetCleanName(), found->second.recordId);
  }

  // A deleted owner's reclaim retires a placed container with what lies in it before this zone hears of it (RCL-6). What
  // the character using it has in it goes back to them at the close, as at every other close (Q28): put back in play here,
  // then returned by Object::Close like any content.
  void returnReclaimedContents(Client *user, Object *object) {
    auto found = open.find(user);
    if (found == open.end() || found->second.object.get() != object) {
      return;
    }
    const auto container = ItemInstancesRepository::FindOne(database, found->second.guid);
    if (!container || container->retirement_state != ItemRetirement::CharacterDeleted) {
      return;
    }
    for (uint8 index = EQ::invbag::SLOT_BEGIN; index <= EQ::invbag::SLOT_END; ++index) {
      const EQ::ItemInstance *content = object->GetItem(index);
      if (!content || !content->HasGUID()) {
        continue;
      }
      if (!ItemLayer::Unretire(database, *content, ItemRetirement::CharacterDeleted)) {
        LogError("Housing: [{}]'s [{}] in housing container record [{}] could not be put back in play; its return is refused", user->GetCleanName(), ItemGUIDText(content->GetGUID()), found->second.recordId);
        continue;
      }
      LogHousing("[{}] gets back [{}] (item [{}]) from housing container record [{}], retired with it by a deleted owner's reclaim", user->GetCleanName(), ItemGUIDText(content->GetGUID()), content->GetID(), found->second.recordId);
    }
  }

  void closeContainersOf(uint32 recordId) {
    for (auto entry = open.begin(); entry != open.end();) {
      auto current = entry++;
      if (current->second.recordId == recordId) {
        forget(current);
      }
    }
  }

  void closeContainerOf(Client *client) {
    auto found = open.find(client);
    if (found != open.end()) {
      forget(found);
    }
  }

  // The client closes a housing container itself (out of range, or the item gone) and tells the server with an empty
  // OP_ClickObjectAction, which closes EQEmu's tradeskill object; the object goes once the player no longer uses it.
  void processContainers() {
    for (auto entry = open.begin(); entry != open.end();) {
      auto current = entry++;
      if (current->first->GetTradeskillObject() != current->second.object.get()) {
        forget(current);
      }
    }
  }
}
