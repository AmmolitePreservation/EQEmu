#ifndef EQEMU_ZONE_HOUSING_HOUSING_ACCESS_H
#define EQEMU_ZONE_HOUSING_HOUSING_ACCESS_H

#include "housingCatalog.h"
#include "housingStore.h"
#include "housingWire.h"

#include "../../common/housingAccessRules.h"

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

  constexpr uint32 houseOfThule = 0x10000;
  constexpr uint32 veilOfAlaris = 0x20000;

  struct AccessList {
    AccessMasks masks{};
    AccessNames names;
  };

  using PropertyLookup = std::function<const Property *(uint32)>;
  using AccessListLookup = std::function<const AccessList *(uint32)>;

  constexpr uint32 typeBit(DefinitionType type) { return 1u << (static_cast<uint32>(type) - 1); }
  DefinitionType typeOf(const Property &property);
  uint32 activeGuildId(const Property &property);
  const Property *ancestorOfType(const Property &property, uint32 typeMask, bool startAtParent, const PropertyLookup &lookup);
  const Property *accessSubject(const Property &property, const PropertyLookup &lookup);
  bool holdsAccessList(DefinitionType type);

  std::map<uint32, AccessList> accessLists(const std::set<uint32> &propertyIds);
  WireWriter accessListMessage(uint32 propertyId, const AccessList &list);
  WireWriter accessListRemoveMessage(uint32 propertyId);
  uint8 accessGroup(Client *client, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf);
  bool permitted(Client *client, Permission permission, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf);
  bool mayTransact(Client *client, const Property &property, const PropertyLookup &lookup, const AccessListLookup &listOf);
  void handleSetAccessList(Client *client, WireReader &reader);
  void applyAccessChange(uint32 propertyId);
}

#endif
