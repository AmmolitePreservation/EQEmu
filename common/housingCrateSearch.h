#ifndef EQEMU_COMMON_HOUSING_CRATE_SEARCH_H
#define EQEMU_COMMON_HOUSING_CRATE_SEARCH_H

#include "types.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace housing {
  // RoF2 takes a plot's crate as the first crate property a depth-first walk of the plot's descendants meets, placed or stored, with
  // each child list's placed carriers ahead of the rest and ids rising within each (0x5b45b0 from 0x5b0140, PLC-34); the walk checks
  // ten levels and goes no deeper. Pack, Unpack, Move to Crate and the one-crate limit all use it.
  inline uint32 firstCrateUnder(uint32 rootId, const std::map<uint32, uint32> &parentOf, const std::set<uint32> &crates, const std::set<uint32> &placedCarriers) {
    std::map<uint32, std::vector<uint32>> children;
    for (const auto &[id, parentId] : parentOf) {
      if (parentId) {
        children[parentId].push_back(id);
      }
    }
    for (auto &[parentId, ids] : children) {
      std::stable_partition(ids.begin(), ids.end(), [&placedCarriers](uint32 id) { return placedCarriers.count(id) != 0; });
    }
    struct Frame {
      uint32 id;
      int level;
    };
    std::vector<Frame> pending{{rootId, 0}};
    while (!pending.empty()) {
      const Frame frame = pending.back();
      pending.pop_back();
      if (frame.level > 0 && crates.count(frame.id)) {
        return frame.id;
      }
      auto found = children.find(frame.id);
      if (frame.level > 9 || found == children.end()) {
        continue;
      }
      for (auto child = found->second.rbegin(); child != found->second.rend(); ++child) {
        pending.push_back({*child, frame.level + 1});
      }
    }
    return 0;
  }
}

#endif
