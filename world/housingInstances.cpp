#include "housingInstances.h"

#include "worlddb.h"
#include "zonelist.h"

#include "../common/eqemu_logsys.h"
#include "../common/housingCopies.h"
#include "../common/housingLedger.h"
#include "../common/rulesys.h"
#include "../common/strings.h"
#include "../common/timer.h"

#include <ctime>
#include <map>
#include <stdexcept>

namespace housing {
  namespace {
    constexpr uint32 sweepMilliseconds = 60000;
    // A copy's zone boots seconds after its entry; one with no zone this long is a crashed or abandoned copy.
    constexpr int64 sweepGraceSeconds = 300;
    constexpr int maximumDepth = 10;

    Timer sweepTimer;
    std::map<uint16, int64> unbootedSince;

    uint32 insertProperty(uint32 parentId, uint32 definitionId, uint32 guildId, const std::string &name) {
      auto inserted = query(database, fmt::format(
        "INSERT INTO real_estate_properties (parent_id, definition_id, guild_id, name) VALUES ({}, {}, {}, '{}')",
        parentId, definitionId, guildId, Strings::Escape(name)
      ));
      return static_cast<uint32>(inserted.LastInsertedID());
    }

    // The children a definition makes: a root's plots and neighborhoods, an apartment plot's fixed house (X4).
    void createChildren(uint32 definitionId, uint32 propertyId, int depth) {
      if (depth > maximumDepth) {
        throw std::runtime_error(fmt::format("definition [{}] nests deeper than [{}]", definitionId, maximumDepth));
      }
      std::set<uint32> childIds;
      for (auto row : query(content_db, fmt::format("SELECT child_id FROM real_estate_definition_children WHERE parent_id = {} ORDER BY child_id", definitionId))) {
        childIds.insert(Strings::ToUnsignedInt(row[0]));
      }
      for (const auto &[childId, type] : definitionTypes(content_db, childIds)) {
        if (type == plotType || type == guildPlotType || type == neighborhoodType || type == playerHouseType || type == guildHallType) {
          createChildren(childId, insertProperty(propertyId, childId, 0, {}), depth + 1);
        }
      }
    }

    void createZoneRoots() {
      if (!RuleB(Housing, EnableZoneRoots)) {
        return;
      }
      for (uint32 definitionId : definitionList(RuleS(Housing, ZoneRootDefinitions), "Housing:ZoneRootDefinitions")) {
        if (definitionTypes(content_db, {definitionId}).at(definitionId) != regularZoneType) {
          throw std::runtime_error(fmt::format("Housing:ZoneRootDefinitions lists definition [{}], which is not a Regular Zone", definitionId));
        }
        if (zoneTemplateOf(content_db, definitionId).zoneVersion != 0) {
          throw std::runtime_error(fmt::format("zone root definition [{}] names a zone version other than 0", definitionId));
        }
        auto existing = query(database, fmt::format("SELECT COUNT(*) FROM real_estate_properties WHERE parent_id = 0 AND definition_id = {}", definitionId));
        if (Strings::ToUnsignedInt(existing.begin()[0]) > 0) {
          continue;
        }
        const uint32 rootId = createPropertyTree(definitionId, 0, {});
        LogHousing("Created zone root [{}] of definition [{}]", rootId, definitionId);
      }
    }

    void sweepCopies() {
      const int64 now = std::time(nullptr);
      std::map<uint16, int64> stillUnbooted;
      for (auto row : query(database, "SELECT instance_id FROM real_estate_instances")) {
        const uint16 instanceId = static_cast<uint16>(Strings::ToUnsignedInt(row[0]));
        if (ZSList::Instance()->FindByInstanceID(instanceId)) {
          continue;
        }
        auto seen = unbootedSince.find(instanceId);
        const int64 since = seen == unbootedSince.end() ? now : seen->second;
        if (now - since < sweepGraceSeconds) {
          stillUnbooted[instanceId] = since;
          continue;
        }
        try {
          discardCopy(database, instanceId);
          LogHousing("Swept housing copy instance [{}], which has had no zone for [{}] s", instanceId, now - since);
        }
        catch (const std::exception &error) {
          LogError("Housing: copy instance [{}] could not be swept: {}", instanceId, error.what());
          stillUnbooted[instanceId] = since;
        }
      }
      unbootedSince = stillUnbooted;
    }
  }

  bool reenterCopy(uint32 characterId, uint32 zoneId, uint32 &instanceId);

  uint32 createPropertyTree(uint32 definitionId, uint32 guildId, const std::string &name) {
    Transaction transaction(database);
    const uint32 propertyId = insertProperty(0, definitionId, guildId, name);
    createChildren(definitionId, propertyId, 0);
    writeDefaultAccessList(database, propertyId, false);
    transaction.commit();
    return propertyId;
  }

  // Live's copy counter carries on through a restart (INT-16), so the row is made once and never reset.
  void startHousingInstances() {
    const InstanceRange range = instanceRange(RuleI(Instances, ReservedInstances));
    query(database, fmt::format("INSERT IGNORE INTO real_estate_instance_counter (id, next_instance_id) VALUES (1, {})", range.first));
    createZoneRoots();
  }

  void processHousingInstances() {
    if (!sweepTimer.Enabled()) {
      sweepTimer.Start(sweepMilliseconds);
    }
    if (!sweepTimer.Check()) {
      return;
    }
    try {
      sweepCopies();
    }
    catch (const std::exception &error) {
      LogError("Housing: the copy sweep failed: {}", error.what());
    }
  }

  // A character saved in a copy that was discarded logs in to a new copy of the same property, where it was (D3, INT-14).
  bool reenterDiscardedCopy(uint32 characterId, uint32 zoneId, uint32 &instanceId) {
    try {
      return reenterCopy(characterId, zoneId, instanceId);
    }
    catch (const std::exception &error) {
      LogError("Housing: character [{}] could not log in to a new copy: {}; EQEmu's safe return applies", characterId, error.what());
      return false;
    }
  }

  bool reenterCopy(uint32 characterId, uint32 zoneId, uint32 &instanceId) {
    auto rows = query(database, fmt::format(
      "SELECT property_id FROM character_instance_safereturns WHERE character_id = {} AND instance_zone_id = {} AND instance_id = {} AND property_id <> 0",
      characterId, zoneId, instanceId
    ));
    if (!rows.RowCount()) {
      return false;
    }
    const uint32 propertyId = Strings::ToUnsignedInt(rows.begin()[0]);
    auto properties = RealEstatePropertiesRepository::GetWhere(database, fmt::format("id = {}", propertyId));
    if (properties.size() != 1 || properties.front().contained_in_item) {
      LogHousing("Character [{}] was in a copy of property [{}], which is gone or picked up; EQEmu's safe return applies", characterId, propertyId);
      return false;
    }
    const ZoneTemplatePoint zoneTemplate = zoneTemplateOf(content_db, properties.front().definition_id);
    if (zoneTemplate.zoneId != zoneId) {
      LogHousing("Character [{}] was in a copy of property [{}], whose template is now another zone; EQEmu's safe return applies", characterId, propertyId);
      return false;
    }
    const uint16 copy = runningCopy(database, content_db, propertyId);
    if (!database.AddClientToInstance(copy, characterId)) {
      throw std::runtime_error(fmt::format("character [{}] could not be added to instance [{}]", characterId, copy));
    }
    query(database, fmt::format("UPDATE character_data SET zone_instance = {} WHERE id = {}", copy, characterId));
    instanceId = copy;
    LogHousing("Character [{}] logs in to new copy [{}] of property [{}]", characterId, copy, propertyId);
    return true;
  }
}
