#ifndef EQEMU_COMMON_HOUSING_EVENT_TYPES_H
#define EQEMU_COMMON_HOUSING_EVENT_TYPES_H

#include "types.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

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

  // An itemListChanged event carries the records whose place changed, so the zone showing their property removes a withdrawn
  // record's world object or spawn and shows a placed one (INT-30); the lists themselves are re-read from the database (D2).
  struct ItemListChangedRecord {
    uint32 recordId;
    uint32 propertyId;
    uint32 itemId;
    uint32 placed;
  };

  inline std::string itemListChangedPayload(const std::vector<ItemListChangedRecord> &records) {
    return std::string(reinterpret_cast<const char *>(records.data()), records.size() * sizeof(ItemListChangedRecord));
  }

  inline std::vector<ItemListChangedRecord> itemListChangedRecords(const uint8 *payload, uint32 size) {
    if (size % sizeof(ItemListChangedRecord) != 0) {
      throw std::invalid_argument("an item list payload of " + std::to_string(size) + " bytes is not whole records of " + std::to_string(sizeof(ItemListChangedRecord)));
    }
    std::vector<ItemListChangedRecord> records(size / sizeof(ItemListChangedRecord));
    if (size) {
      memcpy(records.data(), payload, size);
    }
    return records;
  }

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
