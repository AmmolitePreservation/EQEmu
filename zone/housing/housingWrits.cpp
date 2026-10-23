#include "housing.h"

#include "housingSession.h"
#include "housingWire.h"
#include "housingWrits.h"

#include "../client.h"
#include "../entity.h"

#include <cstring>

extern EntityList entity_list;

namespace housing {
  namespace {
    constexpr size_t nameLength = 64;
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
