#include "housingNpcItems.h"

#include "housingCatalog.h"
#include "housingInteriors.h"
#include "housingItemTail.h"
#include "housingMessages.h"
#include "housingSession.h"
#include "housingWorldObjects.h"

#include "../../common/bodytypes.h"
#include "../../common/eqemu_logsys.h"
#include "../../common/timer.h"
#include "../client.h"
#include "../common.h"
#include "../entity.h"
#include "../npc.h"
#include "../zone.h"
#include "../zonedb.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

extern EntityList entity_list;
extern Zone *zone;

namespace housing {
  namespace {
    constexpr uint32 npcNotFound = 42;
    constexpr size_t bindingGuidLength = 0x14;

    // Live's roaming distance and kick distance are not captured (NPC-20); these keep a pet in the yard it stands in.
    constexpr float roamDistance = 20.0f;
    constexpr float kickDistance = 20.0f;

    struct Spawned {
      PlacedItem row;
      uint16 npcId = 0;
      uint16 mountId = 0;
      int64 respawnAt = 0;
      int64 mountRespawnAt = 0;
    };

    std::map<uint32, Spawned> spawned;
    bool spawnedAtBoot = false;
    Timer respawnTimer(1000);

    NPC *liveSpawn(uint16 entityId, uint32 recordId, bool mount) {
      if (!entityId) {
        return nullptr;
      }
      NPC *npc = entity_list.GetNPCByID(entityId);
      if (!npc || npc->GetDepop() || !npc->IsHousingSpawn() || npc->GetHP() <= 0) {
        return nullptr;
      }
      if (!mount && npc->GetHousingRecordID() != recordId) {
        return nullptr;
      }
      return npc;
    }

    const NPCType *npcType(uint32 npcTypeId) {
      const NPCType *type = content_db.LoadNPCTypesData(npcTypeId);
      if (!type) {
        throw std::runtime_error(fmt::format("NPC type [{}] does not exist", npcTypeId));
      }
      return type;
    }

    NPC *spawnAt(const PlacedItem &row, uint32 npcTypeId) {
      auto npc = new NPC(npcType(npcTypeId), nullptr, glm::vec4(row.x, row.y, row.z, row.heading), GravityBehavior::Water);
      return npc;
    }

    uint16 spawnRider(const PlacedItem &row, const NpcItem &npcItem) {
      NPC *npc = spawnAt(row, npcItem.npcTypeId);
      npc->SetHousingBinding(row.property_id, row.id, itemGuid(row));
      entity_list.AddNPC(npc, true, true);
      return npc->GetID();
    }

    uint16 spawnMount(const PlacedItem &row, const NpcItem &npcItem) {
      NPC *npc = spawnAt(row, npcItem.mountNpcTypeId);
      npc->MarkHousingSpawn();
      entity_list.AddNPC(npc, true, true);
      return npc->GetID();
    }

    // 0x21ea binds the item being placed to a spawn that appeared while it is edited (its respawn).
    void sendBindingToEditors(const PlacedItem &row, uint16 spawnId) {
      forEachSession([&](Client *client, Session &state) {
        if (!state.placement || !state.placement->editing || state.placement->sourceRecordId != row.id) {
          return;
        }
        WireWriter message;
        message.u32(spawnId).fixedString(itemGuid(row), bindingGuidLength).u32(row.property_id).u32(row.id);
        sendMessage(client, OP_RealEstateNpcItemBinding, message);
      });
    }

    uint32 respawnSeconds(uint32 itemId) {
      const EQ::ItemHousingTail *tail = itemHousingTail(itemId);
      if (!tail) {
        throw std::runtime_error(fmt::format("NPC item [{}] has no housing tail", itemId));
      }
      return tail->npcRespawnInterval;
    }

    void stopMoving(NPC *npc) {
      npc->SetFollowID(0);
      npc->AI_SetRoambox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
      npc->StopNavigation();
    }

    void roam(NPC *npc, const PlacedItem &row, bool running) {
      npc->SetFollowID(0);
      npc->SetAppearance(eaStanding);
      npc->SetRunning(running);
      npc->AI_SetRoambox(roamDistance, row.x + roamDistance, row.x - roamDistance, row.y + roamDistance, row.y - roamDistance);
    }

    void kick(Client *client, NPC *npc) {
      client->DoAnim(animKick);
      stopMoving(npc);
      const float dx = npc->GetX() - client->GetX();
      const float dy = npc->GetY() - client->GetY();
      const float length = std::sqrt(dx * dx + dy * dy);
      if (length <= 0.0f) {
        return;
      }
      npc->SetRunning(true);
      npc->RunTo(npc->GetX() + dx / length * kickDistance, npc->GetY() + dy / length * kickDistance, npc->GetZ());
    }

