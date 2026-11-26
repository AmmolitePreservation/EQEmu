#ifndef EQEMU_COMMON_HOUSING_UPKEEP_MATH_H
#define EQEMU_COMMON_HOUSING_UPKEEP_MATH_H

#include "types.h"

#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace housing {
  constexpr int64 secondsPerDay = 86400;

  struct UpkeepCharge {
    std::map<uint32, double> balances;
    std::map<uint32, double> taken;
    bool escrowFoundEmpty = false;
    // A charged currency holds nothing after the charge, whether it was found empty or the charge took the last of it.
    bool escrowLeftEmpty = false;
  };

  inline std::map<uint32, int64> netDailyCost(const std::vector<std::pair<uint32, int64>> &costItems) {
    std::map<uint32, int64> net;
    for (const auto &[currency, amount] : costItems) {
      net[currency] += amount;
    }
    for (auto &[currency, amount] : net) {
      amount = std::max<int64>(0, amount);
    }
    return net;
  }

  inline double proratedCharge(int64 dailyAmount, int percent, int64 elapsedSeconds) {
    if (dailyAmount < 0 || percent < 0 || elapsedSeconds < 0) {
      throw std::invalid_argument(fmt::format("upkeep proration of [{}] at [{}] percent over [{}] s", dailyAmount, percent, elapsedSeconds));
    }
    return static_cast<double>(dailyAmount) * percent / 100.0 * static_cast<double>(elapsedSeconds) / static_cast<double>(secondsPerDay);
  }

  inline int64 upkeepAmountAtPercent(int64 amount, int percent) {
    if (percent < 0) {
      throw std::invalid_argument(fmt::format("upkeep amount [{}] at [{}] percent", amount, percent));
    }
    return amount * percent / 100;
  }

  // A charge takes what is left when the escrow is short; only a charge that finds a currency it needs already empty expires the plot.
  inline UpkeepCharge applyCharge(const std::map<uint32, double> &balances, const std::map<uint32, int64> &dailyCost, int percent, int64 elapsedSeconds) {
    UpkeepCharge result;
    result.balances = balances;
    for (const auto &[currency, daily] : dailyCost) {
      const double due = proratedCharge(daily, percent, elapsedSeconds);
      if (due <= 0.0) {
        continue;
      }
      double &balance = result.balances[currency];
      if (balance <= 0.0) {
        result.escrowFoundEmpty = true;
        result.escrowLeftEmpty = true;
        balance = 0.0;
        continue;
      }
      const double taken = std::min(balance, due);
      balance -= taken;
      result.taken[currency] = taken;
      if (balance <= 0.0) {
        result.escrowLeftEmpty = true;
      }
    }
    return result;
  }

  inline void validateUpkeepClock(int intervalSeconds, int jitterSeconds) {
    if (intervalSeconds <= 0) {
      throw std::invalid_argument(fmt::format("Housing:UpkeepIntervalSeconds is [{}]; it must be above 0", intervalSeconds));
    }
    if (jitterSeconds < 0 || jitterSeconds >= intervalSeconds) {
      throw std::invalid_argument(fmt::format("Housing:UpkeepJitterSeconds is [{}]; it must be 0 or more and below the interval [{}]", jitterSeconds, intervalSeconds));
    }
  }

  inline int64 nextChargeDelay(int intervalSeconds, int jitterSeconds, int64 jitterDraw) {
    validateUpkeepClock(intervalSeconds, jitterSeconds);
    if (jitterDraw < -jitterSeconds || jitterDraw > jitterSeconds) {
      throw std::invalid_argument(fmt::format("upkeep jitter draw [{}] is outside [-{}, {}]", jitterDraw, jitterSeconds, jitterSeconds));
    }
    return static_cast<int64>(intervalSeconds) + jitterDraw;
  }

  inline bool reclaimDue(uint32 expiredTime, int64 now, int evictionDays) {
    return expiredTime != 0 && evictionDays > 0 && now >= static_cast<int64>(expiredTime) + static_cast<int64>(evictionDays) * secondsPerDay;
  }

  inline int64 flooredUnits(double amount) {
    return amount <= 0.0 ? 0 : static_cast<int64>(std::floor(amount + 1e-9));
  }

  // The escrow units a withdrawal of amount takes at unitsPerAmount each (1, or a coin's copper) when the held units cover
  // it. The client's u64 amount is compared before it is multiplied, so no amount wraps into a small one (AUD-20).
  inline std::optional<uint64> withdrawalUnits(uint64 amount, uint64 unitsPerAmount, int64 held) {
    if (unitsPerAmount == 0) {
      throw std::invalid_argument("a withdrawal's units per amount must be positive");
    }
    if (amount == 0 || held <= 0 || amount > static_cast<uint64>(held) / unitsPerAmount) {
      return std::nullopt;
    }
    return amount * unitsPerAmount;
  }
}

#endif
