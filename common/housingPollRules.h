#ifndef EQEMU_COMMON_HOUSING_POLL_RULES_H
#define EQEMU_COMMON_HOUSING_POLL_RULES_H

#include "types.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace housing {
  // The poll record's state (RoF2 0x7f9d10, the Status column).
  enum class PollState : uint32 { initialized = 0, upcoming = 1, open = 2, closed = 3, archived = 4 };

  enum class PollKind : uint8 { property = 1, question = 2 };

  // 1: the Purchase window's Vote button (the plot one stands on); 2: one choice; 3: several, up to the poll's limit.
  enum class PollSelection : uint8 { propertyButton = 1, single = 2, multiple = 3 };

  inline PollState pollState(bool published, bool archived, int64 opensAt, int64 closesAt, int64 now) {
    if (opensAt >= closesAt) {
      throw std::invalid_argument("a poll closes after it opens");
    }
    if (archived) {
      return PollState::archived;
    }
    if (!published) {
      return PollState::initialized;
    }
    if (now < opensAt) {
      return PollState::upcoming;
    }
    return now < closesAt ? PollState::open : PollState::closed;
  }

  // Whether a vote's ticked keys fit the poll: a known option each (any key on a property poll, checked as a plot elsewhere),
  // no repeats, and no more than the selection type allows. An empty set withdraws the vote.
  inline bool voteKeysAllowed(PollKind kind, PollSelection selection, uint32 maxSelections, const std::set<uint32> &options, const std::vector<uint32> &keys) {
    const std::set<uint32> distinct(keys.begin(), keys.end());
    if (distinct.size() != keys.size()) {
      return false;
    }
    const uint32 limit = selection == PollSelection::multiple ? maxSelections : 1;
    if (keys.size() > limit) {
      return false;
    }
    if (kind == PollKind::property) {
      return selection == PollSelection::propertyButton;
    }
    if (selection == PollSelection::propertyButton) {
      return false;
    }
    return std::all_of(keys.begin(), keys.end(), [&](uint32 key) { return options.count(key) > 0; });
  }

  struct PollResult {
    uint32 key;
    uint32 votes;
    uint32 rank;
  };

  // Most votes first, ties sharing a rank and the next rank skipping past them; ties list by key.
  inline std::vector<PollResult> rankPollResults(const std::map<uint32, uint32> &votesByKey) {
    std::vector<PollResult> results;
    for (const auto &[key, votes] : votesByKey) {
      results.push_back(PollResult{key, votes, 0});
    }
    std::stable_sort(results.begin(), results.end(), [](const PollResult &a, const PollResult &b) { return a.votes > b.votes; });
    for (size_t index = 0; index < results.size(); index++) {
      results[index].rank = index && results[index].votes == results[index - 1].votes ? results[index - 1].rank : static_cast<uint32>(index + 1);
    }
    return results;
  }
}

#endif
