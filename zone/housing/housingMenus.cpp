#include "housingMenus.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingContainers.h"
#include "housingEvents.h"
#include "housingGuildServices.h"
#include "housingInteriors.h"
#include "housingNpcItems.h"
#include "housingPlacement.h"
#include "housingWorldObjects.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingLedger.h"
#include "../../common/housingPropertyItem.h"
#include "../../common/patches/rof2.h"
#include "../../common/repositories/character_parcels_repository.h"
#include "../../common/rulesys.h"
#include "../../common/servertalk.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../npc.h"
#include "../worldserver.h"
#include "../zonedb.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <memory>
#include <optional>

extern WorldServer worldserver;

namespace housing {
  namespace {
    constexpr size_t menuSize = 0x300;
    constexpr size_t titleOffset = 0x2c;
    constexpr size_t titleLength = 64;
    constexpr size_t entriesOffset = 0x6c;
    constexpr size_t entryCount = 40;
    constexpr size_t npcSpawnOffset = 0x2fc;
    constexpr uint32 none = 0xFFFFFFFF;

    constexpr MenuEntry enterEntry{1, none, 723, none};
    constexpr MenuEntry adjustPlacementEntry{2, 0, 5244, none};
    constexpr MenuEntry placeEntry{2, 1, 719, none};
    constexpr MenuEntry moveToStorageEntry{3, none, 718, none};
    constexpr MenuEntry moveToInventoryEntry{4, none, 724, none};
    constexpr MenuEntry mailToOwnerEntry{5, none, 742, none};
    constexpr MenuEntry inspectEntry{7, none, 720, none};
    constexpr MenuEntry manageEntry{0, none, 722, none};
    constexpr MenuEntry showItemsEntry{11, none, 5285, none};
    constexpr MenuEntry useEntry{8, 0, 5236, none};
    constexpr MenuEntry swapHousesEntry{17, none, 5336, none};
    constexpr uint32 mailToOwnerSize = 148;

    // Live's training dummy menu (menus.tsv): four submenus of type 9 actions keyed by their header, then Report Statistics.
    constexpr MenuEntry trainingDummyEntries[] = {
      {10, 200, 200, none}, {9, 31453, 201, 200}, {9, 31454, 202, 200}, {9, 31455, 203, 200}, {9, 31456, 204, 200}, {9, 31457, 205, 200},
      {10, 100, 100, none}, {9, 31447, 101, 100}, {9, 31448, 102, 100}, {9, 31449, 103, 100}, {9, 31450, 104, 100}, {9, 31451, 105, 100},
      {9, 31452, 106, 100},
      {10, 400, 400, none}, {9, 31459, 401, 400}, {9, 31460, 402, 400}, {9, 31461, 403, 400}, {9, 44113, 404, 400}, {9, 44114, 405, 400},
      {9, 44115, 406, 400}, {9, 44116, 407, 400},
      {10, 500, 500, none}, {9, 44117, 501, 500}, {9, 44118, 502, 500}, {9, 44119, 503, 500}, {9, 44120, 504, 500},
      {9, 31458, 300, none}
    };

    struct Located {
      PlacedItem item;
      const Property *holder;
      std::optional<Property> carried;
      const Property *plot;
    };

    std::optional<Located> locate(Client *client, const Session &state, uint32 propertyId, uint32 recordId) {
      auto holder = state.properties.find(propertyId);
      if (holder == state.properties.end()) {
        LogHousing("[{}] asked about property [{}], which this zone did not send them", client->GetCleanName(), propertyId);
        return std::nullopt;
      }
      auto rows = RealEstateItemsRepository::GetWhere(database, fmt::format("id = {} AND property_id = {}", recordId, propertyId));
      if (rows.size() != 1) {
        LogHousing("[{}] asked about record [{}] of property [{}], which does not exist", client->GetCleanName(), recordId, propertyId);
        return std::nullopt;
      }
      Located located{rows.front(), &holder->second, std::nullopt, nullptr};
      const uint32 carriedId = carriedPropertyId(located.item.custom_data);
      if (carriedId) {
        auto carried = propertiesById({carriedId});
        if (carried.size() != 1) {
          throw std::runtime_error(fmt::format("record [{}] names property [{}], which does not exist", recordId, carriedId));
        }
        located.carried = carried.front();
      }
      located.plot = ancestorOfType(holder->second, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, state.lookup());
      return located;
    }