    void reportStatistics(Client *client, NPC *npc) {
      client->Message(Chat::White, fmt::format(
        "{}: level {}, armor class {}, hit points {} of {}, body type {}.",
        npc->GetCleanName(), npc->GetLevel(), static_cast<int>(npc->GetNPCStat("ac")), npc->GetHP(), npc->GetMaxHP(),
        npc->GetBodyType() ? BodyType::GetName(npc->GetBodyType()) : "none"
      ).c_str());
    }

    void changeLevel(NPC *npc, int step) {
      const int level = npc->GetLevel() + step;
      if (level < 1 || level > 255) {
        LogHousing("Training dummy [{}] stays at level [{}]: [{}] is out of range", npc->GetCleanName(), npc->GetLevel(), level);
        return;
      }
      npc->SetLevel(static_cast<uint8>(level));
    }

    void changeArmorClass(NPC *npc, const NPCType &type, int percent) {
      const int64 armorClass = static_cast<int64>(npc->GetNPCStat("ac")) + static_cast<int64>(type.AC) * percent / 100;
      npc->ModifyNPCStat("ac", std::to_string(std::max<int64>(0, armorClass)));
    }

    void setHitPoints(NPC *npc, int64 hitPoints) {
      npc->ModifyNPCStat("max_hp", std::to_string(hitPoints));
      npc->SetHP(npc->GetMaxHP());
    }

    // Live's dummy actions by their b codes (menus.tsv): body type, armor class, level and hit points, and a report.
    void trainingDummyAction(Client *client, NPC *npc, const NPCType &type, uint32 action) {
      switch (action) {
        case 31447: npc->SetBodyType(0, false); break;
        case 31448: npc->SetBodyType(BodyType::Animal, false); break;
        case 31449: npc->SetBodyType(BodyType::Construct, false); break;
        case 31450: npc->SetBodyType(BodyType::Humanoid, false); break;
        case 31451: npc->SetBodyType(BodyType::Plant, false); break;
        case 31452: npc->SetBodyType(BodyType::Undead, false); break;
        case 31453: npc->ModifyNPCStat("ac", std::to_string(type.AC)); break;
        case 31454: changeArmorClass(npc, type, -10); break;
        case 31455: changeArmorClass(npc, type, -5); break;
        case 31456: changeArmorClass(npc, type, 5); break;
        case 31457: changeArmorClass(npc, type, 10); break;
        case 31458: reportStatistics(client, npc); return;
        case 31459: npc->SetLevel(type.level); break;
        case 31460: changeLevel(npc, -1); break;
        case 31461: changeLevel(npc, 1); break;
        case 44113: changeLevel(npc, -5); break;
        case 44114: changeLevel(npc, 5); break;
        case 44115: changeLevel(npc, -10); break;
        case 44116: changeLevel(npc, 10); break;
        case 44117: setHitPoints(npc, type.max_hp); break;
        case 44118: setHitPoints(npc, type.max_hp * 10); break;
        case 44119: setHitPoints(npc, type.max_hp * 100); break;
        case 44120: setHitPoints(npc, type.max_hp * 1000); break;
        default:
          throw std::logic_error(fmt::format("[{}] is not a training dummy action", action));
      }
      LogHousing("[{}] used training dummy action [{}] on [{}]", client->GetCleanName(), action, npc->GetCleanName());
    }
  }

  bool isPetAction(uint32 action) {
    switch (static_cast<PetAction>(action)) {
      case PetAction::stay:
      case PetAction::walkAround:
      case PetAction::runAround:
      case PetAction::followMe:
      case PetAction::playDead:
      case PetAction::kickTheGnome:
      case PetAction::sitDown:
        return true;
    }
    return false;
  }

  bool isTrainingDummyAction(uint32 action) {
    return (action >= 31447 && action <= 31461) || (action >= 44113 && action <= 44120);
  }

  bool isNpcItemId(uint32 itemId) {
    const EQ::ItemHousingTail *tail = itemHousingTail(itemId);
    return tail && isNpcItem(tail->housepetFileName, tail->interactiveObject != 0);
  }

  void spawnNpcItem(const PlacedItem &row) {
    const NpcItem *npcItem = catalog().npcItem(row.item_id);
    if (!npcItem) {
      throw std::runtime_error(fmt::format("NPC item [{}] of record [{}] has no real_estate_npc_items row", row.item_id, row.id));
    }
    despawnNpcItem(row.id);
    Spawned entry;
    entry.row = row;
    if (npcItem->mountNpcTypeId) {
      entry.mountId = spawnMount(row, *npcItem);
    }
    entry.npcId = spawnRider(row, *npcItem);
    spawned[row.id] = entry;
    LogHousing("Spawned NPC item record [{}] (item [{}]) of property [{}] as spawn [{}]", row.id, row.item_id, row.property_id, entry.npcId);
  }

