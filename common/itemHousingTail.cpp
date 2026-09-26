#include "itemHousingTail.h"

namespace EQ {
  namespace {
    ItemHousingTailProvider tailProvider = nullptr;
    const ItemHousingTail defaultTail;
  }

  void setItemHousingTailProvider(ItemHousingTailProvider provider) {
    tailProvider = provider;
  }

  const ItemHousingTail &itemHousingTail(uint32 itemId) {
    if (tailProvider) {
      const ItemHousingTail *tail = tailProvider(itemId);
      if (tail) {
        return *tail;
      }
    }
    return defaultTail;
  }
}
