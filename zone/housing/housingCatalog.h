#ifndef EQEMU_ZONE_HOUSING_HOUSING_CATALOG_H
#define EQEMU_ZONE_HOUSING_HOUSING_CATALOG_H

#include "housingWire.h"

#include "../../common/repositories/real_estate_cost_items_repository.h"
#include "../../common/repositories/real_estate_definitions_repository.h"
#include "../../common/repositories/real_estate_regions_repository.h"
#include "../../common/repositories/real_estate_zone_templates_repository.h"

#include <map>
#include <set>
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
    int32 maxNpcItems = -1;
  };

  // A guild hall's teleport service: the Assistant aligns the Device to the key handed to him.
  enum class TeleportRole : uint8 { none = 0, assistant = 1, device = 2 };

  // An NPC item's spawn (real_estate_npc_items): its NPC, a mount it rides, the race tag RoF2 previews it with, a teleport
  // role, and its menu actions (Live's b codes, in Live's order).
  struct NpcItem {
    uint32 npcTypeId;
    uint32 mountNpcTypeId;
    std::string raceTag;
    bool trainingDummy;
    TeleportRole teleportRole;
    std::vector<uint32> actions;
  };

  // A placed item's aura (real_estate_auras): the spell kept on those within the radius and the lines on walking in and out.
  struct AuraItem {
    uint32 spellId;
    float radius;
    std::string nearMessage;
    std::string leaveMessage;
  };

  // A Teleportation Assistant's key (real_estate_teleport_keys): where the Device then sends a traveller.
  struct TeleportKey {
    std::string destination;
    bool guildStandard;
    uint32 zoneId;
    float x;
    float y;
    float z;
    float heading;
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
    const NpcItem *npcItem(uint32 itemId) const;
    const AuraItem *auraItem(uint32 itemId) const;
    const TeleportKey *teleportKey(uint32 itemId) const;
    std::set<uint32> auraSpellIds() const;
    std::set<uint32> auraItemIds() const;
    const std::map<uint32, Definition> &allDefinitions() const { return definitions; }
    const std::map<uint32, RealEstateRegionsRepository::RealEstateRegions> &allRegions() const { return regions; }
    WireWriter message() const;
    const std::vector<std::string> &problems() const { return lastProblems; }

  private:
    std::map<uint32, Definition> definitions;
    std::map<uint32, Cost> costs;
    std::map<uint32, RealEstateRegionsRepository::RealEstateRegions> regions;
    std::map<uint32, ZoneTemplate> templates;
    std::map<uint32, NpcItem> npcItems;
    std::map<uint32, AuraItem> auraItems;
    std::map<uint32, TeleportKey> teleportKeys;
    std::vector<std::string> lastProblems;
    bool loaded = false;
  };

  Catalog &catalog();
}

#endif
