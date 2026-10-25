#include "housingPlacement.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingContainers.h"
#include "housingEvents.h"
#include "housingGeometry.h"
#include "housingItemTail.h"
#include "housingNpcItems.h"
#include "housingWorldObjects.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingLedger.h"
#include "../../common/housingPropertyItem.h"
#include "../../common/patches/rof2.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../npc.h"
#include "../zonedb.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>

namespace housing {
  namespace {
    constexpr size_t placeRequestSize = 60;
    constexpr size_t commitSize = 96;
    constexpr size_t storeRequestSize = 48;
    constexpr size_t houseSwapSize = 24;
    constexpr size_t guidLength = 0x14;
    // RoF2's menu Adjust Placement (an interaction menu type 2 entry) names a placed record by its ids alone, with this GUID.
    constexpr const char *unnamedGuid = "0000000000000000";
    constexpr uint32 none = 0xFFFFFFFF;
    constexpr uint32 possessionsType = 0;

    enum class StorageDestination : uint32 { closet = 2, shed = 3, crate = 6 };

    // A request the server turns down with an 0x1228 code.
    struct ItemRefusal : std::runtime_error {
      ItemRefusal(ItemResult result, const std::string &reason) : std::runtime_error(reason), result(result) {}
      ItemResult result;
    };

    // Fresh property and item rows around the properties a request touches; the session's copies for the rest of the tree.
    struct View {
      std::map<uint32, Property> properties;
      std::map<uint32, std::vector<PlacedItem>> rows;
      const Session *state = nullptr;

      PropertyLookup lookup() const {
        return [this](uint32 id) -> const Property * {
          auto found = properties.find(id);
          return found == properties.end() ? nullptr : &found->second;
        };
      }

      AccessListLookup listOf() const {
        return state->accessLookup();
      }

      std::vector<const Property *> childrenOf(uint32 id) const {
        std::vector<const Property *> children;
        for (const auto &[childId, property] : properties) {
          if (property.parent_id == id) {
            children.push_back(&property);
          }
        }
        return children;
      }

      const std::vector<PlacedItem> &rowsOf(uint32 id) const {
        static const std::vector<PlacedItem> empty;
        auto found = rows.find(id);
        return found == rows.end() ? empty : found->second;
      }

      bool holdsContents(uint32 id) const {
        return !rowsOf(id).empty() || !childrenOf(id).empty();
      }

      const Property &at(uint32 id) const {
        auto found = properties.find(id);
        if (found == properties.end()) {
          throw std::runtime_error(fmt::format("property [{}] is not in the request's view", id));
        }
        return found->second;
      }
    };

    View viewOf(const Session &state, const std::set<uint32> &focus) {
      View view;
      view.state = &state;
      view.properties = state.properties;
      std::set<uint32> loaded(focus);
      for (auto &property : propertiesById(focus)) {
        view.properties[property.id] = property;
      }
      std::set<uint32> generation(focus);
      for (int depth = 0; depth < 2 && !generation.empty(); depth++) {
        std::set<uint32> next;
        for (auto &child : propertiesChildOf(generation)) {
          view.properties[child.id] = child;
          next.insert(child.id);
          loaded.insert(child.id);
        }
        generation = next;
      }
      for (uint32 id : focus) {
        if (!view.properties.count(id)) {
          throw std::runtime_error(fmt::format("property [{}] does not exist", id));
        }
      }
      for (auto &row : itemsOf(loaded)) {
        view.rows[row.property_id].push_back(row);
      }
      return view;
    }

    // The item a request is about: an inventory instance, or a row of a property.
    struct Candidate {
      std::unique_ptr<EQ::ItemInstance> instance;
      const EQ::ItemData *item = nullptr;
      const EQ::ItemHousingTail *tail = nullptr;
      std::optional<PlacedItem> row;
      int16 inventorySlot = -1;
      std::optional<Property> carried;
      CarriedKind kind = CarriedKind::none;
      bool containerHoldsItems = false;
      std::string guid;
    };

    CarriedKind kindOf(uint8 definitionType) {
      switch (definitionType) {
        case playerHouseType:
          return CarriedKind::house;
        case guildHallType:
          return CarriedKind::guildHall;
        case movingCrateType:
          return CarriedKind::crate;
        default:
          throw std::runtime_error(fmt::format("an item names real estate of definition type [{}], which no item creates", definitionType));
      }
    }

    void describeCarrier(Candidate &candidate) {
      candidate.tail = itemHousingTail(candidate.item->ID);
      const uint32 carriedId = carriedPropertyId(candidate.instance->GetCustomDataString());
      if (carriedId) {
        auto carried = propertiesById({carriedId});
        if (carried.size() != 1) {
          throw std::runtime_error(fmt::format("item [{}] names property [{}], which does not exist", candidate.item->ID, carriedId));
        }
        candidate.carried = carried.front();
        candidate.kind = kindOf(static_cast<uint8>(typeOf(*candidate.carried)));
      }
      else if (candidate.tail && candidate.tail->realEstateDefinitionId > 0) {
        const Definition *definition = catalog().definition(static_cast<uint32>(candidate.tail->realEstateDefinitionId));
        if (!definition) {
          throw std::runtime_error(fmt::format("item [{}] creates definition [{}], which does not exist", candidate.item->ID, candidate.tail->realEstateDefinitionId));
        }
        candidate.kind = kindOf(definition->row.type);
      }
    }

