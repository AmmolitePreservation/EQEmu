#ifndef EQEMU_ZONE_HOUSING_HOUSING_SESSION_H
#define EQEMU_ZONE_HOUSING_HOUSING_SESSION_H

#include "housingAccess.h"
#include "housingStore.h"

#include <map>
#include <set>

class Client;

namespace housing {
  struct Session {
    uint32 currentPropertyId = 0;
    std::map<uint32, Property> properties;
    std::map<uint32, AccessList> accessLists;
    PropertyLookup lookup() const;
    AccessListLookup accessLookup() const;
  };

  bool housingActive();
  Session *session(Client *client);
}

#endif
