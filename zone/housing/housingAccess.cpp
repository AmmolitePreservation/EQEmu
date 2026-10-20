#include "housingAccess.h"

#include "housingEvents.h"
#include "housingInteriors.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/guilds.h"
#include "../../common/housingLedger.h"
#include "../../common/repositories/real_estate_access_groups_repository.h"
#include "../../common/repositories/real_estate_access_names_repository.h"
#include "../client.h"
#include "../guild_mgr.h"
#include "../zonedb.h"

#include <stdexcept>

namespace housing {
  namespace {
    constexpr int maximumDepth = 10;

    bool usesPlotAccess(DefinitionType type) {
      return type == DefinitionType::guildHall || type == DefinitionType::playerHouse || type == DefinitionType::movingCrate;
    }

    bool ownsPlayerPlot(Client *client, const Property &property, const PropertyLookup &lookup) {
      const Property *plot = ancestorOfType(property, typeBit(DefinitionType::plot), false, lookup);
      return plot && plot->owner_character_id == client->CharacterID();
    }

    bool leadsGuildPlot(Client *client, const Property &property) {
      const uint32 guild = activeGuildId(property);
      return typeOf(property) == DefinitionType::guildPlot && guild && guild == client->GuildID() && client->GuildRank() == GUILD_LEADER;
    }

    uint32 requiredExpansions(Permission permission, const Property &property, const PropertyLookup &lookup) {
      const uint8 id = static_cast<uint8>(permission);
      if (ancestorOfType(property, typeBit(DefinitionType::guildPlot), false, lookup)) {
        return id >= 2 && id <= 10 ? veilOfAlaris : 0;
      }
      return permission == Permission::enter ? 0 : houseOfThule;
    }

    std::string characterRankKey(Client *client, const Property &subject) {
      const uint32 subjectGuild = activeGuildId(subject);
      if (subjectGuild && client->GuildID() == subjectGuild) {
        return guildRankKey(client->GuildRank());
      }
      return {};
    }

    std::set<std::string> existingCharacterNames(const std::set<std::string> &names) {
      std::set<std::string> found;
      if (names.empty()) {
        return found;
      }
      std::vector<std::string> quoted;
      for (const auto &name : names) {
        quoted.push_back(fmt::format("'{}'", Strings::Escape(name)));
      }
      auto rows = query(database, fmt::format("SELECT name FROM character_data WHERE deleted_at IS NULL AND name IN ({})", Strings::Join(quoted, ", ")));
      for (auto row : rows) {
        found.insert(row[0]);
      }
      return found;
    }
  }

  uint32 activeGuildId(const Property &property) {
    return property.guild_id && guild_mgr.GuildExists(property.guild_id) ? property.guild_id : 0;
  }

  DefinitionType typeOf(const Property &property) {
    const Definition *definition = catalog().definition(property.definition_id);
    return definition ? static_cast<DefinitionType>(definition->row.type) : DefinitionType::none;
  }

  bool holdsAccessList(DefinitionType type) {
    return type == DefinitionType::plot || type == DefinitionType::guildPlot || type == DefinitionType::neighborhood || type == DefinitionType::guildHall;
  }

  const Property *ancestorOfType(const Property &property, uint32 typeMask, bool startAtParent, const PropertyLookup &lookup) {
    const Property *current = startAtParent ? (property.parent_id ? lookup(property.parent_id) : nullptr) : &property;
    for (int depth = 0; current; depth++) {
      if (depth > maximumDepth) {
        LogError("Housing: property [{}] has an ancestor chain deeper than [{}]; treated as having no such ancestor", property.id, maximumDepth);
        return nullptr;
      }
      const DefinitionType type = typeOf(*current);
      if (type != DefinitionType::none && (typeMask & typeBit(type))) {
        return current;
      }
      current = current->parent_id ? lookup(current->parent_id) : nullptr;
    }
    return nullptr;
  }

