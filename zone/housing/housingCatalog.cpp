#include "housingCatalog.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/repositories/real_estate_cost_options_repository.h"
#include "../../common/repositories/real_estate_costs_repository.h"
#include "../../common/repositories/real_estate_definition_children_repository.h"
#include "../zonedb.h"

#include <algorithm>

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

    auto options = RealEstateCostOptionsRepository::All(content_db);
    std::sort(options.begin(), options.end(), [](const auto &a, const auto &b) {
      return a.cost_id != b.cost_id ? a.cost_id < b.cost_id : a.option_key < b.option_key;
    });
    for (const auto &row : options) {
      auto cost = costs.find(row.cost_id);
      if (cost == costs.end()) {
        problems.push_back(fmt::format("cost option [{}] of unknown cost [{}]", row.option_key, row.cost_id));
        continue;
      }
      if (row.option_key != cost->second.options.size()) {
        problems.push_back(fmt::format("cost [{}] option key [{}] is not its position [{}]", row.cost_id, row.option_key, cost->second.options.size()));
        continue;
      }
      cost->second.options.push_back(CostOption{row.option_key, {}});
    }

    for (const auto &row : RealEstateCostItemsRepository::All(content_db)) {
      auto cost = costs.find(row.cost_id);
      if (cost == costs.end() || row.option_key >= cost->second.options.size()) {
        problems.push_back(fmt::format("cost item for unknown cost [{}] option [{}]", row.cost_id, row.option_key));
        continue;
      }
      cost->second.options[row.option_key].items.push_back(row);
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
      if (row.purchase_cost_id && !costs.count(row.purchase_cost_id)) {
        problems.push_back(fmt::format("definition [{}] purchase cost [{}] does not exist", id, row.purchase_cost_id));
      }
      if (row.upkeep_cost_id && !costs.count(row.upkeep_cost_id)) {
        problems.push_back(fmt::format("definition [{}] upkeep cost [{}] does not exist", id, row.upkeep_cost_id));
      }
      if (row.region_id && !regions.count(row.region_id)) {
        problems.push_back(fmt::format("definition [{}] region [{}] does not exist", id, row.region_id));
      }
      if (row.type > static_cast<uint8>(DefinitionType::guildPlot)) {
        problems.push_back(fmt::format("definition [{}] has unknown type [{}]", id, row.type));
      }
    }

    for (const auto &problem : problems) {
      LogError("Housing catalog: {}", problem);
    }
    loaded = problems.empty();
    LogHousing("Catalog [{}]: [{}] definitions, [{}] costs, [{}] regions", loaded ? "valid" : "INVALID, housing disabled in this zone", definitions.size(), costs.size(), regions.size());
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
        .u32(row.unknown_30)
        .u32(row.unknown_34)
        .u32(row.unknown_38)
        .u32(row.type)
        .u32(row.category)
        .lengthString(row.unknown_string)
        .u32(row.unknown_44)
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
