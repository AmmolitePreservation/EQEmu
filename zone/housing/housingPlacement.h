#ifndef EQEMU_ZONE_HOUSING_HOUSING_PLACEMENT_H
#define EQEMU_ZONE_HOUSING_HOUSING_PLACEMENT_H

#include "housingItems.h"
#include "housingSession.h"
#include "housingWire.h"

#include "../../common/housingEventTypes.h"
#include "../../common/housingPlacementRules.h"

#include <string>
#include <vector>

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
  void handleHouseSwap(Client *client, WireReader &reader);
  void handleLoadLayout(Client *client, WireReader &reader);

  void sendItemResult(Client *client, ItemResult result);
  // The plot whose region holds the player, else the current property (the client's +0x94).
  uint32 standingProperty(Client *client, const Session &state);
  void cancelPlacement(Client *client, const std::string &reason);
  // A placed or stored row leaves its place: its world object or spawn goes, an open container closes, editors stop editing it.
  void withdrawRow(const PlacedItem &row);
  // Records another zone placed or withdrew (an item list event): where their property is shown here, a withdrawn record's
  // object or spawn goes and a placed one is read back and shown (INT-30).
  void applyItemListChange(const std::vector<ItemListChangedRecord> &records);
}

#endif
