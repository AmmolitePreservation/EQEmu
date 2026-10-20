#ifndef EQEMU_ZONE_HOUSING_HOUSING_NPC_ITEMS_H
#define EQEMU_ZONE_HOUSING_HOUSING_NPC_ITEMS_H

#include "housingItems.h"

#include <functional>

class Client;
class NPC;

namespace housing {
  // Live's pet actions, by their b code (category 28 labels of the same ids).
  enum class PetAction : uint32 {
    stay = 16175,
    walkAround = 16176,
    runAround = 16177,
    followMe = 16178,
    playDead = 16179,
    kickTheGnome = 16394,
    sitDown = 16492
  };

  bool isPetAction(uint32 action);
  bool isTrainingDummyAction(uint32 action);
  bool isNpcItemId(uint32 itemId);

  void spawnShownNpcItems();
  void spawnNpcItem(const PlacedItem &row);
  void despawnNpcItem(uint32 recordId);
  NPC *npcItemSpawn(uint32 recordId);
  void forEachNpcItemSpawn(const std::function<void(const PlacedItem &, NPC *)> &visit);
  void processNpcItems();
  void resetNpcItems();
  void performNpcItemAction(Client *client, const PlacedItem &row, uint32 action);
}

#endif
