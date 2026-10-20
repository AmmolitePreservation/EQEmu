#include "housingGuildServices.h"

#include "housing.h"
#include "housingAccess.h"
#include "housingAugmentater.h"
#include "housingCatalog.h"
#include "housingInteriors.h"
#include "housingMenus.h"
#include "housingNpcItems.h"
#include "housingWorldObjects.h"

#include "../../common/classes.h"
#include "../../common/eqemu_logsys.h"
#include "../../common/item_instance.h"
#include "../../common/spdat.h"
#include "../../common/strings.h"
#include "../../common/timer.h"
#include "../client.h"
#include "../entity.h"
#include "../npc.h"
#include "../zonedb.h"

#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>

extern EntityList entity_list;

namespace housing {
  namespace {
    constexpr uint32 none = 0xFFFFFFFF;
    constexpr MenuEntry transactEntry{12, none, 6445, none};
    constexpr MenuEntry auraActiveEntry{18, none, 5323, none};
    constexpr MenuEntry permanentAuraEntry{18, none, 5322, none};
    constexpr MenuEntry deactivateAuraEntry{8, deactivateAuraAction, 5340, none};
    constexpr MenuEntry activateAuraEntry{8, activateAuraAction, 5324, none};
    constexpr uint32 notGuildMemberBank = 5427;
    // Live aligned the portal about 8 s after a key was handed in (NPC-15).
    constexpr uint32 alignmentDelayMilliseconds = 8000;
    // Live opened the geode's dialog on its platform and not from 12.7 units off; the platform's own radius is not measured (NPC-25).
    constexpr float platformRadius = 5.0f;
    constexpr float platformHeight = 10.0f;
    constexpr uint16 invisibleMan = 127;
    constexpr uint8 noTargetBodyType = 11;

    // Live's aura NPC is server-only: the client's spawn list never holds it (NPC-6).
    class AuraSpawn : public NPC {
    public:
      AuraSpawn(NPCType *type, const glm::vec4 &position) : NPC(type, nullptr, position, GravityBehavior::Flying) {
        GiveNPCTypeData(type);
      }
      bool ShouldISpawnFor(Client *client) override { return false; }
    };

    struct AuraState {
      PlacedItem row;
      bool active = false;
      uint16 npcId = 0;
      std::set<uint32> inside;
    };

    struct PendingAlignment {
      uint32 propertyId;
      uint32 keyItemId;
      uint16 npcId;
      uint32 dueAt;
    };

    std::map<uint32, AuraState> auras;
    std::vector<PendingAlignment> alignments;
    std::map<uint32, uint32> platformOf;
    std::map<uint32, uint32> geodeOffers;
    Timer serviceTimer(1000);
    Timer auraSyncTimer(5000);
    bool syncedAtBoot = false;

    NPCType *auraNpcType(uint32 spellId) {
      auto type = new NPCType;
      memset(type, 0, sizeof(NPCType));
      strn0cpy(type->name, spells[spellId].name, sizeof(type->name));
      type->current_hp = 2000000000;
      type->max_hp = 2000000000;
      type->race = invisibleMan;
      type->size = 0.1f;
      type->gender = 2;
      type->class_ = Class::Warrior;
      type->level = 100;
      type->bodytype = noTargetBodyType;
      type->findable = 0;
      type->trackable = 0;
      type->untargetable = true;
      type->skip_global_loot = true;
      strcpy(type->special_abilities, "12,1^14,1^16,1^18,1^19,1^20,1^21,1^22,1^23,1^24,1^25,1^35,1^46,1^47,1");
      return type;
    }

    const AuraItem &auraOf(const PlacedItem &row) {
      const AuraItem *aura = catalog().auraItem(row.item_id);
      if (!aura) {
        throw std::logic_error(fmt::format("record [{}] holds item [{}], which keeps no aura", row.id, row.item_id));
      }
      return *aura;
    }

