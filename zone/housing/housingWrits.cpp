#include "housing.h"

#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingSession.h"
#include "housingWire.h"
#include "housingWrits.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/guilds.h"
#include "../../common/housingLedger.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../entity.h"
#include "../guild_mgr.h"
#include "../zonedb.h"

#include <cstring>
#include <map>

extern EntityList entity_list;

namespace housing {
  namespace {
    constexpr uint32 guildCannotBuy = 754;
    constexpr uint32 notInGuild = 755;
    constexpr size_t nameLength = 64;

    std::map<uint32, uint32> writs;
  }

  void loadWrits() {
    writs.clear();
    for (auto row : query(content_db, "SELECT item_id, definition_id FROM real_estate_writs")) {
      const uint32 definitionId = Strings::ToUnsignedInt(row[1]);
      const Definition *definition = catalog().definition(definitionId);
      if (!definition || definition->row.type != static_cast<uint8>(DefinitionType::neighborhood)) {
        throw std::runtime_error(fmt::format("writ [{}] names definition [{}], which is not a neighborhood", row[0], definitionId));
      }
      writs[Strings::ToUnsignedInt(row[0])] = definitionId;
    }
  }

  // Live's writ casts the do-nothing 7652, so the neighborhood is the server's work; who may use one is Peridot's choice, a guild
  // member whose rank may buy guild plots (permission 27), since Live's rule costs Daybreak Cash to observe (EXT-33).
  bool itemCastSpends(Client *client, uint32 inventorySlot) {
    if (!housingActive()) {
      return true;
    }
    const EQ::ItemInstance *instance = client->GetInv().GetItem(static_cast<int16>(inventorySlot));
    if (!instance) {
      return true;
    }
    auto writ = writs.find(instance->GetID());
    if (writ == writs.end()) {
      return true;
    }
    if (!client->IsInAGuild()) {
      client->MessageString(Chat::Red, notInGuild);
      return false;
    }
    if (!guild_mgr.CheckPermission(client->GuildID(), client->GuildRank(), GUILD_ACTION_REAL_ESTATE_GUILD_PLOT_BUY)) {
      client->MessageString(Chat::Red, guildCannotBuy);
      return false;
    }
    WritUsedPayload payload{client->CharacterID(), client->GuildID(), writ->second, {}};
    strncpy(payload.characterName, client->GetCleanName(), sizeof(payload.characterName) - 1);
    sendEvent(EventType::writUsed, EventTarget::guild, client->GuildID(), 0, std::string(reinterpret_cast<const char *>(&payload), sizeof(payload)));
    LogHousing("[{}] used writ [{}] for guild [{}]: neighborhood definition [{}]", client->GetCleanName(), writ->first, client->GuildID(), writ->second);
    return true;
  }

  // 0x1d3c {guild id, char[64] the writ's user, char[64] the neighborhood} prints eqstr 5320 to the guild's members (README).
  void announceGuildNeighborhood(const GuildNeighborhoodCreatedPayload &created) {
    WireWriter message;
    message.u32(created.guildId)
      .fixedString(std::string(created.characterName, strnlen(created.characterName, sizeof(created.characterName))), nameLength)
      .fixedString(std::string(created.neighborhoodName, strnlen(created.neighborhoodName, sizeof(created.neighborhoodName))), nameLength);
    for (const auto &[id, client] : entity_list.GetClientList()) {
      if (client->ClientVersion() == EQ::versions::ClientVersion::RoF2 && client->GuildID() == created.guildId) {
        sendMessage(client, OP_RealEstateGuildNeighborhoodCreated, message);
      }
    }
  }
}
