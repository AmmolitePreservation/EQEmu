#include "housingCatalog.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/rulesys.h"
#include "../../common/repositories/alternate_currency_repository.h"
#include "../../common/repositories/criteria/content_filter_criteria.h"
#include "../../common/repositories/real_estate_cost_options_repository.h"
#include "../../common/repositories/real_estate_costs_repository.h"
#include "../../common/repositories/real_estate_definition_children_repository.h"
#include "../zonedb.h"

#include <algorithm>
#include <set>

namespace housing {
  Catalog &catalog() {
    static Catalog instance;
    return instance;
  }

  bool Catalog::load() {
    definitions.clear();
    costs.clear();
    regions.clear();
    loaded = false;
    std::vector<std::string> problems;

    for (const auto &row : RealEstateDefinitionsRepository::All(content_db)) {
      definitions[row.id].row = row;
    }
    for (const auto &row : RealEstateRegionsRepository::All(content_db)) {
      regions[row.id] = row;
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
        if (item.currency != 0 && !alternateCurrencies.count(static_cast<int32>(item.currency))) {
          problems.push_back(fmt::format("cost [{}] option [{}] pays in currency [{}], which is not in alternate_currency", id, option.key, item.currency));
        }
        if (!currencies.insert(item.currency).second) {
          problems.push_back(fmt::format("cost [{}] offers currency [{}] in more than one option", id, item.currency));
        }
      }
    }

    for (const auto &row : RealEstateDefinitionChildrenRepository::All(content_db)) {
      auto parent = definitions.find(row.parent_id);
      if (parent == definitions.end() || !definitions.count(row.child_id)) {
        problems.push_back(fmt::format("definition child [{}] of [{}] names an unknown definition", row.child_id, row.parent_id));
        continue;
      }
      parent->second.children.push_back(row.child_id);
    }

    for (const auto &[id, entry] : definitions) {
      const auto &row = entry.row;
      if (row.region_id && !regions.count(row.region_id)) {
        problems.push_back(fmt::format("definition [{}] region [{}] does not exist", id, row.region_id));
      }
      if (row.type > static_cast<uint8>(DefinitionType::guildPlot)) {
        problems.push_back(fmt::format("definition [{}] has unknown type [{}]", id, row.type));
      }
      for (const auto &[use, costId] : {std::pair<const char *, uint32>{"purchase", row.purchase_cost_id}, {"upkeep", row.upkeep_cost_id}}) {
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

    const int maxHousesOverride = RuleI(Housing, MaxHousesOverride);
    const int maxHousesAdditive = RuleI(Housing, MaxHousesAdditive);
    if (maxHousesOverride < -1) {
      problems.push_back(fmt::format("Housing:MaxHousesOverride [{}] must be -1 (off) or a count", maxHousesOverride));
    }
    for (auto &[id, entry] : definitions) {
      const bool yard = entry.row.type == static_cast<uint8>(DefinitionType::plot) || entry.row.type == static_cast<uint8>(DefinitionType::guildPlot);
      if (!yard) {
        entry.maxHouses = entry.row.max_houses;
      }
      else if (maxHousesOverride >= 0) {
        entry.maxHouses = static_cast<uint32>(maxHousesOverride);
      }
      else {
        entry.maxHouses = static_cast<uint32>(std::max<int64_t>(0, static_cast<int64_t>(entry.row.max_houses) + maxHousesAdditive));
      }
    }

    for (const auto &problem : problems) {
      LogError("Housing catalog: {}", problem);
    }
    lastProblems = problems;
    loaded = problems.empty();
    LogHousing("Catalog [{}]: [{}] definitions, [{}] costs with [{}] of [{}] options enabled, [{}] regions", loaded ? "valid" : "INVALID, housing disabled in this zone",
      definitions.size(), costs.size(), optionCount, storedOptions.size(), regions.size());
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

  WireWriter Catalog::message() const {
    WireWriter message;
    message.u32(static_cast<uint32>(definitions.size()));
    for (const auto &[id, entry] : definitions) {
      const auto &row = entry.row;
      message.u32(row.id)
        .u32(row.region_id)
        .u32(row.purchase_cost_id)
        .u32(row.upkeep_cost_id)
        .u32(row.max_items)
        .u32(row.max_non_real_estate_items)
        .u32(entry.maxHouses)
        .u32(row.unknown_34)
        .u32(row.unknown_38)
        .u32(row.type)
        .u32(row.category)
        .lengthString(row.unknown_string)
        .u32(row.icon)
        .u32(row.max_pet_items)
        .f32(row.layout_origin_x)
        .f32(row.layout_origin_y)
        .f32(row.layout_origin_z)
        .f32(row.layout_heading)
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
