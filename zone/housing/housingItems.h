#ifndef EQEMU_ZONE_HOUSING_HOUSING_ITEMS_H
#define EQEMU_ZONE_HOUSING_HOUSING_ITEMS_H

#include "housingStore.h"
#include "housingWire.h"

#include "../../common/item_instance.h"
#include "../../common/repositories/real_estate_items_repository.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace housing {
  using PlacedItem = RealEstateItemsRepository::RealEstateItems;

  // Loaded items keyed by ItemGUIDText.
  using HeldInstances = std::map<std::string, std::unique_ptr<EQ::ItemInstance>>;

  std::vector<PlacedItem> itemsOf(const std::set<uint32> &propertyIds);
  HeldInstances itemInstancesOf(const std::set<uint32> &propertyIds);
  std::unique_ptr<EQ::ItemInstance> itemInstance(const PlacedItem &row);
  WireWriter itemList(uint32 propertyId, const std::vector<PlacedItem> &rows, const HeldInstances &instances, const std::map<uint32, CharacterIdentity> &owners);
  std::optional<PlacedItem> placedItem(uint32 propertyId, uint32 recordId);
}

#endif
