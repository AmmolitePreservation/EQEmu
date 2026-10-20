#ifndef EQEMU_ZONE_HOUSING_HOUSING_AUGMENTATER_H
#define EQEMU_ZONE_HOUSING_HOUSING_AUGMENTATER_H

#include "housingItems.h"

#include "../../common/housingAugmentaterRules.h"

#include <memory>
#include <optional>

class Client;
class NPC;
struct NPCType;

namespace housing {
  // A guild assistant is a placed NPC item serving as merchant, banker or tribute master; the Augmentater re-skins only those.
  bool isGuildAssistant(const NPCType &type);

  // The placed copy's look over its NPC type, as a private copy the spawn owns; none while the copy has the standard look.
  std::unique_ptr<NPCType> lookedNpcType(const PlacedItem &row, const NPCType &base);

  void useAugmentater(Client *client);
}

#endif
