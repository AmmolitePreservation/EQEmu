#ifndef EQEMU_ZONE_HOUSING_HOUSING_H
#define EQEMU_ZONE_HOUSING_HOUSING_H

#include "../../common/types.h"

#include <vector>

class Client;
class Doors;
class EQApplicationPacket;
class Mob;
class Seperator;
class ServerPacket;

namespace housing {
  bool onZoneBoot();
  void process();
  void handlePacket(Client *client, const EQApplicationPacket *app);
  void onZoneContents(Client *client);
  void onZoneIn(Client *client);
  void onDisconnect(Client *client);
  void handleServerPacket(ServerPacket *pack);
  void handleFindRequest(Client *client, uint32 type, int32 propertyId, int32 recordId);
  bool handleDoorClick(Client *client, Doors *door);
  void handleAnchorEffect(Mob *caster, Mob *target, uint16 spellEffect);
  std::vector<uint32> trophyBenefitItems(Client *client);
  void handleCommand(Client *client, const Seperator *sep);
}

#endif