    NPC *auraNpc(const AuraState &state) {
      return state.npcId ? entity_list.GetNPCByID(state.npcId) : nullptr;
    }

    void spawnAura(AuraState &state) {
      const AuraItem &aura = auraOf(state.row);
      auto npc = new AuraSpawn(auraNpcType(aura.spellId), glm::vec4(state.row.x, state.row.y, state.row.z, state.row.heading));
      npc->MarkHousingSpawn();
      entity_list.AddNPC(npc, false, false);
      state.npcId = npc->GetID();
      LogHousing("Aura of record [{}] runs as server-only spawn [{}]", state.row.id, state.npcId);
    }

    // Leaving the circle and losing the aura print the same line (Live: Deactivate printed the leave line to those inside).
    void fadeFrom(AuraState &state, Client *client) {
      const AuraItem &aura = auraOf(state.row);
      client->BuffFadeBySpellID(static_cast<uint16>(aura.spellId));
      client->Message(Chat::White, aura.leaveMessage.c_str());
    }

    void stopAura(AuraState &state) {
      for (uint32 characterId : state.inside) {
        if (Client *client = entity_list.GetClientByCharID(characterId)) {
          fadeFrom(state, client);
        }
      }
      state.inside.clear();
      if (NPC *npc = auraNpc(state)) {
        npc->Depop(false);
      }
      state.npcId = 0;
    }

    std::string idList(const std::set<uint32> &ids) {
      std::vector<std::string> texts;
      for (uint32 id : ids) {
        texts.push_back(std::to_string(id));
      }
      return Strings::Join(texts, ",");
    }

    void syncAuras() {
      const std::set<uint32> shown = propertiesShownInZone();
      const std::set<uint32> auraItemIds = catalog().auraItemIds();
      std::map<uint32, PlacedItem> found;
      std::set<uint32> inactive;
      if (!shown.empty() && !auraItemIds.empty()) {
        for (const auto &row : RealEstateItemsRepository::GetWhere(database, fmt::format(
          "placed = 1 AND item_id IN ({}) AND property_id IN ({})", idList(auraItemIds), idList(shown)))) {
          found[row.id] = row;
        }
      }
      if (!found.empty()) {
        std::set<uint32> ids;
        for (const auto &[id, row] : found) {
          ids.insert(id);
        }
        for (auto row : query(database, fmt::format("SELECT real_estate_item_id FROM real_estate_inactive_auras WHERE real_estate_item_id IN ({})", idList(ids)))) {
          inactive.insert(Strings::ToUnsignedInt(row[0]));
        }
      }
      for (auto current = auras.begin(); current != auras.end();) {
        auto row = found.find(current->first);
        const bool moved = row != found.end() && (row->second.x != current->second.row.x || row->second.y != current->second.row.y ||
          row->second.z != current->second.row.z || row->second.property_id != current->second.row.property_id);
        const bool turned = row != found.end() && (inactive.count(current->first) > 0) == current->second.active;
        if (row == found.end() || moved || turned) {
          stopAura(current->second);
          current = auras.erase(current);
          continue;
        }
        ++current;
      }
      for (const auto &[id, row] : found) {
        if (auras.count(id)) {
          continue;
        }
        AuraState state;
        state.row = row;
        state.active = !inactive.count(id);
        if (state.active) {
          spawnAura(state);
        }
        auras[id] = state;
      }
    }

    bool receivesAura(Client *client, const AuraState &state, const AuraItem &aura, NPC *npc) {
      Session *viewer = session(client);
      if (!viewer || client->GetHP() <= 0) {
        return false;
      }
      auto holder = viewer->properties.find(state.row.property_id);
      if (holder == viewer->properties.end() || Distance(client->GetPosition(), npc->GetPosition()) > aura.radius) {
        return false;
      }
      return permitted(client, Permission::receiveBuffs, holder->second, viewer->lookup(), viewer->accessLookup());
    }

