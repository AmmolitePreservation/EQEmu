#ifndef EQEMU_ZONE_HOUSING_HOUSING_SESSION_H
#define EQEMU_ZONE_HOUSING_HOUSING_SESSION_H

#include "housingAccess.h"
#include "housingStore.h"

#include "../../common/housingPlacementRules.h"

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

class Client;

namespace housing {
  // The client's item location: u32 type, then three u16 indexes (RoF2's InventorySlot_Struct); all -1 for a placed item.
  struct ItemLocation {
    uint32 type = 0xFFFFFFFF;
    int16 slot = -1;
    int16 subIndex = -1;
    int16 augIndex = -1;
  };

  // A placement in progress (S5), from the server's begin reply until a commit succeeds, the client starts another or the
  // server cancels it. The client holds its request latch until each request's result arrives; awaitingCommit is the server's
  // side of it, set only between the begin and the commit.
  struct Placement {
    bool editing = false;
    ItemLocation location;
    int16 inventorySlot = -1;
    uint32 sourcePropertyId = 0;
    uint32 sourceRecordId = 0;
    std::string guid;
    uint32 contextPropertyId = 0;
    MountTilts tilts{0.0f, 0.0f};
    bool awaitingCommit = false;
  };

  // Rebuilt at zone-in and on every property refresh; the per-client state below the zone data carries across refreshes.
  struct Session {
    uint32 currentPropertyId = 0;
    std::set<uint32> hostedIds;
    std::vector<uint32> ownedIds;
    std::vector<uint32> carriedIds;
    std::map<uint32, Property> properties;
    std::map<uint32, AccessList> accessLists;
    std::optional<Placement> placement;
    PropertyLookup lookup() const;
    AccessListLookup accessLookup() const;
  };

  bool housingActive();
  bool reloadCatalog();
  Session *session(Client *client);
  void forEachSession(const std::function<void(Client *, Session &)> &visit);
  void refreshClients(const std::set<uint32> &propertyIds, const std::set<uint32> &characterIds);
  void refreshItemLists(const std::set<uint32> &propertyIds);
  std::string propertyName(uint32 propertyId);
  bool hostedHere(const Property &property);
  int64 currentTime();
}

#endif
