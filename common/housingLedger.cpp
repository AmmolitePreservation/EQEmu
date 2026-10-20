#include "housingLedger.h"

#include "database.h"
#include "housingCopies.h"
#include "housingPropertyItem.h"
#include "repositories/criteria/content_filter_criteria.h"
#include "rulesys.h"
#include "strings.h"

#include <cstdlib>
#include <stdexcept>

namespace housing {
  Transaction::Transaction(Database &database) : database(database) {
    query(database, "START TRANSACTION");
  }

  Transaction::~Transaction() {
    if (open) {
      database.QueryDatabase("ROLLBACK");
    }
  }

  void Transaction::commit() {
    query(database, "COMMIT");
    open = false;
  }

  MySQLRequestResult query(Database &database, const std::string &statement) {
    auto result = database.QueryDatabase(statement);
    if (!result.Success()) {
      throw std::runtime_error(fmt::format("housing query failed: {} ({})", result.ErrorMessage(), statement));
    }
    return result;
  }

  std::string idList(const std::set<uint32> &ids) {
    std::vector<std::string> parts;
    for (uint32 id : ids) {
      parts.push_back(std::to_string(id));
    }
    return Strings::Join(parts, ",");
  }

  std::string sqlAmount(double amount) {
    return fmt::format("{:.6f}", amount);
  }

  std::vector<uint32> definitionList(const std::string &text, const char *ruleName) {
    std::vector<uint32> ids;
    for (auto entry : Strings::Split(text, ",")) {
      Strings::Trim(entry);
      if (entry.empty()) {
        continue;
      }
      if (!Strings::IsNumber(entry) || Strings::ToInt(entry) <= 0) {
        throw std::runtime_error(fmt::format("{} lists [{}], which is not a definition id", ruleName, entry));
      }
      ids.push_back(Strings::ToUnsignedInt(entry));
    }
    return ids;
  }

  PropertyRow lockedProperty(Database &database, uint32 propertyId) {
    query(database, fmt::format("SELECT id FROM real_estate_properties WHERE id = {} FOR UPDATE", propertyId));
    auto rows = RealEstatePropertiesRepository::GetWhere(database, fmt::format("id = {}", propertyId));
    if (rows.size() != 1) {
      throw std::runtime_error(fmt::format("property [{}] does not exist", propertyId));
    }
    return rows.front();
  }

  void bumpRevision(Database &database, uint32 propertyId) {
    query(database, fmt::format("UPDATE real_estate_properties SET revision = revision + 1 WHERE id = {}", propertyId));
  }

  std::map<uint32, uint8> definitionTypes(Database &content, const std::set<uint32> &definitionIds) {
    std::map<uint32, uint8> types;
    if (definitionIds.empty()) {
      return types;
    }
    for (auto row : query(content, fmt::format("SELECT id, type FROM real_estate_definitions WHERE id IN ({})", idList(definitionIds)))) {
      types[Strings::ToUnsignedInt(row[0])] = static_cast<uint8>(Strings::ToUnsignedInt(row[1]));
    }
    for (uint32 id : definitionIds) {
      if (!types.count(id)) {
        throw std::runtime_error(fmt::format("definition [{}] does not exist", id));
      }
    }
    return types;
  }

  std::map<uint32, double> readBalances(Database &database, uint32 propertyId) {
    std::map<uint32, double> balances;
    for (auto row : query(database, fmt::format("SELECT currency, amount FROM real_estate_property_balances WHERE property_id = {}", propertyId))) {
      balances[Strings::ToUnsignedInt(row[0])] = std::strtod(row[1], nullptr);
    }
    return balances;
  }

  void writeBalances(Database &database, uint32 propertyId, const std::map<uint32, double> &balances) {
    for (const auto &[currency, amount] : balances) {
      if (amount < 0.0) {
        throw std::logic_error(fmt::format("property [{}] currency [{}] balance would be [{}]", propertyId, currency, amount));
      }
      query(database, fmt::format(
        "INSERT INTO real_estate_property_balances (property_id, currency, amount) VALUES ({}, {}, {}) ON DUPLICATE KEY UPDATE amount = VALUES(amount)",
        propertyId, currency, sqlAmount(amount)
      ));
    }
  }

