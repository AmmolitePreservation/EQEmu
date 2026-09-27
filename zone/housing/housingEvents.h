#ifndef EQEMU_ZONE_HOUSING_HOUSING_EVENTS_H
#define EQEMU_ZONE_HOUSING_HOUSING_EVENTS_H

#include "../../common/types.h"

namespace housing {
  enum class EventType : uint32 {
    catalogChanged = 1
  };

  enum class EventTarget : uint32 {
    allZones = 5
  };

  void sendEvent(EventType type, EventTarget target, uint32 targetId);
}

#endif
