#ifndef EQEMU_COMMON_HOUSING_PRICES_H
#define EQEMU_COMMON_HOUSING_PRICES_H

#include "types.h"

#include <cmath>

namespace housing {
  // Rounded to the nearest coin one tier below the price's smallest coin (168p at 10%: 16p 8g), so the zone's display and
  // world's charge agree.
  inline uint64 plotUpkeepCopper(uint64 priceCopper, double percent) {
    uint64 priceStep = 1;
    for (uint64 step : {1000ULL, 100ULL, 10ULL}) {
      if (priceCopper % step == 0) {
        priceStep = step;
        break;
      }
    }
    const uint64 upkeepStep = priceStep > 1 ? priceStep / 10 : 1;
    const double upkeep = static_cast<double>(priceCopper) * percent / 100.0;
    return static_cast<uint64>(std::llround(upkeep / static_cast<double>(upkeepStep))) * upkeepStep;
  }
}

#endif
