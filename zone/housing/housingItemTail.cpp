#include "housingItemTail.h"

namespace housing {
  std::set<uint32> npcItemIds() {
    std::set<uint32> ids;
    for (const auto &[id, tail] : EQ::placeableTails()) {
      if (tail.housepetFileName[0] && !tail.interactiveObject) {
        ids.insert(id);
      }
    }
    return ids;
  }

  std::set<uint32> itemUpkeepCostIds() {
    std::set<uint32> ids;
    for (const auto &[id, tail] : EQ::placeableTails()) {
      if (tail.realEstateUpkeepId > 0) {
        ids.insert(static_cast<uint32>(tail.realEstateUpkeepId));
      }
    }
    return ids;
  }
}
