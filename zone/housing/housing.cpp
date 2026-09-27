#include "housing.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingItems.h"
#include "housingItemTail.h"
#include "housingMessages.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/rulesys.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../zone.h"
#include "../zonedb.h"

#include <algorithm>
#include <unordered_map>

extern Zone *zone;

namespace housing {
  namespace {
    std::unordered_map<Client *, Session> sessions;
    bool catalogReady = false;

    bool servesClient(Client *client) {
      return housingActive() && client && client->ClientVersion() == EQ::versions::ClientVersion::RoF2;
    }

    std::set<uint32> carriedPropertyIds(uint32 characterId) {
      std::set<uint32> ids;
      auto rows = database.QueryDatabase(fmt::format(
        "SELECT custom_data FROM inventory WHERE character_id = {} AND custom_data LIKE '%{}^%'", characterId, propertyItemKey
      ));
      for (auto row : rows) {
        const uint32 propertyId = carriedPropertyId(row[0] ? row[0] : "");
        if (propertyId) {
          ids.insert(propertyId);
        }
      }
      return ids;
    }

    void addTrees(std::map<uint32, Property> &properties) {
      std::set<uint32> requested;
      while (true) {
        std::set<uint32> missing;
        for (const auto &[id, property] : properties) {
          if (property.parent_id && !properties.count(property.parent_id) && !requested.count(property.parent_id)) {
            missing.insert(property.parent_id);
          }
        }
        if (missing.empty()) {
          break;
        }
        requested.insert(missing.begin(), missing.end());
        for (auto &property : propertiesById(missing)) {
          properties[property.id] = property;
        }
      }
      for (uint32 id : requested) {
        if (!properties.count(id)) {
          LogError("Housing: a property names parent [{}], which does not exist", id);
        }
      }
      std::set<uint32> frontier;
      for (const auto &[id, property] : properties) {
        frontier.insert(id);
      }
      while (!frontier.empty()) {
        std::set<uint32> next;
        for (auto &child : propertiesChildOf(frontier)) {
          if (!properties.count(child.id)) {
            properties[child.id] = child;
            next.insert(child.id);
          }
        }
        frontier = next;
      }
    }

    int depthOf(const Property &property, const std::map<uint32, Property> &properties) {
      int depth = 0;
      for (uint32 parent = property.parent_id; parent && depth < 32; depth++) {
        auto found = properties.find(parent);
        parent = found == properties.end() ? 0 : found->second.parent_id;
      }
      return depth;
    }

    uint32 currentPropertyFor(const std::vector<Property> &hosted, const std::vector<Property> &interior) {
      if (!interior.empty()) {
        return interior.front().id;
      }
      std::set<uint32> hostedIds;
      for (const auto &property : hosted) {
        hostedIds.insert(property.id);
      }
      for (const auto &property : hosted) {
        if (!property.parent_id || !hostedIds.count(property.parent_id)) {
          const DefinitionType type = typeOf(property);
          if (type == DefinitionType::neighborhood || type == DefinitionType::town || type == DefinitionType::regularZone) {
            return property.id;
          }
        }
      }
      return 0;
    }

