#ifndef EQEMU_COMMON_HOUSING_INSTANCE_NUMBERS_H
#define EQEMU_COMMON_HOUSING_INSTANCE_NUMBERS_H

#include "types.h"

#include <fmt/format.h>

#include <optional>
#include <set>
#include <stdexcept>

namespace housing {
  // The instance ids a copy may take: above EQEmu's reserved globals, at most RoF2's and EQEmu's 32,000.
  struct InstanceRange {
    uint32 first;
    uint32 last;
  };

  constexpr uint32 lastInstanceId = 32000;

  inline InstanceRange instanceRange(int reservedInstances) {
    if (reservedInstances < 0 || static_cast<uint32>(reservedInstances) >= lastInstanceId) {
      throw std::invalid_argument(fmt::format("Instances:ReservedInstances is [{}]; it must be 0 to [{}]", reservedInstances, lastInstanceId - 1));
    }
    return InstanceRange{static_cast<uint32>(reservedInstances) + 1, lastInstanceId};
  }

  // Copies count up as Live's do (INT-14): the counter's id, or the next free one above it, wrapping to the first; none when every
  // id is in use (INT-13).
  inline std::optional<uint32> nextInstanceId(uint32 counter, const InstanceRange &range, const std::set<uint32> &used) {
    uint32 candidate = counter < range.first || counter > range.last ? range.first : counter;
    for (uint32 tried = 0; tried <= range.last - range.first; tried++) {
      if (!used.count(candidate)) {
        return candidate;
      }
      candidate = candidate == range.last ? range.first : candidate + 1;
    }
    return std::nullopt;
  }
}

#endif
