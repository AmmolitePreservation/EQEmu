#ifndef EQEMU_WORLD_HOUSING_NEIGHBORHOODS_H
#define EQEMU_WORLD_HOUSING_NEIGHBORHOODS_H

#include "../common/types.h"

class ServerPacket;

namespace housing {
  void balanceNeighborhoods();
  uint32 createNeighborhood(uint32 definitionId, uint32 guildId);
  void onHousingEvent(const ServerPacket *pack);
}

#endif
