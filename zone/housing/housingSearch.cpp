#include "housingHandlers.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/rulesys.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../guild_mgr.h"
#include "../string_ids.h"
#include "../zonedb.h"

#include <algorithm>

namespace housing {
  namespace {
    constexpr size_t searchRequestSize = 224;
    constexpr uint32 nameSearch = 0;
    constexpr uint32 myItems = 1;
    constexpr uint32 neighborhoodDetails = 2;
    constexpr uint32 plotSearch = 4;
    constexpr uint32 plotSearchMyItems = 5;
    constexpr uint8 listKind = static_cast<uint8>(DefinitionType::neighborhood);
    constexpr uint32 noGuild = 0xFFFFFFFF;

    struct Neighborhood {
      Property property;
      std::vector<Property> plots;
    };

    std::set<uint32> definitionsOfType(std::initializer_list<DefinitionType> types) {
      std::set<uint32> ids;
      for (const auto &[id, definition] : catalog().allDefinitions()) {
        for (DefinitionType type : types) {
          if (definition.row.type == static_cast<uint8>(type)) {
            ids.insert(id);
          }
        }
      }
      return ids;
    }

    // Only Live's neighborhoods and custom templates are listed; neighborhoods under a zone root are found by exploring.
    std::vector<Neighborhood> listedNeighborhoods() {
      std::vector<Neighborhood> neighborhoods;
      const auto neighborhoodDefinitions = definitionsOfType({DefinitionType::neighborhood});
      if (neighborhoodDefinitions.empty()) {
        return neighborhoods;
      }
      std::map<uint32, size_t> positions;
      for (auto &property : RealEstatePropertiesRepository::GetWhere(database, fmt::format(
        "parent_id = 0 AND archived = 0 AND definition_id IN ({}) ORDER BY id", idList(neighborhoodDefinitions)))) {
        positions[property.id] = neighborhoods.size();
        neighborhoods.push_back(Neighborhood{property, {}});
      }
      std::set<uint32> ids;
      for (const auto &[id, position] : positions) {
        ids.insert(id);
      }
      const auto plotDefinitions = definitionsOfType({DefinitionType::plot, DefinitionType::guildPlot});
      for (auto &plot : propertiesChildOf(ids)) {
        if (plotDefinitions.count(plot.definition_id)) {
          neighborhoods[positions.at(plot.parent_id)].plots.push_back(plot);
        }
      }
      return neighborhoods;
    }

    bool isGuildPlot(const Property &plot) {
      return typeOf(plot) == DefinitionType::guildPlot;
    }

    bool occupied(const Property &plot) {
      return isGuildPlot(plot) ? activeGuildId(plot) != 0 : plot.owner_character_id != 0;
    }

    std::map<uint32, CharacterIdentity> plotOwners(const std::vector<Neighborhood> &neighborhoods) {
      std::set<uint32> owners;
      for (const auto &neighborhood : neighborhoods) {
        for (const auto &plot : neighborhood.plots) {
          owners.insert(plot.owner_character_id);
        }
      }
      return characterIdentities(owners);
    }

    std::string ownerLabel(const Property &plot, const std::map<uint32, CharacterIdentity> &owners) {
      if (isGuildPlot(plot)) {
        const uint32 guild = activeGuildId(plot);
        return guild ? guild_mgr.GetGuildNameByID(guild) : std::string();
      }
      auto owner = owners.find(plot.owner_character_id);
      return owner == owners.end() ? std::string() : owner->second.name;
    }

    std::set<uint32> plotsHoldingItemsOf(uint32 characterId) {
      std::set<uint32> holders;
      for (auto row : query(database, fmt::format("SELECT DISTINCT property_id FROM real_estate_items WHERE owner_character_id = {}", characterId))) {
        holders.insert(Strings::ToUnsignedInt(row[0]));
      }
      std::map<uint32, Property> known;
      for (auto &property : propertiesById(holders)) {
        known[property.id] = property;
      }
      std::set<uint32> plots;
      for (uint32 holder : holders) {
        for (uint32 id = holder, depth = 0; id && depth <= 10; depth++) {
          if (!known.count(id)) {
            auto rows = propertiesById({id});
            if (rows.empty()) {
              break;
            }
            known[id] = rows.front();
          }
          const Property &property = known.at(id);
          const DefinitionType type = typeOf(property);
          if (type == DefinitionType::plot || type == DefinitionType::guildPlot) {
            plots.insert(id);
            break;
          }
          id = property.parent_id;
        }
      }
      return plots;
    }

    WireWriter replyHeader(uint8 kind) {
      WireWriter message;
      message.u32(0).u8(kind);
      return message;
    }