  void despawnNpcItem(uint32 recordId) {
    auto found = spawned.find(recordId);
    if (found == spawned.end()) {
      return;
    }
    const Spawned entry = found->second;
    spawned.erase(found);
    if (NPC *npc = liveSpawn(entry.npcId, recordId, false)) {
      npc->Depop(false);
    }
    if (NPC *mount = liveSpawn(entry.mountId, recordId, true)) {
      mount->Depop(false);
    }
  }

  NPC *npcItemSpawn(uint32 recordId) {
    auto found = spawned.find(recordId);
    return found == spawned.end() ? nullptr : liveSpawn(found->second.npcId, recordId, false);
  }

  void spawnShownNpcItems() {
    std::set<uint32> shown;
    const auto root = hostedRoot();
    for (const auto &property : hostedProperties()) {
      if (standsOutdoors(property) || (root && property.id == root->id)) {
        shown.insert(property.id);
      }
    }
    for (const auto &row : itemsOf(shown)) {
      if (!row.placed || !isNpcItemId(row.item_id)) {
        continue;
      }
      try {
        spawnNpcItem(row);
      }
      catch (const std::exception &error) {
        LogError("Housing: NPC item record [{}] (item [{}]) of property [{}] could not spawn: {}", row.id, row.item_id, row.property_id, error.what());
      }
    }
  }

  void resetNpcItems() {
    spawned.clear();
    spawnedAtBoot = false;
  }

  void processNpcItems() {
    if (!spawnedAtBoot) {
      spawnedAtBoot = true;
      spawnShownNpcItems();
    }
    if (!respawnTimer.Check()) {
      return;
    }
    const int64 now = currentTime();
    for (auto current = spawned.begin(); current != spawned.end();) {
      const uint32 recordId = current->first;
      Spawned &entry = current->second;
      const NpcItem *npcItem = catalog().npcItem(entry.row.item_id);
      if (!npcItem) {
        LogError("Housing: NPC item [{}] of record [{}] has no real_estate_npc_items row since the catalog reloaded; it no longer respawns", entry.row.item_id, recordId);
        current = spawned.erase(current);
        continue;
      }
      ++current;
      if (entry.npcId && !liveSpawn(entry.npcId, recordId, false)) {
        entry.npcId = 0;
        entry.respawnAt = now + respawnSeconds(entry.row.item_id);
      }
      if (entry.mountId && !liveSpawn(entry.mountId, recordId, true)) {
        entry.mountId = 0;
        entry.mountRespawnAt = now + respawnSeconds(entry.row.item_id);
      }
      if (!entry.npcId && now >= entry.respawnAt) {
        entry.npcId = spawnRider(entry.row, *npcItem);
        sendBindingToEditors(entry.row, entry.npcId);
        LogHousing("Respawned NPC item record [{}] as spawn [{}]", recordId, entry.npcId);
      }
      if (npcItem->mountNpcTypeId && !entry.mountId && now >= entry.mountRespawnAt) {
        entry.mountId = spawnMount(entry.row, *npcItem);
      }
    }
  }

  void performNpcItemAction(Client *client, const PlacedItem &row, uint32 action) {
    const NpcItem *npcItem = catalog().npcItem(row.item_id);
    if (!npcItem) {
      throw std::runtime_error(fmt::format("record [{}] holds item [{}], which is not an NPC item", row.id, row.item_id));
    }
    NPC *npc = npcItemSpawn(row.id);
    if (!npc) {
      WireWriter message;
      message.u32(npcNotFound);
      sendMessage(client, OP_RealEstateItemResult, message);
      return;
    }
    if (npcItem->trainingDummy && isTrainingDummyAction(action)) {
      trainingDummyAction(client, npc, *npcType(npcItem->npcTypeId), action);
      return;
    }
    if (std::find(npcItem->actions.begin(), npcItem->actions.end(), action) == npcItem->actions.end()) {
      LogHousing("[{}] asked NPC item record [{}] for action [{}], which its menu does not offer", client->GetCleanName(), row.id, action);
      return;
    }
    switch (static_cast<PetAction>(action)) {
      case PetAction::stay:
        stopMoving(npc);
        npc->SetAppearance(eaStanding);
        break;
      case PetAction::walkAround:
        roam(npc, row, false);
        break;
      case PetAction::runAround:
        roam(npc, row, true);
        break;
      case PetAction::followMe:
        npc->AI_SetRoambox(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        npc->SetAppearance(eaStanding);
        npc->SetFollowID(client->GetID());
        break;
      case PetAction::playDead:
        stopMoving(npc);
        npc->SetAppearance(eaDead);
        break;
      case PetAction::sitDown:
        stopMoving(npc);
        npc->SetAppearance(eaSitting);
        break;
      case PetAction::kickTheGnome:
        kick(client, npc);
        break;
    }
    LogHousing("[{}] used pet action [{}] on NPC item record [{}]", client->GetCleanName(), action, row.id);
  }
}
