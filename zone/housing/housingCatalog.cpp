#include "housingCatalog.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingPrices.h"
#include "../../common/rulesys.h"
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
      if (row.price && row.type != static_cast<uint8>(DefinitionType::plot) && row.type != static_cast<uint8>(DefinitionType::guildPlot)) {
        problems.push_back(fmt::format("definition [{}] has a price, but only plots and guild plots are sold", id));
      }
      if (row.region_id && !regions.count(row.region_id)) {
        problems.push_back(fmt::format("definition [{}] region [{}] does not exist", id, row.region_id));
      }
      if (row.type > static_cast<uint8>(DefinitionType::guildPlot)) {
        problems.push_back(fmt::format("definition [{}] has unknown type [{}]", id, row.type));
      }
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
    enum class CostUse { plotPurchase, plotUpkeep, itemUpkeep };
    size_t currencyOptions = 0;
    auto addCurrencyOptions = [&](Cost &cost, int64_t coinAmount, CostUse use) {
      const double platinum = static_cast<double>(coinAmount) / 1000.0;
      for (const auto &currency : currencies) {
        const bool accepted = use == CostUse::plotPurchase ? currency.plot_purchase != 0
          : use == CostUse::plotUpkeep ? currency.plot_upkeep != 0
          : currency.item_upkeep != 0;
        if (!accepted) {
          continue;
        }
        RealEstateCostItemsRepository::RealEstateCostItems item{};
        item.cost_id = cost.id;
        item.option_key = static_cast<uint32>(cost.options.size());
        item.currency = currency.alternate_currency_id;
        item.sub_key = currency.sub_key;
        const int64_t units = std::max<int64_t>(1, static_cast<int64_t>(std::llround(std::fabs(platinum) / currency.platinum_per_unit)));
        item.amount = coinAmount < 0 ? -units : units;
        cost.options.push_back(CostOption{item.option_key, currency.alternate_currency_id, {item}});
        currencyOptions++;
      }
    };
    for (auto &[id, cost] : costs) {
      if (cost.options.size() != 1 || cost.options[0].items.size() != 1) {
        problems.push_back(fmt::format("cost [{}] must store exactly one option with one item, its coin amount; currencies add the others", id));
        continue;
      }
      const auto &coin = cost.options[0].items[0];
      if (coin.currency != 0 || coin.sub_key != 0 || coin.amount == 0) {
        problems.push_back(fmt::format("cost [{}] option 0 must be a nonzero coin amount (negative: a reduction)", id));
        continue;
      }
      addCurrencyOptions(cost, coin.amount, CostUse::itemUpkeep);
    }
    const double upkeepPercent = RuleR(Housing, PlotUpkeepPercent);
    size_t plotCosts = 0;
    if (!(upkeepPercent >= 0.0)) {
      problems.push_back(fmt::format("Housing:PlotUpkeepPercent [{}] must be zero or more", upkeepPercent));
    }
    else {
      uint32 nextCostId = costs.empty() ? 1 : costs.rbegin()->first + 1;
      auto derivedCost = [&](uint64 amount, CostUse use) {
        const uint32 id = nextCostId++;
        RealEstateCostItemsRepository::RealEstateCostItems coin{};
        coin.cost_id = id;
        coin.amount = static_cast<int64_t>(amount);
        Cost cost{id, {CostOption{0, 0, {coin}}}};
        addCurrencyOptions(cost, coin.amount, use);
        costs.emplace(id, std::move(cost));
        plotCosts++;
        return id;
      };
      for (auto &[id, entry] : definitions) {
        if (!entry.row.price) {
          continue;
        }
        entry.purchaseCostId = derivedCost(entry.row.price, CostUse::plotPurchase);
        const uint64 upkeep = plotUpkeepCopper(entry.row.price, upkeepPercent);
        entry.upkeepCostId = upkeep ? derivedCost(upkeep, CostUse::plotUpkeep) : 0;
      }
    }
    for (const auto &problem : problems) {
      LogError("Housing catalog: {}", problem);
    }
    loaded = problems.empty();
    LogHousing("Catalog [{}]: [{}] definitions, [{}] costs ([{}] for plots, [{}] currency options from [{}] currencies), [{}] regions", loaded ? "valid" : "INVALID, housing disabled in this zone", definitions.size(), costs.size(), plotCosts, currencyOptions, currencies.size(), regions.size());
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
        .u32(entry.purchaseCostId)
        .u32(entry.upkeepCostId)
        .u32(row.max_items)
        .u32(row.max_non_real_estate_items)
        .u32(row.max_houses)
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
