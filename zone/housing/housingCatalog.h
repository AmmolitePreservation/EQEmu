#ifndef EQEMU_ZONE_HOUSING_HOUSING_CATALOG_H
#define EQEMU_ZONE_HOUSING_HOUSING_CATALOG_H

#include "housingWire.h"

#include "../../common/repositories/real_estate_cost_items_repository.h"
#include "../../common/repositories/real_estate_definitions_repository.h"
#include "../../common/repositories/real_estate_regions_repository.h"
#include "../../common/repositories/real_estate_zone_templates_repository.h"

#include <map>
#include <string>
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

  // RoF2 keys region and current-property placement codes in one map (0x4b2bf0), so region ids stay below every property id.
  constexpr uint32 propertyIdFloor = 100000;

  struct Definition {
    RealEstateDefinitionsRepository::RealEstateDefinitions row;
    std::vector<uint32> children;
    uint32 maxChildren = 0;
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

  using ZoneTemplate = RealEstateZoneTemplatesRepository::RealEstateZoneTemplates;

  class Catalog {
  public:
    bool load();
    bool valid() const { return loaded; }
    const Definition *definition(uint32 id) const;
    const Cost *cost(uint32 id) const;
    const CostOption *option(uint32 costId, uint32 currency) const;
    const RealEstateRegionsRepository::RealEstateRegions *region(uint32 id) const;
    const ZoneTemplate *zoneTemplate(uint32 dzId) const;
    const std::map<uint32, Definition> &allDefinitions() const { return definitions; }
    const std::map<uint32, RealEstateRegionsRepository::RealEstateRegions> &allRegions() const { return regions; }
    WireWriter message() const;
    const std::vector<std::string> &problems() const { return lastProblems; }

  private:
    std::map<uint32, Definition> definitions;
    std::map<uint32, Cost> costs;
    std::map<uint32, RealEstateRegionsRepository::RealEstateRegions> regions;
    std::map<uint32, ZoneTemplate> templates;
    std::vector<std::string> lastProblems;
    bool loaded = false;
  };

  Catalog &catalog();
}

#endif
