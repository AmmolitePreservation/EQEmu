#ifndef EQEMU_ZONE_HOUSING_HOUSING_ITEM_TAIL_H
#define EQEMU_ZONE_HOUSING_HOUSING_ITEM_TAIL_H

#include "../../common/itemHousingTail.h"

namespace housing {
  bool loadItemHousingTails();
  const EQ::ItemHousingTail *itemHousingTail(uint32 itemId);
}

#endif
