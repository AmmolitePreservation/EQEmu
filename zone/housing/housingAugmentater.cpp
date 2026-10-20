#include "housingAugmentater.h"

#include "housing.h"
#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingNpcItems.h"
#include "housingSession.h"

#include "../../common/classes.h"
#include "../../common/eqemu_logsys.h"
#include "../../common/races.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../npc.h"
#include "../zonedb.h"

#include <cstring>
#include <stdexcept>

namespace housing {
  namespace {
    constexpr uint32 augmentaterItemId = 66656;
    constexpr uint16 augmentaterSpellId = 7651;
    // Live's Augmentater reaches 10 units (the runbook's Live notes); the spell's own range is RoF2's check.
    constexpr float augmentaterRange = 10.0f;
    constexpr uint8 firstBodySlot = EQ::textures::armorChest;
    constexpr uint8 lastBodySlot = EQ::textures::armorFeet;

    std::optional<AssistantLook> storedLook(uint32 recordId) {
      auto rows = query(database, fmt::format(
        "SELECT race, gender, face, hair_style, beard, hair_color, body_material, body_tint FROM real_estate_item_looks WHERE real_estate_item_id = {}", recordId
      ));
      if (rows.RowCount() == 0) {
        return std::nullopt;
      }
      auto row = rows.begin();
      AssistantLook look{static_cast<uint16>(Strings::ToUnsignedInt(row[0])), static_cast<uint8>(Strings::ToUnsignedInt(row[1])),
                         static_cast<uint8>(Strings::ToUnsignedInt(row[2])), static_cast<uint8>(Strings::ToUnsignedInt(row[3])),
                         static_cast<uint8>(Strings::ToUnsignedInt(row[4])), static_cast<uint8>(Strings::ToUnsignedInt(row[5])),
                         static_cast<uint8>(Strings::ToUnsignedInt(row[6])), Strings::ToUnsignedInt(row[7])};
      if (!lookValid(look)) {
        throw std::runtime_error(fmt::format("record [{}]'s look (race [{}], gender [{}], face [{}], hair [{}], beard [{}], hair color [{}], material [{}]) "
          "is outside the Augmentater's options", recordId, look.race, look.gender, look.face, look.hairStyle, look.beard, look.hairColor, look.bodyMaterial));
      }
      return look;
    }

    void storeLook(uint32 recordId, const AssistantLook &look) {
      if (!lookValid(look)) {
        throw std::logic_error(fmt::format("record [{}] would keep a look outside the Augmentater's options", recordId));
      }
      query(database, fmt::format(
        "REPLACE INTO real_estate_item_looks (real_estate_item_id, race, gender, face, hair_style, beard, hair_color, body_material, body_tint) "
        "VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {})",
        recordId, look.race, look.gender, look.face, look.hairStyle, look.beard, look.hairColor, look.bodyMaterial, look.bodyTint
      ));
    }

    // The standard look as the Augmentater's options read it: an Erudite's hair and an elf's beard from the face's tens digit.
    AssistantLook standardLook(const NPCType &type) {
      requireAugmentaterRace(type.race, type.gender);
      AssistantLook look{type.race, type.gender, static_cast<uint8>(type.luclinface % 10), type.hairstyle, type.beard, type.haircolor, 0,
                         type.armor_tint.Slot[firstBodySlot].Color};
      if (faceCarriesHair(type.race)) {
        look.hairStyle = static_cast<uint8>(type.luclinface / 10);
      }
      else if (faceCarriesBeard(type.race)) {
        look.beard = static_cast<uint8>(type.luclinface / 10);
      }
      return fitLook(look);
    }

    void applyToSpawn(NPC *npc, const AssistantSpawnLook &look) {
      FaceChange_Struct face{};
      face.haircolor = look.hairColor;
      face.beardcolor = look.beardColor;
      face.eyecolor1 = npc->GetEyeColor1();
      face.eyecolor2 = npc->GetEyeColor2();
      face.hairstyle = look.hairStyle;
      face.beard = look.beard;
      face.face = look.face;
      face.drakkin_heritage = npc->GetDrakkinHeritage();
      face.drakkin_tattoo = npc->GetDrakkinTattoo();
      face.drakkin_details = npc->GetDrakkinDetails();
      npc->SetFaceAppearance(face, false);
      AppearanceStruct appearance;
      appearance.race_id = look.race;
      appearance.gender_id = look.gender;
      appearance.texture = look.texture;
      appearance.helmet_texture = npc->GetHelmTexture();
      appearance.size = GetRaceGenderDefaultHeight(look.race, look.gender);
      npc->SendIllusionPacket(appearance);
      for (uint8 slot = firstBodySlot; slot <= lastBodySlot; slot++) {
        npc->WearChange(slot, look.texture, look.bodyTint, 0);
      }
    }

    void applyStandard(NPC *npc, const NPCType &type) {
      AssistantSpawnLook standard{type.race, type.gender, type.luclinface, type.hairstyle, type.haircolor, type.beard, type.beardcolor, type.texture,
                                  type.armor_tint.Slot[firstBodySlot].Color};
      applyToSpawn(npc, standard);
      for (uint8 slot = firstBodySlot; slot <= lastBodySlot; slot++) {
        npc->WearChange(slot, type.texture, type.armor_tint.Slot[slot].Color, 0);
      }
    }

    const NPCType &npcTypeOf(const PlacedItem &row) {
      const NpcItem *npcItem = catalog().npcItem(row.item_id);
      if (!npcItem) {
        throw std::runtime_error(fmt::format("record [{}] holds item [{}], which is not an NPC item", row.id, row.item_id));
      }
      const NPCType *type = content_db.LoadNPCTypesData(npcItem->npcTypeId);
      if (!type) {
        throw std::runtime_error(fmt::format("NPC type [{}] does not exist", npcItem->npcTypeId));
      }
      return *type;
    }

