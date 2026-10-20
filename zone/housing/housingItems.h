#ifndef EQEMU_ZONE_HOUSING_HOUSING_ITEMS_H
#define EQEMU_ZONE_HOUSING_HOUSING_ITEMS_H

#include "housingStore.h"
#include "housingWire.h"

#include "../../common/item_instance.h"
#include "../../common/repositories/real_estate_items_repository.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace housing {
  using PlacedItem = RealEstateItemsRepository::RealEstateItems;

  std::vector<PlacedItem> itemsOf(const std::set<uint32> &propertyIds);
  std::string itemGuid(const PlacedItem &row);
  std::unique_ptr<EQ::ItemInstance> itemInstance(const PlacedItem &row);
  bool writeItemEntry(WireWriter &message, const PlacedItem &row, const std::map<uint32, CharacterIdentity> &owners);
  WireWriter itemList(uint32 propertyId, const std::vector<PlacedItem> &rows, const std::map<uint32, CharacterIdentity> &owners);
}

#endif
