#ifndef EQEMU_ZONE_HOUSING_HOUSING_ACCESS_H
#define EQEMU_ZONE_HOUSING_HOUSING_ACCESS_H

#include "housingCatalog.h"
#include "housingStore.h"
#include "housingWire.h"

#include <array>
#include <functional>

class Client;

namespace housing {
  enum class Permission : uint8 {
    receiveBuffs = 0,
    enter = 1,
    withdrawUpkeep = 2,
    removeCrate = 3,
    addCrate = 4,
    removeHouse = 5,
    addHouse = 6,
    mailItem = 7,
    removeItem = 8,
    addItem = 9,
    moveItem = 10,
    interactItem = 11,
    manage = 12
  };

  constexpr uint8 accessGroupCount = 7;
  constexpr uint8 ownerGroup = 6;

  struct AccessList {
    std::array<uint32, accessGroupCount> masks{};
    std::vector<std::pair<std::string, uint8>> names;
  };

  using PropertyLookup = std::function<const Property *(uint32)>;
  using AccessListLookup = std::function<const AccessList *(uint32)>;

  constexpr uint32 typeBit(DefinitionType type) { return 1u << (static_cast<uint32>(type) - 1); }
  DefinitionType typeOf(const Property &property);
  const Property *ancestorOfType(const Property &property, uint32 typeMask, bool startAtParent, const PropertyLookup &lookup);

  std::map<uint32, AccessList> accessLists(const std::set<uint32> &propertyIds);
  WireWriter accessListMessage(uint32 propertyId, const AccessList &list);
  uint8 accessGroup(Client *client, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf);
  bool permitted(Client *client, Permission permission, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf);
}

#endif
