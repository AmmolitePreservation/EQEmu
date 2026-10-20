#include "housing.h"

#include "housingAccess.h"
#include "housingHandlers.h"
#include "housingInteriors.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../client.h"

#include <stdexcept>

// Quests reach housing only through its own rules (EXT-28): the Purchase window after the property's record, entry with Enter,
// the upkeep lockout and the shared copies, and the way out /leaverealestate takes.
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
      LogHousing("A quest sent [{}] into property [{}] where housing is off", client->GetCleanName(), propertyId);
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
      LogHousing("A quest took [{}] out of real estate where housing is off", client->GetCleanName());
      return;
    }
    handleLeave(client);
  }
}
