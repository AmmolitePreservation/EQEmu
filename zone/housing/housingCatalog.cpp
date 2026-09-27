#include "housingCatalog.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/repositories/alternate_currency_repository.h"
#include "../../common/repositories/real_estate_cost_options_repository.h"
#include "../../common/repositories/real_estate_costs_repository.h"
#include "../../common/repositories/real_estate_currencies_repository.h"
#include "../../common/repositories/real_estate_definition_children_repository.h"
#include "../zonedb.h"

#include <algorithm>
#include <cmath>
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
      cost->second.options.push_back(CostOption{row.option_key, 0, {}});
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

    enum class CostUse { purchase, propertyUpkeep, itemUpkeep };
    std::map<uint32, CostUse> uses;
    auto noteUse = [&](uint32 costId, CostUse use, uint32 definitionId) {
      if (!costId) {
        return;
      }
      auto [known, inserted] = uses.emplace(costId, use);
      if (!inserted && known->second != use) {
        problems.push_back(fmt::format("cost [{}] is both a purchase and an upkeep cost (definition [{}])", costId, definitionId));
      }
    };
    for (const auto &[id, entry] : definitions) {
      noteUse(entry.row.purchase_cost_id, CostUse::purchase, id);
      noteUse(entry.row.upkeep_cost_id, CostUse::propertyUpkeep, id);
    }
    std::set<int32> alternateCurrencies;
    for (const auto &row : AlternateCurrencyRepository::All(content_db)) {
      alternateCurrencies.insert(row.id);
    }
    auto currencies = RealEstateCurrenciesRepository::All(content_db);
    std::sort(currencies.begin(), currencies.end(), [](const auto &a, const auto &b) {
      return a.alternate_currency_id < b.alternate_currency_id;
    });
    for (const auto &currency : currencies) {
      if (!alternateCurrencies.count(static_cast<int32>(currency.alternate_currency_id))) {
        problems.push_back(fmt::format("currency [{}] is not in alternate_currency", currency.alternate_currency_id));
      }
      if (!(currency.platinum_per_unit > 0.0f)) {
        problems.push_back(fmt::format("currency [{}] needs a positive platinum_per_unit", currency.alternate_currency_id));
      }
    }
    size_t currencyOptions = 0;
    for (auto &[id, cost] : costs) {
      if (cost.options.size() != 1 || cost.options[0].items.size() != 1) {
        problems.push_back(fmt::format("cost [{}] must store exactly one option with one item, its coin amount; currencies add the others", id));
        continue;
      }
      const auto &coin = cost.options[0].items[0];
      if (coin.currency != 0 || coin.sub_key != 0 || coin.amount <= 0) {
        problems.push_back(fmt::format("cost [{}] option 0 must be a positive coin amount", id));
        continue;
      }
      const auto found = uses.find(id);
      const CostUse use = found == uses.end() ? CostUse::itemUpkeep : found->second;
      const double platinum = static_cast<double>(coin.amount) / 1000.0;
      for (const auto &currency : currencies) {
        const bool accepted = use == CostUse::purchase ? currency.plot_purchase != 0
          : use == CostUse::propertyUpkeep ? currency.plot_upkeep != 0
          : currency.item_upkeep != 0;
        if (!accepted) {
          continue;
        }
        RealEstateCostItemsRepository::RealEstateCostItems item{};
        item.cost_id = id;
        item.option_key = static_cast<uint32>(cost.options.size());
        item.currency = currency.alternate_currency_id;
        item.sub_key = currency.sub_key;
        item.amount = std::max<int64_t>(1, static_cast<int64_t>(std::llround(platinum / currency.platinum_per_unit)));
        cost.options.push_back(CostOption{item.option_key, currency.alternate_currency_id, {item}});
        currencyOptions++;
      }
    }
    for (const auto &problem : problems) {
      LogError("Housing catalog: {}", problem);
    }
    loaded = problems.empty();
    LogHousing("Catalog [{}]: [{}] definitions, [{}] costs ([{}] options from [{}] currencies), [{}] regions", loaded ? "valid" : "INVALID, housing disabled in this zone", definitions.size(), costs.size(), currencyOptions, currencies.size(), regions.size());
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
