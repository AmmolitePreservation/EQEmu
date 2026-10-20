#ifndef EQEMU_COMMON_HOUSING_NAMES_H
#define EQEMU_COMMON_HOUSING_NAMES_H

#include "types.h"

#include <fmt/format.h>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace housing {
  constexpr size_t maximumVillageNameLength = 44;

  struct NeighborhoodWords {
    std::vector<std::string> trees;
    std::vector<std::string> middles;
    std::vector<std::string> places;
  };

  // Live's public names: an optional tree, an optional middle word and a place word, with at least one of the first two.
  inline std::vector<std::string> freeNeighborhoodNames(const NeighborhoodWords &words, const std::set<std::string> &taken) {
    std::vector<std::string> names;
    std::vector<std::string> trees(words.trees);
    trees.insert(trees.begin(), std::string());
    std::vector<std::string> middles(words.middles);
    middles.insert(middles.begin(), std::string());
    for (const auto &tree : trees) {
      for (const auto &middle : middles) {
        if (tree.empty() && middle.empty()) {
          continue;
        }
        for (const auto &place : words.places) {
          std::string name = tree;
          for (const std::string *word : {&middle, &place}) {
            if (word->empty()) {
              continue;
            }
            if (!name.empty()) {
              name += ' ';
            }
            name += *word;
          }
          if (!taken.count(name)) {
            names.push_back(name);
          }
        }
      }
    }
    return names;
  }

  inline std::string romanNumeral(uint32 value) {
    static const std::vector<std::pair<uint32, const char *>> numerals = {
      {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"}, {10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"}, {1, "I"}
    };
    if (value == 0) {
      throw std::invalid_argument("there is no Roman numeral for 0");
    }
    std::string result;
    for (const auto &[amount, text] : numerals) {
      while (value >= amount) {
        result += text;
        value -= amount;
      }
    }
    return result;
  }

  // "<guild> Village", then "II", "III" and on while the name is taken (Live: "The Templar Village II").
  inline std::string villageName(const std::string &guildName, const std::set<std::string> &taken) {
    if (guildName.empty()) {
      throw std::invalid_argument("a village needs its guild's name");
    }
    const std::string base = guildName + " Village";
    for (uint32 number = 1;; number++) {
      const std::string name = number == 1 ? base : fmt::format("{} {}", base, romanNumeral(number));
      if (name.size() > maximumVillageNameLength) {
        throw std::invalid_argument(fmt::format("village name [{}] is longer than [{}] characters", name, maximumVillageNameLength));
      }
      if (!taken.count(name)) {
        return name;
      }
    }
  }
}

#endif
