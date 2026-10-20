#include "housingNeighborhoods.h"

#include "wguild_mgr.h"
#include "housingInstances.h"
#include "worlddb.h"
#include "zonelist.h"

#include "../common/eqemu_logsys.h"
#include "../common/housingCopies.h"
#include "../common/housingEventTypes.h"
#include "../common/housingLedger.h"
#include "../common/housingNames.h"
#include "../common/random.h"
#include "../common/rulesys.h"
#include "../common/servertalk.h"
#include "../common/strings.h"

#include <cstring>
#include <memory>
#include <tuple>
#include <stdexcept>

namespace housing {
  namespace {
    constexpr int maximumCreationsPerBalance = 1000;

    EQ::Random random;

    struct RandomNeighborhood {
      uint32 definitionId;
      uint32 playerPlots;
      uint32 ownedPlayerPlots;
    };

    struct NeighborhoodRules {
      std::vector<uint32> definitions;
      int minimum;
      int minimumVacant;
      int growthPercent;
    };

    NeighborhoodRules checkedRules() {
      NeighborhoodRules rules{
        definitionList(RuleS(Housing, RandomNeighborhoodDefinitions), "Housing:RandomNeighborhoodDefinitions"),
        RuleI(Housing, MinimumRandomNeighborhoods),
        RuleI(Housing, MinimumVacantNeighborhoodsPerDefinition),
        RuleI(Housing, NeighborhoodGrowthOccupancyPercent)
      };
      if (rules.minimum < 0) {
        throw std::invalid_argument(fmt::format("Housing:MinimumRandomNeighborhoods is [{}]; it must be 0 or more", rules.minimum));
      }
      if (rules.minimumVacant < 0) {
        throw std::invalid_argument(fmt::format("Housing:MinimumVacantNeighborhoodsPerDefinition is [{}]; it must be 0 or more", rules.minimumVacant));
      }
      if (rules.growthPercent < 1 || rules.growthPercent > 100) {
        throw std::invalid_argument(fmt::format("Housing:NeighborhoodGrowthOccupancyPercent is [{}]; it must be 1 to 100", rules.growthPercent));
      }
      if (rules.definitions.empty() && (rules.minimum > 0 || rules.minimumVacant > 0)) {
        throw std::invalid_argument("Housing:RandomNeighborhoodDefinitions lists no definition, so no neighborhood can be made");
      }
      const std::set<uint32> distinct(rules.definitions.begin(), rules.definitions.end());
      const auto types = definitionTypes(content_db, distinct);
      for (uint32 id : distinct) {
        if (types.at(id) != neighborhoodType) {
          throw std::invalid_argument(fmt::format("Housing:RandomNeighborhoodDefinitions lists definition [{}], which is not a neighborhood", id));
        }
        auto rootParents = query(content_db, fmt::format(
          "SELECT COUNT(*) FROM real_estate_definition_children c JOIN real_estate_definitions d ON d.id = c.parent_id WHERE c.child_id = {} AND d.type = {}",
          id, regularZoneType
        ));
        if (Strings::ToUnsignedInt(rootParents.begin()[0]) > 0) {
          throw std::invalid_argument(fmt::format("Housing:RandomNeighborhoodDefinitions lists definition [{}], which belongs under a zone root", id));
        }
        auto plots = query(content_db, fmt::format(
          "SELECT COUNT(*) FROM real_estate_definition_children c JOIN real_estate_definitions d ON d.id = c.child_id WHERE c.parent_id = {} AND d.type = {}",
          id, plotType
        ));
        if (Strings::ToUnsignedInt(plots.begin()[0]) == 0 && rules.minimumVacant > 0) {
          throw std::invalid_argument(fmt::format("definition [{}] holds no player plot, so it can never have a vacant one", id));
        }
      }
      return rules;
    }

