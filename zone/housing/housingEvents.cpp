#include "housingEvents.h"

#include "housing.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/rulesys.h"
#include "../../common/servertalk.h"
#include "../worldserver.h"
#include "../zone.h"

#include <memory>

extern WorldServer worldserver;
extern Zone *zone;

namespace housing {
  void sendEvent(EventType type, EventTarget target, uint32 targetId) {
    auto pack = std::make_unique<ServerPacket>(ServerOP_Housing, sizeof(ServerHousingEvent_Struct));
    auto *event = reinterpret_cast<ServerHousingEvent_Struct *>(pack->pBuffer);
    event->event_type = static_cast<uint32>(type);
    event->target_kind = static_cast<uint32>(target);
    event->target_id = targetId;
    event->origin_zone_id = zone->GetZoneID();
    event->origin_instance_id = zone->GetInstanceID();
    event->property_id = 0;
    event->revision = 0;
    event->payload_size = 0;
    worldserver.SendPacket(pack.get());
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
    if (!RuleB(Housing, EnableHousing)) {
      return;
    }
    switch (static_cast<EventType>(event->event_type)) {
      case EventType::catalogChanged:
        reloadCatalog();
        break;
      default:
        LogError("Housing: unknown event [{}] from zone [{}] instance [{}]", event->event_type, event->origin_zone_id, event->origin_instance_id);
    }
  }
}