    Candidate inventoryCandidate(Client *client, const ItemRequest &request) {
      if (request.location.type != possessionsType) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("location type [{}] is not the character's possessions", request.location.type));
      }
      const uint32 slot = RoF2::HousingItemLocationToServerSlot(request.location.type, request.location.slot, request.location.subIndex, request.location.augIndex);
      if (slot == static_cast<uint32>(EQ::invslot::SLOT_INVALID)) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("location [{}, {}, {}] names no slot", request.location.slot, request.location.subIndex, request.location.augIndex));
      }
      const EQ::ItemInstance *held = client->GetInv().GetItem(static_cast<int16>(slot));
      if (!held || fmt::format("{:016}", held->GetSerialNumber()) != request.guid) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("slot [{}] does not hold item [{}]", slot, request.guid));
      }
      Candidate candidate;
      candidate.instance.reset(held->Clone());
      candidate.item = held->GetItem();
      candidate.inventorySlot = static_cast<int16>(slot);
      candidate.containerHoldsItems = held->IsClassBag() && held->GetTotalItemCount() > 1;
      candidate.guid = request.guid;
      describeCarrier(candidate);
      return candidate;
    }

    Candidate rowCandidate(const Session &state, const ItemRequest &request) {
      if (!state.properties.count(request.propertyId)) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("property [{}] is not one this zone sent", request.propertyId));
      }
      auto row = placedItem(request.propertyId, request.recordId);
      if (!row || (request.guid != unnamedGuid && itemGuid(*row) != request.guid)) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("record [{}] of property [{}] is not item [{}]", request.recordId, request.propertyId, request.guid));
      }
      Candidate candidate;
      candidate.instance = itemInstance(*row);
      if (!candidate.instance) {
        throw std::runtime_error(fmt::format("record [{}] holds item [{}], which does not exist", row->id, row->item_id));
      }
      candidate.item = candidate.instance->GetItem();
      candidate.row = row;
      candidate.guid = itemGuid(*row);
      describeCarrier(candidate);
      return candidate;
    }

    Candidate candidateFor(Client *client, const Session &state, const ItemRequest &request) {
      return request.propertyId == none ? inventoryCandidate(client, request) : rowCandidate(state, request);
    }

    bool isAncestor(uint32 ancestorId, const Property &property, const PropertyLookup &lookup) {
      const Property *current = property.parent_id ? lookup(property.parent_id) : nullptr;
      for (int depth = 0; current && depth <= 10; depth++) {
        if (current->id == ancestorId) {
          return true;
        }
        current = current->parent_id ? lookup(current->parent_id) : nullptr;
      }
      return false;
    }

    const Property *placedCrateIn(const View &view, uint32 propertyId, uint32 excludedId) {
      for (const Property *child : view.childrenOf(propertyId)) {
        if (child->id == excludedId || typeOf(*child) != DefinitionType::movingCrate) {
          continue;
        }
        for (const auto &row : view.rowsOf(propertyId)) {
          if (row.placed && carriedPropertyId(row.custom_data) == child->id) {
            return child;
          }
        }
      }
      return nullptr;
    }

    const PlacedItem *mergeTarget(const View &view, const Candidate &candidate, uint32 destinationId, uint32 ownerId, uint32 quantity) {
      if (!candidate.instance->IsStackable()) {
        return nullptr;
      }
      for (const auto &row : view.rowsOf(destinationId)) {
        if (row.placed || row.item_id != candidate.item->ID || row.owner_character_id != ownerId || (candidate.row && row.id == candidate.row->id)) {
          continue;
        }
        if (row.charges + quantity <= static_cast<uint32>(candidate.item->StackSize)) {
          return &row;
        }
      }
      return nullptr;
    }

    uint32 movingQuantity(const Candidate &candidate, const ItemRequest &request) {
      const int16 charges = candidate.instance->GetCharges();
      if (!candidate.instance->IsStackable() || request.quantity == 0) {
        return candidate.instance->IsStackable() ? static_cast<uint32>(std::max<int16>(charges, 1)) : 1;
      }
      if (request.quantity > static_cast<uint32>(charges)) {
        throw ItemRefusal(ItemResult::failed, fmt::format("a stack of [{}] cannot give [{}]", charges, request.quantity));
      }
      return request.quantity;
    }

    PlacementFacts factsFor(Client *client, const View &view, const Candidate &candidate, const Property &destination, bool intoStorage, uint32 quantity) {
      const auto lookup = view.lookup();
      const auto listOf = view.listOf();
      const DefinitionType destinationType = typeOf(destination);
      const Definition *definition = catalog().definition(destination.definition_id);
      if (!definition) {
        throw std::runtime_error(fmt::format("property [{}] names unknown definition [{}]", destination.id, destination.definition_id));
      }
      const uint32 expansions = client->GetPP().expansions;
      const uint32 carriedId = candidate.carried ? candidate.carried->id : 0;
      PlacementFacts facts;
      facts.source = !candidate.row ? PlacementSource::inventory
        : candidate.row->property_id == destination.id ? PlacementSource::sameProperty : PlacementSource::otherProperty;
      facts.houseOfThule = expansions & houseOfThule;
      facts.veilOfAlaris = expansions & veilOfAlaris;
      facts.underGuildPlot = ancestorOfType(destination, typeBit(DefinitionType::guildPlot), false, lookup) != nullptr;
      const Property *plot = ancestorOfType(destination, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, lookup);
      facts.plotUpkeepExpired = plot && plot->upkeep_expired_time;
      facts.placeable = candidate.tail && candidate.tail->placeable;
      facts.intoOwnProperty = carriedId && (destination.id == carriedId || isAncestor(carriedId, destination, lookup));
      // A zone root holds plots only: nothing is placed outside a plot there, the Regular Zone items included (Q4).
      facts.allowedHere = destinationType == DefinitionType::movingCrate ? facts.placeable
        : destinationType != DefinitionType::regularZone && facts.placeable && (candidate.tail->placeable & typeBit(destinationType));
      facts.carried = candidate.kind;
      facts.carriedHoldsItems = carriedId && view.holdsContents(carriedId);
      facts.carriedHouseHoldsPlacedCrate = carriedId && isHouseKind(candidate.kind) && placedCrateIn(view, carriedId, 0);
      facts.intoStorage = intoStorage;
      facts.destinationIsCrate = destinationType == DefinitionType::movingCrate;
      for (const Property *child : view.childrenOf(destination.id)) {
        const DefinitionType childType = typeOf(*child);
        if (child->id == carriedId || (childType != DefinitionType::playerHouse && childType != DefinitionType::guildHall)) {
          continue;
        }
        facts.destinationHouses++;
        if (view.holdsContents(child->id)) {
          facts.crateHoldsNonEmptyHouse = facts.destinationIsCrate;
        }
      }
      facts.maxHouses = definition->maxChildren;
      facts.destinationHasPlacedCrate = placedCrateIn(view, destination.id, carriedId) != nullptr;
      for (const auto &row : view.rowsOf(destination.id)) {
        if (candidate.row && row.id == candidate.row->id) {
          continue;
        }
        const EQ::ItemHousingTail *tail = itemHousingTail(row.item_id);
        if (tail && tail->placeable) {
          facts.realEstateCount++;
        }
        else {
          facts.vaultCount++;
        }
        if (isNpcItemId(row.item_id)) {
          facts.npcCount++;
        }
        if (row.item_id == candidate.item->ID) {
          facts.sameItemCount++;
        }
      }
      facts.maxItems = definition->row.max_items;
      facts.maxNonRealEstateItems = definition->row.max_non_real_estate_items;
      facts.npcItem = isNpcItemId(candidate.item->ID);
      facts.npcCap = definition->maxNpcItems;
      facts.mergesIntoStack = intoStorage && mergeTarget(view, candidate, destination.id, client->CharacterID(), quantity);
      facts.containerHoldsItems = candidate.containerHoldsItems;
      const bool destinationCarriesUpkeep = destinationType == DefinitionType::playerHouse || destinationType == DefinitionType::guildHall || destinationType == DefinitionType::movingCrate;
      facts.itemUpkeepExpired = (candidate.row && candidate.row->upkeep_expired_at) || (candidate.carried && candidate.carried->upkeep_expired_time) ||
        (destinationCarriesUpkeep && destination.upkeep_expired_time);
      const Permission add = isHouseKind(candidate.kind) ? Permission::addHouse : candidate.kind == CarriedKind::crate ? Permission::addCrate : Permission::addItem;
      facts.mayAdd = permitted(client, add, destination, lookup, listOf);
      if (candidate.row) {
        const Property &holder = view.at(candidate.row->property_id);
        facts.mayMove = permitted(client, Permission::moveItem, holder, lookup, listOf);
        facts.mayRemoveHouse = permitted(client, Permission::removeHouse, holder, lookup, listOf);
      }
      facts.maxPerRealEstate = candidate.tail ? candidate.tail->maxPerRealEstate : -1;
      return facts;
    }

    ItemResult judge(Client *client, const View &view, const Candidate &candidate, const Property &destination, bool intoStorage, uint32 quantity) {
      // An NPC item the catalog has no usable row for spawns nothing, so it stays in inventory or storage (the catalog logged it at load).
      if (!intoStorage && isNpcItemId(candidate.item->ID) && !catalog().npcItem(candidate.item->ID)) {
        LogHousing("[{}] tried to place NPC item [{}], which has no usable real_estate_npc_items row", client->GetCleanName(), candidate.item->ID);
        return ItemResult::notPlaceable;
      }
      return placementResult(factsFor(client, view, candidate, destination, intoStorage, quantity));
    }

    // The plot whose region holds the point, else the current property (the client's fallback, 0x4b2bf0, 0x4b2f40).
    uint32 propertyAt(const Session &state, float x, float y, float z) {
      for (uint32 id : state.hostedIds) {
        const Property &property = state.properties.at(id);
        const Definition *definition = catalog().definition(property.definition_id);
        if (!definition || !definition->row.area_id) {
          continue;
        }
        const auto *region = catalog().region(definition->row.area_id);
        if (region && regionContains(RegionBox{region->x, region->y, region->z, region->size_x, region->size_y, region->size_z, region->heading}, x, y, z)) {
          return id;
        }
      }
      return state.currentPropertyId;
    }

    std::vector<std::pair<uint32, uint32>> regionPlots(const Session &state) {
      std::vector<std::pair<uint32, uint32>> plots;
      for (uint32 id : state.hostedIds) {
        const Definition *definition = catalog().definition(state.properties.at(id).definition_id);
        if (definition && definition->row.area_id) {
          plots.emplace_back(definition->row.area_id, id);
        }
      }
      return plots;
    }

    // The owner of the plot the property stands on; a guild plot's houses have none (Live: halls carry no owner).
    uint32 ownerFor(const Property &destination, const PropertyLookup &lookup) {
      const Property *plot = ancestorOfType(destination, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, lookup);
      if (plot) {
        return typeOf(*plot) == DefinitionType::plot ? plot->owner_character_id : 0;
      }
      return destination.owner_character_id;
    }

    // Puts the real estate an item carries (or creates the real estate a house or crate item makes) under the destination.
    void attachCarried(Candidate &candidate, PlacedItem &row, const Property &destination, const PropertyLookup &lookup, std::set<uint32> &changed) {
      if (candidate.kind == CarriedKind::none) {
        return;
      }
      const uint32 owner = ownerFor(destination, lookup);
      if (candidate.carried) {
        query(database, fmt::format(
          "UPDATE real_estate_properties SET parent_id = {}, owner_character_id = {}, contained_in_item = 0, archived = 0, revision = revision + 1 WHERE id = {}",
          destination.id, owner, candidate.carried->id
        ));
        changed.insert(candidate.carried->id);
        return;
      }
      auto created = query(database, fmt::format(
        "INSERT INTO real_estate_properties (parent_id, definition_id, owner_character_id) VALUES ({}, {}, {})",
        destination.id, candidate.tail->realEstateDefinitionId, owner
      ));
      const uint32 propertyId = static_cast<uint32>(created.LastInsertedID());
      candidate.instance->SetCustomData(propertyItemKey, std::to_string(propertyId));
      row.custom_data = candidate.instance->GetCustomDataString();
      // A property-carrying item is No Trade per instance for as long as it carries one (STT-6).
      row.instnodrop = 1;
      changed.insert(propertyId);
    }

    // An update in place keeps the row's id, so its GUID and any look stored for it survive (NPC-19).
    void updateRow(const PlacedItem &row) {
      query(database, fmt::format(
        "UPDATE real_estate_items SET property_id = {}, placed = {}, heading = {}, pitch = {}, roll = {}, scale = {}, x = {}, y = {}, z = {}, "
        "charges = {}, instnodrop = {}, custom_data = '{}' WHERE id = {}",
        row.property_id, row.placed, row.heading, row.pitch, row.roll, row.scale, row.x, row.y, row.z, row.charges, row.instnodrop,
        Strings::Escape(row.custom_data), row.id
      ));
    }

    void sendRemoved(const PlacedItem &row) {
      forEachSession([&](Client *client, Session &state) {
        if (!state.properties.count(row.property_id)) {
          return;
        }
        WireWriter message;
        message.u32(row.property_id).u32(row.id);
        sendMessage(client, OP_RealEstateItemRemove, message);
        if (state.placement && state.placement->sourcePropertyId == row.property_id && state.placement->sourceRecordId == row.id) {
          state.placement.reset();
        }
      });
    }

    void refuse(Client *client, const ItemRefusal &refusal, const char *request) {
      LogHousing("Refused [{}]'s {}: {} (code [{}])", client->GetCleanName(), request, refusal.what(), static_cast<uint32>(refusal.result));
      sendItemResult(client, refusal.result);
    }

    struct LayoutRecord {
      bool place = false;
      ItemRequest request;
      ObjectTransform transform{};
      uint32 destinationId = 0;
    };

    // What one or more applied placements and moves changed, for what follows their transactions.
    struct Applied {
      std::set<uint32> itemLists;
      std::map<uint32, PropertyChangedPayload> changedProperties;
      std::vector<PlacedItem> withdrawn;
      std::vector<PlacedItem> shown;
      uint32 count = 0;
    };

    // A commit's rules and changes, the client's request latch aside.
    PlacedItem commitPlacement(Client *client, const Session &state, Candidate &candidate, uint32 destinationId, const ObjectTransform &transform,
                               const MountTilts &tilts, uint32 quantity, Applied &applied) {
      if (std::isnan(transform.x) || std::isnan(transform.y) || std::isnan(transform.z)) {
        throw ItemRefusal(ItemResult::failed, "the commit's position is not a number");
      }
      if (!scaleAllowed(transform.scale, candidate.tail->placeableScaleMin, candidate.tail->placeableScaleMax)) {
        throw ItemRefusal(ItemResult::failed, fmt::format("scale [{}] is outside [{}, {}]", transform.scale, candidate.tail->placeableScaleMin, candidate.tail->placeableScaleMax));
      }
      if (candidate.tail->disablePlacementRotation && !tiltsKept(transform.pitch, transform.roll, tilts)) {
        throw ItemRefusal(ItemResult::failed, fmt::format("pitch [{}] and roll [{}] left [{}, {}] while rotation is disabled", transform.pitch, transform.roll, tilts.pitch, tilts.roll));
      }
      if (!destinationId) {
        throw ItemRefusal(ItemResult::notPlaceableZone, "no property holds the position");
      }
      std::set<uint32> focus{destinationId};
      if (candidate.row) {
        focus.insert(candidate.row->property_id);
      }
      if (candidate.carried) {
        focus.insert(candidate.carried->id);
      }
      PlacedItem placed;
      {
        Transaction transaction(database);
        const Property destination = lockedProperty(database, destinationId);
        View view = viewOf(state, focus);
        view.properties[destinationId] = destination;
        const ItemResult result = judge(client, view, candidate, destination, false, quantity);
        if (result != ItemResult::success) {
          throw ItemRefusal(result, fmt::format("the item may not be placed in property [{}]", destinationId));
        }
        placed = candidate.row ? *candidate.row : rowFromInstance(*candidate.instance, destinationId, client->CharacterID());
        placed.property_id = destinationId;
        placed.placed = 1;
        placed.heading = transform.heading;
        placed.pitch = transform.pitch;
        placed.roll = transform.roll;
        placed.scale = transform.scale;
        placed.x = transform.x;
        placed.y = transform.y;
        placed.z = transform.z;
        if (!candidate.row || candidate.row->property_id != destinationId) {
          std::set<uint32> carriedChanged;
          attachCarried(candidate, placed, destination, view.lookup(), carriedChanged);
          for (uint32 id : carriedChanged) {
            const uint32 previousOwner = candidate.carried ? candidate.carried->owner_character_id : 0;
            applied.changedProperties[id] = PropertyChangedPayload{previousOwner, ownerFor(destination, view.lookup()), 0, 0};
          }
          applied.changedProperties[destinationId] = PropertyChangedPayload{destination.owner_character_id, destination.owner_character_id, destination.guild_id, destination.guild_id};
        }
        if (candidate.row) {
          updateRow(placed);
          bumpRevision(database, candidate.row->property_id);
          applied.itemLists.insert(candidate.row->property_id);
        }
        else {
          placed.charges = static_cast<uint16>(candidate.instance->IsStackable() ? quantity : placed.charges);
          placed = RealEstateItemsRepository::InsertOne(database, placed);
          if (!placed.id) {
            throw std::runtime_error(fmt::format("item [{}] could not be added to property [{}]", candidate.item->ID, destinationId));
          }
        }
        bumpRevision(database, destinationId);
        applied.itemLists.insert(destinationId);
        transaction.commit();
      }
      if (!candidate.row) {
        client->DeleteItemInInventory(candidate.inventorySlot, candidate.instance->IsStackable() ? static_cast<int16>(quantity) : 0, true);
      }
      applied.shown.push_back(placed);
      applied.count++;
      return placed;
    }

    // A move into a property's storage, its rules and changes; the destination is the resolved closet, shed or crate.
    PlacedItem storeInto(Client *client, const Session &state, Candidate &candidate, std::set<uint32> focus, uint32 destinationId, uint32 quantity, Applied &applied) {
      const bool wholeSource = !candidate.instance->IsStackable() || quantity >= static_cast<uint32>(std::max<int16>(candidate.instance->GetCharges(), 1));
      PlacedItem stored;
      {
        Transaction transaction(database);
        const Property destination = lockedProperty(database, destinationId);
        focus.insert(destinationId);
        View fresh = viewOf(state, focus);
        fresh.properties[destination.id] = destination;
        const ItemResult result = judge(client, fresh, candidate, destination, true, quantity);
        if (result != ItemResult::success) {
          throw ItemRefusal(result, fmt::format("the item may not be stored in property [{}]", destination.id));
        }
        std::map<uint32, PropertyChangedPayload> changedProperties;
        const PlacedItem *merge = mergeTarget(fresh, candidate, destination.id, client->CharacterID(), quantity);
        if (merge) {
          stored = *merge;
          query(database, fmt::format("UPDATE real_estate_items SET charges = charges + {} WHERE id = {}", quantity, merge->id));
          stored.charges = static_cast<uint16>(stored.charges + quantity);
        }
        else if (candidate.row && wholeSource) {
          stored = *candidate.row;
          stored.property_id = destination.id;
          stored.placed = 0;
          if (candidate.row->property_id != destination.id) {
            std::set<uint32> carriedChanged;
            attachCarried(candidate, stored, destination, fresh.lookup(), carriedChanged);
            for (uint32 id : carriedChanged) {
              changedProperties[id] = PropertyChangedPayload{candidate.carried ? candidate.carried->owner_character_id : 0, ownerFor(destination, fresh.lookup()), 0, 0};
            }
          }
          updateRow(stored);
        }
        else {
          // REC-11: an item stored without ever being placed has Live's all-zero transform.
          stored = rowFromInstance(*candidate.instance, destination.id, candidate.row ? candidate.row->owner_character_id : client->CharacterID());
          stored.charges = static_cast<uint16>(candidate.instance->IsStackable() ? quantity : stored.charges);
          if (!candidate.row) {
            std::set<uint32> carriedChanged;
            attachCarried(candidate, stored, destination, fresh.lookup(), carriedChanged);
            for (uint32 id : carriedChanged) {
              changedProperties[id] = PropertyChangedPayload{candidate.carried ? candidate.carried->owner_character_id : 0, ownerFor(destination, fresh.lookup()), 0, 0};
            }
          }
          stored = RealEstateItemsRepository::InsertOne(database, stored);
          if (!stored.id) {
            throw std::runtime_error(fmt::format("item [{}] could not be stored in property [{}]", candidate.item->ID, destination.id));
          }
        }
        if (candidate.row) {
          if (merge || !wholeSource) {
            if (wholeSource) {
              query(database, fmt::format("DELETE FROM real_estate_items WHERE id = {}", candidate.row->id));
            }
            else {
              query(database, fmt::format("UPDATE real_estate_items SET charges = charges - {} WHERE id = {}", quantity, candidate.row->id));
            }
          }
          if (wholeSource && candidate.row->placed) {
            applied.withdrawn.push_back(*candidate.row);
            if (candidate.carried && isHouseKind(candidate.kind)) {
              const Property &house = *candidate.carried;
              changedProperties.emplace(house.id, PropertyChangedPayload{house.owner_character_id, house.owner_character_id, house.guild_id, house.guild_id});
            }
          }
          bumpRevision(database, candidate.row->property_id);
          applied.itemLists.insert(candidate.row->property_id);
        }
        if (!changedProperties.empty()) {
          changedProperties[destination.id] = PropertyChangedPayload{destination.owner_character_id, destination.owner_character_id, destination.guild_id, destination.guild_id};
        }
        for (const auto &[id, payload] : changedProperties) {
          applied.changedProperties.emplace(id, payload);
        }
        bumpRevision(database, destination.id);
        applied.itemLists.insert(destination.id);
        transaction.commit();
      }
      if (!candidate.row) {
        client->DeleteItemInInventory(candidate.inventorySlot, candidate.instance->IsStackable() ? static_cast<int16>(quantity) : 0, true);
      }
      applied.count++;
      return stored;
    }

    // A placed row left its place here: its object or spawn goes, its container closes, editors moving it stop.
    void hideRow(const PlacedItem &row, const char *request) {
      closeContainersOf(row.id);
      if (isNpcItemId(row.item_id)) {
        despawnNpcItem(row.id);
      }
      else {
        broadcastWorldObjectRemove(row);
      }
      forEachSession([&](Client *editor, Session &editorState) {
        if (editorState.placement && editorState.placement->sourceRecordId == row.id && editorState.placement->sourcePropertyId == row.property_id) {
          cancelPlacement(editor, fmt::format("record [{}] was moved by {}", row.id, request));
        }
      });
    }

    void showRow(const PlacedItem &row) {
      closeContainersOf(row.id);
      if (isNpcItemId(row.item_id)) {
        spawnNpcItem(row);
      }
      else {
        broadcastWorldObject(row);
      }
    }

    // The events: the owners each changed property had and has, and per item list the records withdrawn from it and placed in it.
    void announce(const Applied &applied) {
      for (const auto &[propertyId, owners] : applied.changedProperties) {
        sendPropertyChanged(propertyId, owners);
      }
      std::map<uint32, std::vector<ItemListChangedRecord>> changes;
      for (const PlacedItem &row : applied.withdrawn) {
        changes[row.property_id].push_back(ItemListChangedRecord{row.id, row.property_id, row.item_id, 0});
      }
      for (const PlacedItem &row : applied.shown) {
        changes[row.property_id].push_back(ItemListChangedRecord{row.id, row.property_id, row.item_id, 1});
      }
      std::set<uint32> lists = applied.itemLists;
      for (const auto &[propertyId, records] : changes) {
        lists.insert(propertyId);
      }
      for (uint32 propertyId : lists) {
        sendItemListChanged(propertyId, changes[propertyId]);
      }
    }

    void finishApplied(const Applied &applied, const char *request) {
      const std::set<uint32> shown = propertiesShownInZone();
      for (const PlacedItem &row : applied.withdrawn) {
        if (shown.count(row.property_id)) {
          hideRow(row, request);
        }
      }
      for (const PlacedItem &row : applied.shown) {
        if (row.placed && shown.count(row.property_id)) {
          showRow(row);
        }
      }
      announce(applied);
    }

    // Store All, Pack and Unpack: every row of the source that passes the individual move rules into the destination moves,
    // like items stacking into one stored entry, and nothing is said (Live, INT-9).
    Candidate bulkCandidate(const PlacedItem &row) {
      Candidate candidate;
      candidate.instance = itemInstance(row);
      if (!candidate.instance) {
        throw std::runtime_error(fmt::format("record [{}] holds item [{}], which does not exist", row.id, row.item_id));
      }
      candidate.item = candidate.instance->GetItem();
      candidate.row = row;
      candidate.guid = itemGuid(row);
      describeCarrier(candidate);
      return candidate;
    }

    Applied moveRows(Client *client, const Session &state, uint32 sourceId, uint32 destinationId, bool placedOnly) {
      Applied result;
      Transaction transaction(database);
      lockedProperty(database, sourceId);
      const Property destination = lockedProperty(database, destinationId);
      View view = viewOf(state, {sourceId, destinationId});
      view.properties[destination.id] = destination;
      const std::vector<PlacedItem> rows = view.rowsOf(sourceId);
      for (const PlacedItem &row : rows) {
        if ((placedOnly && !row.placed) || carriedPropertyId(row.custom_data) == destinationId) {
          continue;
        }
        Candidate candidate = bulkCandidate(row);
        if (isHouseKind(candidate.kind)) {
          continue;
        }
        const uint32 quantity = candidate.instance->IsStackable() ? static_cast<uint32>(std::max<int16>(candidate.instance->GetCharges(), 1)) : 1;
        if (judge(client, view, candidate, destination, true, quantity) != ItemResult::success) {
          continue;
        }
        auto &sourceRows = view.rows[sourceId];
        sourceRows.erase(std::remove_if(sourceRows.begin(), sourceRows.end(), [&](const PlacedItem &entry) { return entry.id == row.id; }), sourceRows.end());
        const PlacedItem *merge = mergeTarget(view, candidate, destinationId, row.owner_character_id, quantity);
        if (merge) {
          const uint32 mergeId = merge->id;
          query(database, fmt::format("UPDATE real_estate_items SET charges = charges + {} WHERE id = {}", quantity, mergeId));
          query(database, fmt::format("DELETE FROM real_estate_items WHERE id = {}", row.id));
          for (auto &entry : view.rows[destinationId]) {
            if (entry.id == mergeId) {
              entry.charges = static_cast<uint16>(entry.charges + quantity);
            }
          }
        }
        else {
          PlacedItem stored = row;
          stored.property_id = destinationId;
          stored.placed = 0;
          if (sourceId != destinationId) {
            std::set<uint32> carriedChanged;
            attachCarried(candidate, stored, destination, view.lookup(), carriedChanged);
            for (uint32 id : carriedChanged) {
              result.changedProperties[id] = PropertyChangedPayload{candidate.carried->owner_character_id, ownerFor(destination, view.lookup()), 0, 0};
            }
          }
          updateRow(stored);
          view.rows[destinationId].push_back(stored);
        }
        if (row.placed) {
          result.withdrawn.push_back(row);
        }
        result.count++;
      }
      if (result.count) {
        bumpRevision(database, sourceId);
        if (destinationId != sourceId) {
          bumpRevision(database, destinationId);
        }
        result.itemLists = {sourceId, destinationId};
        if (!result.changedProperties.empty()) {
          result.changedProperties[destination.id] = PropertyChangedPayload{destination.owner_character_id, destination.owner_character_id, destination.guild_id, destination.guild_id};
        }
      }
      transaction.commit();
      return result;
    }

    void finishBulkMove(Client *client, const Applied &result, const char *request) {
      sendItemResult(client, result.count ? ItemResult::success : ItemResult::nothingToTransfer);
      finishApplied(result, request);
      LogHousing("[{}]'s {} moved [{}] records", client->GetCleanName(), request, result.count);
    }

    // The yard the player stands in, or the house or hall they are inside: Store All's and the crate's "current location".
    uint32 locationOf(Client *client, const Session &state) {
      const uint32 location = propertyAt(state, client->GetX(), client->GetY(), client->GetZ());
      if (!location) {
        return 0;
      }
      const DefinitionType type = typeOf(state.properties.at(location));
      const bool yardOrHouse = type == DefinitionType::plot || type == DefinitionType::guildPlot || type == DefinitionType::playerHouse || type == DefinitionType::guildHall;
      return yardOrHouse ? location : 0;
    }
  }

  bool readItemRequest(WireReader &reader, ItemRequest &request) {
    request.guid = reader.fixedString(guidLength);
    request.location.type = reader.u32();
    request.location.slot = static_cast<int16>(reader.u16());
    request.location.subIndex = static_cast<int16>(reader.u16());
    request.location.augIndex = static_cast<int16>(reader.u16());
    reader.u16();
    request.propertyId = reader.u32();
    request.recordId = reader.u32();
    request.quantity = reader.u32();
    return !reader.failed();
  }

  uint32 standingProperty(Client *client, const Session &state) {
    return propertyAt(state, client->GetX(), client->GetY(), client->GetZ());
  }

  void sendItemResult(Client *client, ItemResult result) {
    WireWriter message;
    message.u32(static_cast<uint32>(result));
    sendMessage(client, OP_RealEstateItemResult, message);
  }

  void cancelPlacement(Client *client, const std::string &reason) {
    Session *state = session(client);
    if (state) {
      state->placement.reset();
    }
    sendMessage(client, OP_RealEstatePlaceCancel, WireWriter{});
    LogHousing("Cancelled [{}]'s placement: {}", client->GetCleanName(), reason);
  }

  void withdrawRow(const PlacedItem &row) {
    closeContainersOf(row.id);
    if (row.placed) {
      if (isNpcItemId(row.item_id)) {
        despawnNpcItem(row.id);
      }
      else {
        broadcastWorldObjectRemove(row);
      }
    }
    sendRemoved(row);
  }

  void applyItemListChange(const std::vector<ItemListChangedRecord> &records) {
    const std::set<uint32> shown = propertiesShownInZone();
    for (const auto &record : records) {
      if (!shown.count(record.propertyId)) {
        continue;
      }
      if (!record.placed) {
        PlacedItem row{};
        row.id = record.recordId;
        row.property_id = record.propertyId;
        row.item_id = record.itemId;
        row.placed = 1;
        hideRow(row, "another zone");
        continue;
      }
      auto row = placedItem(record.propertyId, record.recordId);
      if (!row || !row->placed) {
        LogHousing("Record [{}] of property [{}] was placed from another zone but is not placed there now; a later event carries it", record.recordId, record.propertyId);
        continue;
      }
      showRow(*row);
    }
  }

  void handlePlaceRequest(Client *client, WireReader &reader) {
    if (reader.remaining() != placeRequestSize) {
      LogError("Housing: [{}] sent a [{}]-byte place request; it is [{}] bytes", client->GetCleanName(), reader.remaining(), placeRequestSize);
      return;
    }
    ItemRequest request;
    readItemRequest(reader, request);
    reader.u32();
    reader.u8();
    reader.skip(3);
    const uint32 clickX = reader.u32();
    const uint32 clickY = reader.u32();
    Session *state = session(client);
    if (!state) {
      return;
    }
    try {
      if (!state->currentPropertyId) {
        throw ItemRefusal(ItemResult::notPlaceableZone, "this zone has no current property");
      }
      Candidate candidate = candidateFor(client, *state, request);
      const auto plots = regionPlots(*state);
      std::set<uint32> focus{state->currentPropertyId};
      for (const auto &[region, plotId] : plots) {
        focus.insert(plotId);
      }
      if (candidate.row) {
        focus.insert(candidate.row->property_id);
      }
      if (candidate.carried) {
        focus.insert(candidate.carried->id);
      }
      const View view = viewOf(*state, focus);
      const uint32 quantity = movingQuantity(candidate, request);
      const uint32 standing = propertyAt(*state, client->GetX(), client->GetY(), client->GetZ());
      const ItemResult standingResult = judge(client, view, candidate, view.at(standing), false, quantity);
      if (standingResult != ItemResult::success) {
        throw ItemRefusal(standingResult, fmt::format("the item may not be placed in property [{}]", standing));
      }

      const bool editing = candidate.row && candidate.row->placed;
      const auto tail = candidate.tail;
      const MountTilts tilts = mountTilts(*candidate.instance);
      ObjectTransform transform = editing ? rowTransform(*candidate.row)
        : ObjectTransform{tail->placeableDefaultScale, tail->placeableDefaultHeading, tilts.pitch, tilts.roll, client->GetX(), client->GetY(), client->GetZ()};
      const uint32 propertyId = candidate.row ? candidate.row->property_id : none;
      const uint32 recordId = candidate.row ? candidate.row->id : none;
      NPC *spawn = editing && isNpcItemId(candidate.item->ID) ? npcItemSpawn(candidate.row->id) : nullptr;

      WireWriter message;
      writePlacementObject(message, placementObject(*candidate.instance, candidate.guid, propertyId, recordId, transform));
      message.u8(static_cast<uint8>(request.location.type))
        .u16(static_cast<uint16>(request.location.slot))
        .u16(static_cast<uint16>(request.location.subIndex))
        .u16(static_cast<uint16>(request.location.augIndex))
        .u8(editing)
        .u8(1)
        .u8(0)
        .u32(clickX)
        .u32(clickY)
        .u32(spawn ? spawn->GetID() : 0)
        .u32(static_cast<uint32>(plots.size()));
      for (const auto &[region, plotId] : plots) {
        message.u32(region).u32(static_cast<uint32>(judge(client, view, candidate, view.at(plotId), false, quantity)));
      }
      message.u32(state->currentPropertyId).u32(static_cast<uint32>(judge(client, view, candidate, view.at(state->currentPropertyId), false, quantity)));

      Placement placement;
      placement.editing = editing;
      placement.location = request.location;
      placement.inventorySlot = candidate.inventorySlot;
      placement.sourcePropertyId = candidate.row ? candidate.row->property_id : 0;
      placement.sourceRecordId = candidate.row ? candidate.row->id : 0;
      placement.guid = candidate.guid;
      placement.contextPropertyId = state->currentPropertyId;
      placement.tilts = tilts;
      placement.awaitingCommit = true;
      state->placement = placement;
      sendMessage(client, OP_RealEstatePlace, message);
      LogHousing("[{}] began placing item [{}] ([{}], record [{}] of property [{}])", client->GetCleanName(), candidate.item->ID, candidate.guid, recordId, propertyId);
    }
    catch (const ItemRefusal &refusal) {
      refuse(client, refusal, "place request");
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s place request for [{}] failed: {}", client->GetCleanName(), request.guid, error.what());
      sendItemResult(client, ItemResult::failed);
    }
  }

  void handlePlaceCommit(Client *client, WireReader &reader) {
    if (reader.remaining() != commitSize) {
      LogError("Housing: [{}] sent a [{}]-byte placement commit; it is [{}] bytes", client->GetCleanName(), reader.remaining(), commitSize);
      return;
    }
    ItemRequest request;
    readItemRequest(reader, request);
    // The sender (0x5b1f90) pushes the placement object's +0x80, +0x84, +0x88, +0x7c: scale comes after the rotation here,
    // unlike the object's own order in 0x626e and 0x1045.
    ObjectTransform transform;
    transform.heading = reader.f32();
    transform.pitch = reader.f32();
    transform.roll = reader.f32();
    transform.scale = reader.f32();
    transform.y = reader.f32();
    transform.x = reader.f32();
    transform.z = reader.f32();
    Session *state = session(client);
    if (!state) {
      return;
    }
    Applied applied;
    PlacedItem placed;
    try {
      if (!state->placement || !state->placement->awaitingCommit || state->placement->guid != request.guid) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("no placement of [{}] is in progress", request.guid));
      }
      const Placement placement = *state->placement;
      ItemRequest source = request;
      source.location = placement.location;
      source.propertyId = placement.sourcePropertyId ? placement.sourcePropertyId : none;
      source.recordId = placement.sourcePropertyId ? placement.sourceRecordId : none;
      Candidate candidate = candidateFor(client, *state, source);
      // The server has no model data: it judges the property holding the item's position, not the client's 8-corner box.
      const uint32 destinationId = propertyAt(*state, transform.x, transform.y, transform.z);
      placed = commitPlacement(client, *state, candidate, destinationId, transform, placement.tilts, movingQuantity(candidate, source), applied);
    }
    catch (const ItemRefusal &refusal) {
      refuse(client, refusal, "placement commit");
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s placement commit of [{}] failed: {}", client->GetCleanName(), request.guid, error.what());
      sendItemResult(client, ItemResult::failed);
      return;
    }
    state->placement.reset();
    sendItemResult(client, ItemResult::success);
    finishApplied(applied, "placement commit");
    LogHousing("[{}] placed record [{}] (item [{}]) in property [{}] at ({}, {}, {})", client->GetCleanName(), placed.id, placed.item_id, placed.property_id, placed.x, placed.y, placed.z);
  }

  void handleMoveToInventory(Client *client, WireReader &reader) {
    if (reader.remaining() != itemRequestSize) {
      LogError("Housing: [{}] sent a [{}]-byte move to inventory; it is [{}] bytes", client->GetCleanName(), reader.remaining(), itemRequestSize);
      return;
    }
    ItemRequest request;
    readItemRequest(reader, request);
    Session *state = session(client);
    if (!state) {
      return;
    }
    PlacedItem taken;
    std::unique_ptr<EQ::ItemInstance> given;
    int16 slot = INVALID_INDEX;
    std::map<uint32, PropertyChangedPayload> changedProperties;
    bool wholeRow = true;
    try {
      Candidate candidate = rowCandidate(*state, request);
      taken = *candidate.row;
      const uint32 quantity = movingQuantity(candidate, request);
      wholeRow = !candidate.instance->IsStackable() || quantity >= static_cast<uint32>(std::max<int16>(candidate.instance->GetCharges(), 1));
      Transaction transaction(database);
      const Property holder = lockedProperty(database, taken.property_id);
      std::set<uint32> focus{holder.id};
      if (candidate.carried) {
        focus.insert(candidate.carried->id);
      }
      View view = viewOf(*state, focus);
      const auto lookup = view.lookup();
      const auto listOf = view.listOf();
      const Property *plot = ancestorOfType(holder, typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, lookup);
      const bool carriesItems = candidate.carried && view.holdsContents(candidate.carried->id);
      bool takerCarriesSameKind = false;
      if (carriesItems) {
        for (uint32 carriedId : state->carriedIds) {
          auto carried = view.properties.find(carriedId);
          if (carried != view.properties.end() && kindOf(static_cast<uint8>(typeOf(carried->second))) == candidate.kind && carriedId != candidate.carried->id) {
            takerCarriesSameKind = takerCarriesSameKind || propertiesChildOf({carriedId}).size() + itemsOf({carriedId}).size() > 0;
          }
        }
      }
      given = std::move(candidate.instance);
      if (!wholeRow) {
        given->SetCharges(static_cast<int16>(quantity));
      }
      slot = client->GetInv().FindFirstFreeSlotThatFitsItem(given->GetItem());
      PickUpFacts facts;
      facts.plotUpkeepExpired = plot && plot->upkeep_expired_time;
      facts.crate = candidate.kind == CarriedKind::crate;
      facts.mayRemoveCrate = permitted(client, Permission::removeCrate, holder, lookup, listOf);
      facts.ownerOrPlacer = accessGroup(client, holder, lookup, listOf) == ownerGroup || taken.owner_character_id == client->CharacterID();
      facts.noDrop = !given->GetItem()->NoDrop || given->IsAttuned();
      // An entry without a placer is the plot owner's, as the Items window counts it (RealEstateItemsWnd.md, filters).
      facts.placedByTaker = taken.owner_character_id ? taken.owner_character_id == client->CharacterID() : accessGroup(client, holder, lookup, listOf) == ownerGroup;
      facts.loreHeld = given->GetItem()->LoreFlag && client->CheckLoreConflict(given->GetItem());
      facts.carriesItems = carriesItems;
      facts.takerCarriesSameKindWithItems = takerCarriesSameKind;
      facts.inventoryRoom = slot != INVALID_INDEX;
      const ItemResult result = pickUpResult(facts);
      if (result != ItemResult::success) {
        throw ItemRefusal(result, fmt::format("record [{}] may not be taken", taken.id));
      }
      if (wholeRow) {
        query(database, fmt::format("DELETE FROM real_estate_items WHERE id = {}", taken.id));
      }
      else {
        query(database, fmt::format("UPDATE real_estate_items SET charges = charges - {} WHERE id = {}", quantity, taken.id));
      }
      if (candidate.carried) {
        if (carriesItems) {
          query(database, fmt::format(
            "UPDATE real_estate_properties SET parent_id = 0, owner_character_id = {}, contained_in_item = 1, "
            "archived = 0, revision = revision + 1 WHERE id = {}", client->CharacterID(), candidate.carried->id
          ));
          changedProperties[candidate.carried->id] = PropertyChangedPayload{candidate.carried->owner_character_id, client->CharacterID(), 0, 0};
        }
        else {
          // An empty house or crate picked up loses its real estate (STT-4) and with it the No Trade it carried.
          for (const char *table : {"real_estate_access_groups", "real_estate_access_names", "real_estate_property_balances"}) {
            query(database, fmt::format("DELETE FROM {} WHERE property_id = {}", table, candidate.carried->id));
          }
          query(database, fmt::format("DELETE FROM real_estate_properties WHERE id = {}", candidate.carried->id));
          given->DeleteCustomData(propertyItemKey);
          given->SetAttuned(false);
          changedProperties[candidate.carried->id] = PropertyChangedPayload{candidate.carried->owner_character_id, 0, 0, 0};
        }
        changedProperties[holder.id] = PropertyChangedPayload{holder.owner_character_id, holder.owner_character_id, holder.guild_id, holder.guild_id};
      }
      bumpRevision(database, holder.id);
      transaction.commit();
    }
    catch (const ItemRefusal &refusal) {
      refuse(client, refusal, "move to inventory");
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s move to inventory of record [{}] failed: {}", client->GetCleanName(), request.recordId, error.what());
      sendItemResult(client, ItemResult::failed);
      return;
    }
    Applied applied;
    applied.itemLists.insert(taken.property_id);
    applied.changedProperties = changedProperties;
    if (wholeRow) {
      withdrawRow(taken);
      if (taken.placed) {
        applied.withdrawn.push_back(taken);
      }
    }
    if (!client->PutItemInInventory(slot, *given, true)) {
      LogError("Housing: [{}] could not be given record [{}] (item [{}]) in slot [{}]; the item is lost from the property", client->GetCleanName(), taken.id, taken.item_id, slot);
    }
    sendItemResult(client, ItemResult::success);
    announce(applied);
    LogHousing("[{}] took record [{}] (item [{}]) from property [{}]", client->GetCleanName(), taken.id, taken.item_id, taken.property_id);
  }

  void handleMoveToStorage(Client *client, WireReader &reader) {
    if (reader.remaining() != storeRequestSize) {
      LogError("Housing: [{}] sent a [{}]-byte move to storage; it is [{}] bytes", client->GetCleanName(), reader.remaining(), storeRequestSize);
      return;
    }
    ItemRequest request;
    readItemRequest(reader, request);
    const uint32 destinationCode = reader.u32();
    Session *state = session(client);
    if (!state) {
      return;
    }
    Applied applied;
    PlacedItem stored;
    try {
      Candidate candidate = candidateFor(client, *state, request);
      const uint32 base = candidate.row ? candidate.row->property_id : propertyAt(*state, client->GetX(), client->GetY(), client->GetZ());
      if (!base) {
        throw ItemRefusal(ItemResult::notPlaceableZone, "the player stands in no property");
      }
      std::set<uint32> focus{base};
      const Property *basePlot = ancestorOfType(state->properties.at(base), typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, state->lookup());
      if (basePlot) {
        focus.insert(basePlot->id);
      }
      if (candidate.carried) {
        focus.insert(candidate.carried->id);
      }
      View view = viewOf(*state, focus);
      const Property &baseProperty = view.at(base);
      const DefinitionType baseType = typeOf(baseProperty);
      std::optional<uint32> destinationId;
      switch (static_cast<StorageDestination>(destinationCode)) {
        case StorageDestination::closet:
          if (baseType == DefinitionType::playerHouse || baseType == DefinitionType::guildHall) {
            destinationId = base;
          }
          else if (basePlot) {
            for (const Property *child : view.childrenOf(basePlot->id)) {
              const DefinitionType childType = typeOf(*child);
              if (childType == DefinitionType::playerHouse || childType == DefinitionType::guildHall) {
                destinationId = child->id;
              }
            }
          }
          break;
        case StorageDestination::shed:
          if (basePlot) {
            destinationId = basePlot->id;
          }
          break;
        case StorageDestination::crate:
          if (basePlot) {
            if (const Property *crate = placedCrateIn(view, basePlot->id, 0)) {
              destinationId = crate->id;
            }
          }
          break;
        default:
          throw ItemRefusal(ItemResult::failed, fmt::format("[{}] is not a storage destination", destinationCode));
      }
      if (!destinationId) {
        throw ItemRefusal(destinationCode == static_cast<uint32>(StorageDestination::crate) ? ItemResult::crateRequired : ItemResult::failed,
          fmt::format("property [{}] has no storage [{}]", base, destinationCode));
      }
      stored = storeInto(client, *state, candidate, focus, *destinationId, movingQuantity(candidate, request), applied);
    }
    catch (const ItemRefusal &refusal) {
      refuse(client, refusal, "move to storage");
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s move to storage of [{}] failed: {}", client->GetCleanName(), request.guid, error.what());
      sendItemResult(client, ItemResult::failed);
      return;
    }
    sendItemResult(client, ItemResult::success);
    finishApplied(applied, "move to storage");
    LogHousing("[{}] stored item [{}] as record [{}] in property [{}]", client->GetCleanName(), stored.item_id, stored.id, stored.property_id);
  }

  // 0x21ba (0x735784): the placed house the client found on the yard and the stored house the menu named. The incoming house takes
  // the placed one's transform, the placed one goes to the yard's storage, and its items into the incoming house's storage (5337).
  void handleHouseSwap(Client *client, WireReader &reader) {
    if (reader.remaining() != houseSwapSize) {
      LogError("Housing: [{}] sent a [{}]-byte house swap; it is [{}] bytes", client->GetCleanName(), reader.remaining(), houseSwapSize);
      return;
    }
    reader.u32();
    reader.u32();
    const uint32 placedPropertyId = reader.u32();
    const uint32 placedRecordId = reader.u32();
    const uint32 storedPropertyId = reader.u32();
    const uint32 storedRecordId = reader.u32();
    Session *state = session(client);
    if (!state) {
      return;
    }
    Applied applied;
    PlacedItem incoming;
    try {
      if (!state->properties.count(placedPropertyId) || !state->properties.count(storedPropertyId)) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("property [{}] or [{}] is not one this zone sent", placedPropertyId, storedPropertyId));
      }
      const uint32 standing = propertyAt(*state, client->GetX(), client->GetY(), client->GetZ());
      const Property *standingPlot = standing ? ancestorOfType(state->properties.at(standing), typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, state->lookup()) : nullptr;
      auto outgoingRow = placedItem(placedPropertyId, placedRecordId);
      Candidate outgoing = outgoingRow ? bulkCandidate(*outgoingRow) : Candidate{};
      if (!outgoingRow || !outgoingRow->placed || !isHouseKind(outgoing.kind) || !outgoing.carried || !standingPlot || standingPlot->id != placedPropertyId) {
        throw ItemRefusal(ItemResult::swapNoHousePlaced, fmt::format("record [{}] of property [{}] is no house placed on the yard the player stands in", placedRecordId, placedPropertyId));
      }
      auto incomingRow = placedItem(storedPropertyId, storedRecordId);
      if (!incomingRow) {
        throw ItemRefusal(ItemResult::notFound, fmt::format("record [{}] of property [{}] does not exist", storedRecordId, storedPropertyId));
      }
      if (incomingRow->placed) {
        throw ItemRefusal(ItemResult::swapAlreadyPlaced, fmt::format("record [{}] is placed", storedRecordId));
      }
      Candidate candidate = bulkCandidate(*incomingRow);
      if (!isHouseKind(candidate.kind)) {
        throw ItemRefusal(ItemResult::failed, fmt::format("record [{}] holds no house", storedRecordId));
      }
      const uint32 outgoingHouseId = outgoing.carried->id;
      Transaction transaction(database);
      const Property yard = lockedProperty(database, placedPropertyId);
      lockedProperty(database, outgoingHouseId);
      std::set<uint32> focus{yard.id, outgoingHouseId, storedPropertyId};
      if (candidate.carried) {
        focus.insert(candidate.carried->id);
      }
      View view = viewOf(*state, focus);
      view.properties[yard.id] = yard;
      if (placedCrateIn(view, outgoingHouseId, 0)) {
        throw ItemRefusal(ItemResult::swapPlacedCrate, fmt::format("house [{}] holds a placed crate", outgoingHouseId));
      }
      if (!permitted(client, Permission::removeHouse, yard, view.lookup(), view.listOf()) || !permitted(client, Permission::moveItem, yard, view.lookup(), view.listOf())) {
        throw ItemRefusal(ItemResult::noRemoveHouse, fmt::format("the player may not take house [{}] off yard [{}]", outgoingHouseId, yard.id));
      }
      PlacementFacts facts = factsFor(client, view, candidate, yard, false, 1);
      if (facts.destinationHouses) {
        facts.destinationHouses--;
      }
      const ItemResult rules = placementResult(facts);
      if (rules != ItemResult::success) {
        throw ItemRefusal(rules, fmt::format("record [{}] may not be placed on yard [{}]", storedRecordId, yard.id));
      }
      SwapCapacity space;
      std::vector<std::pair<uint32, uint32>> moving;
      for (const auto &row : view.rowsOf(outgoingHouseId)) {
        const EQ::ItemHousingTail *tail = itemHousingTail(row.item_id);
        (tail && tail->placeable ? space.realEstateMoving : space.vaultMoving)++;
        space.npcMoving += isNpcItemId(row.item_id);
      }
      if (candidate.carried) {
        for (const auto &row : view.rowsOf(candidate.carried->id)) {
          const EQ::ItemHousingTail *tail = itemHousingTail(row.item_id);
          (tail && tail->placeable ? space.realEstateHeld : space.vaultHeld)++;
          space.npcHeld += isNpcItemId(row.item_id);
        }
      }
      const uint32 incomingDefinitionId = candidate.carried ? candidate.carried->definition_id : static_cast<uint32>(candidate.tail->realEstateDefinitionId);
      const Definition *incomingDefinition = catalog().definition(incomingDefinitionId);
      if (!incomingDefinition) {
        throw std::runtime_error(fmt::format("record [{}]'s house names unknown definition [{}]", storedRecordId, incomingDefinitionId));
      }
      space.maxItems = incomingDefinition->row.max_items;
      space.maxVault = incomingDefinition->row.max_non_real_estate_items;
      space.npcCap = incomingDefinition->maxNpcItems;
      const ItemResult room = swapCapacityResult(space);
      if (room != ItemResult::success) {
        throw ItemRefusal(room, fmt::format("house [{}]'s items do not fit the incoming house", outgoingHouseId));
      }

      incoming = *incomingRow;
      incoming.property_id = yard.id;
      incoming.placed = 1;
      incoming.heading = outgoingRow->heading;
      incoming.pitch = outgoingRow->pitch;
      incoming.roll = outgoingRow->roll;
      incoming.scale = outgoingRow->scale;
      incoming.x = outgoingRow->x;
      incoming.y = outgoingRow->y;
      incoming.z = outgoingRow->z;
      std::set<uint32> carriedChanged;
      if (incomingRow->property_id != yard.id || !candidate.carried) {
        attachCarried(candidate, incoming, yard, view.lookup(), carriedChanged);
      }
      updateRow(incoming);
      const uint32 incomingHouseId = carriedPropertyId(incoming.custom_data);
      PlacedItem outgoingStored = *outgoingRow;
      outgoingStored.placed = 0;
      updateRow(outgoingStored);
      for (const auto &row : view.rowsOf(outgoingHouseId)) {
        PlacedItem moved = row;
        moved.property_id = incomingHouseId;
        moved.placed = 0;
        updateRow(moved);
        if (const uint32 carriedId = carriedPropertyId(row.custom_data)) {
          query(database, fmt::format("UPDATE real_estate_properties SET parent_id = {}, revision = revision + 1 WHERE id = {}", incomingHouseId, carriedId));
          applied.changedProperties[carriedId] = PropertyChangedPayload{0, 0, 0, 0};
        }
        if (row.placed) {
          applied.withdrawn.push_back(row);
        }
      }
      for (uint32 id : {yard.id, outgoingHouseId, incomingHouseId, storedPropertyId}) {
        bumpRevision(database, id);
        applied.itemLists.insert(id);
      }
      transaction.commit();
      const Property &outgoingHouse = *outgoing.carried;
      applied.changedProperties[outgoingHouseId] = PropertyChangedPayload{outgoingHouse.owner_character_id, outgoingHouse.owner_character_id, outgoingHouse.guild_id, outgoingHouse.guild_id};
      const uint32 incomingOwner = ownerFor(yard, view.lookup());
      applied.changedProperties[incomingHouseId] = PropertyChangedPayload{candidate.carried ? candidate.carried->owner_character_id : 0, incomingOwner, 0, 0};
      applied.changedProperties[yard.id] = PropertyChangedPayload{yard.owner_character_id, yard.owner_character_id, yard.guild_id, yard.guild_id};
      applied.withdrawn.push_back(*outgoingRow);
      applied.shown.push_back(incoming);
      applied.count = 1;
    }
    catch (const ItemRefusal &refusal) {
      refuse(client, refusal, "house swap");
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s house swap of record [{}] for [{}] failed: {}", client->GetCleanName(), placedRecordId, storedRecordId, error.what());
      sendItemResult(client, ItemResult::failed);
      return;
    }
    sendItemResult(client, ItemResult::success);
    finishApplied(applied, "house swap");
    LogHousing("[{}] swapped record [{}] of property [{}] for record [{}]", client->GetCleanName(), placedRecordId, placedPropertyId, storedRecordId);
  }

  // 0x5b41: the client planned the layout itself (windows/RealEstateItemsWnd.md, How Load plans); the server checks each record
  // with the placement or move rules and applies the ones that pass.
  void handleLoadLayout(Client *client, WireReader &reader) {
    const uint32 yardId = reader.u32();
    const uint32 houseId = reader.u32();
    const uint16 count = reader.u16();
    std::vector<LayoutRecord> records;
    for (uint16 index = 0; index < count && !reader.failed(); index++) {
      LayoutRecord record;
      const uint8 tag = reader.u8();
      if (tag == 1) {
        record.place = true;
        record.request.guid = reader.cString();
        record.request.location.type = reader.u8();
        record.request.location.slot = static_cast<int16>(reader.u16());
        record.request.location.subIndex = static_cast<int16>(reader.u16());
        record.request.location.augIndex = static_cast<int16>(reader.u16());
        record.request.propertyId = reader.u32();
        record.request.recordId = reader.u32();
        record.request.quantity = 0;
        record.transform.heading = reader.f32();
        record.transform.pitch = reader.f32();
        record.transform.roll = reader.f32();
        record.transform.scale = reader.f32();
        record.transform.y = reader.f32();
        record.transform.x = reader.f32();
        record.transform.z = reader.f32();
        record.destinationId = reader.u32();
      }
      else if (tag == 0) {
        record.request.propertyId = reader.u32();
        record.request.recordId = reader.u32();
        record.request.quantity = reader.u32();
        record.destinationId = reader.u32();
      }
      else {
        LogError("Housing: [{}] sent layout record [{}] with tag [{}]; tags are 0 and 1", client->GetCleanName(), index, tag);
        return;
      }
      records.push_back(record);
    }
    if (reader.failed() || reader.remaining()) {
      LogError("Housing: [{}] sent a malformed layout of [{}] records", client->GetCleanName(), count);
      return;
    }
    Session *state = session(client);
    if (!state) {
      return;
    }
    Applied applied;
    std::optional<ItemResult> firstRefusal;
    for (auto &record : records) {
      try {
        if (!state->properties.count(record.destinationId) || (record.destinationId != yardId && record.destinationId != houseId &&
            !(state->properties.at(record.destinationId).parent_id == yardId || state->properties.at(record.destinationId).parent_id == houseId))) {
          throw ItemRefusal(ItemResult::notFound, fmt::format("destination [{}] is not the layout's yard [{}], house [{}] or their storage", record.destinationId, yardId, houseId));
        }
        if (record.place) {
          Candidate candidate = candidateFor(client, *state, record.request);
          // An interior has no regions, so its house is the destination wherever the layout puts an item (chunk 4).
          const DefinitionType destinationType = typeOf(state->properties.at(record.destinationId));
          const bool interior = destinationType == DefinitionType::playerHouse || destinationType == DefinitionType::guildHall;
          if (!interior && propertyAt(*state, record.transform.x, record.transform.y, record.transform.z) != record.destinationId) {
            throw ItemRefusal(ItemResult::notAllowedHere, fmt::format("the position does not lie in destination [{}]", record.destinationId));
          }
          commitPlacement(client, *state, candidate, record.destinationId, record.transform, mountTilts(*candidate.instance), movingQuantity(candidate, record.request), applied);
        }
        else {
          auto row = placedItem(record.request.propertyId, record.request.recordId);
          if (!row) {
            throw ItemRefusal(ItemResult::notFound, fmt::format("record [{}] of property [{}] does not exist", record.request.recordId, record.request.propertyId));
          }
          record.request.guid = itemGuid(*row);
          Candidate candidate = rowCandidate(*state, record.request);
          std::set<uint32> focus{row->property_id};
          if (candidate.carried) {
            focus.insert(candidate.carried->id);
          }
          storeInto(client, *state, candidate, focus, record.destinationId, movingQuantity(candidate, record.request), applied);
        }
      }
      catch (const ItemRefusal &refusal) {
        LogHousing("Refused [{}]'s layout record for record [{}] of property [{}]: {} (code [{}])", client->GetCleanName(), record.request.recordId,
          record.request.propertyId, refusal.what(), static_cast<uint32>(refusal.result));
        if (!firstRefusal) {
          firstRefusal = refusal.result;
        }
      }
      catch (const std::exception &error) {
        LogError("Housing: [{}]'s layout record for record [{}] of property [{}] failed: {}", client->GetCleanName(), record.request.recordId, record.request.propertyId, error.what());
        if (!firstRefusal) {
          firstRefusal = ItemResult::failed;
        }
      }
    }
    // One result ends the client's request latch: 0 when every record applied, else the first refusal's code (INT-29).
    sendItemResult(client, firstRefusal ? *firstRefusal : ItemResult::success);
    finishApplied(applied, "layout load");
    LogHousing("[{}] loaded a layout onto yard [{}] and house [{}]: [{}] of [{}] records applied", client->GetCleanName(), yardId, houseId, applied.count, records.size());
  }

  void handleStoreAll(Client *client, WireReader &reader) {
    if (reader.remaining()) {
      LogError("Housing: [{}] sent a [{}]-byte store all; it has no body", client->GetCleanName(), reader.remaining());
      return;
    }
    Session *state = session(client);
    if (!state) {
      return;
    }
    try {
      const uint32 location = locationOf(client, *state);
      if (!location) {
        throw ItemRefusal(ItemResult::notPlaceableZone, "the player stands in no yard or house");
      }
      finishBulkMove(client, moveRows(client, *state, location, location, true), "store all");
    }
    catch (const ItemRefusal &refusal) {
      refuse(client, refusal, "store all");
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s store all failed: {}", client->GetCleanName(), error.what());
      sendItemResult(client, ItemResult::failed);
    }
  }

  // RoF2 writes 0x613d's byte: 1 from the pack dialog, 0 from the unpack dialog (0x735480; INT-8).
  void handlePack(Client *client, WireReader &reader) {
    const uint8 pack = reader.u8();
    if (reader.failed() || reader.remaining() || pack > 1) {
      LogError("Housing: [{}] sent a malformed pack request", client->GetCleanName());
      return;
    }
    Session *state = session(client);
    if (!state) {
      return;
    }
    const char *request = pack ? "pack" : "unpack";
    try {
      const uint32 location = locationOf(client, *state);
      if (!location) {
        throw ItemRefusal(ItemResult::notPlaceableZone, "the player stands in no yard or house");
      }
      const Property *plot = ancestorOfType(state->properties.at(location), typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, state->lookup());
      if (!plot) {
        throw ItemRefusal(ItemResult::crateRequired, fmt::format("property [{}] stands on no plot", location));
      }
      View view = viewOf(*state, {plot->id});
      const Property *crate = placedCrateIn(view, plot->id, 0);
      if (!crate) {
        throw ItemRefusal(ItemResult::crateRequired, fmt::format("plot [{}] has no placed moving crate", plot->id));
      }
      const uint32 crateId = crate->id;
      finishBulkMove(client, pack ? moveRows(client, *state, location, crateId, false) : moveRows(client, *state, crateId, location, false), request);
    }
    catch (const ItemRefusal &refusal) {
      refuse(client, refusal, request);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s {} failed: {}", client->GetCleanName(), request, error.what());
      sendItemResult(client, ItemResult::failed);
    }
  }
}
