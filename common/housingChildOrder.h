#ifndef EQEMU_COMMON_HOUSING_CHILD_ORDER_H
#define EQEMU_COMMON_HOUSING_CHILD_ORDER_H

#include "types.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace housing {
  // RoF2 takes a plot's house and crate as the first of each in its child list (0x5affc0), so the list holds placed carriers first,
  // ids rising within each (PLC-34).
  inline std::vector<uint32> inClientOrder(std::vector<uint32> ids, const std::set<uint32> &placedCarriers) {
    std::sort(ids.begin(), ids.end());
    std::stable_partition(ids.begin(), ids.end(), [&placedCarriers](uint32 id) { return placedCarriers.count(id) != 0; });
    return ids;
  }

  // A plot's crate is the first crate property a depth-first walk of the plot's descendants meets, placed or stored, through child
  // lists in the client's order (0x5b45b0 from 0x5b0140); the walk checks ten levels and goes no deeper. Pack, Unpack, Move to
  // Crate and the one-crate limit all use it.
  inline uint32 firstCrateUnder(uint32 rootId, const std::map<uint32, uint32> &parentOf, const std::set<uint32> &crates, const std::set<uint32> &placedCarriers) {
    std::map<uint32, std::vector<uint32>> children;
    for (const auto &[id, parentId] : parentOf) {
      if (parentId) {
        children[parentId].push_back(id);
      }
    }
    for (auto &[parentId, ids] : children) {
      ids = inClientOrder(ids, placedCarriers);
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
