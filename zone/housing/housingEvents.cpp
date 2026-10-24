#include "housingEvents.h"

#include "housing.h"
#include "housingAccess.h"
#include "housingInteriors.h"
#include "housingPlacement.h"
#include "housingPolls.h"
#include "housingSession.h"
#include "housingWrits.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/rulesys.h"
#include "../../common/servertalk.h"
#include "../worldserver.h"
#include "../zone.h"

#include <cstring>
#include <memory>

extern WorldServer worldserver;
extern Zone *zone;

namespace housing {
  void sendEvent(EventType type, EventTarget target, uint32 targetId, uint32 propertyId, const std::string &payload) {
    auto pack = std::make_unique<ServerPacket>(ServerOP_Housing, static_cast<uint32>(sizeof(ServerHousingEvent_Struct) + payload.size()));
    auto *event = reinterpret_cast<ServerHousingEvent_Struct *>(pack->pBuffer);
    event->event_type = static_cast<uint32>(type);
    event->target_kind = static_cast<uint32>(target);
    event->target_id = targetId;
    event->origin_zone_id = zone->GetZoneID();
    event->origin_instance_id = zone->GetInstanceID();
    event->property_id = propertyId;
    event->revision = 0;
    event->payload_size = static_cast<uint32>(payload.size());
    if (!payload.empty()) {
      memcpy(event->payload, payload.data(), payload.size());
    }
    worldserver.SendPacket(pack.get());
  }

  void sendPropertyChanged(uint32 propertyId, const PropertyChangedPayload &owners) {
    sendEvent(EventType::propertyChanged, EventTarget::property, propertyId, propertyId, std::string(reinterpret_cast<const char *>(&owners), sizeof(owners)));
  }

  void sendItemListChanged(uint32 propertyId, const std::vector<ItemListChangedRecord> &records) {
    sendEvent(EventType::itemListChanged, EventTarget::property, propertyId, propertyId, itemListChangedPayload(records));
  }

  void handleServerPacket(ServerPacket *pack) {
    if (pack->size < sizeof(ServerHousingEvent_Struct)) {
      LogError("Housing: an event of [{}] bytes is shorter than its header", pack->size);
      return;
    }
    const auto *event = reinterpret_cast<const ServerHousingEvent_Struct *>(pack->pBuffer);
    if (pack->size != sizeof(ServerHousingEvent_Struct) + event->payload_size) {
      LogError("Housing: event [{}] is [{}] bytes, its header says [{}]", event->event_type, pack->size, sizeof(ServerHousingEvent_Struct) + event->payload_size);
      return;
    }
    switch (static_cast<EventType>(event->event_type)) {
      case EventType::catalogChanged:
        reloadCatalog();
        break;
      case EventType::propertyChanged: {
        if (event->payload_size != sizeof(PropertyChangedPayload)) {
          LogError("Housing: property changed event for [{}] carries [{}] bytes, not [{}]", event->property_id, event->payload_size, sizeof(PropertyChangedPayload));
          return;
        }
        PropertyChangedPayload owners;
        memcpy(&owners, event->payload, sizeof(owners));
        checkHostedRoot(event->property_id);
        refreshClients({event->property_id}, {owners.previousOwnerCharacterId, owners.ownerCharacterId});
        break;
      }
      case EventType::accessChanged:
        applyAccessChange(event->property_id);
        break;
      case EventType::itemListChanged: {
        std::vector<ItemListChangedRecord> records;
        try {
          records = itemListChangedRecords(event->payload, event->payload_size);
        }
        catch (const std::invalid_argument &error) {
          LogError("Housing: item list event for [{}] from zone [{}] instance [{}]: {}", event->property_id, event->origin_zone_id, event->origin_instance_id, error.what());
          return;
        }
        // The origin showed and hid its own rows before it sent the event (finishApplied); every other zone acts on the records now.
        if (event->origin_zone_id != zone->GetZoneID() || event->origin_instance_id != zone->GetInstanceID()) {
          applyItemListChange(records);
        }
        refreshItemLists({event->property_id});
        break;
      }
      case EventType::writUsed:
        break;
      case EventType::pollsChanged:
        refreshPolls();
        break;
      case EventType::guildNeighborhoodCreated: {
        if (event->payload_size != sizeof(GuildNeighborhoodCreatedPayload)) {
          LogError("Housing: guild neighborhood event carries [{}] bytes, not [{}]", event->payload_size, sizeof(GuildNeighborhoodCreatedPayload));
          return;
        }
        GuildNeighborhoodCreatedPayload created;
        memcpy(&created, event->payload, sizeof(created));
        announceGuildNeighborhood(created);
        break;
      }
      default:
        LogError("Housing: unknown event [{}] from zone [{}] instance [{}]", event->event_type, event->origin_zone_id, event->origin_instance_id);
    }
  }
}
