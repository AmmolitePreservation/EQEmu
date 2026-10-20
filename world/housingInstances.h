#ifndef EQEMU_WORLD_HOUSING_INSTANCES_H
#define EQEMU_WORLD_HOUSING_INSTANCES_H

#include "../common/types.h"

#include <string>

namespace housing {
  void startHousingInstances();
  void processHousingInstances();
  uint32 createPropertyTree(uint32 definitionId, uint32 guildId, const std::string &name);
  bool reenterDiscardedCopy(uint32 characterId, uint32 zoneId, uint32 &instanceId);
}

#endif
