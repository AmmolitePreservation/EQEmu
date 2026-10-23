#ifndef EQEMU_ZONE_HOUSING_HOUSING_H
#define EQEMU_ZONE_HOUSING_HOUSING_H

#include "../../common/types.h"

#include <vector>

class Client;
class Doors;
class EQApplicationPacket;
class Mob;
class NPC;
class Seperator;
class ServerPacket;

namespace EQ {
  class ItemInstance;
}

namespace housing {
  // Next to client.h's POPUPID_* ids, which end at 1000001.
  constexpr uint32 geodePopupId = 1000002;

  bool onZoneBoot();
  void onZoneShutdown();
  void process();
  void handlePacket(Client *client, const EQApplicationPacket *app);
  void onZoneContents(Client *client);
  void onZoneIn(Client *client);
  void onDisconnect(Client *client);
  uint32 hostedCopyPropertyId();
  void handleServerPacket(ServerPacket *pack);
  void handleFindRequest(Client *client, uint32 type, int32 propertyId, int32 recordId);
  void handleContainerOpened(Client *client, const EQApplicationPacket *app);
  bool handleDoorClick(Client *client, Doors *door);
  void handleAnchorEffect(Mob *caster, Mob *target, uint16 spellEffect, uint16 spellId, int32 anchorItemId);
  void handleItemCastFinished(Client *client, uint16 spellId, uint32 inventorySlot);
  uint32 guildHallGuildId();
  bool refuseGuildBank(Client *client);
  bool questOpenPurchase(Client *client, uint32 propertyId);
  bool questEnter(Client *client, uint32 propertyId);
  void questLeave(Client *client);
  bool questUseNeighborhoodWrit(Client *client, uint32 definitionId);
  bool questHasPermission(Client *client, uint32 propertyId, int permission);
  void handleCommand(Client *client, const Seperator *sep);
}

#endif
