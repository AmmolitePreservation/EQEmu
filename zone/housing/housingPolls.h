#ifndef EQEMU_ZONE_HOUSING_HOUSING_POLLS_H
#define EQEMU_ZONE_HOUSING_HOUSING_POLLS_H

#include "housingWire.h"

class Client;

namespace housing {
  void sendPolls(Client *client);
  void handlePollRequest(Client *client, WireReader &reader);
  void handleVote(Client *client, WireReader &reader);
  void handleVotingOptOut(Client *client, WireReader &reader);
  void handlePollTeleport(Client *client, WireReader &reader);
  void refreshPolls();
  void processPolls();
  void resetPolls();
}

#endif
