#ifndef EQEMU_ZONE_HOUSING_HOUSING_ANCHORS_H
#define EQEMU_ZONE_HOUSING_HOUSING_ANCHORS_H

#include "housingWire.h"

class Client;

namespace housing {
  void handleAnchorAnswer(Client *client, WireReader &reader);
  void forgetAnchorOffer(Client *client);
}

#endif