    void useOnAssistant(Client *client, Session &state, NPC *npc) {
      auto row = placedItem(npc->GetHousingPropertyID(), npc->GetHousingRecordID());
      if (!row) {
        throw std::runtime_error(fmt::format("spawn [{}] is bound to record [{}], which does not exist", npc->GetID(), npc->GetHousingRecordID()));
      }
      const NPCType &type = npcTypeOf(*row);
      if (!isGuildAssistant(type) || !isAugmentaterRace(type.race)) {
        LogHousing("[{}] used the Augmentater on [{}], which is no guild assistant; nothing happens", client->GetCleanName(), npc->GetCleanName());
        return;
      }
      auto holder = state.properties.find(row->property_id);
      // Live's re-skins were the owners'; who else may is not captured, so the Augmentater takes Adjust Placement's gate.
      if (holder == state.properties.end() || !permitted(client, Permission::moveItem, holder->second, state.lookup(), state.accessLookup())) {
        LogHousing("[{}] may not re-skin record [{}] of property [{}]", client->GetCleanName(), row->id, row->property_id);
        return;
      }
      const AugmentaterMode mode = *state.augmentaterMode;
      if (mode == AugmentaterMode::reset) {
        query(database, fmt::format("DELETE FROM real_estate_item_looks WHERE real_estate_item_id = {}", row->id));
        applyStandard(npc, type);
        client->Message(Chat::White, fmt::format("Restoring appearance of {}.", npc->GetCleanName()).c_str());
        LogHousing("[{}] restored record [{}]'s standard look", client->GetCleanName(), row->id);
        return;
      }
      const auto stored = storedLook(row->id);
      AssistantLook look = stored ? *stored : standardLook(type);
      const bool changed = stepLook(look, mode);
      if (changed || !stored) {
        storeLook(row->id, look);
        applyToSpawn(npc, spawnLook(look));
      }
      client->Message(Chat::White, fmt::format("Changing {} of {}.", augmentaterFeatureName(mode), npc->GetCleanName()).c_str());
      LogHousing("[{}] used the Augmentater's [{}] on record [{}]{}", client->GetCleanName(), augmentaterModeName(mode), row->id, changed ? "" : "; this body has no such option");
    }
  }

  bool isGuildAssistant(const NPCType &type) {
    switch (type.class_) {
      case Class::Banker:
      case Class::Merchant:
      case Class::TributeMaster:
      case Class::GuildTributeMaster:
      case Class::GuildBanker:
        return true;
      default:
        return false;
    }
  }

  std::unique_ptr<NPCType> lookedNpcType(const PlacedItem &row, const NPCType &base) {
    const auto stored = storedLook(row.id);
    if (!stored) {
      return nullptr;
    }
    const AssistantSpawnLook look = spawnLook(*stored);
    auto type = std::make_unique<NPCType>();
    memcpy(type.get(), &base, sizeof(NPCType));
    type->race = look.race;
    type->gender = look.gender;
    type->luclinface = look.face;
    type->hairstyle = look.hairStyle;
    type->haircolor = look.hairColor;
    type->beard = look.beard;
    type->beardcolor = look.beardColor;
    type->texture = look.texture;
    type->armtexture = 0;
    type->bracertexture = 0;
    type->handtexture = 0;
    type->legtexture = 0;
    type->feettexture = 0;
    for (uint8 slot = firstBodySlot; slot <= lastBodySlot; slot++) {
      type->armor_tint.Slot[slot].Color = look.bodyTint;
    }
    type->size = GetRaceGenderDefaultHeight(look.race, look.gender);
    type->use_model = 0;
    return type;
  }

  void useAugmentater(Client *client) {
    Session *state = session(client);
    if (!state) {
      LogHousing("[{}] used the Augmentater with no housing session here; nothing happens", client->GetCleanName());
      return;
    }
    Mob *target = client->GetTarget();
    if (!target || target == client) {
      state->augmentaterMode = state->augmentaterMode ? nextAugmentaterMode(*state->augmentaterMode) : AugmentaterMode::race;
      client->Message(Chat::White, augmentaterModeLine(*state->augmentaterMode).c_str());
      return;
    }
    // The mode lives in the session: after a relog the Augmentater does nothing on an assistant until it is used on oneself (Live).
    if (!state->augmentaterMode) {
      LogHousing("[{}] used the Augmentater on [{}] before choosing a mode; nothing happens", client->GetCleanName(), target->GetCleanName());
      return;
    }
    if (!target->IsNPC() || !target->CastToNPC()->IsHousingNpcItem()) {
      LogHousing("[{}] used the Augmentater on [{}], which is no placed NPC item; nothing happens", client->GetCleanName(), target->GetCleanName());
      return;
    }
    if (DistanceNoZ(client->GetPosition(), target->GetPosition()) > augmentaterRange) {
      LogHousing("[{}] used the Augmentater on [{}] from beyond [{}] units; nothing happens", client->GetCleanName(), target->GetCleanName(), augmentaterRange);
      return;
    }
    try {
      useOnAssistant(client, *state, target->CastToNPC());
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s Augmentater use on [{}] failed: {}", client->GetCleanName(), target->GetCleanName(), error.what());
    }
  }

  void handleItemCastFinished(Client *client, uint16 spellId, uint32 inventorySlot) {
    if (spellId != augmentaterSpellId || !housingActive()) {
      return;
    }
    const EQ::ItemInstance *item = client->GetInv().GetItem(static_cast<int16>(inventorySlot));
    if (item && item->GetID() == augmentaterItemId) {
      useAugmentater(client);
    }
  }
}
