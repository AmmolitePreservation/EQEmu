#ifndef EQEMU_COMMON_HOUSING_PLACEMENT_RULES_H
#define EQEMU_COMMON_HOUSING_PLACEMENT_RULES_H

#include "types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace housing {
  // 0x1228's result codes (resultCodes.md); 0 ends a placement as committed.
  enum class ItemResult : uint32 {
    success = 0,
    failed = 1,
    noInventoryRoom = 4,
    notFound = 9,
    full = 10,
    nothingToTransfer = 11,
    vaultFull = 12,
    houseLimit = 15,
    crateLimit = 16,
    notAllowedHere = 17,
    containerHoldsItems = 19,
    crateRequired = 22,
    plotUpkeepExpired = 24,
    itemUpkeepExpired = 25,
    intoOwnProperty = 30,
    itemLimit = 31,
    nonEmptyIntoStorage = 32,
    pickUpLimit = 33,
    notPlaceableZone = 35,
    notPlaceable = 36,
    npcLimit = 38,
    crateHoldsNonEmptyHouse = 39,
    needsHouseOfThule = 47,
    swapItemsFull = 48,
    swapVaultFull = 49,
    swapNpcItemsFull = 50,
    swapNoHousePlaced = 53,
    swapAlreadyPlaced = 55,
    swapPlacedCrate = 59,
    noDropNotOwner = 64,
    loreHeld = 65,
    ownItem = 66,
    carriesRealEstate = 67,
    hallLimit = 68,
    noAddItem = 71,
    noRemoveItem = 72,
    noMoveItem = 73,
    noInteractItem = 74,
    noAddHouse = 75,
    noRemoveHouse = 76,
    noAddCrate = 77,
    noRemoveCrate = 78,
    noMailItem = 79,
    houseHoldsPlacedCrate = 81,
    needsVeilOfAlaris = 82
  };

  // Where the item comes from picks the client's rule set (RealEstateItemsWnd.md, Can-place): from inventory 0x5b69b0, from
  // storage in the destination itself 0x5b6eb0, from another property 0x5b5ff0.
  enum class PlacementSource : uint8 { inventory, sameProperty, otherProperty };

  enum class CarriedKind : uint8 { none, house, guildHall, crate };

  struct PlacementFacts {
    PlacementSource source = PlacementSource::inventory;
    bool houseOfThule = false;
    bool underGuildPlot = false;
    bool veilOfAlaris = false;
    bool plotUpkeepExpired = false;
    bool placeable = false;
    bool intoOwnProperty = false;
    bool allowedHere = false;
    CarriedKind carried = CarriedKind::none;
    bool carriedHoldsItems = false;
    bool carriedHouseHoldsPlacedCrate = false;
    bool intoStorage = false;
    bool destinationIsCrate = false;
    uint32 destinationHouses = 0;
    uint32 maxHouses = 0;
    bool destinationHasPlacedCrate = false;
    bool crateHoldsNonEmptyHouse = false;
    uint32 realEstateCount = 0;
    uint32 maxItems = 0;
    uint32 vaultCount = 0;
    uint32 maxNonRealEstateItems = 0;
    bool npcItem = false;
    uint32 npcCount = 0;
    int32 npcCap = -1;
    bool mergesIntoStack = false;
    bool containerHoldsItems = false;
    bool itemUpkeepExpired = false;
    bool mayAdd = false;
    bool mayMove = false;
    bool mayRemoveHouse = false;
    int32 maxPerRealEstate = -1;
    uint32 sameItemCount = 0;
  };

  inline bool isHouseKind(CarriedKind kind) {
    return kind == CarriedKind::house || kind == CarriedKind::guildHall;
  }

  inline ItemResult addRefusal(CarriedKind kind) {
    if (isHouseKind(kind)) {
      return ItemResult::noAddHouse;
    }
    return kind == CarriedKind::crate ? ItemResult::noAddCrate : ItemResult::noAddItem;
  }

  inline ItemResult houseLimitResult(const PlacementFacts &facts) {
    if (isHouseKind(facts.carried) && !facts.destinationIsCrate && facts.destinationHouses >= facts.maxHouses) {
      return facts.carried == CarriedKind::guildHall ? ItemResult::hallLimit : ItemResult::houseLimit;
    }
    if (facts.carried == CarriedKind::crate && !facts.intoStorage && facts.destinationHasPlacedCrate) {
      return ItemResult::crateLimit;
    }
    return ItemResult::success;
  }

  inline ItemResult crateRuleResult(const PlacementFacts &facts) {
    const bool carrierHoldsItems = facts.carried != CarriedKind::none && facts.carriedHoldsItems;
    if (carrierHoldsItems && facts.intoStorage && !facts.destinationIsCrate) {
      return ItemResult::nonEmptyIntoStorage;
    }
    if (carrierHoldsItems && facts.destinationIsCrate && facts.carried == CarriedKind::crate) {
      return ItemResult::nonEmptyIntoStorage;
    }
    if (carrierHoldsItems && facts.destinationIsCrate && isHouseKind(facts.carried) && facts.crateHoldsNonEmptyHouse) {
      return ItemResult::crateHoldsNonEmptyHouse;
    }
    if (facts.destinationIsCrate && isHouseKind(facts.carried) && facts.carriedHouseHoldsPlacedCrate) {
      return ItemResult::houseHoldsPlacedCrate;
    }
    return ItemResult::success;
  }

  // 10a and 10b: a stack the item merges into waives the capacity codes.
  inline ItemResult capacityResult(const PlacementFacts &facts) {
    if (facts.mergesIntoStack) {
      return ItemResult::success;
    }
    if (facts.placeable && facts.realEstateCount >= facts.maxItems) {
      return ItemResult::full;
    }
    if (!facts.placeable && facts.vaultCount >= facts.maxNonRealEstateItems) {
      return ItemResult::vaultFull;
    }
    if (facts.npcItem && facts.npcCap >= 0 && facts.npcCount >= static_cast<uint32>(facts.npcCap)) {
      return ItemResult::npcLimit;
    }
    return ItemResult::success;
  }

  // A non-placeable item may only be stored (the vault), so the placing rules apply to what is placeable or placed.
  inline bool placesItem(const PlacementFacts &facts) {
    return facts.placeable || !facts.intoStorage;
  }

  inline ItemResult inventoryPlacementResult(const PlacementFacts &facts) {
    if (!facts.houseOfThule) {
      return ItemResult::needsHouseOfThule;
    }
    if (facts.underGuildPlot && !facts.veilOfAlaris) {
      return ItemResult::needsVeilOfAlaris;
    }
    if (facts.plotUpkeepExpired) {
      return ItemResult::plotUpkeepExpired;
    }
    if (placesItem(facts) && !facts.placeable) {
      return ItemResult::notPlaceable;
    }
    if (facts.intoOwnProperty) {
      return ItemResult::intoOwnProperty;
    }
    if (placesItem(facts) && !facts.allowedHere) {
      return ItemResult::notAllowedHere;
    }
    for (ItemResult result : {houseLimitResult(facts), crateRuleResult(facts), capacityResult(facts)}) {
      if (result != ItemResult::success) {
        return result;
      }
    }
    if (facts.containerHoldsItems) {
      return ItemResult::containerHoldsItems;
    }
    if (facts.itemUpkeepExpired) {
      return ItemResult::itemUpkeepExpired;
    }
    if (!facts.mayAdd) {
      return addRefusal(facts.carried);
    }
    return ItemResult::success;
  }

  inline ItemResult samePropertyPlacementResult(const PlacementFacts &facts) {
    if (placesItem(facts) && !facts.placeable) {
      return ItemResult::notPlaceable;
    }
    if (placesItem(facts) && !facts.allowedHere) {
      return ItemResult::notAllowedHere;
    }
    for (ItemResult result : {houseLimitResult(facts), crateRuleResult(facts)}) {
      if (result != ItemResult::success) {
        return result;
      }
    }
    if (!facts.mayMove) {
      return ItemResult::noMoveItem;
    }
    if (facts.plotUpkeepExpired) {
      return ItemResult::plotUpkeepExpired;
    }
    if (facts.itemUpkeepExpired) {
      return ItemResult::itemUpkeepExpired;
    }
    if (isHouseKind(facts.carried) && !facts.mayRemoveHouse) {
      return ItemResult::noRemoveHouse;
    }
    if (isHouseKind(facts.carried) && !facts.mayAdd) {
      return ItemResult::noAddHouse;
    }
    return ItemResult::success;
  }

  inline ItemResult otherPropertyPlacementResult(const PlacementFacts &facts) {
    if (!facts.houseOfThule) {
      return ItemResult::needsHouseOfThule;
    }
    if (facts.underGuildPlot && !facts.veilOfAlaris) {
      return ItemResult::needsVeilOfAlaris;
    }
    if (facts.intoOwnProperty) {
      return ItemResult::intoOwnProperty;
    }
    if (placesItem(facts) && !facts.placeable) {
      return ItemResult::notPlaceable;
    }
    if (placesItem(facts) && !facts.allowedHere) {
      return ItemResult::notAllowedHere;
    }
    for (ItemResult result : {houseLimitResult(facts), crateRuleResult(facts), capacityResult(facts)}) {
      if (result != ItemResult::success) {
        return result;
      }
    }
    if (facts.plotUpkeepExpired) {
      return ItemResult::plotUpkeepExpired;
    }
    if (facts.itemUpkeepExpired) {
      return ItemResult::itemUpkeepExpired;
    }
    if (!facts.mayMove) {
      return ItemResult::noMoveItem;
    }
    if (!facts.mayAdd) {
      return addRefusal(facts.carried);
    }
    return ItemResult::success;
  }

  // The client's rule set for the source, then the per-item limit, which only the server checks (0x1f).
  inline ItemResult placementResult(const PlacementFacts &facts) {
    ItemResult result = ItemResult::success;
    switch (facts.source) {
      case PlacementSource::inventory:
        result = inventoryPlacementResult(facts);
        break;
      case PlacementSource::sameProperty:
        result = samePropertyPlacementResult(facts);
        break;
      case PlacementSource::otherProperty:
        result = otherPropertyPlacementResult(facts);
        break;
    }
    if (result == ItemResult::success && facts.maxPerRealEstate >= 0 && facts.sameItemCount >= static_cast<uint32>(facts.maxPerRealEstate)) {
      return ItemResult::itemLimit;
    }
    return result;
  }

  struct PickUpFacts {
    bool plotUpkeepExpired = false;
    bool crate = false;
    bool mayRemoveCrate = false;
    bool ownerOrPlacer = false;
    bool noDrop = false;
    bool placedByTaker = false;
    bool loreHeld = false;
    bool carriesItems = false;
    bool takerCarriesSameKindWithItems = false;
    bool inventoryRoom = false;
  };

  // Removing a NO DROP item needs its placer, a LORE item is refused while the taker holds one (PLC-4).
  inline ItemResult pickUpResult(const PickUpFacts &facts) {
    if (facts.plotUpkeepExpired) {
      return ItemResult::plotUpkeepExpired;
    }
    if (facts.crate ? !facts.mayRemoveCrate : !facts.ownerOrPlacer) {
      return facts.crate ? ItemResult::noRemoveCrate : ItemResult::noRemoveItem;
    }
    if (facts.noDrop && !facts.placedByTaker) {
      return ItemResult::noDropNotOwner;
    }
    if (facts.loreHeld) {
      return ItemResult::loreHeld;
    }
    if (facts.carriesItems && facts.takerCarriesSameKindWithItems) {
      return ItemResult::pickUpLimit;
    }
    if (!facts.inventoryRoom) {
      return ItemResult::noInventoryRoom;
    }
    return ItemResult::success;
  }

  // A swapped-out house's items move into the incoming house's storage beside what it holds (eqstr 5331-5333).
  struct SwapCapacity {
    uint32 realEstateMoving = 0;
    uint32 vaultMoving = 0;
    uint32 npcMoving = 0;
    uint32 realEstateHeld = 0;
    uint32 vaultHeld = 0;
    uint32 npcHeld = 0;
    uint32 maxItems = 0;
    uint32 maxVault = 0;
    int32 npcCap = -1;
  };

  inline ItemResult swapCapacityResult(const SwapCapacity &space) {
    if (space.realEstateHeld + space.realEstateMoving > space.maxItems) {
      return ItemResult::swapItemsFull;
    }
    if (space.vaultHeld + space.vaultMoving > space.maxVault) {
      return ItemResult::swapVaultFull;
    }
    if (space.npcCap >= 0 && space.npcHeld + space.npcMoving > static_cast<uint32>(space.npcCap)) {
      return ItemResult::swapNpcItemsFull;
    }
    return ItemResult::success;
  }

  // The definition's cap and the rule's both apply; -1 is none, and the lower non-negative cap wins (PLC-15).
  inline int32 npcItemCap(int32 definitionCap, int32 ruleCap) {
    if (definitionCap < -1 || ruleCap < -1) {
      throw std::invalid_argument("an NPC item cap is -1 (none) or a count");
    }
    if (definitionCap < 0) {
      return ruleCap;
    }
    if (ruleCap < 0) {
      return definitionCap;
    }
    return std::min(definitionCap, ruleCap);
  }

  // The client's rule (0x7b1640): a house-pet name and not interactive.
  inline bool isNpcItem(const std::string &housepetFileName, bool interactiveObject) {
    return !housepetFileName.empty() && !interactiveObject;
  }

  constexpr uint32 primarySlotBit = 0x2000;
  constexpr uint32 secondarySlotBit = 0x4000;
  constexpr uint32 rangeSlotBit = 0x800;

  // RoF2 0x7b2210: the item's slots ANDed with every augment's; 0 for an item that isn't of the common class.
  inline uint32 effectiveSlots(uint32 itemSlots, bool classCommon, const std::vector<uint32> &augmentSlots) {
    if (!classCommon) {
      return 0;
    }
    uint32 slots = itemSlots;
    for (uint32 augment : augmentSlots) {
      slots &= augment;
    }
    return slots;
  }

  struct MountTilts {
    float pitch;
    float roll;
  };

  // Live's held-item turn (RoF2 0x7bac00, 0x7bac70): a quarter turn of pitch for a hand, a half turn of roll for the off hand
  // alone, added without wrapping.
  inline MountTilts heldItemTilts(float defaultPitch, float defaultRoll, uint32 slots) {
    MountTilts tilts{defaultPitch, defaultRoll};
    if (slots & (primarySlotBit | secondarySlotBit | rangeSlotBit)) {
      tilts.pitch += 128.0f;
    }
    if ((slots & secondarySlotBit) && !(slots & primarySlotBit)) {
      tilts.roll += 256.0f;
    }
    return tilts;
  }

  // The actor lookup is case-sensitive and every Live object names IT<n>_ACTORDEF.
  inline std::string worldObjectModel(const std::string &idfile, uint32 ornamentIdfile) {
    std::string base = ornamentIdfile ? "IT" + std::to_string(ornamentIdfile) : idfile;
    if (base.size() < 3 || (base[0] != 'I' && base[0] != 'i') || (base[1] != 'T' && base[1] != 't')) {
      throw std::invalid_argument("an item's model is IT and a number, not [" + base + "]");
    }
    std::transform(base.begin(), base.end(), base.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return base + "_ACTORDEF";
  }

  constexpr float scaleTolerance = 0.001f;
  constexpr float tiltTolerance = 0.01f;

  inline bool scaleAllowed(float scale, float minimum, float maximum) {
    if (minimum > maximum) {
      throw std::invalid_argument("an item's scale range runs backwards");
    }
    return scale >= minimum - scaleTolerance && scale <= maximum + scaleTolerance;
  }

  // With rotation disabled only the heading may change from the tilts the placement began at (PLC-5).
  inline bool tiltsKept(float pitch, float roll, const MountTilts &tilts) {
    return std::fabs(pitch - tilts.pitch) <= tiltTolerance && std::fabs(roll - tilts.roll) <= tiltTolerance;
  }
}

#endif
