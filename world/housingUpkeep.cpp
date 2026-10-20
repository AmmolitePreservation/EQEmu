#include "housingUpkeep.h"

#include "housingNeighborhoods.h"
#include "world_config.h"
#include "worlddb.h"
#include "zonelist.h"

#include "../common/eqemu_logsys.h"
#include "../common/housingEventTypes.h"
#include "../common/housingCopies.h"
#include "../common/housingLedger.h"
#include "../common/random.h"
#include "../common/rulesys.h"
#include "../common/servertalk.h"
#include "../common/strings.h"
#include "../common/timer.h"
#include "../common/zone_store.h"

#include <cstring>
#include <ctime>
#include <memory>
#include <stdexcept>

namespace housing {
  namespace {
    constexpr uint32 passMilliseconds = 10000;
    constexpr uint32 duePerPass = 500;
    constexpr uint32 crateOfEvictionDefinition = 31;
    constexpr int maximumDepth = 10;
    constexpr const char *expiryNoticeSubject = "Escrow expired on personal plot - First Notice";
    constexpr const char *reclaimNotice = "Plots will pack up all items, send them to the owner, and become available for purchase again if its escrow has been expired for 90 days.";

    Timer passTimer;
    EQ::Random random;

    void sendEvent(EventType type, uint32 propertyId, const std::string &payload) {
      auto pack = std::make_unique<ServerPacket>(ServerOP_Housing, static_cast<uint32>(sizeof(ServerHousingEvent_Struct) + payload.size()));
      auto *event = reinterpret_cast<ServerHousingEvent_Struct *>(pack->pBuffer);
      event->event_type = static_cast<uint32>(type);
      event->target_kind = static_cast<uint32>(EventTarget::property);
      event->target_id = propertyId;
      event->origin_zone_id = 0;
      event->origin_instance_id = 0;
      event->property_id = propertyId;
      event->revision = 0;
      event->payload_size = static_cast<uint32>(payload.size());
      if (!payload.empty()) {
        memcpy(event->payload, payload.data(), payload.size());
      }
      ZSList::Instance()->SendPacketToBootedZones(pack.get());
    }

    void sendPropertyChanged(uint32 propertyId, const PropertyChangedPayload &owners) {
      sendEvent(EventType::propertyChanged, propertyId, std::string(reinterpret_cast<const char *>(&owners), sizeof(owners)));
    }

    std::map<uint32, PropertyRow> ancestry(const PropertyRow &property) {
      std::map<uint32, PropertyRow> chain{{property.id, property}};
      for (uint32 id = property.parent_id; id;) {
        if (chain.size() > maximumDepth) {
          throw std::runtime_error(fmt::format("property [{}] has an ancestor chain deeper than [{}]", property.id, maximumDepth));
        }
        auto rows = RealEstatePropertiesRepository::GetWhere(database, fmt::format("id = {}", id));
        if (rows.size() != 1) {
          throw std::runtime_error(fmt::format("property [{}] has ancestor [{}], which does not exist", property.id, id));
        }
        chain[id] = rows.front();
        id = rows.front().parent_id;
      }
      return chain;
    }

    std::map<uint32, uint8> chainTypes(const std::map<uint32, PropertyRow> &chain) {
      std::set<uint32> definitionIds;
      for (const auto &[id, property] : chain) {
        definitionIds.insert(property.definition_id);
      }
      return definitionTypes(content_db, definitionIds);
    }

    // The zone's composition: a zone root's zone long name, a neighborhood's own name, a plot's address.
    std::string plotName(const PropertyRow &plot) {
      const auto chain = ancestry(plot);
      const auto types = chainTypes(chain);
      std::vector<const PropertyRow *> path;
      for (const PropertyRow *current = &chain.at(plot.id);; current = &chain.at(current->parent_id)) {
        path.insert(path.begin(), current);
        if (!current->parent_id) {
          break;
        }
      }
      std::vector<std::string> parts;
      for (const PropertyRow *property : path) {
        switch (types.at(property->definition_id)) {
          case regularZoneType: {
            const uint32 zoneId = zoneTemplateOf(content_db, property->definition_id).zoneId;
            const char *longName = ZoneLongName(zoneId);
            if (!longName) {
              throw std::runtime_error(fmt::format("zone root [{}] names zone [{}], which has no zone row", property->id, zoneId));
            }
            parts.push_back(longName);
            break;
          }
          case neighborhoodType:
            parts = {property->name};
            break;
          case plotType:
          case guildPlotType: {
            auto address = query(content_db, fmt::format("SELECT address FROM real_estate_definitions WHERE id = {}", property->definition_id));
            parts.push_back(address.begin()[0]);
            break;
          }
          default:
            throw std::runtime_error(fmt::format("property [{}] is of definition type [{}], which no plot hangs from", property->id, types.at(property->definition_id)));
        }
      }
      return Strings::Join(parts, ", ");
    }

