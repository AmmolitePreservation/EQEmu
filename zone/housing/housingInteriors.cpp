#include "housingInteriors.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingItems.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingCopies.h"
#include "../../common/rulesys.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../entity.h"
#include "../zone.h"
#include "../zonedb.h"

#include <stdexcept>

extern EntityList entity_list;
extern Zone *zone;

namespace housing {
  namespace {
    constexpr uint32 notInHouse = 6866;
    constexpr uint32 upkeepExpired = 6432;
    constexpr int maximumDepth = 10;

    std::optional<Property> root;
    // The yard an interior's house stood on at boot or at the last change: occupants leave by its exit once the house is gone.
    std::optional<Property> rootPlot;

    bool isInterior(const Property &property) {
      const DefinitionType type = typeOf(property);
      return type == DefinitionType::playerHouse || type == DefinitionType::guildHall;
    }

    std::map<uint32, Property> chainOf(const Property &property) {
      std::map<uint32, Property> chain{{property.id, property}};
      for (uint32 id = property.parent_id; id;) {
        if (chain.size() > maximumDepth) {
          throw std::runtime_error(fmt::format("property [{}] has an ancestor chain deeper than [{}]", property.id, maximumDepth));
        }
        auto rows = propertiesById({id});
        if (rows.size() != 1) {
          throw std::runtime_error(fmt::format("property [{}] has ancestor [{}], which does not exist", property.id, id));
        }
        chain[id] = rows.front();
        id = rows.front().parent_id;
      }
      return chain;
    }

    PropertyLookup lookupIn(const std::map<uint32, Property> &chain) {
      return [&chain](uint32 id) -> const Property * {
        auto found = chain.find(id);
        return found == chain.end() ? nullptr : &found->second;
      };
    }

    // A house stored in its yard's shed or swapped out stands in no yard, so it can't be entered and its occupants leave
    // (INT-25); a definition's fixed house (an apartment's) has no item carrying it.
    std::optional<Property> yardOf(const Property &house, const std::map<uint32, Property> &chain) {
      if (house.contained_in_item) {
        return std::nullopt;
      }
      const Property *plot = ancestorOfType(house, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), true, lookupIn(chain));
      if (!plot) {
        return std::nullopt;
      }
      for (const auto &row : itemsOf({house.parent_id})) {
        if (carriedPropertyId(row.custom_data) == house.id && !row.placed) {
          return std::nullopt;
        }
      }
      return *plot;
    }

    const ZoneTemplate &templateOf(const Property &property) {
      const Definition *definition = catalog().definition(property.definition_id);
      if (!definition) {
        throw std::runtime_error(fmt::format("property [{}] names unknown definition [{}]", property.id, property.definition_id));
      }
      const ZoneTemplate *zoneTemplate = definition->row.dz_id ? catalog().zoneTemplate(definition->row.dz_id) : nullptr;
      if (!zoneTemplate) {
        throw std::runtime_error(fmt::format("property [{}]'s definition [{}] names no zone template", property.id, property.definition_id));
      }
      return *zoneTemplate;
    }

    bool templateIsThisZone(const ZoneTemplate &zoneTemplate) {
      return zoneTemplate.zone_id == zone->GetZoneID() && zoneTemplate.zone_version == zone->GetInstanceVersion();
    }

