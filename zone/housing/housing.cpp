#include "housing.h"

#include "housingAccess.h"
#include "housingAnchors.h"
#include "housingCatalog.h"
#include "housingContainers.h"
#include "housingGuildServices.h"
#include "housingWrits.h"
#include "housingHandlers.h"
#include "housingInteriors.h"
#include "housingItems.h"
#include "housingItemTail.h"
#include "housingMenus.h"
#include "housingMessages.h"
#include "housingNpcItems.h"
#include "housingPlacement.h"
#include "housingPolls.h"
#include "housingSession.h"
#include "housingWorldObjects.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingChildOrder.h"
#include "../../common/housingCopies.h"
#include "../../common/housingLedger.h"
#include "../../common/housingRelatedProperties.h"
#include "../../common/repositories/real_estate_items_repository.h"
#include "../../common/rulesys.h"
#include "../../common/strings.h"
#include "../../common/zone_store.h"
#include "../client.h"
#include "../entity.h"
#include "../zone.h"
#include "../zonedb.h"

#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <unordered_map>

extern EntityList entity_list;
extern Zone *zone;

namespace housing {
  namespace {
    constexpr int maximumDepth = 10;

    std::unordered_map<Client *, Session> sessions;
    bool catalogReady = false;
    Timer yardTimer(1000);

    bool isAnyType(DefinitionType type, std::initializer_list<DefinitionType> types) {
      return std::find(types.begin(), types.end(), type) != types.end();
    }