    // Random neighborhoods are the public ones at the top level; guild villages count toward none of D11's three conditions.
    std::vector<RandomNeighborhood> randomNeighborhoods() {
      std::map<uint32, uint32> definitionOf;
      std::set<uint32> definitionIds;
      for (auto row : query(database, "SELECT id, definition_id FROM real_estate_properties WHERE parent_id = 0 AND guild_id = 0 AND archived = 0")) {
        definitionOf[Strings::ToUnsignedInt(row[0])] = Strings::ToUnsignedInt(row[1]);
        definitionIds.insert(Strings::ToUnsignedInt(row[1]));
      }
      const auto types = definitionTypes(content_db, definitionIds);
      std::map<uint32, RandomNeighborhood> neighborhoods;
      for (const auto &[id, definition] : definitionOf) {
        if (types.at(definition) == neighborhoodType) {
          neighborhoods[id] = RandomNeighborhood{definition, 0, 0};
        }
      }
      if (neighborhoods.empty()) {
        return {};
      }
      std::set<uint32> ids;
      for (const auto &[id, neighborhood] : neighborhoods) {
        ids.insert(id);
      }
      std::set<uint32> childDefinitions;
      std::vector<std::tuple<uint32, uint32, uint32>> children;
      for (auto row : query(database, fmt::format("SELECT parent_id, definition_id, owner_character_id FROM real_estate_properties WHERE parent_id IN ({})", idList(ids)))) {
        children.emplace_back(Strings::ToUnsignedInt(row[0]), Strings::ToUnsignedInt(row[1]), Strings::ToUnsignedInt(row[2]));
        childDefinitions.insert(Strings::ToUnsignedInt(row[1]));
      }
      const auto childTypes = definitionTypes(content_db, childDefinitions);
      for (const auto &[parent, definition, owner] : children) {
        if (childTypes.at(definition) == plotType) {
          neighborhoods[parent].playerPlots++;
          neighborhoods[parent].ownedPlayerPlots += owner ? 1 : 0;
        }
      }
      std::vector<RandomNeighborhood> result;
      for (const auto &[id, neighborhood] : neighborhoods) {
        result.push_back(neighborhood);
      }
      return result;
    }

    std::set<std::string> namesInUse() {
      std::set<std::string> names;
      for (auto row : query(database, "SELECT name FROM real_estate_properties WHERE name <> ''")) {
        names.insert(row[0]);
      }
      return names;
    }

    NeighborhoodWords loadWords() {
      NeighborhoodWords words;
      for (auto row : query(content_db, "SELECT position, word FROM real_estate_neighborhood_words ORDER BY position, word")) {
        switch (Strings::ToUnsignedInt(row[0])) {
          case 0: words.trees.push_back(row[1]); break;
          case 1: words.middles.push_back(row[1]); break;
          case 2: words.places.push_back(row[1]); break;
          default: throw std::runtime_error(fmt::format("neighborhood word [{}] has position [{}]; positions are 0 to 2", row[1], row[0]));
        }
      }
      return words;
    }

    std::string newName(uint32 guildId) {
      const auto taken = namesInUse();
      if (guildId) {
        std::string guildName;
        if (!guild_mgr.GetGuildNameByID(guildId, guildName)) {
          throw std::runtime_error(fmt::format("guild [{}] does not exist", guildId));
        }
        return villageName(guildName, taken);
      }
      const auto free = freeNeighborhoodNames(loadWords(), taken);
      if (free.empty()) {
        throw std::runtime_error("every neighborhood name the word table makes is in use");
      }
      return free[static_cast<size_t>(random.Int(0, static_cast<int>(free.size()) - 1))];
    }

    uint32 pick(const std::vector<uint32> &definitions) {
      return definitions[static_cast<size_t>(random.Int(0, static_cast<int>(definitions.size()) - 1))];
    }

    // The neighborhood the balance needs next, or 0 once D11's three conditions hold.
    uint32 neededDefinition(const NeighborhoodRules &rules) {
      const auto neighborhoods = randomNeighborhoods();
      if (neighborhoods.size() < static_cast<size_t>(rules.minimum)) {
        return pick(rules.definitions);
      }
      const std::set<uint32> distinct(rules.definitions.begin(), rules.definitions.end());
      for (uint32 definition : distinct) {
        int vacant = 0;
        for (const auto &neighborhood : neighborhoods) {
          vacant += neighborhood.definitionId == definition && neighborhood.ownedPlayerPlots < neighborhood.playerPlots ? 1 : 0;
        }
        if (vacant < rules.minimumVacant) {
          return definition;
        }
      }
      uint64 plots = 0;
      uint64 owned = 0;
      for (const auto &neighborhood : neighborhoods) {
        plots += neighborhood.playerPlots;
        owned += neighborhood.ownedPlayerPlots;
      }
      if (!rules.definitions.empty() && plots && owned * 100 >= plots * static_cast<uint64>(rules.growthPercent)) {
        return pick(rules.definitions);
      }
      return 0;
    }
  }

