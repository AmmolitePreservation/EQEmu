#ifndef EQEMU_COMMON_HOUSING_LEDGER_H
#define EQEMU_COMMON_HOUSING_LEDGER_H

#include "housingUpkeepMath.h"
#include "mysql_request_result.h"
#include "repositories/real_estate_items_repository.h"
#include "repositories/real_estate_properties_repository.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Database;

namespace housing {
  using PropertyRow = RealEstatePropertiesRepository::RealEstateProperties;
  using ItemRow = RealEstateItemsRepository::RealEstateItems;

  constexpr uint8 regularZoneType = 1;
  constexpr uint8 guildHallType = 2;
  constexpr uint8 playerHouseType = 3;
  constexpr uint8 plotType = 4;
  constexpr uint8 neighborhoodType = 5;
  constexpr uint8 movingCrateType = 7;
  constexpr uint8 guildPlotType = 8;

  class Transaction {
  public:
    explicit Transaction(Database &database);
    ~Transaction();
    void commit();
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;

  private:
    Database &database;
    bool open = true;
  };

  MySQLRequestResult query(Database &database, const std::string &statement);
  std::string idList(const std::set<uint32> &ids);
  std::string sqlAmount(double amount);
  std::vector<uint32> definitionList(const std::string &text, const char *ruleName);

  PropertyRow lockedProperty(Database &database, uint32 propertyId);
  void bumpRevision(Database &database, uint32 propertyId);
  std::map<uint32, uint8> definitionTypes(Database &content, const std::set<uint32> &definitionIds);
  std::map<uint32, double> readBalances(Database &database, uint32 propertyId);
  void writeBalances(Database &database, uint32 propertyId, const std::map<uint32, double> &balances);

  // The placed items whose upkeep the plot pays: its own, and those of the houses and halls placed on it.
  struct PlotContents {
    std::vector<ItemRow> placedItems;
    std::set<uint32> placedHouseIds;
  };
  PlotContents placedUnder(Database &database, Database &content, const PropertyRow &plot);

  struct UpkeepCost {
    uint32 currency;
    int64 amount;
  };
  std::map<uint32, std::vector<UpkeepCost>> enabledCostOptions(Database &content);
  std::map<uint32, int32> itemUpkeepIds(Database &content, const std::set<uint32> &itemIds);
  std::map<uint32, uint32> itemDefinitionIds(Database &content, const std::set<uint32> &itemIds);
  std::vector<std::pair<uint32, int64>> upkeepCostItems(Database &database, Database &content, const PropertyRow &plot, const PlotContents &contents);
  int upkeepPercent(Database &database, Database &content, uint32 hostZoneId);

  uint32 ownedPlotCount(Database &database, Database &content, uint32 characterId);
  uint32 guildPlotCount(Database &database, Database &content, uint32 guildId);
  void packPlotInto(Database &database, uint32 plotId, uint32 crateId);
  void releasePlot(Database &database, uint32 plotId);
  void writeAccessList(Database &database, uint32 propertyId, const std::array<uint32, 7> &masks, const std::vector<std::pair<std::string, uint8>> &names);
  void writeDefaultAccessList(Database &database, uint32 propertyId, bool guildPlot);

  UpkeepCharge settleUpkeep(Database &database, Database &content, const PropertyRow &plot, int64 now);
  void clearPlacedUpkeepStamps(Database &database, const PropertyRow &plot, const PlotContents &contents);
}

#endif