    void addTrees(std::map<uint32, Property> &properties, std::vector<std::string> &problems) {
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
          problems.push_back(fmt::format("a property names parent [{}], which does not exist", id));
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

    std::vector<const Property *> ancestry(const Property &property, const std::map<uint32, Property> &properties, std::vector<std::string> &problems) {
      std::vector<const Property *> chain{&property};
      while (chain.back()->parent_id) {
        if (chain.size() > maximumDepth) {
          problems.push_back(fmt::format("property [{}] has an ancestor chain deeper than [{}]", property.id, maximumDepth));
          return {};
        }
        auto parent = properties.find(chain.back()->parent_id);
        if (parent == properties.end()) {
          return {};
        }
        chain.push_back(&parent->second);
      }
      return chain;
    }

    std::string composedName(const Property &property, const std::map<uint32, Property> &properties, std::vector<std::string> &problems) {
      std::vector<std::string> parts;
      const auto chain = ancestry(property, properties, problems);
      for (auto link = chain.rbegin(); link != chain.rend(); ++link) {
        const Property &current = **link;
        const Definition *definition = catalog().definition(current.definition_id);
        if (!definition) {
          return {};
        }
        switch (static_cast<DefinitionType>(definition->row.type)) {
          case DefinitionType::regularZone: {
            const ZoneTemplate *zoneTemplate = catalog().zoneTemplate(definition->row.dz_id);
            const char *longName = zoneTemplate ? ZoneLongName(zoneTemplate->zone_id) : nullptr;
            if (!longName) {
              problems.push_back(fmt::format("zone root [{}]'s definition [{}] names no zone with a zone row", current.id, current.definition_id));
              return {};
            }
            parts.push_back(longName);
            break;
          }
          case DefinitionType::neighborhood:
            if (current.name.empty()) {
              problems.push_back(fmt::format("neighborhood [{}] has no name", current.id));
              return {};
            }
            parts = {current.name};
            break;
          case DefinitionType::plot:
          case DefinitionType::guildPlot:
            parts.push_back(definition->row.address);
            break;
          case DefinitionType::guildHall:
          case DefinitionType::playerHouse:
          case DefinitionType::movingCrate:
            parts.push_back(definition->row.name_segment);
            break;
          default:
            problems.push_back(fmt::format("property [{}] is of definition type [{}], which has no name rule", current.id, definition->row.type));
            return {};
        }
      }
      return Strings::Join(parts, ", ");
    }

    uint32 upkeepOptionKey(const Property &property, std::vector<std::string> &problems) {
      const Definition *definition = catalog().definition(property.definition_id);
      if (!definition || !definition->row.upkeep_definition_id) {
        return 0;
      }
      const CostOption *option = catalog().option(definition->row.upkeep_definition_id, property.upkeep_currency);
      if (!option) {
        problems.push_back(fmt::format("property [{}] pays upkeep in currency [{}], which its upkeep cost [{}] doesn't offer",
          property.id, property.upkeep_currency, definition->row.upkeep_definition_id));
        return 0;
      }
      return option->key;
    }

    // The current property is the root this zone runs: a zone root, a neighborhood, or the house or hall inside an interior.
    uint32 currentPropertyFor(const std::vector<Property> &hosted, std::vector<std::string> &problems) {
      std::set<uint32> areas;
      for (const auto &property : hosted) {
        const DefinitionType type = typeOf(property);
        if (isAnyType(type, {DefinitionType::plot, DefinitionType::guildPlot})) {
          const uint32 area = catalog().definition(property.definition_id)->row.area_id;
          if (area && !areas.insert(area).second) {
            problems.push_back(fmt::format("two plots hosted here own region [{}]", area));
          }
        }
      }
      const auto root = hostedRoot();
      return root ? root->id : 0;
    }

    std::map<uint32, RecordFields> recordFieldsFor(const std::map<uint32, Property> &properties, std::vector<std::string> &problems) {
      std::set<uint32> ids;
      std::set<uint32> owners;
      std::map<uint32, std::vector<uint32>> children;
      for (const auto &[id, property] : properties) {
        ids.insert(id);
        owners.insert(property.owner_character_id);
        if (property.parent_id) {
          children[property.parent_id].push_back(id);
        }
      }
      std::set<uint32> placedCarriers;
      if (!ids.empty()) {
        for (const auto &row : RealEstateItemsRepository::GetWhere(database, fmt::format("placed = 1 AND carried_property_id IN ({})", idList(ids)))) {
          placedCarriers.insert(row.carried_property_id);
        }
      }
      for (auto &[parentId, childIds] : children) {
        childIds = inClientOrder(childIds, placedCarriers);
      }
      const auto identities = characterIdentities(owners);
      auto money = balances(ids);
      std::map<uint32, RecordFields> fields;
      for (const auto &[id, property] : properties) {
        std::optional<CharacterIdentity> owner;
        if (property.owner_character_id) {
          auto found = identities.find(property.owner_character_id);
          if (found == identities.end()) {
            problems.push_back(fmt::format("property [{}] is owned by character [{}], which does not exist", id, property.owner_character_id));
            continue;
          }
          owner = found->second;
        }
        const bool playerPlot = typeOf(property) == DefinitionType::plot;
        fields[id] = RecordFields{
          playerPlot && !property.owner_character_id,
          recordGuildOf(property, owner),
          owner,
          composedName(property, properties, problems),
          children[id],
          money[id],
          upkeepOptionKey(property, problems)
        };
      }
      return fields;
    }

    std::set<uint32> listHoldersOf(const Session &state) {
      std::set<uint32> holders;
      for (const auto &[id, property] : state.properties) {
        if (holdsAccessList(typeOf(property))) {
          holders.insert(id);
        }
      }
      return holders;
    }

    // The current property's line goes out as records alone. A client inside a house, or standing in a yard, gets that plot whole,
    // records and lists, as Live sends a stranger there the plot, its houses and its crate (AUD-8, AUD-61).
    void setClientView(Client *client, Session &state) {
      const PropertyLookup lookup = state.lookup();
      const AccessListLookup listOf = state.accessLookup();
      std::set<uint32> seeds(state.ownedIds.begin(), state.ownedIds.end());
      seeds.insert(state.carriedIds.begin(), state.carriedIds.end());
      std::map<uint32, PropertyLink> links;
      for (const auto &[id, property] : state.properties) {
        const DefinitionType type = typeOf(property);
        links[id] = PropertyLink{property.parent_id, type == DefinitionType::neighborhood || type == DefinitionType::town || type == DefinitionType::regularZone};
        const uint32 guild = activeGuildId(property);
        if ((guild && guild == client->GuildID()) || (holdsAccessList(type) && accessGroup(client, property, lookup, listOf))) {
          seeds.insert(id);
        }
      }
      if (hostsInterior()) {
        const uint32 plotTypes = typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot);
        if (const Property *visitedPlot = ancestorOfType(state.properties.at(hostedRoot()->id), plotTypes, true, lookup)) {
          seeds.insert(visitedPlot->id);
        }
      }
      if (state.visitedYardId && state.properties.count(state.visitedYardId)) {
        seeds.insert(state.visitedYardId);
      }
      state.relatedIds = relatedProperties(links, seeds, maximumDepth);
      state.knownIds = state.relatedIds;
      const auto currentLine = propertyLine(links, state.currentPropertyId, maximumDepth);
      state.knownIds.insert(currentLine.begin(), currentLine.end());
    }

