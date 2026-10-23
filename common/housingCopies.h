#ifndef EQEMU_COMMON_HOUSING_COPIES_H
#define EQEMU_COMMON_HOUSING_COPIES_H

#include "housingInstanceNumbers.h"
#include "housingLedger.h"

#include <optional>
#include <set>

class Database;

namespace housing {
  // A definition's dz_id template: the zone a copy of it runs and where entering puts a player.
  struct ZoneTemplatePoint {
    uint32 zoneId;
    uint8 zoneVersion;
    float x;
    float y;
    float z;
    float heading;
  };

  ZoneTemplatePoint zoneTemplateOf(Database &content, uint32 definitionId);
  PropertyRow rootOf(Database &database, const PropertyRow &property);
  // A property's zone is its root's template's: a neighborhood's, or a zone root's public zone (consequence 2).
  uint32 hostZoneOf(Database &database, Database &content, const PropertyRow &property);

  uint16 existingCopy(Database &database, uint32 propertyId);
  uint32 propertyOfCopy(Database &database, uint16 instanceId);
  uint16 runningCopy(Database &database, Database &content, uint32 propertyId);
  void discardCopy(Database &database, uint16 instanceId);
}

#endif
