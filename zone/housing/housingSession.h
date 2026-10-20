#ifndef EQEMU_ZONE_HOUSING_HOUSING_SESSION_H
#define EQEMU_ZONE_HOUSING_HOUSING_SESSION_H

#include "housingAccess.h"
#include "housingStore.h"

#include <map>
#include <set>
#include <vector>

class Client;

namespace housing {
  struct Session {
    uint32 currentPropertyId = 0;
    std::set<uint32> hostedIds;
    std::vector<uint32> ownedIds;
    std::vector<uint32> carriedIds;
    std::map<uint32, Property> properties;
    std::map<uint32, AccessList> accessLists;
    PropertyLookup lookup() const;
    AccessListLookup accessLookup() const;
  };

  bool housingActive();
  bool reloadCatalog();
  Session *session(Client *client);
}

#endif