    std::optional<Property> zoneRootHere() {
      if (!RuleB(Housing, EnableZoneRoots)) {
        return std::nullopt;
      }
      std::set<uint32> definitions;
      for (uint32 id : definitionList(RuleS(Housing, ZoneRootDefinitions), "Housing:ZoneRootDefinitions")) {
        const Definition *definition = catalog().definition(id);
        if (!definition || definition->row.type != static_cast<uint8>(DefinitionType::regularZone)) {
          throw std::runtime_error(fmt::format("Housing:ZoneRootDefinitions lists [{}], which is not a Regular Zone definition", id));
        }
        const ZoneTemplate *zoneTemplate = catalog().zoneTemplate(definition->row.dz_id);
        if (!zoneTemplate || zoneTemplate->zone_version != 0) {
          throw std::runtime_error(fmt::format("zone root definition [{}] names no zone template at version 0", id));
        }
        if (templateIsThisZone(*zoneTemplate)) {
          definitions.insert(id);
        }
      }
      if (definitions.empty()) {
        return std::nullopt;
      }
      auto roots = RealEstatePropertiesRepository::GetWhere(database, fmt::format(
        "parent_id = 0 AND archived = 0 AND definition_id IN ({})", idList(definitions)
      ));
      if (roots.size() > 1) {
        throw std::runtime_error(fmt::format("this zone has [{}] zone roots; one is allowed", roots.size()));
      }
      return roots.empty() ? std::nullopt : std::optional<Property>(roots.front());
    }

    std::optional<Property> copyRootHere() {
      const uint32 propertyId = propertyOfCopy(database, static_cast<uint16>(zone->GetInstanceID()));
      if (!propertyId) {
        return std::nullopt;
      }
      auto rows = propertiesById({propertyId});
      if (rows.size() != 1) {
        throw std::runtime_error(fmt::format("instance [{}] is a copy of property [{}], which does not exist", zone->GetInstanceID(), propertyId));
      }
      if (!templateIsThisZone(templateOf(rows.front()))) {
        throw std::runtime_error(fmt::format("instance [{}] is a copy of property [{}], whose template is another zone", zone->GetInstanceID(), propertyId));
      }
      return rows.front();
    }

    // Where a yard's exit leaves to: its root's running place when the exit is in the root's zone, else that zone at instance 0.
    void moveToExit(Client *client, const Property &plot) {
      const Definition *definition = catalog().definition(plot.definition_id);
      if (!definition || !definition->row.exit_zone_id) {
        throw std::runtime_error(fmt::format("plot [{}] has no exit point", plot.id));
      }
      const auto &exit = definition->row;
      const Property plotRoot = rootOf(database, plot);
      const ZoneTemplate &rootTemplate = templateOf(plotRoot);
      uint16 instance = 0;
      if (rootTemplate.zone_id == exit.exit_zone_id && rootTemplate.zone_version == exit.exit_zone_version) {
        if (typeOf(plotRoot) != DefinitionType::regularZone) {
          instance = runningCopy(database, content_db, plotRoot.id);
          if (!database.AddClientToInstance(instance, client->CharacterID())) {
            throw std::runtime_error(fmt::format("[{}] could not be added to instance [{}]", client->GetCleanName(), instance));
          }
        }
      }
      else if (exit.exit_zone_version != 0) {
        throw std::runtime_error(fmt::format("plot [{}] exits to zone [{}] version [{}], which no property of its runs", plot.id, exit.exit_zone_id, exit.exit_zone_version));
      }
      LogHousing("[{}] leaves for plot [{}]'s exit in zone [{}] instance [{}]", client->GetCleanName(), plot.id, exit.exit_zone_id, instance);
      client->MovePC(exit.exit_zone_id, instance, exit.exit_x, exit.exit_y, exit.exit_z, exit.exit_heading);
    }

