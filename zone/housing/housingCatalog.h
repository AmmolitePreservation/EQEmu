#ifndef EQEMU_ZONE_HOUSING_HOUSING_CATALOG_H
#define EQEMU_ZONE_HOUSING_HOUSING_CATALOG_H

#include "housingWire.h"

#include "../../common/repositories/real_estate_cost_items_repository.h"
#include "../../common/repositories/real_estate_definitions_repository.h"
#include "../../common/repositories/real_estate_regions_repository.h"

#include <map>
#include <vector>

namespace housing {
  enum class DefinitionType : uint8 {
    none = 0,
    regularZone = 1,
    guildHall = 2,
    playerHouse = 3,
    plot = 4,
    neighborhood = 5,
    town = 6,
    movingCrate = 7,
    guildPlot = 8
  };

  struct Definition {
    RealEstateDefinitionsRepository::RealEstateDefinitions row;
    std::vector<uint32> children;
  };

  struct CostOption {
    uint32 key;
    uint32 currency;
    std::vector<RealEstateCostItemsRepository::RealEstateCostItems> items;
  };

  struct Cost {
    uint32 id;
    std::vector<CostOption> options;
  };

  class Catalog {
  public:
    bool load();
    bool valid() const { return loaded; }
    const Definition *definition(uint32 id) const;
    const Cost *cost(uint32 id) const;
    const CostOption *option(uint32 costId, uint32 currency) const;
    const RealEstateRegionsRepository::RealEstateRegions *region(uint32 id) const;
    const std::map<uint32, Definition> &allDefinitions() const { return definitions; }
    const std::map<uint32, RealEstateRegionsRepository::RealEstateRegions> &allRegions() const { return regions; }
    WireWriter message() const;

  private:
    std::map<uint32, Definition> definitions;
    std::map<uint32, Cost> costs;
    std::map<uint32, RealEstateRegionsRepository::RealEstateRegions> regions;
    bool loaded = false;
  };

  Catalog &catalog();
}

#endif