  const Property *accessSubject(const Property &property, const PropertyLookup &lookup) {
    if (!usesPlotAccess(typeOf(property))) {
      return &property;
    }
    return ancestorOfType(property, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), true, lookup);
  }

  std::map<uint32, AccessList> accessLists(const std::set<uint32> &propertyIds) {
    std::map<uint32, AccessList> lists;
    if (propertyIds.empty()) {
      return lists;
    }
    const std::string where = fmt::format("property_id IN ({})", idList(propertyIds));
    for (const auto &row : RealEstateAccessGroupsRepository::GetWhere(database, where)) {
      if (row.group_number < accessGroupCount) {
        lists[row.property_id].masks[row.group_number] = row.permissions;
      }
    }
    for (const auto &row : RealEstateAccessNamesRepository::GetWhere(database, where + " ORDER BY property_id, name")) {
      auto list = lists.find(row.property_id);
      if (list == lists.end()) {
        LogError("Housing: property [{}] has access names but no access groups; its names are ignored", row.property_id);
        continue;
      }
      list->second.names.emplace_back(row.name, row.group_number);
    }
    return lists;
  }

  WireWriter accessListMessage(uint32 propertyId, const AccessList &list) {
    WireWriter message;
    message.u32(propertyId);
    for (uint8 group = 0; group < accessGroupCount; group++) {
      message.u32(group).u32(list.masks[group]);
    }
    message.u32(static_cast<uint32>(list.names.size()));
    for (const auto &[name, group] : list.names) {
      message.lengthString(name).u32(group);
    }
    return message;
  }

  // 0x3968 carries a list the client parses and discards, then a u32 whose use is not known; Peridot sends an empty list and 0.
  WireWriter accessListRemoveMessage(uint32 propertyId) {
    WireWriter message = accessListMessage(propertyId, AccessList{});
    message.u32(0);
    return message;
  }

  uint8 accessGroup(Client *client, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf) {
    const Property *subject = accessSubject(property, lookup);
    if (!subject) {
      return 0;
    }
    if (typeOf(*subject) == DefinitionType::plot && subject->owner_character_id == client->CharacterID()) {
      return ownerGroup;
    }
    if (leadsGuildPlot(client, *subject)) {
      return ownerGroup;
    }
    const AccessList *list = listOf(subject->id);
    if (!list) {
      return 0;
    }
    // EQEmu has no fellowships, so the owner's fellowship goes out as 0 and _Fellowship never matches.
    return matchedGroup(list->names, client->GetName(), characterRankKey(client, *subject));
  }

  bool permitted(Client *client, Permission permission, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf) {
    const uint8 id = static_cast<uint8>(permission);
    const bool owner = ownsPlayerPlot(client, property, lookup);
    const bool ownerExempt = owner && (id == 2 || id == 3 || id == 5 || id == 8);
    if (!ownerExempt && (requiredExpansions(permission, property, lookup) & ~client->GetPP().expansions)) {
      return false;
    }
    const Property *subject = accessSubject(property, lookup);
    if (!subject) {
      return false;
    }
    if (typeOf(*subject) == DefinitionType::plot && owner) {
      return true;
    }
    if (leadsGuildPlot(client, *subject)) {
      return true;
    }
    const AccessList *list = listOf(subject->id);
    if (!list) {
      return false;
    }
    const uint8 group = accessGroup(client, *subject, lookup, listOf);
    return (list->masks[group] >> id) & 1u;
  }

  // Live: a member of the owning guild always gets Transact; anyone else only with Interact Item (ACC-3).
  bool mayTransact(Client *client, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf) {
    const Property *guildPlot = ancestorOfType(property, typeBit(DefinitionType::guildPlot), false, lookup);
    if (guildPlot) {
      const uint32 guild = activeGuildId(*guildPlot);
      if (guild && client->GuildID() == guild) {
        return true;
      }
    }
    return permitted(client, Permission::interactItem, property, lookup, listOf);
  }

  void handleSetAccessList(Client *client, WireReader &reader) {
    Session *state = session(client);
    const uint32 propertyId = reader.u32();
    AccessList requested;
    bool wellFormed = true;
    for (uint8 group = 0; group < accessGroupCount; group++) {
      const uint32 number = reader.u32();
      requested.masks[group] = reader.u32();
      wellFormed = wellFormed && number == group;
    }
    const uint32 count = reader.u32();
    for (uint32 entry = 0; entry < count && !reader.failed(); entry++) {
      std::string name = reader.lengthString();
      const uint32 group = reader.u32();
      wellFormed = wellFormed && group <= 0xFF;
      requested.names.emplace_back(std::move(name), static_cast<uint8>(group));
    }
    if (reader.failed() || reader.remaining() || !wellFormed) {
      LogError("Housing: [{}] sent a malformed access list for property [{}]", client->GetCleanName(), propertyId);
      return;
    }
    if (!state || !state->properties.count(propertyId)) {
      LogHousing("[{}] edited the access list of property [{}], which this zone did not send them", client->GetCleanName(), propertyId);
      return;
    }
    const Property &known = state->properties.at(propertyId);
    auto sendCurrent = [&]() {
      auto lists = accessLists({propertyId});
      auto list = lists.find(propertyId);
      if (list != lists.end()) {
        sendMessage(client, OP_RealEstateAccessList, accessListMessage(propertyId, list->second));
      }
    };
    if (!holdsAccessList(typeOf(known))) {
      LogHousing("[{}] edited the access list of property [{}], whose type holds none", client->GetCleanName(), propertyId);
      sendCurrent();
      return;
    }
    try {
      Transaction transaction(database);
      const Property current = lockedProperty(database, propertyId);
      auto lists = accessLists({propertyId});
      if (!lists.count(propertyId)) {
        throw std::runtime_error("the property has no access list to edit");
      }
      const AccessList &stored = lists.at(propertyId);
      std::map<uint32, Property> fresh(state->properties);
      fresh[propertyId] = current;
      auto lookup = [&fresh](uint32 id) -> const Property * {
        auto found = fresh.find(id);
        return found == fresh.end() ? nullptr : &found->second;
      };
      auto listOf = [&](uint32 id) -> const AccessList * {
        return id == propertyId ? &stored : state->accessLookup()(id);
      };
      const uint8 group = accessGroup(client, current, lookup, listOf);
      const bool privileged = client->GetGM() || group == ownerGroup;
      if (!privileged && !permitted(client, Permission::manage, current, lookup, listOf)) {
        throw std::runtime_error("the editor has no Manage permission");
      }
      const std::string refusal = accessEditRefusal(stored.masks, stored.names, requested.masks, requested.names, group, privileged);
      if (!refusal.empty()) {
        throw std::runtime_error(refusal);
      }
      std::set<std::string> characterNames;
      for (const auto &[name, entryGroup] : requested.names) {
        if (name[0] == '_') {
          if (!isClubKey(name)) {
            throw std::runtime_error(fmt::format("[{}] is not a club key", name));
          }
          continue;
        }
        characterNames.insert(name);
      }
      const auto existing = existingCharacterNames(characterNames);
      for (const auto &name : characterNames) {
        if (!existing.count(name)) {
          throw std::runtime_error(fmt::format("[{}] is not a character", name));
        }
      }
      writeAccessList(database, propertyId, requested.masks, requested.names);
      bumpRevision(database, propertyId);
      transaction.commit();
    }
    catch (const std::exception &error) {
      LogHousing("Refused [{}]'s access list for property [{}]: {}", client->GetCleanName(), propertyId, error.what());
      sendCurrent();
      return;
    }
    LogHousing("[{}] set the access list of property [{}]", client->GetCleanName(), propertyId);
    sendEvent(EventType::accessChanged, EventTarget::property, propertyId, propertyId, {});
  }

  void applyAccessChange(uint32 propertyId) {
    const auto lists = accessLists({propertyId});
    auto list = lists.find(propertyId);
    forEachSession([&](Client *client, Session &state) {
      if (!state.properties.count(propertyId)) {
        return;
      }
      if (list == lists.end()) {
        state.accessLists.erase(propertyId);
        sendMessage(client, OP_RealEstateAccessListRemove, accessListRemoveMessage(propertyId));
      }
      else {
        state.accessLists[propertyId] = list->second;
        sendMessage(client, OP_RealEstateAccessList, accessListMessage(propertyId, list->second));
      }
      auto current = state.properties.find(state.currentPropertyId);
      if (current == state.properties.end()) {
        return;
      }
      const DefinitionType type = typeOf(current->second);
      if (type != DefinitionType::playerHouse && type != DefinitionType::guildHall) {
        return;
      }
      const Property *subject = accessSubject(current->second, state.lookup());
      if (subject && subject->id == propertyId && !permitted(client, Permission::enter, current->second, state.lookup(), state.accessLookup())) {
        LogHousing("[{}] lost Enter on property [{}] and leaves it for its exit point", client->GetCleanName(), current->first);
        leaveForExitPoint(client);
      }
    });
  }
}
