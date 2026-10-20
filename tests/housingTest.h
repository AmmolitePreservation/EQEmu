#ifndef EQEMU_TESTS_HOUSING_TEST_H
#define EQEMU_TESTS_HOUSING_TEST_H

#include "cppunit/cpptest.h"
#include "../common/housingAccessRules.h"
#include "../common/housingAugmentaterRules.h"
#include "../common/housingInstanceNumbers.h"
#include "../common/housingNames.h"
#include "../common/housingPlacementRules.h"
#include "../common/housingPollRules.h"
#include "../common/housingTransferOffers.h"
#include "../common/housingUpkeepMath.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <vector>

class HousingTest : public Test::Suite {
  typedef void(HousingTest::*TestFunction)(void);
public:
  HousingTest() {
    TEST_ADD(HousingTest::proratedChargeMatchesLiveCharges);
    TEST_ADD(HousingTest::depositOnExpiredPlotPaysElapsedTimeFromDeposit);
    TEST_ADD(HousingTest::shortChargeTakesWhatIsLeftWithoutExpiring);
    TEST_ADD(HousingTest::chargeFindingEscrowEmptyExpires);
    TEST_ADD(HousingTest::chargeWithNothingDueNeverExpires);
    TEST_ADD(HousingTest::upkeepPercentScalesTheCharge);
    TEST_ADD(HousingTest::reductionsClampEachCurrencyAtZero);
    TEST_ADD(HousingTest::nextChargeDelayIsIntervalPlusJitterDraw);
    TEST_ADD(HousingTest::upkeepClockRejectsOutOfRangeRules);
    TEST_ADD(HousingTest::reclaimComesNinetyDaysAfterTheStamp);
    TEST_ADD(HousingTest::escrowFloorsToWholeUnits);
    TEST_ADD(HousingTest::mostPermissiveMatchingGroupWins);
    TEST_ADD(HousingTest::unmatchedCharacterIsEveryone);
    TEST_ADD(HousingTest::editorChangesOnlyLowerGroupsWithHeldBits);
    TEST_ADD(HousingTest::editorAddsAndRemovesNamesWithinTheirGroup);
    TEST_ADD(HousingTest::ownerEditsAllButTheOwnerColumn);
    TEST_ADD(HousingTest::namedEntriesStayInVisitorToCoOwner);
    TEST_ADD(HousingTest::clubKeysAreFellowshipAndGuildRanks);
    TEST_ADD(HousingTest::neighborhoodNamesFollowLivePatterns);
    TEST_ADD(HousingTest::takenNeighborhoodNamesAreSkipped);
    TEST_ADD(HousingTest::villageNamesNumberWithRomanNumerals);
    TEST_ADD(HousingTest::villageNameLongerThanLiveLimitIsRefused);
    TEST_ADD(HousingTest::transferOfferAnsweredOnlyByRecipientBeforeExpiry);
    TEST_ADD(HousingTest::transferOfferExpiresAfterItsSeconds);
    TEST_ADD(HousingTest::secondTransferOfferWhileOneWaitsIsRefused);
    TEST_ADD(HousingTest::beginMapCodesMatchLive);
    TEST_ADD(HousingTest::inventoryRulesRunInTheClientsOrder);
    TEST_ADD(HousingTest::houseAndHallLimitsCountTheYardsHouses);
    TEST_ADD(HousingTest::crateRulesKeepFullHousesAndCratesApart);
    TEST_ADD(HousingTest::capacityCodesAndTheStackMergeWaiver);
    TEST_ADD(HousingTest::lastInventoryRulesAndTheItemLimit);
    TEST_ADD(HousingTest::storedAndMovedItemsNeedMoveItem);
    TEST_ADD(HousingTest::pickUpRulesFollowTheClientsTexts);
    TEST_ADD(HousingTest::petCapsTakeTheLowerNonNegativeCap);
    TEST_ADD(HousingTest::npcItemsAreNamedAndNotInteractive);
    TEST_ADD(HousingTest::heldItemTurnMatchesLiveObjects);
    TEST_ADD(HousingTest::effectiveSlotsAndEveryAugment);
    TEST_ADD(HousingTest::worldObjectModelsAreLivesActorTags);
    TEST_ADD(HousingTest::commitScaleAndLocks);
    TEST_ADD(HousingTest::copiesCountUpPastIdsInUse);
    TEST_ADD(HousingTest::copyNumbersWrapAboveTheReservedIds);
    TEST_ADD(HousingTest::noCopyWhenEveryIdIsInUse);
    TEST_ADD(HousingTest::augmentaterModesStepInLivesOrder);
    TEST_ADD(HousingTest::raceStepsFromEruditeToWoodElfAndWraps);
    TEST_ADD(HousingTest::featureCountsMatchLivesRuns);
    TEST_ADD(HousingTest::eruditeHairAndElfBeardRideTheFace);
    TEST_ADD(HousingTest::armorColorsAndMaterialsCycleAsOnLive);
    TEST_ADD(HousingTest::modeWithoutOptionChangesNothing);
    TEST_ADD(HousingTest::raceChangeKeepsFeaturesTheNewBodyHas);
    TEST_ADD(HousingTest::lookOutsideTheOptionsIsRefused);
    TEST_ADD(HousingTest::pollStateFollowsPublishingAndDates);
    TEST_ADD(HousingTest::voteKeysFitTheSelectionType);
    TEST_ADD(HousingTest::pollResultsRankTiesTogether);
    TEST_ADD(HousingTest::houseSwapNeedsRoomForThePlacedHousesItems);
  }

private:
  static bool near(double a, double b) {
    return std::fabs(a - b) < 1e-6;
  }

  void proratedChargeMatchesLiveCharges() {
    TEST_ASSERT(near(housing::proratedCharge(12600, 100, 42021), 6128.0625));
    TEST_ASSERT(near(housing::proratedCharge(2100, 100, 41360), 1005.2777777777778));
    TEST_ASSERT(near(housing::proratedCharge(2100, 100, 32), 0.7777777777777778));
  }

