#ifndef EQEMU_ZONE_HOUSING_HOUSING_GUILD_SERVICES_H
#define EQEMU_ZONE_HOUSING_HOUSING_GUILD_SERVICES_H

#include "housingItems.h"
#include "housingSession.h"

#include <vector>

class Client;
class NPC;

namespace EQ {
  class ItemInstance;
}

namespace housing {
  constexpr uint32 activateAuraAction = 1;
  constexpr uint32 deactivateAuraAction = 2;

  bool isAuraAction(uint32 b);
  void toggleAura(Client *client, const Session &state, const Property &holder, const PlacedItem &row, uint32 b);
  void processGuildServices();
  void resetGuildServices();
  void stripAuraBuffs(Client *client);
}

#endif