    // A plot under a zone root that is delisted, or with zone roots off, is dormant and its upkeep pauses (X3).
    bool dormant(const PropertyRow &plot) {
      const auto chain = ancestry(plot);
      const auto types = chainTypes(chain);
      const auto listedIds = definitionList(RuleS(Housing, ZoneRootDefinitions), "Housing:ZoneRootDefinitions");
      const std::set<uint32> listed(listedIds.begin(), listedIds.end());
      for (const auto &[id, property] : chain) {
        if (types.at(property.definition_id) == regularZoneType && (!RuleB(Housing, EnableZoneRoots) || !listed.count(property.definition_id))) {
          return true;
        }
      }
      return false;
    }

    bool ownerDeleted(uint32 characterId) {
      auto rows = query(database, fmt::format("SELECT COUNT(*) FROM character_data WHERE id = {} AND deleted_at IS NULL", characterId));
      return Strings::ToUnsignedInt(rows.begin()[0]) == 0;
    }

    std::set<uint32> descendants(uint32 rootId) {
      std::set<uint32> found;
      std::set<uint32> frontier{rootId};
      for (int depth = 0; !frontier.empty(); depth++) {
        if (depth > maximumDepth) {
          throw std::runtime_error(fmt::format("property [{}] has descendants deeper than [{}]", rootId, maximumDepth));
        }
        std::set<uint32> next;
        for (auto row : query(database, fmt::format("SELECT id FROM real_estate_properties WHERE parent_id IN ({})", idList(frontier)))) {
          next.insert(Strings::ToUnsignedInt(row[0]));
        }
        found.insert(next.begin(), next.end());
        frontier = next;
      }
      return found;
    }

    void deleteProperties(const std::set<uint32> &ids) {
      if (ids.empty()) {
        return;
      }
      const std::string list = idList(ids);
      query(database, fmt::format("DELETE FROM real_estate_items WHERE property_id IN ({})", list));
      query(database, fmt::format("DELETE FROM real_estate_property_balances WHERE property_id IN ({})", list));
      query(database, fmt::format("DELETE FROM real_estate_access_groups WHERE property_id IN ({})", list));
      query(database, fmt::format("DELETE FROM real_estate_access_names WHERE property_id IN ({})", list));
      query(database, fmt::format("DELETE FROM real_estate_properties WHERE id IN ({})", list));
    }

    // Reclaim (D7, RCL-1): everything on the plot goes into a Crate of Eviction archived for the owner, filled whatever its caps;
    // a deleted owner's items are lost (Live, RCL-3).
    uint32 reclaim(const PropertyRow &plot, bool itemsLost) {
      uint32 crateId = 0;
      if (itemsLost) {
        deleteProperties(descendants(plot.id));
        query(database, fmt::format("DELETE FROM real_estate_items WHERE property_id = {}", plot.id));
      }
      else {
        if (definitionTypes(content_db, {crateOfEvictionDefinition}).at(crateOfEvictionDefinition) != movingCrateType) {
          throw std::runtime_error(fmt::format("definition [{}], the Crate of Eviction, is not a moving crate", crateOfEvictionDefinition));
        }
        auto inserted = query(database, fmt::format(
          "INSERT INTO real_estate_properties (parent_id, definition_id, owner_character_id, archived) VALUES (0, {}, {}, 1)",
          crateOfEvictionDefinition, plot.owner_character_id
        ));
        crateId = inserted.LastInsertedID();
        packPlotInto(database, plot.id, crateId);
      }
      releasePlot(database, plot.id);
      return crateId;
    }

