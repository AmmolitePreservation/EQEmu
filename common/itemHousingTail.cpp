#include "itemHousingTail.h"

#include "database.h"
#include "eqemu_logsys.h"
#include "strings.h"

#include <stdexcept>
#include <vector>

namespace EQ {
  namespace {
    std::unordered_map<uint32, ItemHousingTail> tails;
    bool loaded = false;
    const ItemHousingTail notPlaceable;

    void requireLoaded() {
      if (!loaded) {
        throw std::logic_error("an item's housing tail was read before loadItemHousingTails");
      }
    }
  }

  bool loadItemHousingTails(Database &content) {
    tails.clear();
    loaded = false;
    auto results = content.QueryDatabase(
      "SELECT id, placeable, placeable_ignore_collisions, placement_type, real_estate_definition_id, placeable_scale_min, "
      "placeable_scale_max, real_estate_upkeep_id, max_per_real_estate, housepet_file_name, trophy_benefit_id, "
      "disable_placement_rotation, disable_free_placement, npc_respawn_interval, placeable_default_scale, "
      "placeable_default_heading, placeable_default_pitch, placeable_default_roll, interactive_object "
      "FROM items WHERE placeable <> 0"
    );
    if (!results.Success()) {
      LogError("Housing: the item housing tail could not be loaded: {}", results.ErrorMessage());
      return false;
    }
    for (auto row : results) {
      ItemHousingTail tail;
      tail.placeable = Strings::ToUnsignedInt(row[1]);
      tail.placeableIgnoreCollisions = static_cast<uint8>(Strings::ToUnsignedInt(row[2]));
      tail.placementType = Strings::ToUnsignedInt(row[3]);
      tail.realEstateDefinitionId = Strings::ToInt(row[4]);
      tail.placeableScaleMin = Strings::ToFloat(row[5]);
      tail.placeableScaleMax = Strings::ToFloat(row[6]);
      tail.realEstateUpkeepId = Strings::ToInt(row[7]);
      tail.maxPerRealEstate = Strings::ToInt(row[8]);
      strn0cpy(tail.housepetFileName, row[9] ? row[9] : "", sizeof(tail.housepetFileName));
      tail.trophyBenefitId = Strings::ToInt(row[10]);
      tail.disablePlacementRotation = static_cast<uint8>(Strings::ToUnsignedInt(row[11]));
      tail.disableFreePlacement = static_cast<uint8>(Strings::ToUnsignedInt(row[12]));
      tail.npcRespawnInterval = Strings::ToUnsignedInt(row[13]);
      tail.placeableDefaultScale = Strings::ToFloat(row[14]);
      tail.placeableDefaultHeading = Strings::ToFloat(row[15]);
      tail.placeableDefaultPitch = Strings::ToFloat(row[16]);
      tail.placeableDefaultRoll = Strings::ToFloat(row[17]);
      tail.interactiveObject = static_cast<uint8>(Strings::ToUnsignedInt(row[18]));
      tails[Strings::ToUnsignedInt(row[0])] = tail;
    }
    std::vector<uint32> backwards;
    for (const auto &[id, tail] : tails) {
      if (tail.placeableScaleMin > tail.placeableScaleMax) {
        backwards.push_back(id);
      }
    }
    if (!backwards.empty()) {
      LogError("Housing: [{}] placeable items have a scale minimum above their maximum, so none of them could be placed: [{}]",
               backwards.size(), Strings::Join(backwards, ", "));
      tails.clear();
      return false;
    }
    loaded = true;
    LogHousing("Loaded the housing tail of [{}] placeable items", tails.size());
    return true;
  }

  const ItemHousingTail *placeableTail(uint32 itemId) {
    requireLoaded();
    auto found = tails.find(itemId);
    return found == tails.end() ? nullptr : &found->second;
  }

  const ItemHousingTail &itemHousingTail(uint32 itemId) {
    const ItemHousingTail *tail = placeableTail(itemId);
    return tail ? *tail : notPlaceable;
  }

  const std::unordered_map<uint32, ItemHousingTail> &placeableTails() {
    requireLoaded();
    return tails;
  }
}