    bool buildSession(Client *client, Session &state, std::map<uint32, RecordFields> &fields, const std::set<uint32> &alsoKnown, uint32 keptCurrentId) {
      std::vector<std::string> problems;
      try {
        const auto hosted = hostedProperties();
        const auto owned = propertiesOwnedBy(client->CharacterID());
        const auto carried = propertiesById(carriedPropertyIds(database, client->CharacterID()));
        for (const auto &property : hosted) {
          state.hostedIds.insert(property.id);
          state.properties[property.id] = property;
        }
        for (const auto &property : owned) {
          state.ownedIds.push_back(property.id);
          state.properties[property.id] = property;
        }
        for (const auto &property : carried) {
          state.carriedIds.push_back(property.id);
          state.properties[property.id] = property;
        }
        for (auto &property : propertiesById(alsoKnown)) {
          state.properties[property.id] = property;
        }
        addTrees(state.properties, problems);
        for (const auto &[id, property] : state.properties) {
          if (!catalog().definition(property.definition_id)) {
            problems.push_back(fmt::format("property [{}] names unknown definition [{}]", id, property.definition_id));
          }
        }
        if (problems.empty()) {
          state.currentPropertyId = currentPropertyFor(hosted, problems);
          if (keptCurrentId && state.properties.count(keptCurrentId)) {
            state.currentPropertyId = keptCurrentId;
          }
          fields = recordFieldsFor(state.properties, problems);
          state.accessLists = accessLists(listHoldersOf(state));
          setClientView(client, state);
        }
      }
      catch (const std::exception &error) {
        problems.push_back(error.what());
      }
      if (!problems.empty()) {
        for (const auto &problem : problems) {
          LogError("Housing: [{}]: {}", client->GetCleanName(), problem);
        }
        LogError("Housing: no housing data is sent to [{}] in this zone", client->GetCleanName());
        return false;
      }
      return true;
    }

    std::vector<const Property *> parentsFirst(const Session &state) {
      std::vector<std::string> problems;
      std::map<uint32, size_t> depths;
      std::vector<const Property *> ordered;
      for (const auto &[id, property] : state.properties) {
        depths[id] = ancestry(property, state.properties, problems).size();
        ordered.push_back(&property);
      }
      std::stable_sort(ordered.begin(), ordered.end(), [&](const Property *a, const Property *b) {
        return depths[a->id] < depths[b->id];
      });
      return ordered;
    }

    void sendPropertyLists(Client *client, const Session &state) {
      std::vector<uint32> listC;
      for (uint32 id : state.ownedIds) {
        if (state.properties.at(id).archived) {
          listC.push_back(id);
        }
      }
      sendMessage(client, OP_RealEstateOwnedProperties, propertyIdList(state.ownedIds));
      sendMessage(client, OP_RealEstateItemOwnedProperties, propertyIdList(state.carriedIds));
      sendMessage(client, OP_RealEstateArchivedProperties, propertyIdList(listC));
    }

