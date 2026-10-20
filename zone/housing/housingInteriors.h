#ifndef EQEMU_ZONE_HOUSING_HOUSING_INTERIORS_H
#define EQEMU_ZONE_HOUSING_HOUSING_INTERIORS_H

#include "housingStore.h"

#include <glm/vec4.hpp>

#include <optional>
#include <vector>

class Client;

namespace housing {
  // What this zone instance runs: a zone root at instance 0, else the property a copy was started for (D3).
  void loadHostedRoot();
  void clearHostedRoot();
  std::optional<Property> hostedRoot();
  std::optional<Property> hostedRootYard();
  bool hostsInterior();
  std::vector<Property> hostedProperties();

  enum class EntryState : uint8 { open, gone, notInYard, noAccess, upkeepExpired };

  // Whether a character may go into a property now: it exists, an interior stands in a yard, Enter, and its and its yard's upkeep.
  struct EntryCheck {
    EntryState state = EntryState::gone;
    std::optional<Property> property;
  };

  EntryCheck checkEntry(Client *client, uint32 propertyId);
  void moveInto(Client *client, const Property &property, const std::optional<glm::vec4> &point);
  void enterProperty(Client *client, const Property &property);
  void leaveForExitPoint(Client *client);
  void moveToPlot(Client *client, const Property &plot);
  void handleLeave(Client *client);
  void admitOccupant(Client *client);
  void checkHostedRoot(uint32 propertyId);
  void discardHostedCopy();
}

#endif
