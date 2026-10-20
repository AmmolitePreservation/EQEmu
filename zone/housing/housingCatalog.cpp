#include "housingCatalog.h"

#include "housingItemTail.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/rulesys.h"
#include "../../common/repositories/alternate_currency_repository.h"
#include "../../common/repositories/criteria/content_filter_criteria.h"
#include "../../common/repositories/real_estate_cost_options_repository.h"
#include "../../common/repositories/real_estate_costs_repository.h"
#include "../../common/repositories/real_estate_definition_children_repository.h"
#include "../../common/repositories/real_estate_npc_items_repository.h"
#include "../zonedb.h"

#include <algorithm>
#include <set>

namespace housing {
  namespace {
    bool isType(uint8 type, std::initializer_list<DefinitionType> types) {
      for (DefinitionType candidate : types) {
        if (type == static_cast<uint8>(candidate)) {
          return true;
        }
      }
      return false;
    }
  }

  Catalog &catalog() {
    static Catalog instance;
    return instance;
  }

  bool Catalog::load() {
    definitions.clear();
    costs.clear();
    regions.clear();
    templates.clear();
    loaded = false;
    std::vector<std::string> problems;

    for (const auto &row : RealEstateDefinitionsRepository::All(content_db)) {
      definitions[row.id].row = row;
    }
    for (const auto &row : RealEstateRegionsRepository::All(content_db)) {
      regions[row.id] = row;
      if (row.id >= propertyIdFloor) {
        problems.push_back(fmt::format("region [{}] is not below the property id floor [{}]", row.id, propertyIdFloor));
      }
    }
    for (const auto &row : RealEstateZoneTemplatesRepository::All(content_db)) {
      templates[row.dz_id] = row;
    }
    for (const auto &row : RealEstateCostsRepository::All(content_db)) {
      costs[row.id].id = row.id;
    }

    std::set<std::pair<uint32, uint32>> storedOptions;
    for (const auto &row : RealEstateCostOptionsRepository::All(content_db)) {
      storedOptions.insert({row.cost_id, row.option_key});
      if (!costs.count(row.cost_id)) {
        problems.push_back(fmt::format("cost option [{}] of unknown cost [{}]", row.option_key, row.cost_id));
      }
    }
    // Options outside the server's expansion or content flags are left out; the client indexes the rest by position.
    auto enabledOptions = RealEstateCostOptionsRepository::GetWhere(content_db, fmt::format("TRUE {}", ContentFilterCriteria::apply()));
    std::sort(enabledOptions.begin(), enabledOptions.end(), [](const auto &a, const auto &b) {
      return a.cost_id != b.cost_id ? a.cost_id < b.cost_id : a.option_key < b.option_key;
    });
    std::map<std::pair<uint32, uint32>, size_t> positions;
    size_t optionCount = 0;
    for (const auto &row : enabledOptions) {
      auto cost = costs.find(row.cost_id);
      if (cost == costs.end()) {
        continue;
      }
      positions[{row.cost_id, row.option_key}] = cost->second.options.size();
      cost->second.options.push_back(CostOption{static_cast<uint32>(cost->second.options.size()), 0, {}});
      optionCount++;
    }
    for (const auto &row : RealEstateCostItemsRepository::All(content_db)) {
      const std::pair<uint32, uint32> key{row.cost_id, row.option_key};
      if (!storedOptions.count(key)) {
        problems.push_back(fmt::format("cost item for unknown cost [{}] option [{}]", row.cost_id, row.option_key));
        continue;
      }
      auto position = positions.find(key);
      if (position == positions.end()) {
        continue;
      }
      costs[row.cost_id].options[position->second].items.push_back(row);
    }

    std::set<int32> alternateCurrencies;
    for (const auto &row : AlternateCurrencyRepository::All(content_db)) {
      alternateCurrencies.insert(row.id);
    }
    for (auto &[id, cost] : costs) {
      std::set<uint32> currencies;
      for (auto &option : cost.options) {
        if (option.items.size() != 1) {
          problems.push_back(fmt::format("cost [{}] option [{}] must pay in exactly one currency, not [{}]", id, option.key, option.items.size()));
          continue;
        }
        const auto &item = option.items[0];
        option.currency = item.currency;
        if (item.amount == 0) {
          problems.push_back(fmt::format("cost [{}] option [{}] has no amount (a negative amount is a reduction)", id, option.key));
        }
        if (item.currency == 0 && item.sub_key != 0) {
          problems.push_back(fmt::format("cost [{}] option [{}] pays in coin with sub key [{}]", id, option.key, item.sub_key));
        }
        // The client matches an alternate currency cost item by currency and sub key 1 (0x7f7e70), the sub key EQEmu sends.
        if (item.currency != 0 && item.sub_key != 1) {
          problems.push_back(fmt::format("cost [{}] option [{}] pays in currency [{}] with sub key [{}], not 1", id, option.key, item.currency, item.sub_key));
        }
        if (item.currency != 0 && !alternateCurrencies.count(static_cast<int32>(item.currency))) {
          problems.push_back(fmt::format("cost [{}] option [{}] pays in currency [{}], which is not in alternate_currency", id, option.key, item.currency));
        }
        if (!currencies.insert(item.currency).second) {
          problems.push_back(fmt::format("cost [{}] offers currency [{}] in more than one option", id, item.currency));
        }
      }
    }

    std::set<uint32> childrenOfZoneRoots;
    for (const auto &row : RealEstateDefinitionChildrenRepository::All(content_db)) {
      auto parent = definitions.find(row.parent_id);
      if (parent == definitions.end() || !definitions.count(row.child_id)) {
        problems.push_back(fmt::format("definition child [{}] of [{}] names an unknown definition", row.child_id, row.parent_id));
        continue;
      }
      parent->second.children.push_back(row.child_id);
      if (parent->second.row.type == static_cast<uint8>(DefinitionType::regularZone)) {
        childrenOfZoneRoots.insert(row.child_id);
      }
    }

    for (const auto &[id, entry] : definitions) {
      const auto &row = entry.row;
      if (row.type < static_cast<uint8>(DefinitionType::regularZone) || row.type > static_cast<uint8>(DefinitionType::guildPlot)) {
        problems.push_back(fmt::format("definition [{}] has unknown type [{}]", id, row.type));
        continue;
      }
      const bool yard = isType(row.type, {DefinitionType::plot, DefinitionType::guildPlot});
      if (row.area_id && !yard) {
        problems.push_back(fmt::format("definition [{}] of type [{}] has area [{}]; only plots and guild plots own a region", id, row.type, row.area_id));
      }
      if (row.area_id && !regions.count(row.area_id)) {
        problems.push_back(fmt::format("definition [{}] area [{}] does not exist", id, row.area_id));
      }
      if (yard && row.address.empty()) {
        problems.push_back(fmt::format("definition [{}] is a plot with no address to name its properties", id));
      }
      if (isType(row.type, {DefinitionType::guildHall, DefinitionType::playerHouse, DefinitionType::movingCrate}) && row.name_segment.empty()) {
        problems.push_back(fmt::format("definition [{}] of type [{}] has no name segment to name its properties", id, row.type));
      }
      const bool exitSet = row.exit_zone_id || row.exit_zone_version || row.exit_x || row.exit_y || row.exit_z || row.exit_heading;
      if (yard && !row.exit_zone_id) {
        problems.push_back(fmt::format("definition [{}] is a plot with no exit zone", id));
      }
      if (!yard && exitSet) {
        problems.push_back(fmt::format("definition [{}] of type [{}] has an exit point; only plots and guild plots do", id, row.type));
      }
      // The client takes the first ancestor with a nonzero dz_id as the zone's (0x5b46e0), which gates Locate (0x5b0cc0).
      if (isType(row.type, {DefinitionType::guildHall, DefinitionType::playerHouse})) {
        if (!row.dz_id) {
          problems.push_back(fmt::format("definition [{}] of type [{}] has no dz_id", id, row.type));
        }
        else if (!templates.count(row.dz_id)) {
          problems.push_back(fmt::format("definition [{}] dz_id [{}] has no zone template", id, row.dz_id));
        }
      }
      else if (row.type == static_cast<uint8>(DefinitionType::neighborhood) && childrenOfZoneRoots.count(id)) {
        if (row.dz_id) {
          problems.push_back(fmt::format("definition [{}] is a neighborhood under a zone root with dz_id [{}]; it must be 0", id, row.dz_id));
        }
      }
      else if (isType(row.type, {DefinitionType::regularZone, DefinitionType::neighborhood})) {
        if (!row.dz_id) {
          problems.push_back(fmt::format("definition [{}] of type [{}] has no dz_id", id, row.type));
        }
      }
      else if (row.dz_id) {
        problems.push_back(fmt::format("definition [{}] of type [{}] has dz_id [{}]; it must be 0", id, row.type, row.dz_id));
      }
      for (const auto &[use, costId] : {std::pair<const char *, uint32>{"purchase", row.cost_definition_id}, {"upkeep", row.upkeep_definition_id}}) {
        if (!costId) {
          continue;
        }
        auto cost = costs.find(costId);
        if (cost == costs.end()) {
          problems.push_back(fmt::format("definition [{}] {} cost [{}] does not exist", id, use, costId));
        }
        else if (cost->second.options.empty()) {
          problems.push_back(fmt::format("definition [{}] {} cost [{}] has no option this server's expansion enables", id, use, costId));
        }
      }
    }

    std::set<uint32> boundNpcItems;
    for (const auto &row : RealEstateNpcItemsRepository::All(content_db)) {
      boundNpcItems.insert(row.item_id);
    }
    for (uint32 itemId : npcItemIds()) {
      if (!boundNpcItems.count(itemId)) {
        problems.push_back(fmt::format("NPC item [{}] has no real_estate_npc_items row naming the NPC it spawns", itemId));
      }
    }

    const int maxChildrenOverride = RuleI(Housing, MaxHousesOverride);
    const int maxChildrenAdditive = RuleI(Housing, MaxHousesAdditive);
    if (maxChildrenOverride < -1) {
      problems.push_back(fmt::format("Housing:MaxHousesOverride [{}] must be -1 (off) or a count", maxChildrenOverride));
    }
    for (auto &[id, entry] : definitions) {
      if (!isType(entry.row.type, {DefinitionType::plot, DefinitionType::guildPlot})) {
        entry.maxChildren = entry.row.max_children;
      }
      else if (maxChildrenOverride >= 0) {
        entry.maxChildren = static_cast<uint32>(maxChildrenOverride);
      }
      else {
        entry.maxChildren = static_cast<uint32>(std::max<int64_t>(0, static_cast<int64_t>(entry.row.max_children) + maxChildrenAdditive));
      }
    }

    for (const auto &problem : problems) {
      LogError("Housing catalog: {}", problem);
    }
    lastProblems = problems;
    loaded = problems.empty();
    LogHousing("Catalog [{}]: [{}] definitions, [{}] costs with [{}] of [{}] options enabled, [{}] regions, [{}] zone templates",
      loaded ? "valid" : "INVALID, housing disabled in this zone", definitions.size(), costs.size(), optionCount, storedOptions.size(), regions.size(), templates.size());
    return loaded;
  }