    bool isHouse(const Property *property) {
      if (!property) {
        return false;
      }
      const DefinitionType type = typeOf(*property);
      return type == DefinitionType::playerHouse || type == DefinitionType::guildHall;
    }

    bool holdsContents(uint32 propertyId) {
      auto rows = query(database, fmt::format(
        "SELECT (SELECT COUNT(*) FROM real_estate_items WHERE property_id = {0}) + (SELECT COUNT(*) FROM real_estate_properties WHERE parent_id = {0})",
        propertyId
      ));
      return Strings::ToUnsignedInt(rows.begin()[0]) > 0;
    }

    // RoF2 finds the yard's placed house itself (0x5b3170) and swaps it for the stored one the menu is for (0x21ba).
    bool yardHasPlacedHouse(const Session &state, const Property &yard, uint32 exceptRecordId) {
      for (const auto &row : itemsOf({yard.id})) {
        auto carried = state.properties.find(carriedPropertyId(row.custom_data));
        if (row.placed && row.id != exceptRecordId && carried != state.properties.end() && isHouse(&carried->second)) {
          return true;
        }
      }
      return false;
    }

    bool maySwap(Client *client, const Session &state, const Located &located, bool plotExpired) {
      const DefinitionType holderType = typeOf(*located.holder);
      if (plotExpired || located.item.placed || !isHouse(located.carried ? &*located.carried : nullptr) ||
          (holderType != DefinitionType::plot && holderType != DefinitionType::guildPlot)) {
        return false;
      }
      const auto lookup = state.lookup();
      const auto listOf = state.accessLookup();
      return permitted(client, Permission::moveItem, *located.holder, lookup, listOf) && permitted(client, Permission::removeHouse, *located.holder, lookup, listOf) &&
        permitted(client, Permission::addHouse, *located.holder, lookup, listOf) && yardHasPlacedHouse(state, *located.holder, located.item.id);
    }

    bool mayEnter(Client *client, const Session &state, const Located &located, bool inItemZone) {
      if (!isHouse(located.carried ? &*located.carried : nullptr) || !located.item.placed || !inItemZone) {
        return false;
      }
      std::map<uint32, Property> known(state.properties);
      known[located.carried->id] = *located.carried;
      auto lookup = [&known](uint32 id) -> const Property * {
        auto found = known.find(id);
        return found == known.end() ? nullptr : &found->second;
      };
      return permitted(client, Permission::enter, *located.carried, lookup, state.accessLookup());
    }

    bool upkeepCurrent(const Located &located) {
      return !(located.plot && located.plot->upkeep_expired_time) && !located.item.upkeep_expired_at && !(located.carried && located.carried->upkeep_expired_time);
    }

    // Adjust Placement and an item's interactions come only to a viewer in the item's own zone (full menu scans on Live).
    bool inItemZoneOf(const Session &state, const PlacedItem &item) {
      auto holder = state.properties.find(item.property_id);
      return holder != state.properties.end() && shownHere(holder->second, state);
    }

    struct Action {
      uint32 propertyId;
      uint32 recordId;
      uint32 b;
    };

    std::optional<Action> readAction(Client *client, WireReader &reader, const char *name) {
      Action action;
      action.propertyId = reader.u32();
      action.recordId = reader.u32();
      action.b = reader.u32();
      if (reader.failed() || reader.remaining()) {
        LogError("Housing: [{}] sent a malformed {}", client->GetCleanName(), name);
        return std::nullopt;
      }
      return action;
    }

