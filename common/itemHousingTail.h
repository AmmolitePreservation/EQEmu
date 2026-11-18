#ifndef EQEMU_COMMON_ITEM_HOUSING_TAIL_H
#define EQEMU_COMMON_ITEM_HOUSING_TAIL_H

#include "types.h"

#include <unordered_map>

class Database;

namespace EQ {
  struct ItemHousingTail {
    uint32 placeable = 0;
    uint8 placeableIgnoreCollisions = 0;
    uint32 placementType = 0;
    int32 realEstateDefinitionId = 0;
    float placeableScaleMin = 0.0f;
    float placeableScaleMax = 0.0f;
    int32 realEstateUpkeepId = 0;
    int32 maxPerRealEstate = -1;
    char housepetFileName[32] = {};
    uint8 interactiveObject = 0;
    int32 trophyBenefitId = -1;
    uint8 disablePlacementRotation = 0;
    uint8 disableFreePlacement = 0;
    uint32 npcRespawnInterval = 0;
    float placeableDefaultScale = 0.0f;
    float placeableDefaultHeading = 0.0f;
    float placeableDefaultPitch = 0.0f;
    float placeableDefaultRoll = 0.0f;
  };

  // The items table's housing columns for every placeable item; false, with the reason logged, when they cannot be read or a
  // scale range runs backwards. Every process that serializes items or finds slots for them loads them at boot.
  bool loadItemHousingTails(Database &content);
  const ItemHousingTail *placeableTail(uint32 itemId);
  // What RoF2 reads after Heirloom for any item: a placeable item's own tail, otherwise the values of an item that is not one.
  const ItemHousingTail &itemHousingTail(uint32 itemId);
  const std::unordered_map<uint32, ItemHousingTail> &placeableTails();
}

#endif