    void sendItemLists(Client *client, const std::set<uint32> &holders) {
      const auto items = itemsOf(holders);
      const auto instances = itemInstancesOf(holders);
      std::set<uint32> itemOwners;
      for (const auto &item : items) {
        itemOwners.insert(item.owner_character_id);
      }
      const auto itemOwnerIdentities = characterIdentities(itemOwners);
      for (uint32 id : holders) {
        sendMessage(client, OP_RealEstateItemList, itemList(id, items, instances, itemOwnerIdentities));
      }
    }

    uint32 yardUnder(Client *client, const Session &state) {
      if (hostsInterior()) {
        return 0;
      }
      const uint32 standing = standingProperty(client, state);
      auto found = state.properties.find(standing);
      if (found == state.properties.end()) {
        return 0;
      }
      const DefinitionType type = typeOf(found->second);
      return type == DefinitionType::plot || type == DefinitionType::guildPlot ? standing : 0;
    }

    bool isUnder(const Session &state, uint32 id, uint32 ancestorId) {
      for (int depth = 0; id && depth <= maximumDepth; depth++) {
        if (id == ancestorId) {
          return true;
        }
        auto found = state.properties.find(id);
        if (found == state.properties.end()) {
          return false;
        }
        id = found->second.parent_id;
      }
      return false;
    }

    // Live sends whoever walks into a yard the plot, its houses and its crate with their access lists and item lists, and the
    // client holds none of them once the player has walked out, unless something else relates the player to them (AUD-61).
    void changeVisitedYard(Client *client, Session &state, uint32 yardId) {
      const uint32 previousYardId = state.visitedYardId;
      const std::set<uint32> relatedBefore = state.relatedIds;
      const std::set<uint32> knownBefore = state.knownIds;
      state.visitedYardId = yardId;
      setClientView(client, state);
      std::set<uint32> arrived;
      for (uint32 id : state.relatedIds) {
        if (!relatedBefore.count(id)) {
          arrived.insert(id);
        }
      }
      std::set<uint32> left;
      for (uint32 id : relatedBefore) {
        if (!state.relatedIds.count(id) && previousYardId && isUnder(state, id, previousYardId)) {
          left.insert(id);
        }
      }
      for (uint32 id : knownBefore) {
        if (!left.count(id)) {
          state.knownIds.insert(id);
        }
      }
      const auto ordered = parentsFirst(state);
      if (!arrived.empty()) {
        std::vector<std::string> problems;
        const auto fields = recordFieldsFor(state.properties, problems);
        if (!problems.empty()) {
          throw std::runtime_error(Strings::Join(problems, "; "));
        }
        for (const Property *property : ordered) {
          if (arrived.count(property->id)) {
            sendMessage(client, OP_RealEstatePropertyUpdate, propertyRecord(*property, fields.at(property->id)));
          }
        }
        for (uint32 id : arrived) {
          auto list = state.accessLists.find(id);
          if (list != state.accessLists.end()) {
            sendMessage(client, OP_RealEstateAccessList, accessListMessage(id, list->second));
          }
        }
        sendItemLists(client, arrived);
      }
      for (auto property = ordered.rbegin(); property != ordered.rend(); ++property) {
        const uint32 id = (*property)->id;
        if (!left.count(id)) {
          continue;
        }
        WireWriter drop;
        drop.u32(id);
        sendMessage(client, OP_RealEstateItemListDrop, drop);
        if (state.accessLists.count(id)) {
          sendMessage(client, OP_RealEstateAccessListRemove, accessListRemoveMessage(id));
        }
        sendMessage(client, OP_RealEstateProperty, propertyRecord(**property, RecordFields{false, 0, std::nullopt, {}, {}, {}, 0}));
      }
      if (!arrived.empty() || !left.empty()) {
        LogHousing("[{}] stands in yard [{}] (was [{}]): sent properties [{}], took back [{}]", client->GetCleanName(), yardId, previousYardId,
          idList(arrived), idList(left));
      }
    }