    // Live's order (MNU-1): the item's own interactions, then Enter, Adjust Placement, Place, Move to Storage, Move to Inventory,
    // Mail to Owner, Swap Houses, Inspect, Manage, Show Items. Help is left out: RoF2's type 0xe opens a text entry, not Live's help window (MNU-5).
    MenuInteractions buildMenu(Client *client, const Session &state, const Located &located) {
      const bool inItemZone = inItemZoneOf(state, located.item);
      const bool plotExpired = located.plot && located.plot->upkeep_expired_time;
      const Property *carried = located.carried ? &*located.carried : nullptr;
      const MenuSubject subject{state, *located.holder, located.item, carried, inItemZone, plotExpired};
      const auto lookup = state.lookup();
      const auto listOf = state.accessLookup();
      MenuInteractions menu;
      addNpcItemInteractions(client, subject, menu);
      addContainerInteractions(client, subject, menu);
      addGuildServiceInteractions(client, subject, menu);
      const bool crate = carried && typeOf(*carried) == DefinitionType::movingCrate;
      const bool house = isHouse(carried);
      const bool mayMove = !plotExpired && permitted(client, Permission::moveItem, *located.holder, lookup, listOf);
      if (mayEnter(client, state, located, inItemZone) && !plotExpired && upkeepCurrent(located)) {
        menu.entries.push_back(enterEntry);
      }
      if (mayMove && located.item.placed && inItemZone) {
        menu.entries.push_back(adjustPlacementEntry);
      }
      if (mayMove) {
        menu.entries.push_back(placeEntry);
      }
      if (mayMove && !(house && holdsContents(carried->id))) {
        menu.entries.push_back(moveToStorageEntry);
      }
      const bool owner = accessGroup(client, *located.holder, lookup, listOf) == ownerGroup;
      if (!plotExpired && (crate ? permitted(client, Permission::removeCrate, *located.holder, lookup, listOf) : (owner || located.item.owner_character_id == client->CharacterID()))) {
        menu.entries.push_back(moveToInventoryEntry);
      }
      if (located.item.owner_character_id && located.item.owner_character_id != client->CharacterID() && permitted(client, Permission::mailItem, *located.holder, lookup, listOf)) {
        menu.entries.push_back(mailToOwnerEntry);
      }
      // Live's menus never showed Swap Houses (no stored house beside a placed one was scanned); it sits before Inspect (MNU-11).
      if (maySwap(client, state, located, plotExpired)) {
        menu.entries.push_back(swapHousesEntry);
      }
      menu.entries.push_back(inspectEntry);
      if (house && permitted(client, Permission::manage, *carried, lookup, listOf)) {
        menu.entries.push_back(manageEntry);
      }
      menu.entries.push_back(showItemsEntry);
      if (menu.entries.size() > entryCount) {
        throw std::logic_error(fmt::format("the menu of record [{}] has [{}] entries; the client holds [{}]", located.item.id, menu.entries.size(), entryCount));
      }
      return menu;
    }
  }

  void handleMenuRequest(Client *client, WireReader &reader) {
    if (reader.remaining() != menuSize) {
      LogError("Housing: [{}] sent a [{}]-byte menu request; it is [{}] bytes", client->GetCleanName(), reader.remaining(), menuSize);
      return;
    }
    std::string request(menuSize, '\0');
    for (size_t index = 0; index < menuSize; index++) {
      request[index] = static_cast<char>(reader.u8());
    }
    uint32 propertyId;
    uint32 recordId;
    memcpy(&propertyId, request.data() + 0x20, sizeof(propertyId));
    memcpy(&recordId, request.data() + 0x24, sizeof(recordId));
    Session *state = session(client);
    if (!state) {
      return;
    }
    try {
      const auto located = locate(client, *state, propertyId, recordId);
      if (!located) {
        return;
      }
      const EQ::ItemData *item = database.GetItem(located->item.item_id);
      if (!item) {
        throw std::runtime_error(fmt::format("record [{}] holds item [{}], which does not exist", recordId, located->item.item_id));
      }
      const MenuInteractions menu = buildMenu(client, *state, *located);
      std::string reply(request);
      std::string title(titleLength, '\0');
      memcpy(title.data(), item->Name, std::min(strlen(item->Name), titleLength - 1));
      reply.replace(titleOffset, titleLength, title);
      for (size_t index = 0; index < entryCount; index++) {
        const MenuEntry entry = index < menu.entries.size() ? menu.entries[index] : MenuEntry{none, none, none, none};
        const uint32 fields[] = {entry.type, entry.b, entry.label, entry.submenu};
        memcpy(reply.data() + entriesOffset + index * sizeof(fields), fields, sizeof(fields));
      }
      memcpy(reply.data() + npcSpawnOffset, &menu.npcSpawnId, sizeof(menu.npcSpawnId));
      WireWriter message;
      message.bytes(reply.data(), reply.size());
      sendMessage(client, OP_RealEstateInteractionMenu, message);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s menu for record [{}] of property [{}] failed: {}", client->GetCleanName(), recordId, propertyId, error.what());
    }
  }

