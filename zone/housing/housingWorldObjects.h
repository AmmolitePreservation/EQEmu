#ifndef EQEMU_ZONE_HOUSING_HOUSING_WORLD_OBJECTS_H
#define EQEMU_ZONE_HOUSING_HOUSING_WORLD_OBJECTS_H

#include "housingItems.h"
#include "housingSession.h"
#include "housingWire.h"

#include "../../common/housingPlacementRules.h"

#include <set>
#include <string>

class Client;

namespace housing {
  // The placement object's transform, in its own order: scale, heading, pitch, roll, then the position (worldLayer.md).
  struct ObjectTransform {
    float scale;
    float heading;
    float pitch;
    float roll;
    float x;
    float y;
    float z;
  };

  struct PlacementObject {
    std::string model;
    bool npcItem;
    float npcSize;
    uint32 itemId;
    std::string guid;
    uint32 propertyId;
    uint32 recordId;
    ObjectTransform transform;
    bool ignoreCollisions;
    bool disablePlacementRotation;
    bool disableFreePlacement;
    uint32 placementType;
    float scaleMinimum;
    float scaleMaximum;
    float scaleDefault;
    float headingOffset;
    MountTilts tilts;
    uint32 lightType;
  };

  MountTilts mountTilts(const EQ::ItemInstance &instance);
  PlacementObject placementObject(const EQ::ItemInstance &instance, const std::string &guid, uint32 propertyId, uint32 recordId, const ObjectTransform &transform);
  ObjectTransform rowTransform(const PlacedItem &row);
  void writePlacementObject(WireWriter &message, const PlacementObject &object);

  // Whose placed items stand in this zone: the outdoor properties hosted here, and the current property inside an interior.
  bool standsOutdoors(const Property &property);
  bool shownHere(const Property &property, const Session &state);
  void sendWorldObjects(Client *client, const Session &state);
  void broadcastWorldObject(const PlacedItem &row);
  void broadcastWorldObjectRemove(const PlacedItem &row);
  void sendPlacedItemMoved(Client *client, const PlacedItem &row, const ItemLocation &location);
  WireWriter itemReference(const std::string &guid, const ItemLocation &location, uint32 propertyId, uint32 recordId);
}

#endif
