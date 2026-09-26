#include "housingAccess.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/guilds.h"
#include "../../common/repositories/real_estate_access_groups_repository.h"
#include "../../common/repositories/real_estate_access_names_repository.h"
#include "../client.h"
#include "../zonedb.h"

namespace housing {
  namespace {
    constexpr int maximumDepth = 10;
    constexpr uint32 houseOfThule = 0x10000;
    constexpr uint32 veilOfAlaris = 0x20000;

    bool usesPlotAccess(DefinitionType type) {
      return type == DefinitionType::guildHall || type == DefinitionType::playerHouse || type == DefinitionType::movingCrate;
    }

    bool ownsPlayerPlot(Client *client, const Property &property, const PropertyLookup &lookup) {
      const Property *plot = ancestorOfType(property, typeBit(DefinitionType::plot), false, lookup);
      return plot && plot->owner_character_id == client->CharacterID();
    }

    bool leadsGuildPlot(Client *client, const Property &property) {
      return typeOf(property) == DefinitionType::guildPlot && property.guild_id && property.guild_id == client->GuildID() && client->GuildRank() == GUILD_LEADER;
    }

    uint32 requiredExpansions(Permission permission, const Property &property, const PropertyLookup &lookup) {
      const uint8 id = static_cast<uint8>(permission);
      if (ancestorOfType(property, typeBit(DefinitionType::guildPlot), false, lookup)) {
        return id >= 2 && id <= 10 ? veilOfAlaris : 0;
      }
      return permission == Permission::enter ? 0 : houseOfThule;
    }
  }

  DefinitionType typeOf(const Property &property) {
    const Definition *definition = catalog().definition(property.definition_id);
    return definition ? static_cast<DefinitionType>(definition->row.type) : DefinitionType::none;
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

  uint8 accessGroup(Client *client, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf) {
    const Property *subject = &property;
    if (usesPlotAccess(typeOf(property))) {
      subject = ancestorOfType(property, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), true, lookup);
      if (!subject) {
        return 0;
      }
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
    for (const auto &[name, group] : list->names) {
      if (name == client->GetName()) {
        return group;
      }
    }
    if (subject->guild_id && client->GuildID() == subject->guild_id) {
      const std::string rankKey = fmt::format("_Guild{}", client->GuildRank());
      for (const auto &[name, group] : list->names) {
        if (name == rankKey) {
          return group;
        }
      }
    }
    return 0;
  }

  bool permitted(Client *client, Permission permission, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf) {
    const uint8 id = static_cast<uint8>(permission);
    const bool owner = ownsPlayerPlot(client, property, lookup);
    const bool ownerExempt = owner && (id == 2 || id == 3 || id == 5 || id == 8);
    if (!ownerExempt && (requiredExpansions(permission, property, lookup) & ~client->GetPP().expansions)) {
      return false;
    }
    const Property *subject = &property;
    if (usesPlotAccess(typeOf(property))) {
      subject = ancestorOfType(property, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), true, lookup);
      if (!subject) {
        return false;
      }
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
}