    void tickAuras() {
      for (auto &[id, state] : auras) {
        NPC *npc = auraNpc(state);
        if (!state.active || !npc) {
          continue;
        }
        const AuraItem &aura = auraOf(state.row);
        std::set<uint32> now;
        for (const auto &[entityId, client] : entity_list.GetClientList()) {
          if (!receivesAura(client, state, aura, npc)) {
            continue;
          }
          now.insert(client->CharacterID());
          if (!state.inside.count(client->CharacterID())) {
            npc->SpellFinished(static_cast<uint16>(aura.spellId), client);
            client->Message(Chat::White, aura.nearMessage.c_str());
          }
        }
        for (uint32 characterId : state.inside) {
          if (!now.count(characterId)) {
            if (Client *client = entity_list.GetClientByCharID(characterId)) {
              fadeFrom(state, client);
            }
          }
        }
        state.inside = now;
      }
    }

    std::optional<uint32> alignmentOf(uint32 propertyId) {
      auto rows = query(database, fmt::format("SELECT key_item_id FROM real_estate_teleport_alignments WHERE property_id = {}", propertyId));
      if (rows.RowCount() == 0) {
        return std::nullopt;
      }
      return Strings::ToUnsignedInt(rows.begin()[0]);
    }

    void tickAlignments() {
      const uint32 now = Timer::GetCurrentTime();
      for (auto pending = alignments.begin(); pending != alignments.end();) {
        if (now < pending->dueAt) {
          ++pending;
          continue;
        }
        const TeleportKey *key = catalog().teleportKey(pending->keyItemId);
        if (key) {
          query(database, fmt::format("REPLACE INTO real_estate_teleport_alignments (property_id, key_item_id) VALUES ({}, {})", pending->propertyId, pending->keyItemId));
          if (NPC *npc = entity_list.GetNPCByID(pending->npcId)) {
            npc->Say("The Guildhall Portal has been aligned to %s.", key->destination.c_str());
          }
          LogHousing("Property [{}]'s teleportation device is aligned to key [{}]", pending->propertyId, pending->keyItemId);
        }
        pending = alignments.erase(pending);
      }
    }

    void tickPlatforms() {
      std::map<uint32, uint32> standing;
      forEachNpcItemSpawn([&](const PlacedItem &row, NPC *npc) {
        const NpcItem *npcItem = catalog().npcItem(row.item_id);
        if (!npcItem || npcItem->teleportRole != TeleportRole::device) {
          return;
        }
        for (const auto &[entityId, client] : entity_list.GetClientList()) {
          if (DistanceNoZ(client->GetPosition(), npc->GetPosition()) <= platformRadius && std::fabs(client->GetZ() - npc->GetZ()) <= platformHeight) {
            standing[client->CharacterID()] = row.id;
          }
        }
      });
      for (const auto &[characterId, recordId] : standing) {
        auto previous = platformOf.find(characterId);
        if (previous != platformOf.end() && previous->second == recordId) {
          continue;
        }
        Client *client = entity_list.GetClientByCharID(characterId);
        NPC *device = npcItemSpawn(recordId);
        if (!client || !device) {
          continue;
        }
        const auto aligned = alignmentOf(device->GetHousingPropertyID());
        const TeleportKey *key = aligned ? catalog().teleportKey(*aligned) : nullptr;
        if (!key) {
          LogHousing("[{}] stepped onto device record [{}], which is aligned nowhere", client->GetCleanName(), recordId);
          continue;
        }
        geodeOffers[characterId] = *aligned;
        client->SendFullPopup("The Crystal Geode",
          fmt::format("The crystal geode is set to go to {}.  Would you like to travel there now?", key->destination).c_str(),
          geodePopupId, 0, 1, 0, "Yes", "No");
      }
      platformOf = standing;
    }