  void handleInteract(Client *client, WireReader &reader) {
    const uint32 propertyId = reader.u32();
    const uint32 recordId = reader.u32();
    const uint32 b = reader.u32();
    if (reader.failed() || reader.remaining()) {
      LogError("Housing: [{}] sent a malformed interaction", client->GetCleanName());
      return;
    }
    Session *state = session(client);
    if (!state) {
      return;
    }
    try {
      if (recordId == none) {
        auto neighborhood = propertiesById({propertyId});
        if (neighborhood.size() != 1 || typeOf(neighborhood.front()) != DefinitionType::neighborhood || neighborhood.front().parent_id) {
          LogHousing("[{}] asked to go to property [{}], which is not a listed neighborhood", client->GetCleanName(), propertyId);
          return;
        }
        enterProperty(client, neighborhood.front());
        return;
      }
      // b is 0 only on a left click RoF2 found on a model's POINT1-4 quad (0x5384d0), the door of a house's exterior (INT-19).
      const bool doorClick = b == 0;
      if (b != none && !doorClick) {
        LogHousing("[{}] clicked record [{}] of property [{}] with action [{}]; only Enter is handled here", client->GetCleanName(), recordId, propertyId, b);
        return;
      }
      const auto located = locate(client, *state, propertyId, recordId);
      if (!located) {
        return;
      }
      if (!isHouse(located->carried ? &*located->carried : nullptr) || !located->item.placed || !inItemZoneOf(*state, located->item)) {
        LogHousing("[{}] {} record [{}] of property [{}], which is no placed house here; nothing happens", client->GetCleanName(),
          doorClick ? "left clicked" : "chose Enter on", recordId, propertyId);
        return;
      }
      enterProperty(client, *located->carried);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s interaction with record [{}] of property [{}] failed: {}", client->GetCleanName(), recordId, propertyId, error.what());
    }
  }

  void handleInspect(Client *client, WireReader &reader) {
    const uint32 propertyId = reader.u32();
    const uint32 recordId = reader.u32();
    if (reader.failed() || reader.remaining()) {
      LogError("Housing: [{}] sent a malformed inspect request", client->GetCleanName());
      return;
    }
    Session *state = session(client);
    if (!state) {
      return;
    }
    try {
      const auto located = locate(client, *state, propertyId, recordId);
      if (!located) {
        return;
      }
      auto instance = itemInstance(located->item);
      if (!instance) {
        throw std::runtime_error(fmt::format("record [{}] holds item [{}], which does not exist", recordId, located->item.item_id));
      }
      const std::string serialized = RoF2::SerializeItemForHousing(instance.get(), itemGuid(located->item), EQ::invslot::SLOT_INVALID, ItemPacketViewLink);
      WireWriter message;
      message.bytes(serialized.data(), serialized.size());
      sendMessage(client, OP_RealEstateInspect, message);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s inspection of record [{}] of property [{}] failed: {}", client->GetCleanName(), recordId, propertyId, error.what());
    }
  }
  void addNpcItemInteractions(Client *client, const MenuSubject &subject, MenuInteractions &interactions) {
    if (!subject.item.placed || !subject.inItemZone || !isNpcItemId(subject.item.item_id)) {
      return;
    }
    const NpcItem *npcItem = catalog().npcItem(subject.item.item_id);
    if (!npcItem) {
      throw std::runtime_error(fmt::format("NPC item [{}] has no real_estate_npc_items row", subject.item.item_id));
    }
    NPC *spawn = npcItemSpawn(subject.item.id);
    interactions.npcSpawnId = spawn ? spawn->GetID() : 0;
    for (uint32 action : npcItem->actions) {
      interactions.entries.push_back(MenuEntry{9, action, action, none});
    }
    // Live offered the dummy's actions to the owning guild's members and to no visitor (menus.tsv); Transact's gate is the nearest rule known.
    if (npcItem->trainingDummy && mayTransact(client, subject.holder, subject.state.lookup(), subject.state.accessLookup())) {
      interactions.entries.insert(interactions.entries.end(), std::begin(trainingDummyEntries), std::end(trainingDummyEntries));
    }
  }

