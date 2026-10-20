#ifndef EQEMU_COMMON_HOUSING_ACCESS_RULES_H
#define EQEMU_COMMON_HOUSING_ACCESS_RULES_H

#include "types.h"

#include <algorithm>
#include <array>
#include <fmt/format.h>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace housing {
  constexpr uint8 accessGroupCount = 7;
  constexpr uint8 ownerGroup = 6;
  constexpr uint8 lowestNamedGroup = 1;
  constexpr uint8 highestNamedGroup = 5;
  constexpr uint32 permissionMaskBits = 0x1FFF;
  constexpr const char *fellowshipKey = "_Fellowship";

  using AccessMasks = std::array<uint32, accessGroupCount>;
  using AccessNames = std::vector<std::pair<std::string, uint8>>;

  inline std::string guildRankKey(uint8 rank) {
    return fmt::format("_Guild{}", rank);
  }

  inline bool isClubKey(const std::string &name) {
    if (name == fellowshipKey) {
      return true;
    }
    return name.size() == 7 && name.compare(0, 6, "_Guild") == 0 && name[6] >= '1' && name[6] <= '8';
  }

  // Live's lists hold no deny entry, so the most permissive matching group is the highest; an explicit deny would beat it (owner's decision).
  inline uint8 matchedGroup(const AccessNames &names, const std::string &characterName, const std::string &rankKey) {
    uint8 group = 0;
    for (const auto &[name, entryGroup] : names) {
      if (name == characterName || (!rankKey.empty() && name == rankKey)) {
        group = std::max(group, entryGroup);
      }
    }
    return group;
  }

  // The Manage window's edit rules (0x744200), replayed: a cell only for a group below the editor's that the editor's group holds,
  // names added no higher than the editor's group, removed or moved only from groups below it; owners and GMs edit everything but the Owner column.
  inline std::string accessEditRefusal(const AccessMasks &oldMasks, const AccessNames &oldNames, const AccessMasks &newMasks, const AccessNames &newNames, uint8 editorGroup, bool privileged) {
    if (newMasks[ownerGroup] != oldMasks[ownerGroup]) {
      return "the Owner group's mask cannot change";
    }
    std::set<std::string> seen;
    for (const auto &[name, group] : newNames) {
      if (name.empty()) {
        return "an entry has no name";
      }
      if (!seen.insert(name).second) {
        return fmt::format("[{}] is listed twice", name);
      }
      if (group < lowestNamedGroup || group > highestNamedGroup) {
        return fmt::format("[{}] is in group [{}]; names and clubs go in groups {} to {}", name, group, lowestNamedGroup, highestNamedGroup);
      }
    }
    for (uint8 group = 0; group < accessGroupCount; group++) {
      if (newMasks[group] & ~permissionMaskBits) {
        return fmt::format("group [{}] mask [{:#x}] sets bits past permission 12", group, newMasks[group]);
      }
    }
    if (privileged) {
      return {};
    }
    for (uint8 group = 0; group < accessGroupCount; group++) {
      const uint32 changed = oldMasks[group] ^ newMasks[group];
      if (!changed) {
        continue;
      }
      if (group >= editorGroup) {
        return fmt::format("group [{}] is not below the editor's group [{}]", group, editorGroup);
      }
      if (changed & ~oldMasks[editorGroup]) {
        return fmt::format("group [{}] changes permissions [{:#x}] the editor's group [{}] does not hold", group, changed & ~oldMasks[editorGroup], editorGroup);
      }
    }
    std::map<std::string, uint8> before(oldNames.begin(), oldNames.end());
    std::map<std::string, uint8> after(newNames.begin(), newNames.end());
    for (const auto &[name, group] : before) {
      auto found = after.find(name);
      if ((found == after.end() || found->second != group) && group >= editorGroup) {
        return fmt::format("[{}] is in group [{}], not below the editor's group [{}]", name, group, editorGroup);
      }
    }
    for (const auto &[name, group] : after) {
      auto found = before.find(name);
      if ((found == before.end() || found->second != group) && group > editorGroup) {
        return fmt::format("[{}] would be put in group [{}], above the editor's group [{}]", name, group, editorGroup);
      }
    }
    return {};
  }
}

#endif
