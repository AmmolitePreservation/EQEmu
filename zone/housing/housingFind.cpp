#include "housing.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingItems.h"
#include "housingNpcItems.h"
#include "housingSession.h"
#include "housingWorldObjects.h"

#include "../../common/eqemu_logsys.h"
#include "../client.h"
#include "../npc.h"

#include <stdexcept>

namespace housing {
  namespace {
    constexpr uint32 locateItem = 3;
    constexpr uint32 findPlot = 4;

    std::optional<glm::vec3> placedItemPosition(const Session &state, uint32 propertyId, uint32 recordId) {
      auto holder = state.properties.find(propertyId);
      if (holder == state.properties.end() || !shownHere(holder->second, state)) {
        return std::nullopt;
      }
      const auto row = placedItem(propertyId, recordId);
      if (!row || !row->placed) {
        return std::nullopt;
      }
      if (isNpcItemId(row->item_id)) {
        NPC *spawn = npcItemSpawn(row->id);
        return spawn ? std::optional<glm::vec3>(glm::vec3(spawn->GetX(), spawn->GetY(), spawn->GetZ())) : std::nullopt;
      }
      return glm::vec3(row->x, row->y, row->z);
    }

    // Plot Search's Go sends back the row's value, the plot's property id (housingSearch.cpp), and the wisp leads to its region's center.
    std::optional<glm::vec3> plotPosition(const Session &state, uint32 propertyId) {
      if (!state.hostedIds.count(propertyId)) {
        return std::nullopt;
      }
      const Definition *definition = catalog().definition(state.properties.at(propertyId).definition_id);
      const auto *region = definition && definition->row.area_id ? catalog().region(definition->row.area_id) : nullptr;
      return region ? std::optional<glm::vec3>(glm::vec3(region->x, region->y, region->z)) : std::nullopt;
    }
  }

  // Locate (type 3) and Plot Search's Go (type 4) take the client's /find path (OP_FindPersonReply); an empty reply says no path.
  void handleFindRequest(Client *client, uint32 type, int32 propertyId, int32 recordId) {
    Session *state = session(client);
    std::optional<glm::vec3> destination;
    try {
      if (state && housingActive()) {
        destination = type == locateItem ? placedItemPosition(*state, static_cast<uint32>(propertyId), static_cast<uint32>(recordId))
          : type == findPlot ? plotPosition(*state, static_cast<uint32>(propertyId)) : std::nullopt;
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s find type [{}] for [{}] [{}] failed: {}", client->GetCleanName(), type, propertyId, recordId, error.what());
    }
    if (!destination) {
      LogHousing("[{}] asked to find [{}] [{}] (type [{}]), which is not here", client->GetCleanName(), propertyId, recordId, type);
      client->SendPath(static_cast<Mob *>(nullptr));
      return;
    }
    client->SendPath(*destination, 0.0f);
  }
}