    void sendZoneIn(Client *client, Session &state) {
      sendMessage(client, OP_RealEstateDefinitions, catalog().message());

      const auto hosted = propertiesHostedAt(zone->GetZoneID(), zone->GetInstanceID());
      const auto interior = propertiesWithInterior(zone->GetInstanceID());
      const auto owned = propertiesOwnedBy(client->CharacterID());
      std::vector<Property> carried;
      for (auto &property : propertiesById(carriedPropertyIds(client->CharacterID()))) {
        if (property.contained_in_item) {
          carried.push_back(property);
        }
      }

      const std::vector<const std::vector<Property> *> groups = {&hosted, &interior, &owned, &carried};
      state.properties.clear();
      for (const auto *group : groups) {
        for (const auto &property : *group) {
          state.properties[property.id] = property;
        }
      }
      addTrees(state.properties);

      std::set<uint32> ids;
      std::set<uint32> owners;
      std::map<uint32, std::vector<uint32>> children;
      for (const auto &[id, property] : state.properties) {
        ids.insert(id);
        owners.insert(property.owner_character_id);
        if (property.parent_id) {
          children[property.parent_id].push_back(id);
        }
      }
      const auto names = characterNames(owners);
      const auto money = balances(ids);
      std::vector<const Property *> ordered;
      for (const auto &[id, property] : state.properties) {
        ordered.push_back(&property);
      }
      std::stable_sort(ordered.begin(), ordered.end(), [&](const Property *a, const Property *b) {
        return depthOf(*a, state.properties) < depthOf(*b, state.properties);
      });
      auto upkeepOptionKey = [&](const Property &property) -> uint32 {
        const Definition *definition = catalog().definition(property.definition_id);
        if (!definition) {
          LogError("Housing: property [{}] names unknown definition [{}]", property.id, property.definition_id);
          return 0;
        }
        if (!definition->row.upkeep_cost_id) {
          return 0;
        }
        const CostOption *option = catalog().option(definition->row.upkeep_cost_id, property.upkeep_currency);
        if (!option) {
          LogError("Housing: property [{}] pays upkeep in currency [{}], which its upkeep cost [{}] doesn't offer", property.id, property.upkeep_currency, definition->row.upkeep_cost_id);
          return 0;
        }
        return option->key;
      };
      auto recordOf = [&](const Property &property) {
        auto name = names.find(property.owner_character_id);
        auto propertyMoney = money.find(property.id);
        return propertyRecord(property, activeGuildId(property), children[property.id], name == names.end() ? "" : name->second, propertyMoney == money.end() ? std::vector<Balance>{} : propertyMoney->second, upkeepOptionKey(property));
      };
      for (const Property *property : ordered) {
        sendMessage(client, OP_RealEstateProperty, recordOf(*property));
      }

      state.currentPropertyId = currentPropertyFor(hosted, interior);
      if (state.currentPropertyId) {
        sendMessage(client, OP_RealEstateCurrentProperty, recordOf(state.properties.at(state.currentPropertyId)));
      }

      std::vector<uint32> listA;
      std::vector<uint32> listC;
      for (const auto &property : owned) {
        listA.push_back(property.id);
        if (property.archived) {
          listC.push_back(property.id);
        }
      }
      std::vector<uint32> listB;
      for (const auto &property : carried) {
        listB.push_back(property.id);
      }
      sendMessage(client, OP_RealEstateOwnedProperties, propertyIdList(listA));
      sendMessage(client, OP_RealEstateItemOwnedProperties, propertyIdList(listB));
      sendMessage(client, OP_RealEstateArchivedProperties, propertyIdList(listC));

      std::set<uint32> regionIds;
      for (const auto &property : hosted) {
        const Definition *definition = catalog().definition(property.definition_id);
        if (definition && definition->row.region_id) {
          regionIds.insert(definition->row.region_id);
        }
      }
      std::vector<RealEstateRegionsRepository::RealEstateRegions> regions;
      for (uint32 id : regionIds) {
        regions.push_back(*catalog().region(id));
      }
      sendMessage(client, OP_RealEstateRegions, regionList(regions));

      std::set<uint32> itemHolders;
      for (const auto *group : groups) {
        for (const auto &property : *group) {
          itemHolders.insert(property.id);
        }
      }
      const auto items = itemsOf(itemHolders);
      std::set<uint32> itemOwners;
      for (const auto &item : items) {
        itemOwners.insert(item.owner_character_id);
      }
      const auto itemOwnerNames = characterNames(itemOwners);
      for (uint32 id : itemHolders) {
        sendMessage(client, OP_RealEstateItemList, itemList(id, items, itemOwnerNames));
      }

      std::set<uint32> plots;
      for (const auto &[id, property] : state.properties) {
        const DefinitionType type = typeOf(property);
        if (type == DefinitionType::plot || type == DefinitionType::guildPlot) {
          plots.insert(id);
        }
      }
      state.accessLists = accessLists(plots);
      for (const auto &[id, list] : state.accessLists) {
        sendMessage(client, OP_RealEstateAccessList, accessListMessage(id, list));
      }
      LogHousing("Zone-in for [{}]: [{}] properties, current [{}], [{}] regions, [{}] item lists, [{}] access lists",
        client->GetCleanName(), state.properties.size(), state.currentPropertyId, regions.size(), itemHolders.size(), state.accessLists.size());
    }
  }

  PropertyLookup Session::lookup() const {
    return [this](uint32 id) -> const Property * {
      auto found = properties.find(id);
      return found == properties.end() ? nullptr : &found->second;
    };
  }

  AccessListLookup Session::accessLookup() const {
    return [this](uint32 id) -> const AccessList * {
      auto found = accessLists.find(id);
      return found == accessLists.end() ? nullptr : &found->second;
    };
  }

  bool housingActive() {
    return catalogReady && RuleB(Housing, EnableHousing);
  }

  bool reloadCatalog() {
    catalogReady = catalog().load();
    if (!catalogReady) {
      return false;
    }
    for (auto &[client, state] : sessions) {
      sendZoneIn(client, state);
    }
    return true;
  }

  Session *session(Client *client) {
    auto found = sessions.find(client);
    return found == sessions.end() ? nullptr : &found->second;
  }

  void onZoneBoot() {
    if (!RuleB(Housing, EnableHousing)) {
      LogHousing("Player housing is disabled by rule Housing:EnableHousing");
      return;
    }
    loadItemHousingTails();
    catalogReady = catalog().load();
  }

  void onZoneIn(Client *client) {
    if (!servesClient(client)) {
      return;
    }
    Session &state = sessions[client];
    state = Session{};
    sendZoneIn(client, state);
  }

  void onDisconnect(Client *client) {
    sessions.erase(client);
  }
}
