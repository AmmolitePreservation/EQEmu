#ifndef EQEMU_COMMON_HOUSING_PROPERTY_ITEM_H
#define EQEMU_COMMON_HOUSING_PROPERTY_ITEM_H

#include "item_instance.h"
#include "types.h"

#include <algorithm>
#include <cctype>
#include <fmt/format.h>
#include <stdexcept>
#include <string>

namespace housing {
  // An item carrying a property (a house, a crate with contents) names it in its custom data; the RoF2 item header sends
  // it as the item's RealEstateID, which the client's item display looks up (playerHousing/serverPlan.md, chunk 4).
  constexpr const char *propertyItemKey = "real_estate_property_id";

  inline uint32 carriedPropertyId(const std::string &customData) {
    const auto entries = EQ::ItemInstance::ParseCustomDataString(customData);
    auto found = entries.find(propertyItemKey);
    if (found == entries.end()) {
      return 0;
    }
    const std::string &value = found->second;
    if (value.empty() || value.size() > 10 || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); }) ||
        std::stoull(value) == 0 || std::stoull(value) > 0xFFFFFFFFull) {
      throw std::invalid_argument(fmt::format("item custom data [{}] names property [{}], which is not a property id", customData, value));
    }
    return static_cast<uint32>(std::stoull(value));
  }
}

#endif
