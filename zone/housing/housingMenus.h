#ifndef EQEMU_ZONE_HOUSING_HOUSING_MENUS_H
#define EQEMU_ZONE_HOUSING_HOUSING_MENUS_H

#include "housingItems.h"
#include "housingSession.h"
#include "housingWire.h"

#include <vector>

class Client;

namespace housing {
  struct MenuEntry {
    uint32 type;
    uint32 b;
    uint32 label;
    uint32 submenu;
  };

  // What a menu is built from: the viewer's view of one placed or stored item.
  struct MenuSubject {
    const Session &state;
    const Property &holder;
    const PlacedItem &item;
    const Property *carried;
    bool inItemZone;
    bool plotExpired;
  };

  struct MenuInteractions {
    std::vector<MenuEntry> entries;
    uint32 npcSpawnId = 0;
  };

  void handleMenuRequest(Client *client, WireReader &reader);
  void handleInteract(Client *client, WireReader &reader);
  void handleInspect(Client *client, WireReader &reader);

  void addNpcItemInteractions(Client *client, const MenuSubject &subject, MenuInteractions &interactions);
  void addContainerInteractions(Client *client, const MenuSubject &subject, MenuInteractions &interactions);
  void addGuildServiceInteractions(Client *client, const MenuSubject &subject, MenuInteractions &interactions);
}

#endif
