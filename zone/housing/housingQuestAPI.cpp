#include "housing.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingHandlers.h"
#include "housingInteriors.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../client.h"

#include <cstring>
#include <stdexcept>

// Quests reach housing only through its own rules (EXT-28): the Purchase window after the property's record, entry with Enter,
// the upkeep lockout and the shared copies, the way out /leaverealestate takes, a writ's neighborhood for the user's guild, and
// whether a client holds a permission on a property.
namespace housing {
  bool questOpenPurchase(Client *client, uint32 propertyId) {
    if (!session(client)) {
      LogHousing("A quest opened the Purchase window of property [{}] for [{}], who has no housing session here", propertyId, client->GetCleanName());
      return false;
    }
    try {
      auto rows = propertiesById({propertyId});
      const DefinitionType type = rows.size() == 1 ? typeOf(rows.front()) : DefinitionType::none;
      if (type != DefinitionType::plot && type != DefinitionType::guildPlot) {
        LogHousing("A quest opened the Purchase window of property [{}], which is no plot", propertyId);
        return false;
      }
      if (!session(client)->properties.count(propertyId)) {
        refreshClients({propertyId}, {client->CharacterID()});
      }
      Session *state = session(client);
      if (!state || !state->properties.count(propertyId)) {
        throw std::runtime_error(fmt::format("property [{}]'s record did not reach [{}]", propertyId, client->GetCleanName()));
      }
      sendPropertyWindow(client, state->properties.at(propertyId), true);
      return true;
    }
    catch (const std::exception &error) {
      LogError("Housing: a quest's Purchase window of property [{}] for [{}] failed: {}", propertyId, client->GetCleanName(), error.what());
      return false;
    }
  }

  bool questEnter(Client *client, uint32 propertyId) {
    if (!housingActive()) {
      LogHousing("A quest sent [{}] into property [{}] with no housing catalog here", client->GetCleanName(), propertyId);
      return false;
    }
    try {
      auto rows = propertiesById({propertyId});
      if (rows.size() != 1) {
        LogHousing("A quest sent [{}] into property [{}], which does not exist", client->GetCleanName(), propertyId);
        return false;
      }
      const DefinitionType type = typeOf(rows.front());
      const bool listedNeighborhood = type == DefinitionType::neighborhood && !rows.front().parent_id;
      if (type != DefinitionType::playerHouse && type != DefinitionType::guildHall && !listedNeighborhood) {
        LogHousing("A quest sent [{}] into property [{}], which has no interior or neighborhood to enter", client->GetCleanName(), propertyId);
        return false;
      }
      const bool open = checkEntry(client, propertyId).state == EntryState::open;
      enterProperty(client, rows.front());
      return open;
    }
    catch (const std::exception &error) {
      LogError("Housing: a quest's entry of [{}] into property [{}] failed: {}", client->GetCleanName(), propertyId, error.what());
      return false;
    }
  }

  void questLeave(Client *client) {
    if (!housingActive()) {
      LogHousing("A quest took [{}] out of real estate with no housing catalog here", client->GetCleanName());
      return;
    }
    handleLeave(client);
  }

  // Live's writ casts the do-nothing 7652, so the neighborhood is the server's work (EXT-33); the writ's item script checks the guild
  // and rank and calls this before the cast (EXT-36). A client already casting would have the writ's cast refused after the village
  // was made, and the writ would survive.
  bool questUseNeighborhoodWrit(Client *client, uint32 definitionId) {
    if (!housingActive()) {
      LogHousing("A quest used a neighborhood writ for [{}] with no housing catalog here", client->GetCleanName());
      return false;
    }
    const Definition *definition = catalog().definition(definitionId);
    if (!definition || definition->row.type != static_cast<uint8>(DefinitionType::neighborhood)) {
      LogHousing("A quest used a neighborhood writ for [{}] naming definition [{}], which is no neighborhood", client->GetCleanName(), definitionId);
      return false;
    }
    if (!client->IsInAGuild()) {
      LogHousing("A quest used a neighborhood writ for [{}], who is in no guild", client->GetCleanName());
      return false;
    }
    if (client->IsCasting()) {
      LogHousing("A quest used a neighborhood writ for [{}] while they were casting", client->GetCleanName());
      return false;
    }
    WritUsedPayload payload{client->CharacterID(), client->GuildID(), definitionId, {}};
    strncpy(payload.characterName, client->GetCleanName(), sizeof(payload.characterName) - 1);
    sendEvent(EventType::writUsed, EventTarget::guild, client->GuildID(), 0, std::string(reinterpret_cast<const char *>(&payload), sizeof(payload)));
    LogHousing("[{}] used a neighborhood writ for guild [{}]: neighborhood definition [{}]", client->GetCleanName(), client->GuildID(), definitionId);
    return true;
  }

  // Whether the client holds a permission (Permission, 0 Receive Buffs to 12 Manage) on a property this zone sent them; the
  // pool's aura script asks for Receive Buffs (NPC-34).
  bool questHasPermission(Client *client, uint32 propertyId, int permission) {
    if (permission < 0 || permission > static_cast<int>(Permission::manage)) {
      throw std::invalid_argument(fmt::format("real estate permission [{}] is outside 0-{}", permission, static_cast<int>(Permission::manage)));
    }
    Session *state = session(client);
    if (!state) {
      return false;
    }
    auto property = state->properties.find(propertyId);
    if (property == state->properties.end()) {
      LogHousing("A quest asked whether [{}] holds permission [{}] on property [{}], which this zone did not send them", client->GetCleanName(), permission, propertyId);
      return false;
    }
    return permitted(client, static_cast<Permission>(permission), property->second, state->lookup(), state->accessLookup());
  }
}