    void processYards() {
      if (!yardTimer.Check()) {
        return;
      }
      for (const auto &[entityId, client] : entity_list.GetClientList()) {
        Session *state = session(client);
        if (!state) {
          continue;
        }
        const uint32 yardId = yardUnder(client, *state);
        if (yardId == state->visitedYardId) {
          continue;
        }
        try {
          changeVisitedYard(client, *state, yardId);
        }
        catch (const std::exception &error) {
          LogError("Housing: [{}]'s move to yard [{}] failed: {}", client->GetCleanName(), yardId, error.what());
        }
      }
    }

    // The Manage and Purchase windows work out daily cost and time left from the catalog's upkeep costs, so a zone sends them at the
    // percent world charges the plots it hosts (EXT-37); a zone hosting none has no host zone and sends them as defined.
    int catalogUpkeepPercent() {
      const auto hosted = hostedRoot();
      return hosted ? upkeepPercent(database, content_db, hostZoneOf(database, content_db, *hosted)) : 100;
    }

    void sendZoneContents(Client *client) {
      sessions.erase(client);
      if (!housingActive()) {
        return;
      }
      try {
        deliverPendingCrates(client);
      }
      catch (const std::exception &error) {
        LogError("Housing: pending crates for [{}] could not be delivered: {}", client->GetCleanName(), error.what());
      }
      int upkeepPercentHere = 0;
      try {
        // A copy of a house packed into a crate has no host zone to price by, and its occupants leave for bind on arrival (AUD-3).
        if (hostsInterior() && !hostedInteriorPlot()) {
          LogHousing("[{}] gets no housing data in a copy of property [{}], which belongs to no plot now", client->GetCleanName(), hostedRoot()->id);
          return;
        }
        upkeepPercentHere = catalogUpkeepPercent();
      }
      catch (const std::exception &error) {
        LogError("Housing: [{}]: {}", client->GetCleanName(), error.what());
        LogError("Housing: no housing data is sent to [{}] in this zone", client->GetCleanName());
        return;
      }
      Session state;
      std::map<uint32, RecordFields> fields;
      if (!buildSession(client, state, fields, {}, 0)) {
        return;
      }
      state.catalogGeneration = catalog().loadGeneration();
      sendMessage(client, OP_RealEstateDefinitions, catalog().message(upkeepPercentHere));
      for (const Property *property : parentsFirst(state)) {
        if (state.knownIds.count(property->id)) {
          sendMessage(client, OP_RealEstatePropertyUpdate, propertyRecord(*property, fields.at(property->id)));
        }
      }
      if (state.currentPropertyId) {
        sendMessage(client, OP_RealEstateCurrentProperty, propertyRecord(state.properties.at(state.currentPropertyId), fields.at(state.currentPropertyId)));
      }
      sessions[client] = std::move(state);
    }

    void sendZoneIn(Client *client) {
      Session *state = session(client);
      if (!state) {
        sendMessage(client, OP_RealEstateRegions, regionList({}));
        return;
      }
      sendPropertyLists(client, *state);

      std::vector<RealEstateRegionsRepository::RealEstateRegions> regions;
      for (uint32 id : state->hostedIds) {
        const uint32 area = catalog().definition(state->properties.at(id).definition_id)->row.area_id;
        if (area) {
          regions.push_back(*catalog().region(area));
        }
      }
      sendMessage(client, OP_RealEstateRegions, regionList(regions));

      sendItemLists(client, state->relatedIds);

      size_t accessListsSent = 0;
      for (const auto &[id, list] : state->accessLists) {
        if (state->relatedIds.count(id)) {
          sendMessage(client, OP_RealEstateAccessList, accessListMessage(id, list));
          accessListsSent++;
        }
      }
      sendWorldObjects(client, *state);
      sendPolls(client);
      LogHousing("Zone-in for [{}]: [{}] of [{}] properties sent, current [{}], [{}] regions, [{}] item lists, [{}] access lists",
        client->GetCleanName(), state->knownIds.size(), state->properties.size(), state->currentPropertyId, regions.size(), state->relatedIds.size(),
        accessListsSent);
    }

