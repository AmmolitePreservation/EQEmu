#include "housingGuildServices.h"

#include "housing.h"
#include "housingAccess.h"
#include "housingAugmentater.h"
#include "housingCatalog.h"
#include "housingInteriors.h"
#include "housingMenus.h"
#include "housingWorldObjects.h"

#include "../../common/classes.h"
#include "../../common/eqemu_logsys.h"
#include "../../common/strings.h"
#include "../../common/timer.h"
#include "../aura.h"
#include "../client.h"
#include "../entity.h"
#include "../npc.h"
#include "../zonedb.h"

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
    // The aura's quest script reads the pool's property from this entity variable.
    constexpr const char *auraPropertyVariable = "housing_property_id";

    struct AuraState {
      PlacedItem row;
      bool active = false;
      uint16 npcId = 0;
    };

    std::map<uint32, AuraState> auras;
    Timer auraSyncTimer(5000);
    bool syncedAtBoot = false;

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

    // The pool's aura is one of EQEmu's (NPC-34): a stationary FullyScripted aura with no owner, from its auras row, whose quest
    // script gets an event as each one comes within its distance and goes out; the client is never sent it (NPC-6).
    void spawnAura(AuraState &state) {
      const AuraItem &aura = auraOf(state.row);
      AuraRecord record{};
      if (!content_db.GetAuraEntry(static_cast<uint16>(aura.auraType), record)) {
        throw std::runtime_error(fmt::format("aura [{}] of item [{}] has no auras row", aura.auraType, state.row.item_id));
      }
      const NPCType *base = content_db.LoadNPCTypesData(record.npc_type);
      if (!base) {
        throw std::runtime_error(fmt::format("aura [{}] names NPC type [{}], which does not exist", aura.auraType, record.npc_type));
      }
      auto type = new NPCType;
      memcpy(type, base, sizeof(NPCType));
      strn0cpy(type->name, record.name, sizeof(type->name));
      auto npc = new Aura(type, glm::vec4(state.row.x, state.row.y, state.row.z, state.row.heading), record);
      npc->MarkHousingSpawn();
      npc->SetEntityVariable(auraPropertyVariable, std::to_string(state.row.property_id));
      entity_list.AddNPC(npc, false);
      state.npcId = npc->GetID();
      LogHousing("Aura of record [{}] runs as aura spawn [{}]", state.row.id, state.npcId);
    }

    // The depop hands the script a leave event for everyone inside (Live: Deactivate printed the leave line to those inside).
    void stopAura(AuraState &state) {
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
  }

  void resetGuildServices() {
    auras.clear();
    syncedAtBoot = false;
  }

  // An aura's buff never leaves its zone: a character arrives without it and gets it again inside a circle.
  void stripAuraBuffs(Client *client) {
    for (uint32 spellId : catalog().auraSpellIds()) {
      client->BuffFadeBySpellID(static_cast<uint16>(spellId));
    }
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
