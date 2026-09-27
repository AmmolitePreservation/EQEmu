#ifndef EQEMU_COMMON_HOUSING_PROPERTY_ITEM_H
#define EQEMU_COMMON_HOUSING_PROPERTY_ITEM_H

#include "strings.h"
#include "types.h"

#include <string>

namespace housing {
  // An item carrying a property (a house, a crate with contents) names it in its custom data; the RoF2 item header sends
  // it as the item's RealEstateID, which the client's item display looks up (playerHousing/serverPlan.md, chunk 4).
  constexpr const char *propertyItemKey = "real_estate_property_id";

  // customData is an item's whole custom data string, "key^value^key^value^" (ItemInstance::GetCustomDataString and the
  // inventory table's custom_data); 0 when it names no property.
  inline uint32 carriedPropertyId(const std::string &customData) {
    const auto parts = Strings::Split(customData, "^");
    for (size_t i = 0; i + 1 < parts.size(); i += 2) {
      if (parts[i] == propertyItemKey) {
        return Strings::IsNumber(parts[i + 1]) ? Strings::ToUnsignedInt(parts[i + 1]) : 0;
      }
    }
    return 0;
  }
}

#endif
