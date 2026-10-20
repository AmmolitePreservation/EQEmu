#ifndef EQEMU_ZONE_HOUSING_HOUSING_ITEM_TAIL_H
#define EQEMU_ZONE_HOUSING_HOUSING_ITEM_TAIL_H

#include "../../common/itemHousingTail.h"

#include <set>

namespace housing {
  bool loadItemHousingTails();
  const EQ::ItemHousingTail *itemHousingTail(uint32 itemId);
  std::set<uint32> npcItemIds();
}

#endif