    void sendNeighborhoodList(Client *client, uint32 mode, uint32 selected, const std::vector<const Neighborhood *> &rows) {
      WireWriter message = replyHeader(listKind);
      message.u32(mode).u16(static_cast<uint16>(rows.size())).u8(1);
      if (!rows.empty()) {
        message.u32(selected);
        for (const Neighborhood *neighborhood : rows) {
          uint32 playerPlots = 0;
          uint32 playerPlotsUsed = 0;
          uint32 guildPlots = 0;
          uint32 guildPlotsUsed = 0;
          for (const auto &plot : neighborhood->plots) {
            uint32 &total = isGuildPlot(plot) ? guildPlots : playerPlots;
            uint32 &used = isGuildPlot(plot) ? guildPlotsUsed : playerPlotsUsed;
            total++;
            used += occupied(plot) ? 1 : 0;
          }
          const uint32 guild = activeGuildId(neighborhood->property);
          message.u32(neighborhood->property.id)
            .lengthString(neighborhood->property.name)
            .u32(guild ? guild : noGuild)
            .u32(playerPlots)
            .u32(playerPlotsUsed)
            .u32(guildPlots)
            .u32(guildPlotsUsed);
        }
      }
      sendMessage(client, OP_RealEstateSearch, message);
    }

    std::vector<const Neighborhood *> limited(std::vector<const Neighborhood *> rows) {
      const int limit = RuleI(Housing, NeighborhoodSearchLimit);
      if (limit < 0) {
        throw std::runtime_error(fmt::format("Housing:NeighborhoodSearchLimit is [{}]; it must be 0 (no limit) or more", limit));
      }
      if (limit > 0 && rows.size() > static_cast<size_t>(limit)) {
        rows.resize(static_cast<size_t>(limit));
      }
      return rows;
    }

    void searchNeighborhoods(Client *client, uint32 selected, const std::string &neighborhoodName, const std::string &playerName, const std::string &guildName) {
      const auto neighborhoods = listedNeighborhoods();
      const auto owners = plotOwners(neighborhoods);
      std::vector<const Neighborhood *> rows;
      for (const auto &neighborhood : neighborhoods) {
        if (!neighborhoodName.empty() && !Strings::ContainsLower(neighborhood.property.name, neighborhoodName)) {
          continue;
        }
        if (!playerName.empty() && std::none_of(neighborhood.plots.begin(), neighborhood.plots.end(), [&](const Property &plot) {
          return !isGuildPlot(plot) && Strings::EqualFold(ownerLabel(plot, owners), playerName);
        })) {
          continue;
        }
        if (!guildName.empty()) {
          const uint32 guild = activeGuildId(neighborhood.property);
          const bool villageOf = guild && Strings::EqualFold(guild_mgr.GetGuildNameByID(guild), guildName);
          const bool holdsPlotOf = std::any_of(neighborhood.plots.begin(), neighborhood.plots.end(), [&](const Property &plot) {
            return isGuildPlot(plot) && Strings::EqualFold(ownerLabel(plot, owners), guildName);
          });
          if (!villageOf && !holdsPlotOf) {
            continue;
          }
        }
        rows.push_back(&neighborhood);
      }
      sendNeighborhoodList(client, nameSearch, selected, limited(rows));
    }

    void searchMyItems(Client *client, uint32 selected) {
      const auto neighborhoods = listedNeighborhoods();
      const auto plots = plotsHoldingItemsOf(client->CharacterID());
      std::vector<const Neighborhood *> rows;
      for (const auto &neighborhood : neighborhoods) {
        if (std::any_of(neighborhood.plots.begin(), neighborhood.plots.end(), [&](const Property &plot) { return plots.count(plot.id) > 0; })) {
          rows.push_back(&neighborhood);
        }
      }
      sendNeighborhoodList(client, myItems, selected, limited(rows));
    }

    void sendNeighborhoodDetails(Client *client, uint32 neighborhoodId) {
      const auto neighborhoods = listedNeighborhoods();
      auto found = std::find_if(neighborhoods.begin(), neighborhoods.end(), [&](const Neighborhood &neighborhood) {
        return neighborhood.property.id == neighborhoodId;
      });
      if (found == neighborhoods.end()) {
        LogHousing("[{}] asked for the details of neighborhood [{}], which is not listed", client->GetCleanName(), neighborhoodId);
        return;
      }
      const auto owners = plotOwners(neighborhoods);
      std::vector<std::string> playerOwners;
      std::vector<uint32> guildOwners;
      for (const auto &plot : found->plots) {
        if (!occupied(plot)) {
          continue;
        }
        if (isGuildPlot(plot)) {
          guildOwners.push_back(activeGuildId(plot));
        }
        else {
          playerOwners.push_back(ownerLabel(plot, owners));
        }
      }
      WireWriter message = replyHeader(listKind);
      message.u32(neighborhoodDetails).u16(static_cast<uint16>(playerOwners.size()));
      for (const auto &name : playerOwners) {
        message.lengthString(name);
      }
      message.u16(static_cast<uint16>(guildOwners.size()));
      for (uint32 guild : guildOwners) {
        message.u32(guild);
      }
      sendMessage(client, OP_RealEstateSearch, message);
    }