  // Live offered Use on crafting stations to the owning guild's members and the plot's owner and to no visitor (menus.tsv).
  void addContainerInteractions(Client *client, const MenuSubject &subject, MenuInteractions &interactions) {
    if (!subject.item.placed || !subject.inItemZone || !isTradeskillContainer(subject.item.item_id)) {
      return;
    }
    if (mayTransact(client, subject.holder, subject.state.lookup(), subject.state.accessLookup())) {
      interactions.entries.push_back(useEntry);
    }
  }

  void handleUse(Client *client, WireReader &reader) {
    const auto action = readAction(client, reader, "use");
    Session *state = session(client);
    if (!action || !state) {
      return;
    }
    if (action->b == none) {
      LogHousing("[{}] clicked near record [{}] of property [{}]; a left click does nothing", client->GetCleanName(), action->recordId, action->propertyId);
      return;
    }
    try {
      const auto located = locate(client, *state, action->propertyId, action->recordId);
      if (!located) {
        return;
      }
      if (isAuraAction(action->b) && inItemZoneOf(*state, located->item)) {
        toggleAura(client, *state, *located->holder, located->item, action->b);
        return;
      }
      if (action->b != useEntry.b || !located->item.placed || !inItemZoneOf(*state, located->item) || !isTradeskillContainer(located->item.item_id)) {
        LogHousing("[{}] asked record [{}] of property [{}] for use [{}], which its menu does not offer", client->GetCleanName(), action->recordId, action->propertyId, action->b);
        return;
      }
      if (!mayTransact(client, *located->holder, state->lookup(), state->accessLookup())) {
        sendItemResult(client, ItemResult::noInteractItem);
        return;
      }
      openContainer(client, located->item);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s use of record [{}] of property [{}] failed: {}", client->GetCleanName(), action->recordId, action->propertyId, error.what());
    }
  }

  void handleMenuAction(Client *client, WireReader &reader) {
    const auto action = readAction(client, reader, "menu action");
    Session *state = session(client);
    if (!action || !state) {
      return;
    }
    try {
      const auto located = locate(client, *state, action->propertyId, action->recordId);
      if (!located) {
        return;
      }
      if (!located->item.placed || !inItemZoneOf(*state, located->item) || !isNpcItemId(located->item.item_id)) {
        LogHousing("[{}] asked record [{}] of property [{}] for action [{}], which its menu does not offer", client->GetCleanName(), action->recordId, action->propertyId, action->b);
        return;
      }
      if (isTrainingDummyAction(action->b) && !mayTransact(client, *located->holder, state->lookup(), state->accessLookup())) {
        sendItemResult(client, ItemResult::noInteractItem);
        return;
      }
      performNpcItemAction(client, located->item, action->b);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s action [{}] on record [{}] of property [{}] failed: {}", client->GetCleanName(), action->b, action->recordId, action->propertyId, error.what());
    }
  }