  std::map<uint32, std::vector<UpkeepCost>> enabledCostOptions(Database &content) {
    std::map<uint32, std::vector<UpkeepCost>> options;
    std::map<std::pair<uint32, uint32>, uint32> itemCounts;
    auto rows = query(content, fmt::format(
      "SELECT o.cost_id, o.option_key, i.currency, i.amount FROM real_estate_cost_options o "
      "JOIN real_estate_cost_items i ON i.cost_id = o.cost_id AND i.option_key = o.option_key WHERE TRUE {} ORDER BY o.cost_id, o.option_key",
      ContentFilterCriteria::apply("o")
    ));
    for (auto row : rows) {
      const uint32 costId = Strings::ToUnsignedInt(row[0]);
      const uint32 optionKey = Strings::ToUnsignedInt(row[1]);
      if (++itemCounts[{costId, optionKey}] > 1) {
        throw std::runtime_error(fmt::format("cost [{}] option [{}] pays in more than one currency", costId, optionKey));
      }
      options[costId].push_back(UpkeepCost{Strings::ToUnsignedInt(row[2]), Strings::ToBigInt(row[3])});
    }
    return options;
  }

  std::map<uint32, int32> itemUpkeepIds(Database &content, const std::set<uint32> &itemIds) {
    std::map<uint32, int32> ids;
    if (itemIds.empty()) {
      return ids;
    }
    for (auto row : query(content, fmt::format("SELECT id, real_estate_upkeep_id FROM items WHERE id IN ({})", idList(itemIds)))) {
      ids[Strings::ToUnsignedInt(row[0])] = Strings::ToInt(row[1]);
    }
    for (uint32 id : itemIds) {
      if (!ids.count(id)) {
        throw std::runtime_error(fmt::format("item [{}] does not exist", id));
      }
    }
    return ids;
  }

  std::map<uint32, uint32> itemDefinitionIds(Database &content, const std::set<uint32> &itemIds) {
    std::map<uint32, uint32> ids;
    if (itemIds.empty()) {
      return ids;
    }
    for (auto row : query(content, fmt::format("SELECT id, real_estate_definition_id FROM items WHERE id IN ({})", idList(itemIds)))) {
      const int32 definition = Strings::ToInt(row[1]);
      ids[Strings::ToUnsignedInt(row[0])] = definition > 0 ? static_cast<uint32>(definition) : 0;
    }
    for (uint32 id : itemIds) {
      if (!ids.count(id)) {
        throw std::runtime_error(fmt::format("item [{}] does not exist", id));
      }
    }
    return ids;
  }

  PlotContents placedUnder(Database &database, Database &content, const PropertyRow &plot) {
    PlotContents contents;
    std::set<uint32> holders{plot.id};
    for (auto &row : RealEstateItemsRepository::GetWhere(database, fmt::format("property_id = {} AND placed = 1 ORDER BY id", plot.id))) {
      const uint32 carried = carriedPropertyId(row.custom_data);
      contents.placedItems.push_back(row);
      if (carried) {
        holders.insert(carried);
      }
    }
    std::set<uint32> carriedIds(holders);
    carriedIds.erase(plot.id);
    if (!carriedIds.empty()) {
      std::set<uint32> definitionIds;
      std::map<uint32, uint32> definitionOf;
      for (const auto &row : RealEstatePropertiesRepository::GetWhere(database, fmt::format("id IN ({})", idList(carriedIds)))) {
        definitionIds.insert(row.definition_id);
        definitionOf[row.id] = row.definition_id;
      }
      const auto types = definitionTypes(content, definitionIds);
      for (uint32 id : carriedIds) {
        auto definition = definitionOf.find(id);
        if (definition == definitionOf.end()) {
          throw std::runtime_error(fmt::format("an item placed on property [{}] names property [{}], which does not exist", plot.id, id));
        }
        const uint8 type = types.at(definition->second);
        if (type == guildHallType || type == playerHouseType) {
          contents.placedHouseIds.insert(id);
        }
      }
      if (!contents.placedHouseIds.empty()) {
        for (auto &row : RealEstateItemsRepository::GetWhere(database, fmt::format("property_id IN ({}) AND placed = 1 ORDER BY id", idList(contents.placedHouseIds)))) {
          contents.placedItems.push_back(row);
        }
      }
    }
    return contents;
  }

