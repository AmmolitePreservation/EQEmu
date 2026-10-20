#ifndef EQEMU_COMMON_HOUSING_AUGMENTATER_RULES_H
#define EQEMU_COMMON_HOUSING_AUGMENTATER_RULES_H

#include "types.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace housing {
  // Live's nine modes, in the order a use on oneself steps through them (livedumps augmentRuns.tsv).
  enum class AugmentaterMode : uint8 { race, gender, face, hair, beard, hairColor, armorStyle, armorColor, reset };

  constexpr uint8 augmentaterModeCount = 9;
  constexpr uint8 noFeature = 255;

  inline AugmentaterMode nextAugmentaterMode(AugmentaterMode mode) {
    return static_cast<AugmentaterMode>((static_cast<uint8>(mode) + 1) % augmentaterModeCount);
  }

  inline const char *augmentaterModeName(AugmentaterMode mode) {
    static const char *names[] = {"RACE", "GENDER", "FACE", "HAIR", "BEARD", "HAIR COLOR", "ARMOR STYLE", "ARMOR COLOR", "RESET"};
    return names[static_cast<uint8>(mode)];
  }

  inline const char *augmentaterFeatureName(AugmentaterMode mode) {
    static const char *names[] = {"race", "gender", "face", "hair style", "beard style", "hair color", "armor material", "armor color"};
    if (mode == AugmentaterMode::reset) {
      throw std::invalid_argument("RESET changes no feature");
    }
    return names[static_cast<uint8>(mode)];
  }

  inline std::string augmentaterModeLine(AugmentaterMode mode) {
    if (mode == AugmentaterMode::reset) {
      return "The Augmentater whirrs: You will now -- RESET -- a targeted guild assistant back to their standard appearance.";
    }
    return std::string("The Augmentater whirrs: You will now modify the -- ") + augmentaterModeName(mode) + " -- of a targeted guild assistant.";
  }

  // A placed copy's look as real_estate_item_looks keeps it: the options a player picks, not the spawn's fields.
  struct AssistantLook {
    uint16 race;
    uint8 gender;
    uint8 face;
    uint8 hairStyle;
    uint8 beard;
    uint8 hairColor;
    uint8 bodyMaterial;
    uint32 bodyTint;

    bool operator==(const AssistantLook &other) const {
      return race == other.race && gender == other.gender && face == other.face && hairStyle == other.hairStyle && beard == other.beard &&
        hairColor == other.hairColor && bodyMaterial == other.bodyMaterial && bodyTint == other.bodyTint;
    }
  };

  // What the spawn carries for a look; RoF2 reads an Erudite's hair and an elf's beard from the face's tens digit (Live's spawns).
  struct AssistantSpawnLook {
    uint16 race;
    uint8 gender;
    uint8 face;
    uint8 hairStyle;
    uint8 hairColor;
    uint8 beard;
    uint8 beardColor;
    uint8 texture;
    uint32 bodyTint;
  };

  // The 16 player races, in the order RACE steps through them from an Erudite: id order with Human, Barbarian and Erudite last.
  inline const std::vector<uint16> &augmentaterRaces() {
    static const std::vector<uint16> races{4, 5, 6, 7, 8, 9, 10, 11, 12, 128, 130, 330, 522, 1, 2, 3};
    return races;
  }

  inline const std::vector<uint8> &augmentaterMaterials() {
    static const std::vector<uint8> materials{0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
    return materials;
  }

  inline const std::vector<uint32> &augmentaterTints() {
    static const std::vector<uint32> tints{0xFF00000A, 0xFFFF0000, 0xFF800000, 0xFFFFFF00, 0xFF808000, 0xFF00FF00, 0xFF008000, 0xFF00FFFF, 0xFF008080,
                                           0xFF0000FF, 0xFF000080, 0xFFFF00FF, 0xFF800080, 0xFFFF3300, 0xFFFEFEFE, 0xFFBFBFBF, 0xFF808080};
    return tints;
  }

  constexpr uint8 augmentaterFaces = 8;
  constexpr uint8 augmentaterHairColors = 19;

  enum class PlayerRace : uint16 {
    human = 1, barbarian = 2, erudite = 3, woodElf = 4, highElf = 5, darkElf = 6, halfElf = 7, dwarf = 8, troll = 9, ogre = 10,
    halfling = 11, gnome = 12, iksar = 128, vahShir = 130, froglok = 330, drakkin = 522
  };

  inline bool isAugmentaterRace(uint16 race) {
    const auto &races = augmentaterRaces();
    return std::find(races.begin(), races.end(), race) != races.end();
  }

  inline void requireAugmentaterRace(uint16 race, uint8 gender) {
    if (!isAugmentaterRace(race) || gender > 1) {
      throw std::invalid_argument("a guild assistant's look is one of the 16 player races, male or female");
    }
  }

  inline bool faceCarriesHair(uint16 race) {
    return race == static_cast<uint16>(PlayerRace::erudite);
  }

  inline bool faceCarriesBeard(uint16 race) {
    return race == static_cast<uint16>(PlayerRace::highElf) || race == static_cast<uint16>(PlayerRace::darkElf) || race == static_cast<uint16>(PlayerRace::halfElf);
  }

  // HAIR's options in order: three styles and none (255); a Drakkin's four styles, no none; an Erudite's 6 or 9 face tens; none.
  inline std::vector<uint8> hairOptions(uint16 race, uint8 gender) {
    requireAugmentaterRace(race, gender);
    const auto kind = static_cast<PlayerRace>(race);
    if (kind == PlayerRace::erudite) {
      std::vector<uint8> tens(gender ? 9 : 6);
      for (uint8 index = 0; index < tens.size(); index++) {
        tens[index] = index;
      }
      return tens;
    }
    if (kind == PlayerRace::drakkin) {
      return {0, 1, 2, 3};
    }
    if (kind == PlayerRace::iksar || kind == PlayerRace::vahShir || kind == PlayerRace::froglok ||
        (!gender && (kind == PlayerRace::troll || kind == PlayerRace::ogre))) {
      return {};
    }
    return {0, 1, 2, noFeature};
  }

  inline uint8 beardOptionCount(uint16 race, uint8 gender) {
    requireAugmentaterRace(race, gender);
    const auto kind = static_cast<PlayerRace>(race);
    if (kind == PlayerRace::dwarf || kind == PlayerRace::drakkin) {
      return 6;
    }
    if (faceCarriesBeard(race)) {
      return 5;
    }
    if (!gender && (kind == PlayerRace::human || kind == PlayerRace::barbarian || kind == PlayerRace::erudite || kind == PlayerRace::halfling || kind == PlayerRace::gnome)) {
      return 6;
    }
    return 0;
  }

  inline bool hasHairColor(uint16 race, uint8 gender) {
    requireAugmentaterRace(race, gender);
    const auto kind = static_cast<PlayerRace>(race);
    if (kind == PlayerRace::ogre || kind == PlayerRace::iksar || kind == PlayerRace::vahShir || kind == PlayerRace::froglok) {
      return false;
    }
    if (!gender && kind == PlayerRace::troll) {
      return false;
    }
    if (gender && (kind == PlayerRace::erudite || kind == PlayerRace::highElf)) {
      return false;
    }
    return true;
  }

  // An elf woman's "beard" is a facial detail in the face's tens digit; her spawn carries no beard field.
  inline bool spawnCarriesBeard(uint16 race, uint8 gender) {
    return beardOptionCount(race, gender) > 0 && !(faceCarriesBeard(race) && gender);
  }

  template <typename T>
  T nextOption(const std::vector<T> &options, T current) {
    if (options.empty()) {
      throw std::invalid_argument("a feature with no options does not step");
    }
    auto found = std::find(options.begin(), options.end(), current);
    if (found == options.end() || ++found == options.end()) {
      return options.front();
    }
    return *found;
  }

  inline bool lookValid(const AssistantLook &look) {
    if (!isAugmentaterRace(look.race) || look.gender > 1 || look.face >= augmentaterFaces) {
      return false;
    }
    const auto hair = hairOptions(look.race, look.gender);
    if (hair.empty() ? look.hairStyle != noFeature : std::find(hair.begin(), hair.end(), look.hairStyle) == hair.end()) {
      return false;
    }
    const uint8 beards = beardOptionCount(look.race, look.gender);
    if (beards ? look.beard >= beards : look.beard != noFeature) {
      return false;
    }
    if (look.hairColor >= augmentaterHairColors) {
      return false;
    }
    const auto &materials = augmentaterMaterials();
    return std::find(materials.begin(), materials.end(), look.bodyMaterial) != materials.end();
  }

  // A race or gender change keeps every feature the new body also has and starts the others at their first option.
  inline AssistantLook fitLook(AssistantLook look) {
    const auto hair = hairOptions(look.race, look.gender);
    if (hair.empty()) {
      look.hairStyle = noFeature;
    }
    else if (std::find(hair.begin(), hair.end(), look.hairStyle) == hair.end()) {
      look.hairStyle = hair.front();
    }
    const uint8 beards = beardOptionCount(look.race, look.gender);
    if (!beards) {
      look.beard = noFeature;
    }
    else if (look.beard >= beards) {
      look.beard = 0;
    }
    if (look.hairColor >= augmentaterHairColors) {
      look.hairColor = 0;
    }
    return look;
  }

  // One use on an assistant in a mode other than RESET; false when the mode has no option for this body (Live: nothing changes).
  inline bool stepLook(AssistantLook &look, AugmentaterMode mode) {
    switch (mode) {
      case AugmentaterMode::race:
        look.race = nextOption(augmentaterRaces(), look.race);
        look = fitLook(look);
        return true;
      case AugmentaterMode::gender:
        look.gender = look.gender ? 0 : 1;
        look = fitLook(look);
        return true;
      case AugmentaterMode::face:
        look.face = static_cast<uint8>((look.face + 1) % augmentaterFaces);
        return true;
      case AugmentaterMode::hair: {
        const auto hair = hairOptions(look.race, look.gender);
        if (hair.empty()) {
          return false;
        }
        look.hairStyle = nextOption(hair, look.hairStyle);
        return true;
      }
      case AugmentaterMode::beard: {
        const uint8 beards = beardOptionCount(look.race, look.gender);
        if (!beards) {
          return false;
        }
        look.beard = static_cast<uint8>((look.beard + 1) % beards);
        return true;
      }
      case AugmentaterMode::hairColor:
        if (!hasHairColor(look.race, look.gender) && !spawnCarriesBeard(look.race, look.gender)) {
          return false;
        }
        look.hairColor = static_cast<uint8>((look.hairColor + 1) % augmentaterHairColors);
        return true;
      case AugmentaterMode::armorStyle:
        look.bodyMaterial = nextOption(augmentaterMaterials(), look.bodyMaterial);
        return true;
      case AugmentaterMode::armorColor:
        look.bodyTint = nextOption(augmentaterTints(), look.bodyTint);
        return true;
      case AugmentaterMode::reset:
        break;
    }
    throw std::invalid_argument("RESET restores the standard look; it steps no feature");
  }

  inline AssistantSpawnLook spawnLook(const AssistantLook &look) {
    if (!lookValid(look)) {
      throw std::invalid_argument("a guild assistant's look is outside the Augmentater's options");
    }
    AssistantSpawnLook spawn{};
    spawn.race = look.race;
    spawn.gender = look.gender;
    spawn.face = look.face;
    spawn.hairStyle = look.hairStyle;
    if (faceCarriesHair(look.race)) {
      spawn.face = static_cast<uint8>(look.face + 10 * look.hairStyle);
      spawn.hairStyle = noFeature;
    }
    else if (faceCarriesBeard(look.race)) {
      spawn.face = static_cast<uint8>(look.face + 10 * look.beard);
    }
    const bool hairShown = hasHairColor(look.race, look.gender) && spawn.hairStyle != noFeature;
    spawn.hairColor = hairShown ? look.hairColor : noFeature;
    const bool beardShown = spawnCarriesBeard(look.race, look.gender);
    spawn.beard = beardShown ? look.beard : noFeature;
    spawn.beardColor = beardShown ? look.hairColor : noFeature;
    spawn.texture = look.bodyMaterial;
    spawn.bodyTint = look.bodyTint;
    return spawn;
  }
}

#endif
