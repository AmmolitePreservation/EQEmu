#ifndef EQEMU_ZONE_HOUSING_HOUSING_PLACEMENT_H
#define EQEMU_ZONE_HOUSING_HOUSING_PLACEMENT_H

#include "housingItems.h"
#include "housingSession.h"
#include "housingWire.h"

#include "../../common/housingPlacementRules.h"

#include <string>

class Client;

namespace housing {
  // The common header of the item requests (0x626e, 0x711a, 0x2036, 0x3424; windows/RealEstateItemsWnd.md, Request structs).
  struct ItemRequest {
    std::string guid;
    ItemLocation location;
    uint32 propertyId;
    uint32 recordId;
    uint32 quantity;
  };

  constexpr size_t itemRequestSize = 0x2c;
  bool readItemRequest(WireReader &reader, ItemRequest &request);

  void handlePlaceRequest(Client *client, WireReader &reader);
  void handlePlaceCommit(Client *client, WireReader &reader);
  void handleMoveToInventory(Client *client, WireReader &reader);
  void handleMoveToStorage(Client *client, WireReader &reader);
  void handleStoreAll(Client *client, WireReader &reader);
  void handlePack(Client *client, WireReader &reader);

  void sendItemResult(Client *client, ItemResult result);
  void cancelPlacement(Client *client, const std::string &reason);
  // A placed or stored row leaves its place: its world object or spawn goes, an open container closes, editors stop editing it.
  void withdrawRow(const PlacedItem &row);
}

#endif
