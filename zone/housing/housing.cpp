#include "housing.h"

#include "housingAccess.h"
#include "housingAnchors.h"
#include "housingCatalog.h"
#include "housingContainers.h"
#include "housingDoors.h"
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

    bool isRoF2(Client *client) {
      return client->ClientVersion() == EQ::versions::ClientVersion::RoF2;
    }

    bool isAnyType(DefinitionType type, std::initializer_list<DefinitionType> types) {
      return std::find(types.begin(), types.end(), type) != types.end();
    }

    std::set<uint32> carriedPropertyIds(uint32 characterId) {
      std::set<uint32> ids;
      auto rows = database.QueryDatabase(fmt::format(
        "SELECT custom_data FROM inventory WHERE character_id = {} AND custom_data LIKE '%{}^%'", characterId, propertyItemKey
      ));
      if (!rows.Success()) {
        throw std::runtime_error(fmt::format("the carried properties of character [{}] could not be read: {}", characterId, rows.ErrorMessage()));
      }
      for (auto row : rows) {
        const uint32 propertyId = carriedPropertyId(row[0]);
        if (propertyId) {
          ids.insert(propertyId);
        }
      }
      return ids;
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
      const auto identities = characterIdentities(owners);
      auto money = balances(ids);
      std::map<uint32, RecordFields> fields;
      for (const auto &[id, property] : properties) {
        const CharacterIdentity *owner = nullptr;
        if (property.owner_character_id) {
          auto found = identities.find(property.owner_character_id);
          if (found == identities.end()) {
            problems.push_back(fmt::format("property [{}] is owned by character [{}], which does not exist", id, property.owner_character_id));
            continue;
          }
          owner = &found->second;
        }
        fields[id] = RecordFields{
          typeOf(property) == DefinitionType::plot && !property.owner_character_id,
          activeGuildId(property),
          owner,
          composedName(property, properties, problems),
          children[id],
          money[id],
          upkeepOptionKey(property, problems)
        };
      }
      return fields;
    }

    bool buildSession(Client *client, Session &state, std::map<uint32, RecordFields> &fields, const std::set<uint32> &alsoKnown) {
      std::vector<std::string> problems;
      try {
        const auto hosted = hostedProperties();
        const auto owned = propertiesOwnedBy(client->CharacterID());
        const auto carried = propertiesById(carriedPropertyIds(client->CharacterID()));
        for (const auto &property : hosted) {
          state.hostedIds.insert(property.id);
          state.properties[property.id] = property;
        }
        for (const auto &property : owned) {
          state.ownedIds.push_back(property.id);
          state.properties[property.id] = property;
        }
        for (const auto &property : carried) {
          if (!property.contained_in_item) {
            problems.push_back(fmt::format("an item the character carries names property [{}], which is not in an item", property.id));
            continue;
          }
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
          fields = recordFieldsFor(state.properties, problems);
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

    std::set<uint32> listHoldersOf(const Session &state) {
      std::set<uint32> holders;
      for (const auto &[id, property] : state.properties) {
        if (holdsAccessList(typeOf(property))) {
          holders.insert(id);
        }
      }
      return holders;
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
      std::set<uint32> itemOwners;
      for (const auto &item : items) {
        itemOwners.insert(item.owner_character_id);
      }
      const auto itemOwnerIdentities = characterIdentities(itemOwners);
      for (uint32 id : holders) {
        sendMessage(client, OP_RealEstateItemList, itemList(id, items, itemOwnerIdentities));
      }
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
      Session state;
      std::map<uint32, RecordFields> fields;
      if (!buildSession(client, state, fields, {})) {
        return;
      }
      sendMessage(client, OP_RealEstateDefinitions, catalog().message());
      for (const Property *property : parentsFirst(state)) {
        sendMessage(client, OP_RealEstatePropertyUpdate, propertyRecord(*property, fields.at(property->id)));
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

      std::set<uint32> itemHolders(state->hostedIds);
      itemHolders.insert(state->ownedIds.begin(), state->ownedIds.end());
      itemHolders.insert(state->carriedIds.begin(), state->carriedIds.end());
      sendItemLists(client, itemHolders);

      state->accessLists = accessLists(listHoldersOf(*state));
      for (const auto &[id, list] : state->accessLists) {
        sendMessage(client, OP_RealEstateAccessList, accessListMessage(id, list));
      }
      sendWorldObjects(client, *state);
      sendPolls(client);
      LogHousing("Zone-in for [{}]: [{}] properties, current [{}], [{}] regions, [{}] item lists, [{}] access lists",
        client->GetCleanName(), state->properties.size(), state->currentPropertyId, regions.size(), itemHolders.size(), state->accessLists.size());
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
    if (catalogReady) {
      try {
        loadHostedRoot();
      }
      catch (const std::exception &error) {
        LogError("Housing: the property this zone runs could not be found after the catalog reload: {}", error.what());
        clearHostedRoot();
      }
    }
    for (const auto &[id, client] : entity_list.GetClientList()) {
      if (isRoF2(client)) {
        sendZoneContents(client);
        sendZoneIn(client);
      }
    }
    return catalogReady;
  }

  Session *session(Client *client) {
    auto found = sessions.find(client);
    return found == sessions.end() ? nullptr : &found->second;
  }

  bool onZoneBoot() {
    if (!loadItemHousingTails()) {
      return false;
    }
    if (!RuleB(Housing, EnableHousing)) {
      LogHousing("Player housing is disabled by rule Housing:EnableHousing");
      return true;
    }
    catalogReady = catalog().load();
    if (catalogReady) {
      try {
        loadHostedRoot();
        loadDoorMap();
        loadWrits();
      }
      catch (const std::exception &error) {
        LogError("Housing: the property this zone runs or its doors could not be loaded: {}", error.what());
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
    if (isRoF2(client)) {
      sendZoneContents(client);
    }
  }

  void onZoneIn(Client *client) {
    if (isRoF2(client)) {
      sendZoneIn(client);
    }
    if (housingActive()) {
      stripAuraBuffs(client);
      admitOccupant(client);
    }
  }

  void onDisconnect(Client *client) {
    closeContainerOf(client);
    forgetAnchorOffer(client);
    forgetGuildServiceClient(client);
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
      std::map<uint32, RecordFields> fields;
      if (!buildSession(client, fresh, fields, existing)) {
        sessions.erase(client);
        continue;
      }
      if (fresh.properties.count(old->currentPropertyId)) {
        fresh.currentPropertyId = old->currentPropertyId;
      }
      fresh.placement = old->placement;
      fresh.augmentaterMode = old->augmentaterMode;
      std::set<uint32> added;
      for (const auto &[id, property] : fresh.properties) {
        if (!old->properties.count(id)) {
          added.insert(id);
        }
      }
      for (const Property *property : parentsFirst(fresh)) {
        if (propertyIds.count(property->id) || added.count(property->id)) {
          sendMessage(client, OP_RealEstatePropertyUpdate, propertyRecord(*property, fields.at(property->id)));
        }
      }
      for (uint32 id : propertyIds) {
        if (!existing.count(id) && old->properties.count(id)) {
          sendMessage(client, OP_RealEstateProperty, propertyRecord(old->properties.at(id), RecordFields{false, 0, nullptr, {}, {}, {}, 0}));
        }
      }
      sendPropertyLists(client, fresh);
      fresh.accessLists = accessLists(listHoldersOf(fresh));
      for (const auto &[id, list] : fresh.accessLists) {
        if (propertyIds.count(id) || added.count(id)) {
          sendMessage(client, OP_RealEstateAccessList, accessListMessage(id, list));
        }
      }
      for (uint32 id : propertyIds) {
        if (old->accessLists.count(id) && !fresh.accessLists.count(id)) {
          sendMessage(client, OP_RealEstateAccessListRemove, accessListRemoveMessage(id));
        }
      }
      std::set<uint32> newHolders;
      for (uint32 id : added) {
        const bool owned = std::find(fresh.ownedIds.begin(), fresh.ownedIds.end(), id) != fresh.ownedIds.end();
        const bool carried = std::find(fresh.carriedIds.begin(), fresh.carriedIds.end(), id) != fresh.carriedIds.end();
        if (fresh.hostedIds.count(id) || owned || carried) {
          newHolders.insert(id);
        }
      }
      sendItemLists(client, newHolders);
      sessions[client] = std::move(fresh);
    }
  }

  void refreshItemLists(const std::set<uint32> &propertyIds) {
    if (!housingActive()) {
      return;
    }
    forEachSession([&](Client *client, Session &state) {
      std::set<uint32> known;
      for (uint32 id : propertyIds) {
        if (state.properties.count(id)) {
          known.insert(id);
        }
      }
      if (!known.empty()) {
        sendItemLists(client, known);
      }
    });
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
    if (!isRoF2(client)) {
      LogError("Housing: [{}] sent housing opcode [{}] from a client that is not RoF2", client->GetCleanName(), static_cast<uint32>(app->GetOpcode()));
      return;
    }
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
    }
  }
}
