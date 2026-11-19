#ifndef EQEMU_COMMON_HOUSING_RELATED_PROPERTIES_H
#define EQEMU_COMMON_HOUSING_RELATED_PROPERTIES_H

#include "types.h"

#include <fmt/format.h>
#include <map>
#include <set>
#include <stdexcept>

namespace housing {
  struct PropertyLink {
    uint32 parentId = 0;
    bool zoneRoot = false;
  };

  inline std::set<uint32> propertyLine(const std::map<uint32, PropertyLink> &links, uint32 id, int maximumDepth) {
    std::set<uint32> line;
    int depth = 0;
    for (uint32 at = id; at;) {
      auto link = links.find(at);
      if (link == links.end()) {
        throw std::invalid_argument(fmt::format("property [{}] in the line of [{}] is unknown", at, id));
      }
      if (++depth > maximumDepth) {
        throw std::invalid_argument(fmt::format("property [{}] has an ancestor chain deeper than [{}]", id, maximumDepth));
      }
      line.insert(at);
      at = link->second.parentId;
    }
    return line;
  }

  // Live gives a client the records and lists of its related properties only (ZIN-1): every seed (own, carried, guild,
  // access-granted, and the plot of a house it is inside) with its line and, below anything but a zone-level root, its
  // descendants. A guild village's root is a guild seed, and its plots stay out.
  inline std::set<uint32> relatedProperties(const std::map<uint32, PropertyLink> &links, const std::set<uint32> &seeds, int maximumDepth) {
    std::set<uint32> related;
    for (uint32 id : seeds) {
      const auto line = propertyLine(links, id, maximumDepth);
      related.insert(line.begin(), line.end());
    }
    std::map<uint32, std::set<uint32>> children;
    for (const auto &[id, link] : links) {
      if (link.parentId) {
        children[link.parentId].insert(id);
      }
    }
    std::set<uint32> frontier;
    for (uint32 id : seeds) {
      if (!links.at(id).zoneRoot) {
        frontier.insert(id);
      }
    }
    std::set<uint32> visited(frontier);
    for (int depth = 0; !frontier.empty(); depth++) {
      if (depth > maximumDepth) {
        throw std::invalid_argument(fmt::format("a related property has descendants deeper than [{}]", maximumDepth));
      }
      std::set<uint32> next;
      for (uint32 id : frontier) {
        auto found = children.find(id);
        if (found == children.end()) {
          continue;
        }
        for (uint32 child : found->second) {
          if (visited.insert(child).second) {
            related.insert(child);
            next.insert(child);
          }
        }
      }
      frontier = next;
    }
    return related;
  }
}

#endif