  const Definition *Catalog::definition(uint32 id) const {
    auto found = definitions.find(id);
    return found == definitions.end() ? nullptr : &found->second;
  }

  const Cost *Catalog::cost(uint32 id) const {
    auto found = costs.find(id);
    return found == costs.end() ? nullptr : &found->second;
  }

  const CostOption *Catalog::option(uint32 costId, uint32 currency) const {
    auto found = costs.find(costId);
    if (found == costs.end()) {
      return nullptr;
    }
    for (const auto &option : found->second.options) {
      if (option.currency == currency) {
        return &option;
      }
    }
    return nullptr;
  }

  const RealEstateRegionsRepository::RealEstateRegions *Catalog::region(uint32 id) const {
    auto found = regions.find(id);
    return found == regions.end() ? nullptr : &found->second;
  }

  const ZoneTemplate *Catalog::zoneTemplate(uint32 dzId) const {
    auto found = templates.find(dzId);
    return found == templates.end() ? nullptr : &found->second;
  }

  WireWriter Catalog::message() const {
    WireWriter message;
    message.u32(static_cast<uint32>(definitions.size()));
    for (const auto &[id, entry] : definitions) {
      const auto &row = entry.row;
      message.u32(row.id)
        .u32(row.area_id)
        .u32(row.cost_definition_id)
        .u32(row.upkeep_definition_id)
        .u32(row.max_items)
        .u32(row.max_non_real_estate_items)
        .u32(entry.maxChildren)
        .u32(row.zone_id)
        .u32(row.dz_id)
        .u32(row.type)
        .u32(row.placement_group_id)
        .lengthString(row.address)
        .u32(row.icon)
        .i32(row.max_npc_items)
        .f32(row.switch_y)
        .f32(row.switch_x)
        .f32(row.switch_z)
        .f32(row.switch_heading)
        .u32(static_cast<uint32>(entry.children.size()));
      for (uint32 child : entry.children) {
        message.u32(child);
      }
    }
    message.u32(static_cast<uint32>(costs.size()));
    for (const auto &[id, cost] : costs) {
      message.u32(cost.id).u32(static_cast<uint32>(cost.options.size()));
      for (const auto &option : cost.options) {
        message.u32(option.key).u32(static_cast<uint32>(option.items.size()));
        for (const auto &item : option.items) {
          message.u32(item.currency).u32(item.sub_key).i64(item.amount);
        }
      }
    }
    return message;
  }
}