    std::string guildBankerName(Client *client) {
      Mob *target = client->GetTarget();
      if (target && target->IsNPC() && target->GetClass() == Class::GuildBanker) {
        return target->GetCleanName();
      }
      NPC *nearest = nullptr;
      float best = 0.0f;
      for (const auto &[id, npc] : entity_list.GetNPCList()) {
        if (npc->GetClass() != Class::GuildBanker) {
          continue;
        }
        const float distance = DistanceSquared(client->GetPosition(), npc->GetPosition());
        if (!nearest || distance < best) {
          nearest = npc;
          best = distance;
        }
      }
      if (!nearest) {
        throw std::runtime_error("the guild bank was asked for in a hall with no guild banker");
      }
      return nearest->GetCleanName();
    }
  }

  bool isAuraAction(uint32 b) {
    return b == activateAuraAction || b == deactivateAuraAction;
  }

  void addGuildServiceInteractions(Client *client, const MenuSubject &subject, MenuInteractions &interactions) {
    if (!subject.item.placed || !subject.inItemZone) {
      return;
    }
    const auto lookup = subject.state.lookup();
    const auto listOf = subject.state.accessLookup();
    if (const NpcItem *npcItem = catalog().npcItem(subject.item.item_id)) {
      const NPCType *type = content_db.LoadNPCTypesData(npcItem->npcTypeId);
      if (!type) {
        throw std::runtime_error(fmt::format("NPC type [{}] does not exist", npcItem->npcTypeId));
      }
      if (isGuildAssistant(*type) && interactions.npcSpawnId && mayTransact(client, subject.holder, lookup, listOf)) {
        interactions.entries.push_back(transactEntry);
      }
      return;
    }
    if (!catalog().auraItem(subject.item.item_id) || !mayTransact(client, subject.holder, lookup, listOf)) {
      return;
    }
    auto state = auras.find(subject.item.id);
    NPC *npc = state == auras.end() ? nullptr : auraNpc(state->second);
    if (npc && state->second.active) {
      interactions.entries.push_back(auraActiveEntry);
      interactions.entries.push_back(permanentAuraEntry);
      interactions.entries.push_back(deactivateAuraEntry);
      interactions.npcSpawnId = npc->GetID();
    }
    else {
      interactions.entries.push_back(activateAuraEntry);
    }
  }

  void toggleAura(Client *client, const Session &state, const Property &holder, const PlacedItem &row, uint32 b) {
    if (!catalog().auraItem(row.item_id) || !row.placed) {
      LogHousing("[{}] asked record [{}] for aura action [{}], which it has none of", client->GetCleanName(), row.id, b);
      return;
    }
    if (!mayTransact(client, holder, state.lookup(), state.accessLookup())) {
      LogHousing("[{}] may not turn record [{}]'s aura", client->GetCleanName(), row.id);
      return;
    }
    if (b == deactivateAuraAction) {
      query(database, fmt::format("INSERT IGNORE INTO real_estate_inactive_auras (real_estate_item_id) VALUES ({})", row.id));
    }
    else {
      query(database, fmt::format("DELETE FROM real_estate_inactive_auras WHERE real_estate_item_id = {}", row.id));
    }
    syncAuras();
    LogHousing("[{}] turned record [{}]'s aura {}", client->GetCleanName(), row.id, b == activateAuraAction ? "on" : "off");
  }

  void processGuildServices() {
    if (!syncedAtBoot || auraSyncTimer.Check()) {
      syncedAtBoot = true;
      try {
        syncAuras();
      }
      catch (const std::exception &error) {
        LogError("Housing: the placed auras could not be read: {}", error.what());
      }
    }
    if (!serviceTimer.Check()) {
      return;
    }
    try {
      tickAuras();
      tickAlignments();
      tickPlatforms();
    }
    catch (const std::exception &error) {
      LogError("Housing: the guild services' tick failed: {}", error.what());
    }
  }

  void resetGuildServices() {
    auras.clear();
    alignments.clear();
    platformOf.clear();
    geodeOffers.clear();
    syncedAtBoot = false;
  }

