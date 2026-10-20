#include "housingContainers.h"

#include "housing.h"

#include "../../common/eq_packet_structs.h"
#include "../../common/eqemu_logsys.h"
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
      bool confirmed = false;
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

  // A transient Object holds the container item while it is open, so combines run through EQEmu's own code and nothing in it
  // persists (serverPlan.md, housing tradeskill containers).
  void openContainer(Client *client, const PlacedItem &row) {
    auto found = open.find(client);
    if (found != open.end()) {
      forget(found);
    }
    if (client->GetTradeskillObject()) {
      LogHousing("[{}] already has a world container open; record [{}] stays closed", client->GetCleanName(), row.id);
      return;
    }
    auto instance = itemInstance(row);
    if (!instance || !isTradeskillContainer(row.item_id)) {
      throw std::runtime_error(fmt::format("record [{}] holds item [{}], which is not a tradeskill container", row.id, row.item_id));
    }
    const EQ::ItemData *item = instance->GetItem();
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

    open[client] = OpenContainer{std::move(object), row.id};
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
    found->second.confirmed = true;
    LogHousing("[{}]'s client opened housing container record [{}]", client->GetCleanName(), found->second.recordId);
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