  std::vector<std::pair<uint32, int64>> upkeepCostItems(Database &database, Database &content, const PropertyRow &plot, const PlotContents &contents) {
    std::vector<std::pair<uint32, int64>> costItems;
    const auto options = enabledCostOptions(content);
    auto definition = query(content, fmt::format("SELECT upkeep_definition_id FROM real_estate_definitions WHERE id = {}", plot.definition_id));
    if (definition.RowCount() != 1) {
      throw std::runtime_error(fmt::format("property [{}] names definition [{}], which does not exist", plot.id, plot.definition_id));
    }
    const uint32 plotCostId = Strings::ToUnsignedInt(definition.begin()[0]);
    if (plotCostId) {
      auto cost = options.find(plotCostId);
      bool found = false;
      if (cost != options.end()) {
        for (const auto &option : cost->second) {
          if (option.currency == plot.upkeep_currency) {
            costItems.emplace_back(option.currency, option.amount);
            found = true;
            break;
          }
        }
      }
      if (!found) {
        throw std::runtime_error(fmt::format("property [{}] pays upkeep in currency [{}], which its upkeep cost [{}] doesn't offer", plot.id, plot.upkeep_currency, plotCostId));
      }
    }
    std::set<uint32> itemIds;
    for (const auto &row : contents.placedItems) {
      itemIds.insert(row.item_id);
    }
    const auto upkeepIds = itemUpkeepIds(content, itemIds);
    for (const auto &row : contents.placedItems) {
      const int32 upkeepId = upkeepIds.at(row.item_id);
      if (upkeepId <= 0) {
        continue;
      }
      auto cost = options.find(static_cast<uint32>(upkeepId));
      if (cost == options.end() || cost->second.empty()) {
        throw std::runtime_error(fmt::format("item [{}] on property [{}] pays upkeep [{}], which has no enabled option", row.item_id, row.property_id, upkeepId));
      }
      costItems.emplace_back(cost->second.front().currency, cost->second.front().amount);
    }
    return costItems;
  }

  // The host zone's ruleset tunes upkeep (X3); its rule_values hold only overrides of the default ruleset, which world fills at boot.
  int upkeepPercent(Database &database, Database &content, uint32 hostZoneId) {
    auto zoneRow = query(content, fmt::format("SELECT ruleset FROM zone WHERE zoneidnumber = {} AND version = 0", hostZoneId));
    if (zoneRow.RowCount() != 1) {
      throw std::runtime_error(fmt::format("host zone [{}] has [{}] version 0 rows; one is needed for its ruleset", hostZoneId, zoneRow.RowCount()));
    }
    const uint32 rulesetId = Strings::ToUnsignedInt(zoneRow.begin()[0]);
    auto value = query(database, fmt::format(
      "SELECT v.rule_value FROM rule_values v JOIN rule_sets s ON s.ruleset_id = v.ruleset_id "
      "WHERE v.rule_name = 'Housing:UpkeepPercent' AND (v.ruleset_id = {} OR s.name = 'default') "
      "ORDER BY v.ruleset_id = {} DESC LIMIT 1",
      rulesetId, rulesetId
    ));
    if (value.RowCount() != 1) {
      throw std::runtime_error("rule Housing:UpkeepPercent is in neither the host zone's ruleset nor the default ruleset");
    }
    const std::string text = value.begin()[0] ? value.begin()[0] : "";
    if (!Strings::IsNumber(text) || Strings::ToInt(text) < 0) {
      throw std::runtime_error(fmt::format("rule Housing:UpkeepPercent is [{}] for host zone [{}]; it must be 0 or more", text, hostZoneId));
    }
    return Strings::ToInt(text);
  }

  UpkeepCharge settleUpkeep(Database &database, Database &content, const PropertyRow &plot, int64 now) {
    if (plot.upkeep_settled_at <= 0) {
      throw std::runtime_error(fmt::format("property [{}] is owned but has no upkeep_settled_at", plot.id));
    }
    const int64 elapsed = std::max<int64>(0, now - static_cast<int64>(plot.upkeep_settled_at));
    const PlotContents contents = placedUnder(database, content, plot);
    const auto daily = netDailyCost(upkeepCostItems(database, content, plot, contents));
    const UpkeepCharge charge = applyCharge(readBalances(database, plot.id), daily, upkeepPercent(database, content, hostZoneOf(database, content, plot)), elapsed);
    writeBalances(database, plot.id, charge.balances);
    query(database, fmt::format("UPDATE real_estate_properties SET upkeep_settled_at = FROM_UNIXTIME({}) WHERE id = {}", now, plot.id));
    return charge;
  }

