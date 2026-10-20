#ifndef EQEMU_ZONE_HOUSING_HOUSING_CONTAINERS_H
#define EQEMU_ZONE_HOUSING_HOUSING_CONTAINERS_H

#include "housingItems.h"

class Client;

namespace housing {
  bool isTradeskillContainer(uint32 itemId);
  void openContainer(Client *client, const PlacedItem &row);
  void closeContainersOf(uint32 recordId);
  void closeContainerOf(Client *client);
  void processContainers();
}

#endif