  // An aura's buff never leaves its zone: a character arrives without it and gets it again inside a circle.
  void stripAuraBuffs(Client *client) {
    for (uint32 spellId : catalog().auraSpellIds()) {
      client->BuffFadeBySpellID(static_cast<uint16>(spellId));
    }
  }

  void forgetGuildServiceClient(Client *client) {
    platformOf.erase(client->CharacterID());
    geodeOffers.erase(client->CharacterID());
    for (auto &[id, state] : auras) {
      state.inside.erase(client->CharacterID());
    }
  }

  void handleTrade(Client *client, NPC *npc, std::vector<EQ::ItemInstance *> &items) {
    if (!housingActive() || !npc->IsHousingNpcItem()) {
      return;
    }
    try {
      auto row = placedItem(npc->GetHousingPropertyID(), npc->GetHousingRecordID());
      const NpcItem *npcItem = row ? catalog().npcItem(row->item_id) : nullptr;
      if (!npcItem || npcItem->teleportRole != TeleportRole::assistant) {
        return;
      }
      // He consumes every key he is handed, a whole stack included, and aligns the portal once for each (Live, NPC-15).
      for (auto &inst : items) {
        if (!inst || !inst->GetItem() || !catalog().teleportKey(inst->GetID())) {
          continue;
        }
        const int16 count = inst->IsStackable() ? std::max<int16>(inst->GetCharges(), 1) : 1;
        for (int16 index = 0; index < count; index++) {
          alignments.push_back(PendingAlignment{row->property_id, inst->GetID(), npc->GetID(), Timer::GetCurrentTime() + alignmentDelayMilliseconds});
        }
        LogHousing("[{}] handed [{}] of key [{}] to the teleportation assistant of property [{}]", client->GetCleanName(), count, inst->GetID(), row->property_id);
        inst = nullptr;
      }
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s trade with [{}] failed: {}", client->GetCleanName(), npc->GetCleanName(), error.what());
    }
  }

  bool handlePopupResponse(Client *client, uint32 popupId) {
    if (popupId != geodePopupId) {
      return false;
    }
    auto offer = geodeOffers.find(client->CharacterID());
    if (offer == geodeOffers.end()) {
      LogHousing("[{}] answered a geode dialog the server did not open", client->GetCleanName());
      return true;
    }
    const TeleportKey *key = catalog().teleportKey(offer->second);
    geodeOffers.erase(offer);
    if (!key) {
      LogError("Housing: [{}]'s geode is aligned to a key the catalog no longer has", client->GetCleanName());
      return true;
    }
    // EQEmu has no planted guild standards, so a standard key's Yes moves nobody, as Live's did for a guild with none planted.
    if (key->guildStandard) {
      LogHousing("[{}]'s geode goes to the guild's standard, which EQEmu has none of; nothing happens", client->GetCleanName());
      return true;
    }
    LogHousing("[{}] takes the crystal geode to {}", client->GetCleanName(), key->destination);
    client->MovePC(key->zoneId, 0, key->x, key->y, key->z, key->heading);
    return true;
  }

  uint32 guildHallGuildId() {
    if (!housingActive() || !hostsInterior()) {
      return 0;
    }
    const auto root = hostedRoot();
    const auto yard = hostedRootYard();
    return typeOf(*root) == DefinitionType::guildHall && yard && typeOf(*yard) == DefinitionType::guildPlot ? activeGuildId(*yard) : 0;
  }

  bool refuseGuildBank(Client *client) {
    const uint32 guildId = guildHallGuildId();
    if (!guildId || client->GuildID() == guildId) {
      return false;
    }
    try {
      client->MessageString(Chat::Tell, notGuildMemberBank, guildBankerName(client).c_str());
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s guild bank refusal could not be told: {}", client->GetCleanName(), error.what());
    }
    return true;
  }
}
