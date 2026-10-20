#ifndef EQEMU_TESTS_HOUSING_TEST_H
#define EQEMU_TESTS_HOUSING_TEST_H

#include "cppunit/cpptest.h"
#include "../common/housingAccessRules.h"
#include "../common/housingNames.h"
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
};

#endif