  void clearPlacedUpkeepStamps(Database &database, const PropertyRow &plot, const PlotContents &contents) {
    std::set<uint32> stamped{plot.id};
    stamped.insert(contents.placedHouseIds.begin(), contents.placedHouseIds.end());
    query(database, fmt::format("UPDATE real_estate_properties SET upkeep_expired_time = 0 WHERE id IN ({})", idList(stamped)));
    std::set<uint32> itemRows;
    for (const auto &row : contents.placedItems) {
      itemRows.insert(row.id);
    }
    if (!itemRows.empty()) {
      query(database, fmt::format("UPDATE real_estate_items SET upkeep_expired_at = 0 WHERE id IN ({})", idList(itemRows)));
    }
  }

  namespace {
    uint32 countOfType(Database &database, Database &content, const std::string &where, uint8 type) {
      std::set<uint32> definitionIds;
      std::vector<uint32> definitionOfEach;
      for (auto row : query(database, fmt::format("SELECT definition_id FROM real_estate_properties WHERE {}", where))) {
        definitionOfEach.push_back(Strings::ToUnsignedInt(row[0]));
        definitionIds.insert(definitionOfEach.back());
      }
      const auto types = definitionTypes(content, definitionIds);
      uint32 count = 0;
      for (uint32 definition : definitionOfEach) {
        if (types.at(definition) == type) {
          count++;
        }
      }
      return count;
    }

    uint32 checkedMask(int value, const char *rule) {
      if (value < 0 || static_cast<uint32>(value) & ~0x1FFFu) {
        throw std::runtime_error(fmt::format("rule {} is [{}]; a permission mask uses bits 0 to 12", rule, value));
      }
      return static_cast<uint32>(value);
    }

    uint8 checkedGroup(int value, const char *rule) {
      if (value < 1 || value > 5) {
        throw std::runtime_error(fmt::format("rule {} is [{}]; a guild rank goes in group 1 to 5", rule, value));
      }
      return static_cast<uint8>(value);
    }
  }

  uint32 ownedPlotCount(Database &database, Database &content, uint32 characterId) {
    return countOfType(database, content, fmt::format("owner_character_id = {}", characterId), plotType);
  }

  uint32 guildPlotCount(Database &database, Database &content, uint32 guildId) {
    return countOfType(database, content, fmt::format("guild_id = {}", guildId), guildPlotType);
  }

  // Everything on the plot goes into the crate: its item rows stored there, its houses and other crates under it with their contents.
  void packPlotInto(Database &database, uint32 plotId, uint32 crateId) {
    std::set<uint32> moved;
    for (const auto &row : RealEstateItemsRepository::GetWhere(database, fmt::format("property_id = {}", plotId))) {
      if (carriedPropertyId(row.custom_data) != crateId) {
        moved.insert(row.id);
      }
    }
    if (!moved.empty()) {
      query(database, fmt::format("UPDATE real_estate_items SET property_id = {}, placed = 0 WHERE id IN ({})", crateId, idList(moved)));
    }
    query(database, fmt::format("UPDATE real_estate_properties SET parent_id = {} WHERE parent_id = {} AND id <> {}", crateId, plotId, crateId));
  }

  void releasePlot(Database &database, uint32 plotId) {
    query(database, fmt::format("DELETE FROM real_estate_property_balances WHERE property_id = {}", plotId));
    query(database, fmt::format("DELETE FROM real_estate_access_groups WHERE property_id = {}", plotId));
    query(database, fmt::format("DELETE FROM real_estate_access_names WHERE property_id = {}", plotId));
    query(database, fmt::format(
      "UPDATE real_estate_properties SET owner_character_id = 0, guild_id = 0, upkeep_currency = 0, upkeep_expired_time = 0, "
      "upkeep_settled_at = NULL, next_upkeep_at = NULL, voting_opt_out = 0, revision = revision + 1 WHERE id = {}",
      plotId
    ));
  }