    void searchPlots(Client *client, uint32 mode, uint32 neighborhoodId, const std::string &ownerName) {
      Session *state = session(client);
      if (!state || neighborhoodId != state->currentPropertyId || typeOf(state->properties.at(state->currentPropertyId)) != DefinitionType::neighborhood) {
        client->MessageString(Chat::Red, 8078);
        return;
      }
      const Property &neighborhood = state->properties.at(neighborhoodId);
      std::vector<Property> plots;
      for (auto &child : propertiesChildOf({neighborhoodId})) {
        const DefinitionType type = typeOf(child);
        if (type == DefinitionType::plot || type == DefinitionType::guildPlot) {
          plots.push_back(child);
        }
      }
      std::set<uint32> owners;
      for (const auto &plot : plots) {
        owners.insert(plot.owner_character_id);
      }
      const auto identities = characterIdentities(owners);
      const auto holdingMine = mode == plotSearchMyItems ? plotsHoldingItemsOf(client->CharacterID()) : std::set<uint32>();
      WireWriter rows;
      uint16 count = 0;
      for (const auto &plot : plots) {
        const std::string owner = ownerLabel(plot, identities);
        if (mode == plotSearchMyItems && !holdingMine.count(plot.id)) {
          continue;
        }
        if (mode == plotSearch && !ownerName.empty() && !Strings::EqualFold(owner, ownerName)) {
          continue;
        }
        // rowValue is what Go sends back in OP_FindPersonRequest type 4: the plot's property id. The fill (0x74c4c0) shows the first
        // string under Owner and the second under Address.
        rows.u32(plot.id)
          .u32(static_cast<uint32>(typeOf(plot)))
          .u32(plot.id)
          .lengthString(owner)
          .lengthString(catalog().definition(plot.definition_id)->row.address);
        count++;
      }
      WireWriter message = replyHeader(listKind);
      message.u32(mode).u16(count).lengthString(propertyName(neighborhood.id)).bytes(rows.content().data(), rows.content().size());
      sendMessage(client, OP_RealEstateSearch, message);
    }
  }

  void handleSearch(Client *client, WireReader &reader) {
    if (reader.remaining() != searchRequestSize) {
      LogError("Housing: [{}] sent a [{}]-byte search; it is [{}] bytes", client->GetCleanName(), reader.remaining(), searchRequestSize);
      return;
    }
    const uint32 type = reader.u32();
    const uint32 id = reader.u32();
    reader.u32();
    const std::string neighborhoodName = reader.fixedString(80);
    const std::string playerName = reader.fixedString(64);
    const std::string guildName = reader.fixedString(64);
    try {
      switch (type) {
        case nameSearch:
          searchNeighborhoods(client, id, neighborhoodName, playerName, guildName);
          break;
        case myItems:
          searchMyItems(client, id);
          break;
        case neighborhoodDetails:
          sendNeighborhoodDetails(client, id);
          break;
        case plotSearch:
        case plotSearchMyItems:
          searchPlots(client, type, id, playerName);
          break;
        default:
          LogError("Housing: [{}] sent search type [{}], which the client never sends", client->GetCleanName(), type);
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s search type [{}] failed: {}", client->GetCleanName(), type, error.what());
    }
  }

  // A neighborhood-list door (type 159) answers as an empty name search does: every listed neighborhood, the window shown.
  void openNeighborhoodList(Client *client) {
    searchNeighborhoods(client, 0, {}, {}, {});
  }

  void sendPropertyWindow(Client *client, const Property &property, bool openPurchase) {
    const DefinitionType type = typeOf(property);
    if (type != DefinitionType::plot && type != DefinitionType::guildPlot) {
      throw std::logic_error(fmt::format("property [{}] is of type [{}]; the property window shows only plots", property.id, static_cast<uint32>(type)));
    }
    const auto owners = characterIdentities({property.owner_character_id});
    auto owner = owners.find(property.owner_character_id);
    const Definition &definition = *catalog().definition(property.definition_id);
    const uint32 guild = activeGuildId(property);
    WireWriter message = replyHeader(static_cast<uint8>(type));
    // The four u32s RoF2 skips are Live's Capacity, Vault and Pets, then 0 (OPC-5).
    message.u32(0)
      .u16(1)
      .u32(property.id)
      .u32(property.parent_id)
      .u32(property.definition_id)
      .u32(guild ? guild : noGuild)
      .u32(0)
      .lengthString("")
      .lengthString(propertyName(property.id))
      .lengthString(owner == owners.end() ? "" : std::to_string(owner->second.accountId))
      .lengthString(owner == owners.end() ? "" : owner->second.name)
      .u32(definition.row.max_items)
      .u32(definition.row.max_non_real_estate_items)
      .i32(definition.row.max_npc_items)
      .u32(0)
      .u8(openPurchase ? 1 : 0);
    sendMessage(client, OP_RealEstateSearch, message);
  }
}
