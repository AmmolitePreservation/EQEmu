#ifndef EQEMU_ZONE_HOUSING_HOUSING_MESSAGES_H
#define EQEMU_ZONE_HOUSING_HOUSING_MESSAGES_H

#include "housingStore.h"
#include "housingWire.h"

#include "../../common/repositories/real_estate_regions_repository.h"

namespace housing {
  WireWriter propertyRecord(const Property &property, uint32 guildId, const std::vector<uint32> &childIds, const std::string &ownerName, const std::vector<Balance> &propertyBalances, uint32 upkeepOptionKey);
  WireWriter propertyIdList(const std::vector<uint32> &ids);
  WireWriter regionList(const std::vector<RealEstateRegionsRepository::RealEstateRegions> &regions);
}

#endif
