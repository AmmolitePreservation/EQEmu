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
  constexpr int maximumPropertyDepth = 10;

  // A DBcore scope whose commit throws; raw START TRANSACTION here would commit under the item layer's own scopes.
  class Transaction {
  public:
    explicit Transaction(Database &database);
    void commit();

  private:
    DBcore::Transaction scope;
  };

  MySQLRequestResult query(Database &database, const std::string &statement);
  // A write that must touch exactly one row; the connection counts matched rows (CLIENT_FOUND_ROWS), so an unchanged row
  // counts too. A row another zone moved or removed first fails the act (AUD-32).
  void queryOneRow(Database &database, const std::string &statement);
  std::string idList(const std::set<uint32> &ids);
  std::string sqlAmount(double amount);
  std::vector<uint32> definitionList(const std::string &text, const char *ruleName);

  PropertyRow lockedProperty(Database &database, uint32 propertyId);
  // The properties whose own item a character holds: in the inventory, on the cursor or in the bank, loose or inside a bag.
  std::set<uint32> carriedPropertyIds(Database &database, uint32 characterId);
  // Whether the property's own item is anywhere but on a property.
  bool isCarried(Database &database, uint32 propertyId);
  // Whether items are stored or placed in the property or properties stand under it. A crate waiting for room in its
  // owner's inventory holds its own item with no real_estate_items row, and that item is not contents.
  bool holdsContents(Database &database, uint32 propertyId);

  // What a vacant plot still holds: escrow, item rows, and properties an item carries (a definition's fixed house carries none).
  struct PlotLeftovers {
    uint32 balances = 0;
    uint32 items = 0;
    uint32 carriedChildren = 0;
  };
  PlotLeftovers plotLeftovers(Database &database, uint32 plotId);
  void bumpRevision(Database &database, uint32 propertyId);

  // The properties whose access lists name the character.
  std::set<uint32> propertiesNaming(Database &database, const std::string &name);
  // A character's name entries follow it: a rename renames them and a deletion removes them, in the caller's transaction,
  // so a later character of the same name inherits no group (AUD-24).
  void renameAccessNames(Database &database, const std::string &oldName, const std::string &newName);
  void removeAccessNames(Database &database, const std::string &name);
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
  std::map<uint32, int32> chargedUpkeepIds(Database &content, const std::set<uint32> &itemIds);
  std::map<uint32, uint32> itemDefinitionIds(Database &content, const std::set<uint32> &itemIds);
  std::vector<std::pair<uint32, int64>> upkeepCostItems(Database &database, Database &content, const PropertyRow &plot, const PlotContents &contents);
  int upkeepPercent(Database &database, Database &content, uint32 hostZoneId);

  uint32 ownedPlotCount(Database &database, Database &content, uint32 characterId);
  uint32 guildPlotCount(Database &database, Database &content, uint32 guildId);
  // The properties under a root, its children and theirs, read before a change moves or deletes them.
  std::set<uint32> descendants(Database &database, uint32 rootId);
  // Returns the plot's rows that stood placed, which leave the world.
  std::vector<ItemRow> packPlotInto(Database &database, uint32 plotId, uint32 crateId);
  void releasePlot(Database &database, uint32 plotId);
  void writeAccessList(Database &database, uint32 propertyId, const std::array<uint32, 7> &masks, const std::vector<std::pair<std::string, uint8>> &names);
  void writeDefaultAccessList(Database &database, uint32 propertyId, bool guildPlot);

  UpkeepCharge settleUpkeep(Database &database, Database &content, const PropertyRow &plot, int64 now);
  // Returns the placed houses whose stamp it cleared, whose records clients hold stamped (AUD-38).
  std::set<uint32> clearPlacedUpkeepStamps(Database &database, const PropertyRow &plot, const PlotContents &contents);
  void clearPlotUpkeepStamp(Database &database, const PropertyRow &plot);
}

#endif