  // Mail to Owner sends a placed item to the character who placed it, as a parcel (EQEmu's item mail).
  void handleMailToOwner(Client *client, WireReader &reader) {
    if (reader.remaining() != mailToOwnerSize) {
      LogError("Housing: [{}] sent a [{}]-byte Mail to Owner; it is [{}] bytes", client->GetCleanName(), reader.remaining(), mailToOwnerSize);
      return;
    }
    const uint32 propertyId = reader.u32();
    const uint32 recordId = reader.u32();
    Session *state = session(client);
    if (!state) {
      return;
    }
    PlacedItem mailed;
    std::string ownerName;
    int parcelSlot = INVALID_INDEX;
    try {
      const auto located = locate(client, *state, propertyId, recordId);
      if (!located) {
        sendItemResult(client, ItemResult::notFound);
        return;
      }
      mailed = located->item;
      ItemResult result = ItemResult::success;
      if (!permitted(client, Permission::mailItem, *located->holder, state->lookup(), state->accessLookup())) {
        result = ItemResult::noMailItem;
      }
      else if (!mailed.owner_character_id || mailed.owner_character_id == client->CharacterID()) {
        result = ItemResult::ownItem;
      }
      else if (located->carried) {
        result = ItemResult::carriesRealEstate;
      }
      if (result != ItemResult::success) {
        LogHousing("Refused [{}]'s Mail to Owner of record [{}]: code [{}]", client->GetCleanName(), recordId, static_cast<uint32>(result));
        sendItemResult(client, result);
        return;
      }
      const auto owners = characterIdentities({mailed.owner_character_id});
      if (!owners.count(mailed.owner_character_id)) {
        throw std::runtime_error(fmt::format("record [{}] was placed by character [{}], which does not exist", recordId, mailed.owner_character_id));
      }
      ownerName = owners.at(mailed.owner_character_id).name;
      const std::string note = propertyName(propertyId);
      Transaction transaction(database);
      lockedProperty(database, propertyId);
      parcelSlot = CharacterParcelsRepository::GetNextFreeParcelSlot(database, mailed.owner_character_id, RuleI(Parcel, ParcelMaxItems));
      if (parcelSlot == INVALID_INDEX) {
        LogHousing("[{}]'s parcels are full; record [{}] stays", ownerName, recordId);
        sendItemResult(client, ItemResult::failed);
        return;
      }
      auto parcel = CharacterParcelsRepository::NewEntity();
      parcel.char_id = mailed.owner_character_id;
      parcel.item_id = mailed.item_id;
      parcel.aug_slot_1 = mailed.augment_one;
      parcel.aug_slot_2 = mailed.augment_two;
      parcel.aug_slot_3 = mailed.augment_three;
      parcel.aug_slot_4 = mailed.augment_four;
      parcel.aug_slot_5 = mailed.augment_five;
      parcel.aug_slot_6 = mailed.augment_six;
      parcel.slot_id = static_cast<uint32>(parcelSlot);
      parcel.quantity = std::max<uint32>(mailed.charges, 1);
      parcel.from_name = client->GetCleanName();
      parcel.note = note;
      parcel.sent_date = std::time(nullptr);
      if (!CharacterParcelsRepository::InsertOne(database, parcel).id) {
        throw std::runtime_error(fmt::format("the parcel of record [{}] to [{}] could not be written", recordId, ownerName));
      }
      query(database, fmt::format("DELETE FROM real_estate_items WHERE id = {}", recordId));
      bumpRevision(database, propertyId);
      transaction.commit();
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s Mail to Owner of record [{}] of property [{}] failed: {}", client->GetCleanName(), recordId, propertyId, error.what());
      sendItemResult(client, ItemResult::failed);
      return;
    }
    withdrawRow(mailed);
    auto pack = std::make_unique<ServerPacket>(ServerOP_ParcelDelivery, static_cast<uint32>(sizeof(Parcel_Struct)));
    auto *delivery = reinterpret_cast<Parcel_Struct *>(pack->pBuffer);
    delivery->item_slot = static_cast<uint32>(parcelSlot);
    strn0cpy(delivery->send_to, ownerName.c_str(), sizeof(delivery->send_to));
    worldserver.SendPacket(pack.get());
    sendItemResult(client, ItemResult::success);
    std::vector<ItemListChangedRecord> withdrawn;
    if (mailed.placed) {
      withdrawn.push_back(ItemListChangedRecord{mailed.id, mailed.property_id, mailed.item_id, 0});
    }
    sendItemListChanged(propertyId, withdrawn);
    LogHousing("[{}] mailed record [{}] (item [{}]) to its owner [{}]", client->GetCleanName(), recordId, mailed.item_id, ownerName);
  }
}