    // The charge that finds the escrow empty stamps the plot, its placed houses and every placed item with an upkeep id with its own
    // second, and stores those items but houses (UPK-2). Returns the properties whose item lists changed.
    std::set<uint32> stamp(const PropertyRow &plot, int64 now) {
      const PlotContents contents = placedUnder(database, content_db, plot);
      std::set<uint32> properties{plot.id};
      properties.insert(contents.placedHouseIds.begin(), contents.placedHouseIds.end());
      query(database, fmt::format("UPDATE real_estate_properties SET upkeep_expired_time = {} WHERE id IN ({})", now, idList(properties)));
      std::set<uint32> itemIds;
      for (const auto &row : contents.placedItems) {
        itemIds.insert(row.item_id);
      }
      const auto upkeepIds = itemUpkeepIds(content_db, itemIds);
      const auto definitionIds = itemDefinitionIds(content_db, itemIds);
      std::set<uint32> deedDefinitions;
      for (const auto &[itemId, definitionId] : definitionIds) {
        if (definitionId) {
          deedDefinitions.insert(definitionId);
        }
      }
      const auto deedTypes = definitionTypes(content_db, deedDefinitions);
      std::set<uint32> stampedRows;
      std::set<uint32> storedRows;
      std::set<uint32> changedLists;
      for (const auto &row : contents.placedItems) {
        if (upkeepIds.at(row.item_id) <= 0) {
          continue;
        }
        stampedRows.insert(row.id);
        changedLists.insert(row.property_id);
        const uint32 definitionId = definitionIds.at(row.item_id);
        const uint8 deedType = definitionId ? deedTypes.at(definitionId) : 0;
        if (deedType != guildHallType && deedType != playerHouseType) {
          storedRows.insert(row.id);
        }
      }
      if (!stampedRows.empty()) {
        query(database, fmt::format("UPDATE real_estate_items SET upkeep_expired_at = {} WHERE id IN ({})", now, idList(stampedRows)));
      }
      if (!storedRows.empty()) {
        query(database, fmt::format("UPDATE real_estate_items SET placed = 0 WHERE id IN ({})", idList(storedRows)));
      }
      return changedLists;
    }

    // Live mails the owner at the expiring second, the body the two chat lines (UPK-5, UPK-6); the sender is the server's own name.
    void mailExpiryNotice(const PropertyRow &plot, const std::string &firstLine, int64 now) {
      const std::string sender = fmt::format("@{}@", Strings::ToLower(WorldConfig::get()->ShortName));
      query(database, fmt::format(
        "INSERT INTO mail (charid, timestamp, `from`, subject, body, `to`, status) VALUES ({}, {}, '{}', '{}', '{}', '', 1)",
        plot.owner_character_id, now, Strings::Escape(sender), Strings::Escape(expiryNoticeSubject),
        Strings::Escape(fmt::format("{}\n\n{}", firstLine, reclaimNotice))
      ));
    }

    struct ChargeOutcome {
      bool changed = false;
      bool reclaimed = false;
      uint32 crateId = 0;
      std::string noticeLine;
      std::string ownerName;
      std::set<uint32> itemListsChanged;
    };

    ChargeOutcome chargeProperty(uint32 propertyId, int64 now) {
      ChargeOutcome outcome;
      Transaction transaction(database);
      const PropertyRow plot = lockedProperty(database, propertyId);
      if (plot.next_upkeep_at <= 0 || plot.next_upkeep_at > now) {
        return outcome;
      }
      const uint8 type = definitionTypes(content_db, {plot.definition_id}).at(plot.definition_id);
      const bool guildPlot = type == guildPlotType;
      if (type != plotType && !guildPlot) {
        throw std::runtime_error(fmt::format("property [{}] has an upkeep clock but is of type [{}], not a plot", plot.id, type));
      }
      const int jitter = RuleI(Housing, UpkeepJitterSeconds);
      const int64 nextCharge = now + nextChargeDelay(RuleI(Housing, UpkeepIntervalSeconds), jitter, random.Int(-jitter, jitter));
      if (dormant(plot)) {
        query(database, fmt::format("UPDATE real_estate_properties SET upkeep_settled_at = FROM_UNIXTIME({}), next_upkeep_at = FROM_UNIXTIME({}) WHERE id = {}", now, nextCharge, plot.id));
        transaction.commit();
        return outcome;
      }
      outcome.changed = true;
      if (plot.upkeep_expired_time) {
        const bool deleted = !guildPlot && ownerDeleted(plot.owner_character_id);
        if (deleted || reclaimDue(plot.upkeep_expired_time, now, RuleI(Housing, UnpaidUpkeepEvictionDays))) {
          if (guildPlot) {
            LogError("Housing: guild plot [{}] has been expired since [{}] and is due for reclaim, but who receives a guild plot's Crate of Eviction is undecided; it is not reclaimed", plot.id, plot.upkeep_expired_time);
          }
          else {
            outcome.crateId = reclaim(plot, deleted);
            outcome.reclaimed = true;
            outcome.itemListsChanged.insert(plot.id);
            if (outcome.crateId) {
              outcome.itemListsChanged.insert(outcome.crateId);
            }
            transaction.commit();
            LogHousing("Reclaimed plot [{}] from character [{}]; crate [{}] (0: a deleted owner's items are lost)", plot.id, plot.owner_character_id, outcome.crateId);
            return outcome;
          }
        }
      }
      const UpkeepCharge charge = settleUpkeep(database, content_db, plot, now);
      if (charge.escrowFoundEmpty && !plot.upkeep_expired_time) {
        const auto changedLists = stamp(plot, now);
        outcome.itemListsChanged.insert(changedLists.begin(), changedLists.end());
        if (!guildPlot) {
          auto owner = query(database, fmt::format("SELECT name FROM character_data WHERE id = {} AND deleted_at IS NULL", plot.owner_character_id));
          if (owner.RowCount() == 1) {
            outcome.ownerName = owner.begin()[0];
            outcome.noticeLine = fmt::format("Your player plot at {}'s escrow has expired!", plotName(plot));
            mailExpiryNotice(plot, outcome.noticeLine, now);
          }
        }
        else {
          LogHousing("Guild plot [{}] expired; Live's guild plot notice and its recipient are unverified, so none is sent", plot.id);
        }
        LogHousing("Plot [{}] found its escrow empty and is expired at [{}]", plot.id, now);
      }
      query(database, fmt::format("UPDATE real_estate_properties SET next_upkeep_at = FROM_UNIXTIME({}), revision = revision + 1 WHERE id = {}", nextCharge, plot.id));
      transaction.commit();
      return outcome;
    }

