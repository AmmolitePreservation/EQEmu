#ifndef EQEMU_COMMON_HOUSING_EVENT_TYPES_H
#define EQEMU_COMMON_HOUSING_EVENT_TYPES_H

#include "types.h"

namespace housing {
  enum class EventType : uint32 {
    catalogChanged = 1,
    propertyChanged = 2,
    accessChanged = 3,
    itemListChanged = 4,
    writUsed = 5,
    guildNeighborhoodCreated = 6,
    pollsChanged = 7
  };

  enum class EventTarget : uint32 {
    character = 1,
    guild = 2,
    property = 3,
    zoneInstance = 4,
    allZones = 5
  };

  // A propertyChanged event names the owners before and after, so their property lists follow and world sees ownership change.
  struct PropertyChangedPayload {
    uint32 previousOwnerCharacterId;
    uint32 ownerCharacterId;
    uint32 previousGuildId;
    uint32 guildId;
  };

  // A zone tells world a writ was used; world makes the guild's village of the writ's definition (EXT-33).
  struct WritUsedPayload {
    uint32 characterId;
    uint32 guildId;
    uint32 definitionId;
    char characterName[64];
  };

  // World tells the zones a guild's village exists, for 0x1d3c to the guild's members.
  struct GuildNeighborhoodCreatedPayload {
    uint32 guildId;
    char characterName[64];
    char neighborhoodName[64];
  };
}

#endif
