#ifndef EQEMU_ZONE_HOUSING_HOUSING_INTERIORS_H
#define EQEMU_ZONE_HOUSING_HOUSING_INTERIORS_H

#include "housingStore.h"

class Client;

namespace housing {
  void enterProperty(Client *client, const Property &property);
  void leaveForExitPoint(Client *client);
}

#endif