    void chargeDueUpkeep() {
      const int64 now = static_cast<int64>(std::time(nullptr));
      std::vector<std::pair<uint32, PropertyRow>> due;
      for (const auto &row : RealEstatePropertiesRepository::GetWhere(database, fmt::format(
        "next_upkeep_at IS NOT NULL AND next_upkeep_at <= FROM_UNIXTIME({}) ORDER BY next_upkeep_at LIMIT {}", now, duePerPass))) {
        due.emplace_back(row.id, row);
      }
      bool ownershipChanged = false;
      for (const auto &[id, before] : due) {
        ChargeOutcome outcome;
        try {
          outcome = chargeProperty(id, now);
        }
        catch (const std::exception &error) {
          LogError("Housing: the upkeep charge of property [{}] failed: {}", id, error.what());
          continue;
        }
        if (!outcome.changed) {
          continue;
        }
        if (!outcome.ownerName.empty()) {
          ZSList::Instance()->SendEmoteMessageRaw(outcome.ownerName.c_str(), 0, AccountStatus::Player, Chat::White, outcome.noticeLine.c_str());
          ZSList::Instance()->SendEmoteMessageRaw(outcome.ownerName.c_str(), 0, AccountStatus::Player, Chat::White, reclaimNotice);
        }
        if (outcome.reclaimed) {
          ownershipChanged = true;
          sendPropertyChanged(id, PropertyChangedPayload{before.owner_character_id, 0, before.guild_id, 0});
          if (outcome.crateId) {
            sendPropertyChanged(outcome.crateId, PropertyChangedPayload{0, before.owner_character_id, 0, 0});
          }
        }
        else {
          sendPropertyChanged(id, PropertyChangedPayload{before.owner_character_id, before.owner_character_id, before.guild_id, before.guild_id});
        }
        for (uint32 holder : outcome.itemListsChanged) {
          sendEvent(EventType::itemListChanged, holder, {});
        }
      }
      if (ownershipChanged) {
        balanceNeighborhoods();
      }
    }
  }

  bool startUpkeep() {
    try {
      validateUpkeepClock(RuleI(Housing, UpkeepIntervalSeconds), RuleI(Housing, UpkeepJitterSeconds));
      if (RuleI(Housing, UnpaidUpkeepEvictionDays) < 0) {
        throw std::invalid_argument(fmt::format("Housing:UnpaidUpkeepEvictionDays is [{}]; it must be 0 (never) or more", RuleI(Housing, UnpaidUpkeepEvictionDays)));
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: {}", error.what());
      return false;
    }
    return true;
  }

  void processUpkeep() {
    if (!RuleB(Housing, EnableHousing) || !RuleB(Housing, EnableUpkeep)) {
      return;
    }
    if (!passTimer.Enabled()) {
      passTimer.Start(passMilliseconds);
      chargeDueUpkeep();
      return;
    }
    if (passTimer.Check()) {
      chargeDueUpkeep();
    }
  }
}
