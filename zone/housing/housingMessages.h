#ifndef EQEMU_ZONE_HOUSING_HOUSING_MESSAGES_H
#define EQEMU_ZONE_HOUSING_HOUSING_MESSAGES_H

#include "housingStore.h"
#include "housingWire.h"

#include "../../common/repositories/real_estate_regions_repository.h"

#include <optional>

namespace housing {
  struct RecordFields {
    bool vacantPlayerPlot;
    uint32 guildId;
    std::optional<CharacterIdentity> owner;
    std::string name;
    std::vector<uint32> childIds;
    std::vector<Balance> balances;
    uint32 upkeepOptionKey;
  };

  WireWriter propertyRecord(const Property &property, const RecordFields &fields);
  WireWriter propertyIdList(const std::vector<uint32> &ids);
  WireWriter regionList(const std::vector<RealEstateRegionsRepository::RealEstateRegions> &regions);
}

#endif
