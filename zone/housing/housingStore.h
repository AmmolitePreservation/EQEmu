#ifndef EQEMU_ZONE_HOUSING_HOUSING_STORE_H
#define EQEMU_ZONE_HOUSING_HOUSING_STORE_H

#include "../../common/repositories/real_estate_properties_repository.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace housing {
  using Property = RealEstatePropertiesRepository::RealEstateProperties;

  struct Balance {
    uint32 currency;
    double amount;
  };

  std::vector<Property> propertiesHostedAt(uint32 zoneId, uint32 instanceId);
  std::vector<Property> propertiesWithInterior(uint32 instanceId);
  std::vector<Property> propertiesOwnedBy(uint32 characterId);
  std::vector<Property> propertiesCarriedBy(uint32 characterId);
  std::vector<Property> propertiesById(const std::set<uint32> &ids);
  std::vector<Property> propertiesChildOf(const std::set<uint32> &parentIds);
  std::map<uint32, std::vector<Balance>> balances(const std::set<uint32> &propertyIds);
  std::map<uint32, std::string> characterNames(const std::set<uint32> &characterIds);
  std::string idList(const std::set<uint32> &ids);
}

#endif