  void writeAccessList(Database &database, uint32 propertyId, const std::array<uint32, 7> &masks, const std::vector<std::pair<std::string, uint8>> &names) {
    query(database, fmt::format("DELETE FROM real_estate_access_groups WHERE property_id = {}", propertyId));
    query(database, fmt::format("DELETE FROM real_estate_access_names WHERE property_id = {}", propertyId));
    std::vector<std::string> groups;
    for (size_t group = 0; group < masks.size(); group++) {
      groups.push_back(fmt::format("({}, {}, {})", propertyId, group, masks[group]));
    }
    query(database, fmt::format("INSERT INTO real_estate_access_groups (property_id, group_number, permissions) VALUES {}", Strings::Join(groups, ", ")));
    if (names.empty()) {
      return;
    }
    std::vector<std::string> entries;
    for (const auto &[name, group] : names) {
      entries.push_back(fmt::format("({}, '{}', {})", propertyId, Strings::Escape(name), group));
    }
    query(database, fmt::format("INSERT INTO real_estate_access_names (property_id, name, group_number) VALUES {}", Strings::Join(entries, ", ")));
  }

  void writeDefaultAccessList(Database &database, uint32 propertyId, bool guildPlot) {
    std::array<uint32, 7> masks{};
    std::vector<std::pair<std::string, uint8>> names;
    if (guildPlot) {
      masks = {
        checkedMask(RuleI(Housing, DefaultGuildPlotAccessEveryone), "Housing:DefaultGuildPlotAccessEveryone"),
        checkedMask(RuleI(Housing, DefaultGuildPlotAccessVisitor), "Housing:DefaultGuildPlotAccessVisitor"),
        checkedMask(RuleI(Housing, DefaultGuildPlotAccessAcquaintance), "Housing:DefaultGuildPlotAccessAcquaintance"),
        checkedMask(RuleI(Housing, DefaultGuildPlotAccessFriend), "Housing:DefaultGuildPlotAccessFriend"),
        checkedMask(RuleI(Housing, DefaultGuildPlotAccessTrustee), "Housing:DefaultGuildPlotAccessTrustee"),
        checkedMask(RuleI(Housing, DefaultGuildPlotAccessCoOwner), "Housing:DefaultGuildPlotAccessCoOwner"),
        checkedMask(RuleI(Housing, DefaultGuildPlotAccessOwner), "Housing:DefaultGuildPlotAccessOwner")
      };
      names = {
        {"_Guild2", checkedGroup(RuleI(Housing, DefaultGuildPlotAccessGuild2), "Housing:DefaultGuildPlotAccessGuild2")},
        {"_Guild3", checkedGroup(RuleI(Housing, DefaultGuildPlotAccessGuild3), "Housing:DefaultGuildPlotAccessGuild3")},
        {"_Guild4", checkedGroup(RuleI(Housing, DefaultGuildPlotAccessGuild4), "Housing:DefaultGuildPlotAccessGuild4")},
        {"_Guild5", checkedGroup(RuleI(Housing, DefaultGuildPlotAccessGuild5), "Housing:DefaultGuildPlotAccessGuild5")},
        {"_Guild6", checkedGroup(RuleI(Housing, DefaultGuildPlotAccessGuild6), "Housing:DefaultGuildPlotAccessGuild6")},
        {"_Guild7", checkedGroup(RuleI(Housing, DefaultGuildPlotAccessGuild7), "Housing:DefaultGuildPlotAccessGuild7")},
        {"_Guild8", checkedGroup(RuleI(Housing, DefaultGuildPlotAccessGuild8), "Housing:DefaultGuildPlotAccessGuild8")}
      };
    }
    else {
      masks = {
        checkedMask(RuleI(Housing, DefaultAccessEveryone), "Housing:DefaultAccessEveryone"),
        checkedMask(RuleI(Housing, DefaultAccessVisitor), "Housing:DefaultAccessVisitor"),
        checkedMask(RuleI(Housing, DefaultAccessAcquaintance), "Housing:DefaultAccessAcquaintance"),
        checkedMask(RuleI(Housing, DefaultAccessFriend), "Housing:DefaultAccessFriend"),
        checkedMask(RuleI(Housing, DefaultAccessTrustee), "Housing:DefaultAccessTrustee"),
        checkedMask(RuleI(Housing, DefaultAccessCoOwner), "Housing:DefaultAccessCoOwner"),
        checkedMask(RuleI(Housing, DefaultAccessOwner), "Housing:DefaultAccessOwner")
      };
    }
    writeAccessList(database, propertyId, masks, names);
  }
}