  void depositOnExpiredPlotPaysElapsedTimeFromDeposit() {
    const std::map<uint32, int64> daily{{0, 2100}};
    auto first = housing::applyCharge({{0, 1000.0}}, daily, 100, 1782);
    TEST_ASSERT(near(first.balances.at(0), 956.6875));
    TEST_ASSERT(!first.escrowFoundEmpty);
    auto second = housing::applyCharge({{0, first.balances.at(0) + 1000.0}}, daily, 100, 296);
    TEST_ASSERT(near(second.balances.at(0), 1949.4930555555557));
  }

  void shortChargeTakesWhatIsLeftWithoutExpiring() {
    auto charge = housing::applyCharge({{0, 26.7361}}, {{0, 2100}}, 100, 47061);
    TEST_ASSERT(near(charge.balances.at(0), 0.0));
    TEST_ASSERT(near(charge.taken.at(0), 26.7361));
    TEST_ASSERT(!charge.escrowFoundEmpty);
  }

  void chargeFindingEscrowEmptyExpires() {
    auto charge = housing::applyCharge({{0, 0.0}}, {{0, 2100}}, 100, 44724);
    TEST_ASSERT(charge.escrowFoundEmpty);
    TEST_ASSERT(near(charge.balances.at(0), 0.0));
    auto unfunded = housing::applyCharge({}, {{0, 2100}}, 100, 40967);
    TEST_ASSERT(unfunded.escrowFoundEmpty);
  }

  void chargeWithNothingDueNeverExpires() {
    auto charge = housing::applyCharge({}, {{0, 0}}, 100, 43200);
    TEST_ASSERT(!charge.escrowFoundEmpty);
    TEST_ASSERT(charge.balances.empty());
  }

  void upkeepPercentScalesTheCharge() {
    auto half = housing::applyCharge({{0, 5000.0}}, {{0, 2100}}, 50, 86400);
    TEST_ASSERT(near(half.balances.at(0), 3950.0));
    auto none = housing::applyCharge({{0, 5000.0}}, {{0, 2100}}, 0, 86400);
    TEST_ASSERT(near(none.balances.at(0), 5000.0));
    TEST_ASSERT(!none.escrowFoundEmpty);
  }

  void reductionsClampEachCurrencyAtZero() {
    auto net = housing::netDailyCost({{0, 21000}, {0, 700}, {0, -42000}, {15, 300}});
    TEST_ASSERT_EQUALS(net.at(0), 0);
    TEST_ASSERT_EQUALS(net.at(15), 300);
    TEST_ASSERT_EQUALS(net.size(), 2u);
  }

  void nextChargeDelayIsIntervalPlusJitterDraw() {
    TEST_ASSERT_EQUALS(housing::nextChargeDelay(43200, 4320, -4320), 38880);
    TEST_ASSERT_EQUALS(housing::nextChargeDelay(43200, 4320, 4320), 47520);
    TEST_ASSERT_EQUALS(housing::nextChargeDelay(43200, 0, 0), 43200);
    bool refused = false;
    try {
      housing::nextChargeDelay(43200, 4320, 4321);
    }
    catch (const std::invalid_argument &) {
      refused = true;
    }
    TEST_ASSERT(refused);
  }

  void upkeepClockRejectsOutOfRangeRules() {
    int refusals = 0;
    for (const auto &[interval, jitter] : std::vector<std::pair<int, int>>{{0, 0}, {-1, 0}, {43200, -1}, {43200, 43200}}) {
      try {
        housing::validateUpkeepClock(interval, jitter);
      }
      catch (const std::invalid_argument &) {
        refusals++;
      }
    }
    TEST_ASSERT_EQUALS(refusals, 4);
  }

  void reclaimComesNinetyDaysAfterTheStamp() {
    const uint32 stamp = 1790666988;
    TEST_ASSERT(!housing::reclaimDue(stamp, stamp + 90 * 86400 - 1, 90));
    TEST_ASSERT(housing::reclaimDue(stamp, stamp + 90 * 86400, 90));
    TEST_ASSERT(!housing::reclaimDue(0, stamp + 365 * 86400, 90));
    TEST_ASSERT(!housing::reclaimDue(stamp, stamp + 365 * 86400, 0));
  }

  void escrowFloorsToWholeUnits() {
    TEST_ASSERT_EQUALS(housing::flooredUnits(30009.9722), 30009);
    TEST_ASSERT_EQUALS(housing::flooredUnits(14699.708), 14699);
    TEST_ASSERT_EQUALS(housing::flooredUnits(0.999999), 0);
    TEST_ASSERT_EQUALS(housing::flooredUnits(-3.0), 0);
  }

  void mostPermissiveMatchingGroupWins() {
    const housing::AccessNames names{{"Gondemare", 1}, {"_Guild8", 2}, {"_Guild2", 5}};
    TEST_ASSERT_EQUALS(housing::matchedGroup(names, "Gondemare", "_Guild8"), 2);
    TEST_ASSERT_EQUALS(housing::matchedGroup(names, "Gondemare", ""), 1);
    TEST_ASSERT_EQUALS(housing::matchedGroup(names, "Pariator", "_Guild2"), 5);
  }

  void unmatchedCharacterIsEveryone() {
    const housing::AccessNames names{{"Gondemare", 1}, {"_Guild8", 2}};
    TEST_ASSERT_EQUALS(housing::matchedGroup(names, "Sukarin", "_Guild3"), 0);
    TEST_ASSERT_EQUALS(housing::matchedGroup(names, "gondemare", ""), 0);
  }

