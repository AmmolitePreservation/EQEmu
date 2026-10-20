#ifndef EQEMU_ZONE_HOUSING_HOUSING_EVENTS_H
#define EQEMU_ZONE_HOUSING_HOUSING_EVENTS_H

#include "../../common/housingEventTypes.h"

#include <string>

namespace housing {
  void sendEvent(EventType type, EventTarget target, uint32 targetId, uint32 propertyId, const std::string &payload);
  void sendPropertyChanged(uint32 propertyId, const PropertyChangedPayload &owners);
}

#endif