  // A neighborhood is rows only: it runs in copies of its template started on entry (D3), like an interior.
  uint32 createNeighborhood(uint32 definitionId, uint32 guildId) {
    if (definitionTypes(content_db, {definitionId}).at(definitionId) != neighborhoodType) {
      throw std::invalid_argument(fmt::format("definition [{}] is not a neighborhood", definitionId));
    }
    zoneTemplateOf(content_db, definitionId);
    const std::string name = newName(guildId);
    const uint32 neighborhoodId = createPropertyTree(definitionId, guildId, name);
    LogHousing("Created neighborhood [{}] [{}] of definition [{}]{}", neighborhoodId, name, definitionId,
      guildId ? fmt::format(" for guild [{}]", guildId) : std::string());
    return neighborhoodId;
  }

  // A writ's village (EXT-33), announced by 0x1d3c to the guild's members only.
  void createGuildNeighborhood(const WritUsedPayload &writ) {
    try {
      const uint32 neighborhoodId = createNeighborhood(writ.definitionId, writ.guildId);
      auto name = query(database, fmt::format("SELECT name FROM real_estate_properties WHERE id = {}", neighborhoodId));
      GuildNeighborhoodCreatedPayload created{writ.guildId, {}, {}};
      strncpy(created.characterName, writ.characterName, sizeof(created.characterName) - 1);
      strncpy(created.neighborhoodName, name.begin()[0], sizeof(created.neighborhoodName) - 1);
      auto announcement = std::make_unique<ServerPacket>(ServerOP_Housing, static_cast<uint32>(sizeof(ServerHousingEvent_Struct) + sizeof(created)));
      auto *event = reinterpret_cast<ServerHousingEvent_Struct *>(announcement->pBuffer);
      event->event_type = static_cast<uint32>(EventType::guildNeighborhoodCreated);
      event->target_kind = static_cast<uint32>(EventTarget::guild);
      event->target_id = writ.guildId;
      event->origin_zone_id = 0;
      event->origin_instance_id = 0;
      event->property_id = neighborhoodId;
      event->revision = 0;
      event->payload_size = sizeof(created);
      memcpy(event->payload, &created, sizeof(created));
      ZSList::Instance()->SendPacketToBootedZones(announcement.get());
    }
    catch (const std::exception &error) {
      LogError("Housing: character [{}]'s writ for definition [{}] made no neighborhood for guild [{}]: {}", writ.characterId, writ.definitionId, writ.guildId, error.what());
    }
  }

  void balanceNeighborhoods() {
    if (!RuleB(Housing, EnableHousing)) {
      return;
    }
    try {
      const NeighborhoodRules rules = checkedRules();
      for (int created = 0;; created++) {
        const uint32 definition = neededDefinition(rules);
        if (!definition) {
          return;
        }
        if (created >= maximumCreationsPerBalance) {
          throw std::logic_error(fmt::format("the neighborhood balance created [{}] neighborhoods and still needs more", created));
        }
        createNeighborhood(definition, 0);
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: random neighborhoods are not balanced: {}", error.what());
    }
  }

  void onHousingEvent(const ServerPacket *pack) {
    if (pack->size < sizeof(ServerHousingEvent_Struct)) {
      return;
    }
    const auto *event = reinterpret_cast<const ServerHousingEvent_Struct *>(pack->pBuffer);
    if (pack->size != sizeof(ServerHousingEvent_Struct) + event->payload_size) {
      return;
    }
    if (static_cast<EventType>(event->event_type) == EventType::writUsed && event->payload_size == sizeof(WritUsedPayload)) {
      WritUsedPayload writ;
      memcpy(&writ, event->payload, sizeof(writ));
      createGuildNeighborhood(writ);
      return;
    }
    if (static_cast<EventType>(event->event_type) != EventType::propertyChanged || event->payload_size != sizeof(PropertyChangedPayload)) {
      return;
    }
    PropertyChangedPayload owners;
    memcpy(&owners, event->payload, sizeof(owners));
    if (owners.previousOwnerCharacterId != owners.ownerCharacterId || owners.previousGuildId != owners.guildId) {
      balanceNeighborhoods();
    }
  }
}
