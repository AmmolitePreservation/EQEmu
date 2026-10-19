#ifndef EQEMU_COMMON_ITEM_HOUSING_TAIL_H
#define EQEMU_COMMON_ITEM_HOUSING_TAIL_H

#include "types.h"

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

  using ItemHousingTailProvider = const ItemHousingTail *(*)(uint32 itemId);

  void setItemHousingTailProvider(ItemHousingTailProvider provider);
  const ItemHousingTail &itemHousingTail(uint32 itemId);
}

#endif
