#ifndef EQEMU_ZONE_HOUSING_HOUSING_ITEM_TAIL_H
#define EQEMU_ZONE_HOUSING_HOUSING_ITEM_TAIL_H

#include "../../common/itemHousingTail.h"

#include <set>

namespace housing {
  std::set<uint32> npcItemIds();
  std::set<uint32> itemUpkeepCostIds();
}

#endif
