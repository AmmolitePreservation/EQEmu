#include "housingMenus.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingInteriors.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingPropertyItem.h"
#include "../../common/patches/rof2.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../zonedb.h"

#include <algorithm>
#include <cstring>
#include <optional>

namespace housing {
  namespace {
    constexpr size_t menuSize = 0x300;
    constexpr size_t titleOffset = 0x2c;
    constexpr size_t titleLength = 64;
    constexpr size_t entriesOffset = 0x6c;
    constexpr size_t entryCount = 40;
    constexpr size_t npcSpawnOffset = 0x2fc;
    constexpr uint32 none = 0xFFFFFFFF;
    constexpr uint32 upkeepExpired = 6432;

    constexpr MenuEntry enterEntry{1, none, 723, none};
    constexpr MenuEntry adjustPlacementEntry{2, 0, 5244, none};
    constexpr MenuEntry placeEntry{2, 1, 719, none};
    constexpr MenuEntry moveToStorageEntry{3, none, 718, none};
    constexpr MenuEntry moveToInventoryEntry{4, none, 724, none};
    constexpr MenuEntry mailToOwnerEntry{5, none, 742, none};
    constexpr MenuEntry inspectEntry{7, none, 720, none};
    constexpr MenuEntry manageEntry{0, none, 722, none};
    constexpr MenuEntry showItemsEntry{11, none, 5285, none};

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

    // Live's order (MNU-1): the item's own interactions, then Enter, Adjust Placement, Place, Move to Storage, Move to Inventory,
    // Mail to Owner, Inspect, Manage, Show Items. Help is left out: RoF2's type 0xe opens a text entry, not Live's help window (MNU-5).
    MenuInteractions buildMenu(Client *client, const Session &state, const Located &located) {
      const bool inItemZone = state.hostedIds.count(located.item.property_id) || state.currentPropertyId == located.item.property_id;
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
        auto lists = accessLists({propertyId});
        std::map<uint32, Property> known(state->properties);
        known[propertyId] = neighborhood.front();
        auto lookup = [&known](uint32 id) -> const Property * {
          auto found = known.find(id);
          return found == known.end() ? nullptr : &found->second;
        };
        auto listOf = [&lists](uint32 id) -> const AccessList * {
          auto found = lists.find(id);
          return found == lists.end() ? nullptr : &found->second;
        };
        if (!permitted(client, Permission::enter, neighborhood.front(), lookup, listOf)) {
          LogHousing("[{}] may not enter neighborhood [{}]", client->GetCleanName(), propertyId);
          return;
        }
        enterProperty(client, neighborhood.front());
        return;
      }
      if (b != none) {
        LogHousing("[{}] clicked record [{}] of property [{}] with action [{}]; only the menu's Enter is handled here", client->GetCleanName(), recordId, propertyId, b);
        return;
      }
      const auto located = locate(client, *state, propertyId, recordId);
      if (!located) {
        return;
      }
      const bool inItemZone = state->hostedIds.count(located->item.property_id) || state->currentPropertyId == located->item.property_id;
      if (!mayEnter(client, *state, *located, inItemZone)) {
        LogHousing("[{}] may not enter the house of record [{}] of property [{}]", client->GetCleanName(), recordId, propertyId);
        return;
      }
      if (!upkeepCurrent(*located)) {
        client->MessageString(Chat::Red, upkeepExpired, propertyName(located->carried->id).c_str());
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
}
