#include "housing.h"

#include "housingAccess.h"
#include "housingHandlers.h"
#include "housingInteriors.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingLedger.h"
#include "../client.h"
#include "../doors.h"

#include <stdexcept>

namespace housing {
  namespace {
    constexpr uint8 neighborhoodListDoor = 159;
    constexpr uint8 plotDoor = 160;
    constexpr uint8 frontDoor = 161;

    bool isInterior(const Property &property) {
      const DefinitionType type = typeOf(property);
      return type == DefinitionType::playerHouse || type == DefinitionType::guildHall;
    }

    const Property *hostedPlotOf(const Session &state, uint32 definitionId) {
      const Property *found = nullptr;
      for (uint32 id : state.hostedIds) {
        const Property &property = state.properties.at(id);
        const DefinitionType type = typeOf(property);
        if (property.definition_id != definitionId || (type != DefinitionType::plot && type != DefinitionType::guildPlot)) {
          continue;
        }
        if (found) {
          throw std::runtime_error(fmt::format("plots [{}] and [{}] here are both of definition [{}]", found->id, id, definitionId));
        }
        found = &property;
      }
      return found;
    }

    // An apartment (X4) is a plot with a fixed house: its entrance opens the Purchase window while vacant and enters the house once owned.
    void enterApartment(Client *client, const Property &plot) {
      const bool vacant = typeOf(plot) == DefinitionType::guildPlot ? !activeGuildId(plot) : !plot.owner_character_id;
      if (!vacant) {
        for (const auto &child : propertiesChildOf({plot.id})) {
          if (isInterior(child)) {
            enterProperty(client, child);
            return;
          }
        }
      }
      sendPropertyWindow(client, plot, true);
    }
  }

  // A click on 159-161 is a plain OP_ClickDoor with the door id alone (DOR-5), so the door's RealEstateDoorID, a column on EQEmu's
  // doors (DOR-19), says what it opens (DOR-3, EXT-27).
  bool handleDoorClick(Client *client, Doors *door) {
    const uint8 openType = door->GetOpenType();
    const uint32 realEstateDoorId = door->GetRealEstateDoorID();
    if (openType != neighborhoodListDoor && openType != plotDoor && openType != frontDoor && !realEstateDoorId) {
      return false;
    }
    Session *state = session(client);
    if (!housingActive() || !state) {
      LogHousing("[{}] clicked housing door [{}] with no housing session here", client->GetCleanName(), door->GetDoorID());
      return true;
    }
    try {
      switch (openType) {
        case neighborhoodListDoor:
          openNeighborhoodList(client);
          break;
        case plotDoor: {
          const Property *plot = hostedPlotOf(*state, realEstateDoorId);
          if (!plot) {
            LogHousing("[{}] clicked border door [{}] of plot definition [{}], which no plot here has", client->GetCleanName(), door->GetDoorID(), realEstateDoorId);
            break;
          }
          sendPropertyWindow(client, *plot, true);
          break;
        }
        default:
          if (openType == frontDoor && !realEstateDoorId) {
            if (hostsInterior()) {
              leaveForExitPoint(client);
            }
            else {
              LogHousing("[{}] clicked front door [{}] outside an interior", client->GetCleanName(), door->GetDoorID());
            }
            break;
          }
          if (const Property *plot = hostedPlotOf(*state, realEstateDoorId)) {
            enterApartment(client, *plot);
          }
          else {
            LogHousing("[{}] clicked entrance [{}] of plot definition [{}], which no plot here has", client->GetCleanName(), door->GetDoorID(), realEstateDoorId);
          }
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s click on door [{}] failed: {}", client->GetCleanName(), door->GetDoorID(), error.what());
    }
    return true;
  }
}