    bool mayEnter(Client *client, const Property &property, const std::map<uint32, Property> &chain) {
      std::set<uint32> ids;
      for (const auto &[id, link] : chain) {
        ids.insert(id);
      }
      const auto lists = accessLists(ids);
      auto listOf = [&lists](uint32 id) -> const AccessList * {
        auto found = lists.find(id);
        return found == lists.end() ? nullptr : &found->second;
      };
      return permitted(client, Permission::enter, property, lookupIn(chain), listOf);
    }
  }

  void loadHostedRoot() {
    clearHostedRoot();
    root = zone->GetInstanceID() ? copyRootHere() : zoneRootHere();
    if (root && isInterior(*root)) {
      rootPlot = yardOf(*root, chainOf(*root));
    }
    LogHousing("This zone runs property [{}]", root ? root->id : 0);
  }

  void clearHostedRoot() {
    root.reset();
    rootPlot.reset();
  }

  std::optional<Property> hostedRoot() {
    return root;
  }

  std::optional<Property> hostedRootYard() {
    return rootPlot;
  }

  bool hostsInterior() {
    return root && isInterior(*root);
  }

  std::vector<Property> hostedProperties() {
    if (!root) {
      return {};
    }
    auto current = propertiesById({root->id});
    if (current.size() != 1 || isInterior(current.front())) {
      return current;
    }
    std::vector<Property> hosted(current);
    std::set<uint32> frontier{root->id};
    for (int depth = 0; !frontier.empty(); depth++) {
      if (depth > maximumDepth) {
        throw std::runtime_error(fmt::format("property [{}] has descendants deeper than [{}]", root->id, maximumDepth));
      }
      std::set<uint32> next;
      for (auto &child : propertiesChildOf(frontier)) {
        if (!child.archived) {
          hosted.push_back(child);
          next.insert(child.id);
        }
      }
      frontier = next;
    }
    return hosted;
  }

  bool hostedHere(const Property &property) {
    if (!root) {
      return false;
    }
    if (isInterior(*root)) {
      return property.id == root->id;
    }
    return rootOf(database, property).id == root->id;
  }

  EntryCheck checkEntry(Client *client, uint32 propertyId) {
    EntryCheck check;
    auto rows = propertiesById({propertyId});
    if (rows.size() != 1) {
      check.state = EntryState::gone;
      return check;
    }
    check.property = rows.front();
    const auto chain = chainOf(*check.property);
    std::optional<Property> yard;
    if (isInterior(*check.property)) {
      yard = yardOf(*check.property, chain);
      if (!yard) {
        check.state = EntryState::notInYard;
        return check;
      }
    }
    if (!mayEnter(client, *check.property, chain)) {
      check.state = EntryState::noAccess;
      return check;
    }
    const Property *plot = yard ? &*yard : ancestorOfType(*check.property, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, lookupIn(chain));
    if (check.property->upkeep_expired_time || (plot && plot->upkeep_expired_time)) {
      check.state = EntryState::upkeepExpired;
      return check;
    }
    check.state = EntryState::open;
    return check;
  }

  // An interior runs in its own copy; anything else stands where its root runs: a neighborhood's copy, or a zone root's instance 0.
  void moveInto(Client *client, const Property &property, const std::optional<glm::vec4> &point) {
    const Property place = isInterior(property) ? property : rootOf(database, property);
    const ZoneTemplate &zoneTemplate = templateOf(place);
    uint16 instance = 0;
    if (typeOf(place) != DefinitionType::regularZone) {
      instance = runningCopy(database, content_db, place.id);
      if (!database.AddClientToInstance(instance, client->CharacterID())) {
        throw std::runtime_error(fmt::format("[{}] could not be added to instance [{}]", client->GetCleanName(), instance));
      }
    }
    const glm::vec4 destination = point ? *point : glm::vec4(zoneTemplate.zone_in_x, zoneTemplate.zone_in_y, zoneTemplate.zone_in_z, zoneTemplate.zone_in_heading);
    LogHousing("[{}] goes to property [{}] in zone [{}] instance [{}]", client->GetCleanName(), property.id, zoneTemplate.zone_id, instance);
    client->MovePC(zoneTemplate.zone_id, instance, destination.x, destination.y, destination.z, destination.w);
  }

  void enterProperty(Client *client, const Property &requested) {
    try {
      const DefinitionType type = typeOf(requested);
      if (!isInterior(requested) && (type != DefinitionType::neighborhood || requested.parent_id)) {
        throw std::logic_error(fmt::format("property [{}] of type [{}] has no interior to enter", requested.id, static_cast<uint32>(type)));
      }
      const EntryCheck check = checkEntry(client, requested.id);
      switch (check.state) {
        case EntryState::open:
          moveInto(client, *check.property, std::nullopt);
          return;
        case EntryState::upkeepExpired:
          client->MessageString(Chat::Red, upkeepExpired, propertyName(requested.id).c_str());
          return;
        default:
          LogHousing("[{}] may not enter property [{}] (state [{}])", client->GetCleanName(), requested.id, static_cast<uint32>(check.state));
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}] could not enter property [{}]: {}", client->GetCleanName(), requested.id, error.what());
    }
  }

  // A plot is reached at its exit point, where its house's front door leaves to (a poll's teleport, S11).
  void moveToPlot(Client *client, const Property &plot) {
    moveToExit(client, plot);
  }

  void leaveForExitPoint(Client *client) {
    if (!hostsInterior()) {
      throw std::logic_error("only an interior's occupants leave for an exit point");
    }
    try {
      if (!rootPlot) {
        throw std::runtime_error(fmt::format("interior [{}] stood in no yard when this copy started", root->id));
      }
      moveToExit(client, *rootPlot);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}] could not leave property [{}]: {}", client->GetCleanName(), root->id, error.what());
    }
  }

  // 0x1da9's four body bytes are stale send-buffer content (INT-10), so the request carries nothing to read.
  void handleLeave(Client *client) {
    if (!hostsInterior()) {
      client->MessageString(Chat::Red, notInHouse);
      return;
    }
    leaveForExitPoint(client);
  }

  // Anyone in a copy whose Enter was revoked, or whose house left its yard, while they were away from it goes out as they come in.
  void admitOccupant(Client *client) {
    if (!hostsInterior()) {
      return;
    }
    try {
      auto rows = propertiesById({root->id});
      if (rows.size() == 1) {
        const auto chain = chainOf(rows.front());
        if (yardOf(rows.front(), chain) && mayEnter(client, rows.front(), chain)) {
          return;
        }
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s entry to property [{}] could not be checked: {}", client->GetCleanName(), root->id, error.what());
    }
    LogHousing("[{}] came into property [{}] without Enter or with the house gone from its yard; they leave", client->GetCleanName(), root->id);
    leaveForExitPoint(client);
  }

  // A house picked up, stored or swapped out takes its occupants out first (D3: a removed instance player row moves nobody).
  void checkHostedRoot(uint32 propertyId) {
    if (!hostsInterior() || propertyId != root->id) {
      return;
    }
    std::optional<Property> yard;
    try {
      auto rows = propertiesById({root->id});
      if (rows.size() == 1) {
        yard = yardOf(rows.front(), chainOf(rows.front()));
        root = rows.front();
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: property [{}]'s yard could not be read: {}", root->id, error.what());
    }
    if (yard) {
      rootPlot = yard;
      return;
    }
    LogHousing("Property [{}] is no longer in a yard; its occupants leave", root->id);
    std::vector<Client *> occupants;
    for (const auto &[id, client] : entity_list.GetClientList()) {
      occupants.push_back(client);
    }
    for (Client *client : occupants) {
      leaveForExitPoint(client);
    }
  }

  void discardHostedCopy() {
    if (!zone->GetInstanceID() || !propertyOfCopy(database, static_cast<uint16>(zone->GetInstanceID()))) {
      return;
    }
    // EQEmu's shutdown state save would write the copy's spawns back under the freed id, and ids come back (INT-34).
    zone->SetSaveZoneState(false);
    try {
      discardCopy(database, static_cast<uint16>(zone->GetInstanceID()));
      LogHousing("Discarded copy instance [{}] of property [{}]", zone->GetInstanceID(), root ? root->id : 0);
    }
    catch (const std::exception &error) {
      LogError("Housing: copy instance [{}] could not be discarded; world's sweep removes it: {}", zone->GetInstanceID(), error.what());
    }
  }

  uint32 hostedCopyPropertyId() {
    return zone->GetInstanceID() && root ? root->id : 0;
  }
}