    // A copy discarded between a door's hand-out and the arrival leaves the player alone in a dead instance, which EQEmu would send
    // to bind only after 150 s while the next click starts a second copy (INT-33).
    bool arrivedInDiscardedCopy(Client *client) {
      const uint16 instanceId = static_cast<uint16>(zone->GetInstanceID());
      if (!instanceId || !catalogReady || !catalog().isZoneTemplate(zone->GetZoneID(), zone->GetInstanceVersion()) || database.CheckInstanceExists(instanceId)) {
        return false;
      }
      LogHousing("[{}] arrived in instance [{}], a discarded copy; they go to bind", client->GetCleanName(), instanceId);
      client->Message(Chat::Yellow, "Instance no longer exists...");
      client->GoToBind();
      return true;
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
    return catalogReady;
  }

  // Clients get the new catalog at their next zone-in, the only time Live sends one: RoF2's heap was corrupted by the catalog,
  // records and region list sent again while a placement was active (PLC-22).
  bool reloadCatalog() {
    catalogReady = catalog().load();
    if (!catalogReady) {
      sessions.clear();
      return false;
    }
    try {
      loadHostedRoot();
    }
    catch (const std::exception &error) {
      LogError("Housing: the property this zone runs could not be found after the catalog reload: {}", error.what());
      clearHostedRoot();
    }
    return true;
  }

  Session *session(Client *client) {
    auto found = sessions.find(client);
    return found == sessions.end() ? nullptr : &found->second;
  }

  bool onZoneBoot() {
    if (!EQ::loadItemHousingTails(content_db)) {
      return false;
    }
    catalogReady = catalog().load();
    if (catalogReady) {
      try {
        loadHostedRoot();
      }
      catch (const std::exception &error) {
        LogError("Housing: the property this zone runs could not be loaded: {}", error.what());
        return false;
      }
    }
    return true;
  }

  void onZoneShutdown() {
    discardHostedCopy();
    resetGuildServices();
    resetPolls();
    resetNpcItems();
    clearHostedRoot();
    sessions.clear();
  }

  void onZoneContents(Client *client) {
    sendZoneContents(client);
  }

  void onZoneIn(Client *client) {
    if (arrivedInDiscardedCopy(client)) {
      return;
    }
    sendZoneIn(client);
    if (housingActive()) {
      stripAuraBuffs(client);
      admitOccupant(client);
    }
  }

  void onDisconnect(Client *client) {
    closeContainerOf(client);
    forgetAnchorOffer(client);
    sessions.erase(client);
    // A copy's discard delay runs from its last occupant's leaving, a camp included (Live kept an empty house 30.8 minutes after a camp).
    if (zone && hostedRoot() && zone->GetInstanceID()) {
      zone->ResetShutdownTimer();
    }
  }

  void forEachSession(const std::function<void(Client *, Session &)> &visit) {
    for (const auto &[id, client] : entity_list.GetClientList()) {
      Session *state = session(client);
      if (state) {
        visit(client, *state);
      }
    }
  }