  void editorChangesOnlyLowerGroupsWithHeldBits() {
    const housing::AccessMasks live{0x3, 0x3, 0x803, 0xe03, 0x1f87, 0x1fff, 0};
    auto visitorGainsMoveItem = live;
    visitorGainsMoveItem[1] |= 1u << 10;
    TEST_ASSERT(housing::accessEditRefusal(live, {}, visitorGainsMoveItem, {}, 4, false).empty());
    auto visitorGainsAddCrate = live;
    visitorGainsAddCrate[1] |= 1u << 4;
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, visitorGainsAddCrate, {}, 4, false).empty());
    auto trusteeChangesOwnGroup = live;
    trusteeChangesOwnGroup[4] &= ~(1u << 9);
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, trusteeChangesOwnGroup, {}, 4, false).empty());
    auto trusteeChangesCoOwner = live;
    trusteeChangesCoOwner[5] &= ~1u;
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, trusteeChangesCoOwner, {}, 4, false).empty());
  }

  void editorAddsAndRemovesNamesWithinTheirGroup() {
    const housing::AccessMasks live{0x3, 0x3, 0x803, 0xe03, 0x1f87, 0x1fff, 0};
    const housing::AccessNames before{{"Pariator", 4}, {"Gondemare", 1}};
    TEST_ASSERT(housing::accessEditRefusal(live, before, live, {{"Pariator", 4}, {"Gondemare", 1}, {"Sukarin", 4}}, 4, false).empty());
    TEST_ASSERT(!housing::accessEditRefusal(live, before, live, {{"Pariator", 4}, {"Gondemare", 1}, {"Sukarin", 5}}, 4, false).empty());
    TEST_ASSERT(housing::accessEditRefusal(live, before, live, {{"Pariator", 4}}, 4, false).empty());
    TEST_ASSERT(!housing::accessEditRefusal(live, before, live, {{"Gondemare", 1}}, 4, false).empty());
  }

  void ownerEditsAllButTheOwnerColumn() {
    const housing::AccessMasks live{0x3, 0x3, 0x803, 0xe03, 0x1f87, 0x1fff, 0};
    auto everything = live;
    everything[5] = 0;
    everything[0] = 0x1fff;
    TEST_ASSERT(housing::accessEditRefusal(live, {}, everything, {{"Sukarin", 5}}, housing::ownerGroup, true).empty());
    auto ownerColumn = live;
    ownerColumn[housing::ownerGroup] = 1;
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, ownerColumn, {}, housing::ownerGroup, true).empty());
    auto pastPermissionTwelve = live;
    pastPermissionTwelve[0] = 0x2000;
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, pastPermissionTwelve, {}, housing::ownerGroup, true).empty());
  }

  void namedEntriesStayInVisitorToCoOwner() {
    const housing::AccessMasks live{0x3, 0x3, 0x803, 0xe03, 0x1f87, 0x1fff, 0};
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, live, {{"Sukarin", 0}}, housing::ownerGroup, true).empty());
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, live, {{"Sukarin", 6}}, housing::ownerGroup, true).empty());
    TEST_ASSERT(!housing::accessEditRefusal(live, {}, live, {{"Sukarin", 2}, {"Sukarin", 3}}, housing::ownerGroup, true).empty());
  }

  void clubKeysAreFellowshipAndGuildRanks() {
    TEST_ASSERT(housing::isClubKey("_Fellowship"));
    TEST_ASSERT(housing::isClubKey("_Guild1"));
    TEST_ASSERT(housing::isClubKey("_Guild8"));
    TEST_ASSERT(!housing::isClubKey("_Guild9"));
    TEST_ASSERT(!housing::isClubKey("_Guild"));
    TEST_ASSERT(!housing::isClubKey("_Guild12"));
    TEST_ASSERT_EQUALS(housing::guildRankKey(3), std::string("_Guild3"));
  }

  static housing::NeighborhoodWords liveWords() {
    return housing::NeighborhoodWords{
      {"Cedar", "Dogwood", "Elm", "Ivy", "Maple", "Oak", "Pine", "Spruce", "Walnut", "Willow"},
      {"Bridge", "Circle", "Colonial", "Country", "Sunset"},
      {"Acres", "Bay", "Brook", "Creek", "Estates", "Farms", "Fields", "Hills", "Hollows", "Island", "Lake", "Landing", "Meadows", "Point", "Woods"}
    };
  }

  void neighborhoodNamesFollowLivePatterns() {
    const auto names = housing::freeNeighborhoodNames(liveWords(), {});
    TEST_ASSERT_EQUALS(names.size(), 975u);
    const std::set<std::string> all(names.begin(), names.end());
    TEST_ASSERT_EQUALS(all.size(), 975u);
    TEST_ASSERT(all.count("Maple Bridge Point"));
    TEST_ASSERT(all.count("Walnut Lake"));
    TEST_ASSERT(all.count("Circle Hills"));
    TEST_ASSERT(!all.count("Lake"));
    TEST_ASSERT(!all.count("Bridge Maple Point"));
  }

  void takenNeighborhoodNamesAreSkipped() {
    const auto names = housing::freeNeighborhoodNames(liveWords(), {"Walnut Lake", "Circle Hills"});
    TEST_ASSERT_EQUALS(names.size(), 973u);
    TEST_ASSERT(std::find(names.begin(), names.end(), "Walnut Lake") == names.end());
  }

  void villageNamesNumberWithRomanNumerals() {
    TEST_ASSERT_EQUALS(housing::villageName("The Templar", {}), std::string("The Templar Village"));
    TEST_ASSERT_EQUALS(housing::villageName("The Templar", {"The Templar Village"}), std::string("The Templar Village II"));
    TEST_ASSERT_EQUALS(housing::villageName("Fu World Order", {"Fu World Order Village", "Fu World Order Village II"}), std::string("Fu World Order Village III"));
    TEST_ASSERT_EQUALS(housing::romanNumeral(4), std::string("IV"));
    TEST_ASSERT_EQUALS(housing::romanNumeral(9), std::string("IX"));
  }

  void villageNameLongerThanLiveLimitIsRefused() {
    TEST_ASSERT_EQUALS(housing::villageName(std::string(36, 'a'), {}).size(), 44u);
    bool refused = false;
    try {
      housing::villageName(std::string(37, 'a'), {});
    }
    catch (const std::invalid_argument &) {
      refused = true;
    }
    TEST_ASSERT(refused);
  }

  void transferOfferAnsweredOnlyByRecipientBeforeExpiry() {
    housing::TransferOffers offers;
    offers.offer(117754208, 7, 8, 3, 1000, 300);
    TEST_ASSERT(!offers.answer(117754208, 9, 1100));
    auto answered = offers.answer(117754208, 8, 1299);
    TEST_ASSERT(answered.has_value());
    TEST_ASSERT_EQUALS(answered->ownerCharacterId, 7u);
    TEST_ASSERT_EQUALS(answered->revision, 3u);
    TEST_ASSERT(!offers.answer(117754208, 8, 1299));
  }

  void transferOfferExpiresAfterItsSeconds() {
    housing::TransferOffers offers;
    offers.offer(117754208, 7, 8, 3, 1000, 300);
    TEST_ASSERT(offers.expire(1299).empty());
    const auto expired = offers.expire(1300);
    TEST_ASSERT_EQUALS(expired.size(), 1u);
    TEST_ASSERT_EQUALS(expired.front().recipientCharacterId, 8u);
    TEST_ASSERT(!offers.answer(117754208, 8, 1300));
  }

  void secondTransferOfferWhileOneWaitsIsRefused() {
    housing::TransferOffers offers;
    offers.offer(117754208, 7, 8, 3, 1000, 300);
    bool refused = false;
    try {
      offers.offer(117754208, 7, 9, 3, 1100, 300);
    }
    catch (const std::logic_error &) {
      refused = true;
    }
    TEST_ASSERT(refused);
    TEST_ASSERT(offers.pending(117754208, 1100));
    TEST_ASSERT(!offers.pending(117754208, 1300));
  }

  static housing::PlacementFacts yardPlacement() {
    housing::PlacementFacts facts;
    facts.source = housing::PlacementSource::inventory;
    facts.houseOfThule = true;
    facts.veilOfAlaris = true;
    facts.placeable = true;
    facts.allowedHere = true;
    facts.maxHouses = 1;
    facts.maxItems = 100;
    facts.maxNonRealEstateItems = 10;
    facts.npcCap = -1;
    facts.mayAdd = true;
    facts.mayMove = true;
    facts.mayRemoveHouse = true;
    return facts;
  }

  static uint32 code(const housing::PlacementFacts &facts) {
    return static_cast<uint32>(housing::placementResult(facts));
  }

  // PLC-14: Pariator's begin map on 107 Terminus Street held 0 for his region, 24 for the two expired plots he is a trustee
  // of, 71 for plots without add-item access and 17 for the neighborhood itself.
  void beginMapCodesMatchLive() {
    auto own = yardPlacement();
    TEST_ASSERT_EQUALS(code(own), 0u);
    auto expired = yardPlacement();
    expired.plotUpkeepExpired = true;
    TEST_ASSERT_EQUALS(code(expired), 24u);
    auto stranger = yardPlacement();
    stranger.mayAdd = false;
    TEST_ASSERT_EQUALS(code(stranger), 71u);
    auto neighborhood = yardPlacement();
    neighborhood.allowedHere = false;
    neighborhood.mayAdd = false;
    TEST_ASSERT_EQUALS(code(neighborhood), 17u);
  }

  void inventoryRulesRunInTheClientsOrder() {
    auto facts = yardPlacement();
    facts.houseOfThule = false;
    facts.plotUpkeepExpired = true;
    facts.placeable = false;
    TEST_ASSERT_EQUALS(code(facts), 47u);
    facts.houseOfThule = true;
    facts.underGuildPlot = true;
    facts.veilOfAlaris = false;
    TEST_ASSERT_EQUALS(code(facts), 82u);
    facts.veilOfAlaris = true;
    TEST_ASSERT_EQUALS(code(facts), 24u);
    facts.plotUpkeepExpired = false;
    TEST_ASSERT_EQUALS(code(facts), 36u);
    facts.placeable = true;
    facts.intoOwnProperty = true;
    facts.allowedHere = false;
    TEST_ASSERT_EQUALS(code(facts), 30u);
    facts.intoOwnProperty = false;
    TEST_ASSERT_EQUALS(code(facts), 17u);
  }

  void houseAndHallLimitsCountTheYardsHouses() {
    auto house = yardPlacement();
    house.carried = housing::CarriedKind::house;
    house.destinationHouses = 1;
    house.mayAdd = false;
    TEST_ASSERT_EQUALS(code(house), 15u);
    auto hall = house;
    hall.carried = housing::CarriedKind::guildHall;
    TEST_ASSERT_EQUALS(code(hall), 68u);
    house.destinationHouses = 0;
    TEST_ASSERT_EQUALS(code(house), 75u);
    auto crate = yardPlacement();
    crate.carried = housing::CarriedKind::crate;
    crate.destinationHasPlacedCrate = true;
    TEST_ASSERT_EQUALS(code(crate), 16u);
    crate.destinationHasPlacedCrate = false;
    crate.mayAdd = false;
    TEST_ASSERT_EQUALS(code(crate), 77u);
  }

  void crateRulesKeepFullHousesAndCratesApart() {
    auto shed = yardPlacement();
    shed.carried = housing::CarriedKind::house;
    shed.carriedHoldsItems = true;
    shed.intoStorage = true;
    TEST_ASSERT_EQUALS(code(shed), 32u);
    auto crateInCrate = yardPlacement();
    crateInCrate.carried = housing::CarriedKind::crate;
    crateInCrate.carriedHoldsItems = true;
    crateInCrate.intoStorage = true;
    crateInCrate.destinationIsCrate = true;
    TEST_ASSERT_EQUALS(code(crateInCrate), 32u);
    auto secondHouse = shed;
    secondHouse.destinationIsCrate = true;
    secondHouse.crateHoldsNonEmptyHouse = true;
    TEST_ASSERT_EQUALS(code(secondHouse), 39u);
    secondHouse.crateHoldsNonEmptyHouse = false;
    secondHouse.carriedHouseHoldsPlacedCrate = true;
    TEST_ASSERT_EQUALS(code(secondHouse), 81u);
    secondHouse.carriedHouseHoldsPlacedCrate = false;
    TEST_ASSERT_EQUALS(code(secondHouse), 0u);
  }

  void capacityCodesAndTheStackMergeWaiver() {
    auto full = yardPlacement();
    full.realEstateCount = 100;
    TEST_ASSERT_EQUALS(code(full), 10u);
    full.intoStorage = true;
    full.mergesIntoStack = true;
    TEST_ASSERT_EQUALS(code(full), 0u);
    auto vault = yardPlacement();
    vault.placeable = false;
    vault.intoStorage = true;
    vault.vaultCount = 10;
    TEST_ASSERT_EQUALS(code(vault), 12u);
    vault.vaultCount = 9;
    TEST_ASSERT_EQUALS(code(vault), 0u);
    auto pets = yardPlacement();
    pets.npcItem = true;
    pets.npcCount = 40;
    TEST_ASSERT_EQUALS(code(pets), 0u);
    pets.npcCap = 0;
    pets.npcCount = 0;
    TEST_ASSERT_EQUALS(code(pets), 38u);
  }

  void lastInventoryRulesAndTheItemLimit() {
    auto facts = yardPlacement();
    facts.containerHoldsItems = true;
    facts.itemUpkeepExpired = true;
    TEST_ASSERT_EQUALS(code(facts), 19u);
    facts.containerHoldsItems = false;
    TEST_ASSERT_EQUALS(code(facts), 25u);
    facts.itemUpkeepExpired = false;
    facts.maxPerRealEstate = 1;
    facts.sameItemCount = 1;
    TEST_ASSERT_EQUALS(code(facts), 31u);
    facts.maxPerRealEstate = -1;
    TEST_ASSERT_EQUALS(code(facts), 0u);
  }

  void storedAndMovedItemsNeedMoveItem() {
    auto stored = yardPlacement();
    stored.source = housing::PlacementSource::sameProperty;
    stored.houseOfThule = false;
    stored.mayAdd = false;
    stored.mayMove = false;
    TEST_ASSERT_EQUALS(code(stored), 73u);
    stored.mayMove = true;
    TEST_ASSERT_EQUALS(code(stored), 0u);
    stored.carried = housing::CarriedKind::house;
    stored.mayRemoveHouse = false;
    TEST_ASSERT_EQUALS(code(stored), 76u);
    auto moved = yardPlacement();
    moved.source = housing::PlacementSource::otherProperty;
    moved.houseOfThule = false;
    TEST_ASSERT_EQUALS(code(moved), 47u);
    moved.houseOfThule = true;
    moved.mayAdd = false;
    TEST_ASSERT_EQUALS(code(moved), 71u);
  }

  void pickUpRulesFollowTheClientsTexts() {
    housing::PickUpFacts facts;
    facts.ownerOrPlacer = true;
    facts.placedByTaker = true;
    facts.inventoryRoom = true;
    TEST_ASSERT_EQUALS(static_cast<uint32>(housing::pickUpResult(facts)), 0u);
    facts.noDrop = true;
    facts.placedByTaker = false;
    TEST_ASSERT_EQUALS(static_cast<uint32>(housing::pickUpResult(facts)), 64u);
    facts.placedByTaker = true;
    facts.loreHeld = true;
    TEST_ASSERT_EQUALS(static_cast<uint32>(housing::pickUpResult(facts)), 65u);
    facts.loreHeld = false;
    facts.carriesItems = true;
    facts.takerCarriesSameKindWithItems = true;
    TEST_ASSERT_EQUALS(static_cast<uint32>(housing::pickUpResult(facts)), 33u);
    facts.takerCarriesSameKindWithItems = false;
    facts.inventoryRoom = false;
    TEST_ASSERT_EQUALS(static_cast<uint32>(housing::pickUpResult(facts)), 4u);
    facts.crate = true;
    TEST_ASSERT_EQUALS(static_cast<uint32>(housing::pickUpResult(facts)), 78u);
    facts.crate = false;
    facts.ownerOrPlacer = false;
    TEST_ASSERT_EQUALS(static_cast<uint32>(housing::pickUpResult(facts)), 72u);
  }

  void petCapsTakeTheLowerNonNegativeCap() {
    TEST_ASSERT_EQUALS(housing::npcItemCap(-1, -1), -1);
    TEST_ASSERT_EQUALS(housing::npcItemCap(12, -1), 12);
    TEST_ASSERT_EQUALS(housing::npcItemCap(-1, 5), 5);
    TEST_ASSERT_EQUALS(housing::npcItemCap(12, 5), 5);
    TEST_ASSERT_EQUALS(housing::npcItemCap(4, 6), 4);
    TEST_ASSERT_EQUALS(housing::npcItemCap(0, -1), 0);
    bool refused = false;
    try {
      housing::npcItemCap(-2, -1);
    }
    catch (const std::invalid_argument &) {
      refused = true;
    }
    TEST_ASSERT(refused);
  }

  void npcItemsAreNamedAndNotInteractive() {
    TEST_ASSERT(housing::isNpcItem("NGH_Merchant_General", false));
    TEST_ASSERT(!housing::isNpcItem("NGH_Merchant_General", true));
    TEST_ASSERT(!housing::isNpcItem("", false));
  }

  // Live: 50616's shield sits at pitch 128 and roll 256, 105582's cake (default pitch 384, primary) at 512, a decorative
  // shield with no slots at 0 and 0, and a weapon for either hand (Dragonkiller 71672) turns its pitch only.
  void heldItemTurnMatchesLiveObjects() {
    const auto shield = housing::heldItemTilts(0.0f, 0.0f, housing::secondarySlotBit);
    TEST_ASSERT(near(shield.pitch, 128.0) && near(shield.roll, 256.0));
    const auto cake = housing::heldItemTilts(384.0f, 0.0f, housing::primarySlotBit);
    TEST_ASSERT(near(cake.pitch, 512.0) && near(cake.roll, 0.0));
    const auto decoration = housing::heldItemTilts(0.0f, 0.0f, 0);
    TEST_ASSERT(near(decoration.pitch, 0.0) && near(decoration.roll, 0.0));
    const auto eitherHand = housing::heldItemTilts(0.0f, 0.0f, housing::primarySlotBit | housing::secondarySlotBit);
    TEST_ASSERT(near(eitherHand.pitch, 128.0) && near(eitherHand.roll, 0.0));
    const auto ranged = housing::heldItemTilts(0.0f, 0.0f, housing::rangeSlotBit);
    TEST_ASSERT(near(ranged.pitch, 128.0) && near(ranged.roll, 0.0));
  }

  void effectiveSlotsAndEveryAugment() {
    TEST_ASSERT_EQUALS(housing::effectiveSlots(0x6000, true, {0x2000}), 0x2000u);
    TEST_ASSERT_EQUALS(housing::effectiveSlots(0x6000, true, {}), 0x6000u);
    TEST_ASSERT_EQUALS(housing::effectiveSlots(0x6000, true, {0x2000, 0x4000}), 0u);
    TEST_ASSERT_EQUALS(housing::effectiveSlots(0x6000, false, {}), 0u);
  }

  void worldObjectModelsAreLivesActorTags() {
    TEST_ASSERT_EQUALS(housing::worldObjectModel("IT10864", 0), std::string("IT10864_ACTORDEF"));
    TEST_ASSERT_EQUALS(housing::worldObjectModel("it63", 0), std::string("IT63_ACTORDEF"));
    TEST_ASSERT_EQUALS(housing::worldObjectModel("IT10864", 11927), std::string("IT11927_ACTORDEF"));
    bool refused = false;
    try {
      housing::worldObjectModel("", 0);
    }
    catch (const std::invalid_argument &) {
      refused = true;
    }
    TEST_ASSERT(refused);
  }

  void commitScaleAndLocks() {
    TEST_ASSERT(housing::scaleAllowed(2.5f, 0.1f, 2.5f));
    TEST_ASSERT(housing::scaleAllowed(0.1f, 0.1f, 2.5f));
    TEST_ASSERT(!housing::scaleAllowed(2.6f, 0.1f, 2.5f));
    TEST_ASSERT(!housing::scaleAllowed(0.05f, 0.1f, 2.5f));
    const housing::MountTilts tilts{128.0f, 256.0f};
    TEST_ASSERT(housing::tiltsKept(128.0f, 256.0f, tilts));
    TEST_ASSERT(!housing::tiltsKept(0.0f, 256.0f, tilts));
  }

  // Live, 2026-10-18: a house took 4638, then a neighborhood 4641 and a hall 4644 with other copies made between (INT-14).
  void copiesCountUpPastIdsInUse() {
    const housing::InstanceRange range = housing::instanceRange(100);
    TEST_ASSERT(range.first == 101 && range.last == 32000);
    TEST_ASSERT(housing::nextInstanceId(4638, range, {}) == std::optional<uint32>(4638));
    TEST_ASSERT(housing::nextInstanceId(4639, range, {4638, 4639, 4640}) == std::optional<uint32>(4641));
    TEST_ASSERT(housing::nextInstanceId(4642, range, {101, 102, 4638}) == std::optional<uint32>(4642));
  }

  void copyNumbersWrapAboveTheReservedIds() {
    const housing::InstanceRange range = housing::instanceRange(100);
    TEST_ASSERT(housing::nextInstanceId(32000, range, {32000}) == std::optional<uint32>(101));
    TEST_ASSERT(housing::nextInstanceId(32001, range, {101}) == std::optional<uint32>(102));
    TEST_ASSERT(housing::nextInstanceId(50, range, {}) == std::optional<uint32>(101));
    bool refused = false;
    try {
      housing::instanceRange(32000);
    }
    catch (const std::invalid_argument &) {
      refused = true;
    }
    TEST_ASSERT(refused);
  }

  void noCopyWhenEveryIdIsInUse() {
    const housing::InstanceRange range = housing::instanceRange(31990);
    std::set<uint32> used;
    for (uint32 id = 31991; id <= 32000; id++) {
      used.insert(id);
    }
    TEST_ASSERT(!housing::nextInstanceId(31995, range, used).has_value());
    used.erase(31993);
    TEST_ASSERT(housing::nextInstanceId(31995, range, used) == std::optional<uint32>(31993));
  }
  static housing::AssistantLook erudite() {
    return housing::AssistantLook{3, 0, 1, 2, 1, 0, 0, 0xFFFEFEFE};
  }

  void augmentaterModesStepInLivesOrder() {
    std::vector<std::string> names;
    housing::AugmentaterMode mode = housing::AugmentaterMode::race;
    for (int index = 0; index < 10; index++) {
      names.push_back(housing::augmentaterModeName(mode));
      mode = housing::nextAugmentaterMode(mode);
    }
    TEST_ASSERT(names == std::vector<std::string>({"RACE", "GENDER", "FACE", "HAIR", "BEARD", "HAIR COLOR", "ARMOR STYLE", "ARMOR COLOR", "RESET", "RACE"}));
    TEST_ASSERT(housing::augmentaterModeLine(housing::AugmentaterMode::race) == "The Augmentater whirrs: You will now modify the -- RACE -- of a targeted guild assistant.");
    TEST_ASSERT(housing::augmentaterModeLine(housing::AugmentaterMode::reset) ==
      "The Augmentater whirrs: You will now -- RESET -- a targeted guild assistant back to their standard appearance.");
    TEST_ASSERT(std::string(housing::augmentaterFeatureName(housing::AugmentaterMode::armorStyle)) == "armor material");
  }

  void raceStepsFromEruditeToWoodElfAndWraps() {
    housing::AssistantLook look = erudite();
    std::vector<uint16> races;
    for (int index = 0; index < 16; index++) {
      TEST_ASSERT(housing::stepLook(look, housing::AugmentaterMode::race));
      races.push_back(look.race);
    }
    TEST_ASSERT(races == std::vector<uint16>({4, 5, 6, 7, 8, 9, 10, 11, 12, 128, 130, 330, 522, 1, 2, 3}));
  }

  void featureCountsMatchLivesRuns() {
    TEST_ASSERT(housing::hairOptions(3, 0).size() == 6);
    TEST_ASSERT(housing::hairOptions(3, 1).size() == 9);
    TEST_ASSERT(housing::hairOptions(4, 1) == std::vector<uint8>({0, 1, 2, 255}));
    TEST_ASSERT(housing::hairOptions(522, 0) == std::vector<uint8>({0, 1, 2, 3}));
    TEST_ASSERT(housing::hairOptions(9, 0).empty() && housing::hairOptions(10, 0).empty() && housing::hairOptions(128, 1).empty());
    TEST_ASSERT(housing::hairOptions(9, 1).size() == 4 && housing::hairOptions(10, 1).size() == 4);
    TEST_ASSERT(housing::beardOptionCount(8, 1) == 6 && housing::beardOptionCount(522, 1) == 6 && housing::beardOptionCount(12, 0) == 6);
    TEST_ASSERT(housing::beardOptionCount(5, 1) == 5 && housing::beardOptionCount(6, 0) == 5 && housing::beardOptionCount(7, 1) == 5);
    TEST_ASSERT(housing::beardOptionCount(4, 0) == 0 && housing::beardOptionCount(1, 1) == 0 && housing::beardOptionCount(128, 0) == 0);
    TEST_ASSERT(!housing::hasHairColor(3, 1) && !housing::hasHairColor(5, 1) && !housing::hasHairColor(10, 1) && !housing::hasHairColor(9, 0));
    TEST_ASSERT(housing::hasHairColor(9, 1) && housing::hasHairColor(6, 1));
  }

  void eruditeHairAndElfBeardRideTheFace() {
    const housing::AssistantSpawnLook man = housing::spawnLook(erudite());
    TEST_ASSERT(man.face == 21 && man.hairStyle == 255 && man.hairColor == 255 && man.beard == 1 && man.beardColor == 0);
    housing::AssistantLook look = erudite();
    std::vector<uint8> faces;
    for (int index = 0; index < 6; index++) {
      housing::stepLook(look, housing::AugmentaterMode::hair);
      faces.push_back(housing::spawnLook(look).face);
    }
    TEST_ASSERT(faces == std::vector<uint8>({31, 41, 51, 1, 11, 21}));
    const housing::AssistantSpawnLook highElfWoman = housing::spawnLook(housing::AssistantLook{5, 1, 3, 0, 2, 4, 0, 0});
    TEST_ASSERT(highElfWoman.face == 23 && highElfWoman.beard == 255 && highElfWoman.hairColor == 255);
    const housing::AssistantSpawnLook darkElfMan = housing::spawnLook(housing::AssistantLook{6, 0, 6, 1, 1, 9, 0, 0});
    TEST_ASSERT(darkElfMan.face == 16 && darkElfMan.beard == 1 && darkElfMan.beardColor == 9 && darkElfMan.hairColor == 9);
    const housing::AssistantSpawnLook bald = housing::spawnLook(housing::AssistantLook{4, 0, 0, 255, 255, 17, 0, 0});
    TEST_ASSERT(bald.hairStyle == 255 && bald.hairColor == 255);
  }

  void armorColorsAndMaterialsCycleAsOnLive() {
    housing::AssistantLook look = erudite();
    std::vector<uint32> tints;
    for (int index = 0; index < 3; index++) {
      housing::stepLook(look, housing::AugmentaterMode::armorColor);
      tints.push_back(look.bodyTint);
    }
    TEST_ASSERT(tints == std::vector<uint32>({0xFFBFBFBF, 0xFF808080, 0xFF00000A}));
    TEST_ASSERT(housing::augmentaterTints().size() == 17);
    look.bodyMaterial = 8;
    housing::stepLook(look, housing::AugmentaterMode::armorStyle);
    TEST_ASSERT(look.bodyMaterial == 10);
    look.bodyMaterial = 23;
    housing::stepLook(look, housing::AugmentaterMode::armorStyle);
    TEST_ASSERT(look.bodyMaterial == 0);
    TEST_ASSERT(housing::augmentaterMaterials().size() == 23);
  }

  void modeWithoutOptionChangesNothing() {
    housing::AssistantLook iksar{128, 0, 3, 255, 255, 0, 0, 0};
    const housing::AssistantLook before = iksar;
    TEST_ASSERT(!housing::stepLook(iksar, housing::AugmentaterMode::hair));
    TEST_ASSERT(!housing::stepLook(iksar, housing::AugmentaterMode::beard));
    TEST_ASSERT(!housing::stepLook(iksar, housing::AugmentaterMode::hairColor));
    TEST_ASSERT(iksar == before);
    housing::AssistantLook woman = erudite();
    woman.gender = 1;
    woman = housing::fitLook(woman);
    TEST_ASSERT(!housing::stepLook(woman, housing::AugmentaterMode::hairColor));
  }

  void raceChangeKeepsFeaturesTheNewBodyHas() {
    housing::AssistantLook look = erudite();
    housing::stepLook(look, housing::AugmentaterMode::race);
    TEST_ASSERT(look.race == 4 && look.hairStyle == 2 && look.beard == 255 && look.face == 1);
    housing::stepLook(look, housing::AugmentaterMode::race);
    TEST_ASSERT(look.race == 5 && look.beard == 0);
    housing::AssistantLook troll{9, 1, 0, 2, 255, 13, 0, 0};
    housing::stepLook(troll, housing::AugmentaterMode::gender);
    TEST_ASSERT(troll.gender == 0 && troll.hairStyle == 255 && troll.hairColor == 13);
  }

  void lookOutsideTheOptionsIsRefused() {
    TEST_ASSERT(!housing::lookValid(housing::AssistantLook{3, 0, 1, 6, 1, 0, 0, 0}));
    TEST_ASSERT(!housing::lookValid(housing::AssistantLook{4, 0, 8, 0, 255, 0, 0, 0}));
    TEST_ASSERT(!housing::lookValid(housing::AssistantLook{4, 0, 0, 0, 0, 0, 0, 0}));
    TEST_ASSERT(!housing::lookValid(housing::AssistantLook{4, 0, 0, 0, 255, 19, 0, 0}));
    TEST_ASSERT(!housing::lookValid(housing::AssistantLook{4, 0, 0, 0, 255, 0, 9, 0}));
    TEST_ASSERT(!housing::lookValid(housing::AssistantLook{75, 0, 0, 0, 255, 0, 0, 0}));
    TEST_ASSERT(housing::lookValid(housing::AssistantLook{3, 1, 7, 8, 255, 0, 23, 0}));
    bool refused = false;
    try {
      housing::spawnLook(housing::AssistantLook{3, 0, 1, 6, 1, 0, 0, 0});
    }
    catch (const std::invalid_argument &) {
      refused = true;
    }
    TEST_ASSERT(refused);
  }

  void pollStateFollowsPublishingAndDates() {
    TEST_ASSERT(housing::pollState(false, false, 100, 200, 150) == housing::PollState::initialized);
    TEST_ASSERT(housing::pollState(true, false, 100, 200, 99) == housing::PollState::upcoming);
    TEST_ASSERT(housing::pollState(true, false, 100, 200, 100) == housing::PollState::open);
    TEST_ASSERT(housing::pollState(true, false, 100, 200, 200) == housing::PollState::closed);
    TEST_ASSERT(housing::pollState(true, true, 100, 200, 150) == housing::PollState::archived);
    bool refused = false;
    try {
      housing::pollState(true, false, 200, 200, 150);
    }
    catch (const std::invalid_argument &) {
      refused = true;
    }
    TEST_ASSERT(refused);
  }

  void voteKeysFitTheSelectionType() {
    using housing::PollKind;
    using housing::PollSelection;
    const std::set<uint32> options{10, 11, 12};
    TEST_ASSERT(housing::voteKeysAllowed(PollKind::question, PollSelection::single, 3, options, {11}));
    TEST_ASSERT(!housing::voteKeysAllowed(PollKind::question, PollSelection::single, 3, options, {10, 11}));
    TEST_ASSERT(housing::voteKeysAllowed(PollKind::question, PollSelection::multiple, 2, options, {10, 12}));
    TEST_ASSERT(!housing::voteKeysAllowed(PollKind::question, PollSelection::multiple, 2, options, {10, 11, 12}));
    TEST_ASSERT(!housing::voteKeysAllowed(PollKind::question, PollSelection::multiple, 3, options, {10, 10}));
    TEST_ASSERT(!housing::voteKeysAllowed(PollKind::question, PollSelection::single, 3, options, {13}));
    TEST_ASSERT(housing::voteKeysAllowed(PollKind::question, PollSelection::single, 3, options, {}));
    TEST_ASSERT(housing::voteKeysAllowed(PollKind::property, PollSelection::propertyButton, 1, {}, {117730767}));
    TEST_ASSERT(!housing::voteKeysAllowed(PollKind::property, PollSelection::single, 1, {}, {117730767}));
  }

  void pollResultsRankTiesTogether() {
    const auto ranked = housing::rankPollResults({{10, 3}, {11, 5}, {12, 3}, {13, 1}});
    TEST_ASSERT(ranked.size() == 4);
    TEST_ASSERT(ranked[0].key == 11 && ranked[0].rank == 1);
    TEST_ASSERT(ranked[1].key == 10 && ranked[1].rank == 2 && ranked[2].key == 12 && ranked[2].rank == 2);
    TEST_ASSERT(ranked[3].key == 13 && ranked[3].rank == 4);
  }

  void houseSwapNeedsRoomForThePlacedHousesItems() {
    housing::SwapCapacity space;
    space.maxItems = 100;
    space.maxVault = 10;
    space.realEstateHeld = 40;
    space.realEstateMoving = 60;
    TEST_ASSERT(housing::swapCapacityResult(space) == housing::ItemResult::success);
    space.realEstateMoving = 61;
    TEST_ASSERT(static_cast<uint32>(housing::swapCapacityResult(space)) == 48);
    space.realEstateMoving = 60;
    space.vaultMoving = 11;
    TEST_ASSERT(static_cast<uint32>(housing::swapCapacityResult(space)) == 49);
    space.vaultMoving = 10;
    space.npcCap = 2;
    space.npcHeld = 1;
    space.npcMoving = 2;
    TEST_ASSERT(static_cast<uint32>(housing::swapCapacityResult(space)) == 50);
    space.npcCap = -1;
    TEST_ASSERT(housing::swapCapacityResult(space) == housing::ItemResult::success);
    TEST_ASSERT(static_cast<uint32>(housing::ItemResult::swapNoHousePlaced) == 53 && static_cast<uint32>(housing::ItemResult::swapAlreadyPlaced) == 55 &&
      static_cast<uint32>(housing::ItemResult::swapPlacedCrate) == 59);
  }
};

#endif