  void refreshClients(const std::set<uint32> &propertyIds, const std::set<uint32> &characterIds) {
    if (!housingActive()) {
      return;
    }
    std::set<uint32> existing;
    for (const auto &property : propertiesById(propertyIds)) {
      existing.insert(property.id);
    }
    for (const auto &[entityId, client] : entity_list.GetClientList()) {
      Session *old = session(client);
      if (!old) {
        continue;
      }
      bool concerned = characterIds.count(client->CharacterID()) > 0;
      for (uint32 id : propertyIds) {
        concerned = concerned || old->properties.count(id);
      }
      if (!concerned) {
        continue;
      }
      Session fresh;
      fresh.visitedYardId = old->visitedYardId;
      std::map<uint32, RecordFields> fields;
      std::set<uint32> alsoKnown(existing);
      alsoKnown.insert(old->knownIds.begin(), old->knownIds.end());
      if (!buildSession(client, fresh, fields, alsoKnown, old->currentPropertyId)) {
        sessions.erase(client);
        continue;
      }
      fresh.placement = old->placement;
      fresh.augmentaterMode = old->augmentaterMode;
      fresh.catalogGeneration = old->catalogGeneration;
      for (uint32 id : old->knownIds) {
        if (fresh.properties.count(id)) {
          fresh.knownIds.insert(id);
        }
      }
      std::set<uint32> newlyKnown;
      for (uint32 id : fresh.knownIds) {
        if (!old->knownIds.count(id)) {
          newlyKnown.insert(id);
        }
      }
      std::set<uint32> newlyRelated;
      for (uint32 id : fresh.relatedIds) {
        if (!old->relatedIds.count(id)) {
          newlyRelated.insert(id);
        }
      }
      for (const Property *property : parentsFirst(fresh)) {
        if (fresh.knownIds.count(property->id) && (propertyIds.count(property->id) || newlyKnown.count(property->id))) {
          sendMessage(client, OP_RealEstatePropertyUpdate, propertyRecord(*property, fields.at(property->id)));
        }
      }
      for (uint32 id : propertyIds) {
        if (!existing.count(id) && old->knownIds.count(id)) {
          sendMessage(client, OP_RealEstateProperty, propertyRecord(old->properties.at(id), RecordFields{false, 0, std::nullopt, {}, {}, {}, 0}));
        }
      }
      sendPropertyLists(client, fresh);
      for (const auto &[id, list] : fresh.accessLists) {
        const bool held = fresh.relatedIds.count(id) || old->relatedIds.count(id);
        if ((held && propertyIds.count(id)) || newlyRelated.count(id)) {
          sendMessage(client, OP_RealEstateAccessList, accessListMessage(id, list));
        }
      }
      for (uint32 id : propertyIds) {
        if (old->relatedIds.count(id) && old->accessLists.count(id) && !fresh.accessLists.count(id)) {
          sendMessage(client, OP_RealEstateAccessListRemove, accessListRemoveMessage(id));
        }
      }
      sendItemLists(client, newlyRelated);
      sessions[client] = std::move(fresh);
    }
  }

  void refreshItemLists(const std::set<uint32> &propertyIds) {
    if (!housingActive()) {
      return;
    }
    forEachSession([&](Client *client, Session &state) {
      std::set<uint32> held;
      for (uint32 id : propertyIds) {
        if (state.relatedIds.count(id)) {
          held.insert(id);
        }
      }
      if (!held.empty()) {
        sendItemLists(client, held);
      }
    });
  }

  void revealProperties(Client *client, const std::set<uint32> &propertyIds) {
    Session *state = session(client);
    if (!state) {
      throw std::runtime_error(fmt::format("[{}] has no housing session to receive properties [{}]", client->GetCleanName(), idList(propertyIds)));
    }
    std::set<uint32> revealed;
    for (uint32 id : propertyIds) {
      int depth = 0;
      for (uint32 at = id; at;) {
        auto found = state->properties.find(at);
        if (found == state->properties.end()) {
          throw std::runtime_error(fmt::format("property [{}] in the line of [{}] is not in [{}]'s session", at, id, client->GetCleanName()));
        }
        if (++depth > maximumDepth) {
          throw std::runtime_error(fmt::format("property [{}] has an ancestor chain deeper than [{}]", id, maximumDepth));
        }
        if (!state->knownIds.count(at)) {
          revealed.insert(at);
        }
        at = found->second.parent_id;
      }
    }
    if (revealed.empty()) {
      return;
    }
    std::vector<std::string> problems;
    const auto fields = recordFieldsFor(state->properties, problems);
    if (!problems.empty()) {
      throw std::runtime_error(Strings::Join(problems, "; "));
    }
    for (const Property *property : parentsFirst(*state)) {
      if (revealed.count(property->id)) {
        sendMessage(client, OP_RealEstatePropertyUpdate, propertyRecord(*property, fields.at(property->id)));
        state->knownIds.insert(property->id);
      }
    }
    LogHousing("Sent [{}] the records of properties [{}] on demand", client->GetCleanName(), idList(revealed));
  }

  std::string propertyName(uint32 propertyId) {
    std::map<uint32, Property> chain;
    std::vector<std::string> problems;
    for (uint32 id = propertyId; id;) {
      if (chain.size() > maximumDepth) {
        throw std::runtime_error(fmt::format("property [{}] has an ancestor chain deeper than [{}]", propertyId, maximumDepth));
      }
      auto rows = propertiesById({id});
      if (rows.size() != 1) {
        throw std::runtime_error(fmt::format("property [{}] has ancestor [{}], which does not exist", propertyId, id));
      }
      chain[id] = rows.front();
      id = rows.front().parent_id;
    }
    const std::string name = composedName(chain.at(propertyId), chain, problems);
    if (!problems.empty() || name.empty()) {
      throw std::runtime_error(fmt::format("property [{}] has no name: {}", propertyId, Strings::Join(problems, "; ")));
    }
    return name;
  }

  int64 currentTime() {
    return static_cast<int64>(std::time(nullptr));
  }

  void handlePacket(Client *client, const EQApplicationPacket *app) {
    if (!housingActive()) {
      LogHousing("Housing is off in this zone; [{}]'s request [{}] is refused", client->GetCleanName(), static_cast<uint32>(app->GetOpcode()));
      return;
    }
    WireReader reader(app->pBuffer, app->size);
    switch (app->GetOpcode()) {
      case OP_RealEstateSearch:
        handleSearch(client, reader);
        break;
      case OP_RealEstatePurchase:
        handlePurchase(client, reader);
        break;
      case OP_RealEstateSell:
        handleSell(client, reader);
        break;
      case OP_RealEstateTransfer:
        handleTransfer(client, reader);
        break;
      case OP_RealEstateTransferOffer:
        handleTransferAnswer(client, reader);
        break;
      case OP_RealEstateAccount:
        handleAccount(client, reader);
        break;
      case OP_RealEstateSetAccessList:
        handleSetAccessList(client, reader);
        break;
      case OP_RealEstateInteractionMenu:
        handleMenuRequest(client, reader);
        break;
      case OP_RealEstateInteract:
        handleInteract(client, reader);
        break;
      case OP_RealEstateInspect:
        handleInspect(client, reader);
        break;
      case OP_RealEstateUse:
        handleUse(client, reader);
        break;
      case OP_RealEstateMenuType9:
        handleMenuAction(client, reader);
        break;
      case OP_RealEstateMenuType5:
        handleMailToOwner(client, reader);
        break;
      case OP_RealEstatePlace:
        handlePlaceRequest(client, reader);
        break;
      case OP_RealEstatePlaceCommit:
        handlePlaceCommit(client, reader);
        break;
      case OP_RealEstateMoveToInventory:
        handleMoveToInventory(client, reader);
        break;
      case OP_RealEstateMoveToStorage:
        handleMoveToStorage(client, reader);
        break;
      case OP_RealEstateStoreAll:
        handleStoreAll(client, reader);
        break;
      case OP_RealEstatePack:
        handlePack(client, reader);
        break;
      case OP_RealEstateHouseSwap:
        handleHouseSwap(client, reader);
        break;
      case OP_RealEstateLoadLayout:
        handleLoadLayout(client, reader);
        break;
      case OP_RealEstateLeave:
        handleLeave(client);
        break;
      case OP_RealEstateAnchorTeleport:
        handleAnchorAnswer(client, reader);
        break;
      case OP_PollList:
        handlePollRequest(client, reader);
        break;
      case OP_PollVote:
        handleVote(client, reader);
        break;
      case OP_PollVotingOptOut:
        handleVotingOptOut(client, reader);
        break;
      case OP_PollTeleport:
        handlePollTeleport(client, reader);
        break;
      default:
        LogHousing("Unhandled housing opcode [{}] from [{}]", static_cast<uint32>(app->GetOpcode()), client->GetCleanName());
    }
  }

  void process() {
    if (housingActive()) {
      expireTransferOffers();
      processNpcItems();
      processContainers();
      processGuildServices();
      processPolls();
      processYards();
    }
  }
}
