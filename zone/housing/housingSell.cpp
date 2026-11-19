#include "housingHandlers.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingItems.h"
#include "housingPlacement.h"
#include "housingPurse.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/guilds.h"
#include "../../common/housingLedger.h"
#include "../../common/item_layer.h"
#include "../../common/repositories/character_parcels_repository.h"
#include "../../common/rulesys.h"
#include "../../common/servertalk.h"
#include "../../common/strings.h"
#include "../client.h"
#include "../guild_mgr.h"
#include "../worldserver.h"
#include "../zone.h"
#include "../zonedb.h"

#include <algorithm>
#include <ctime>
#include <limits>
#include <memory>
#include <stdexcept>

extern WorldServer worldserver;
extern Zone *zone;

namespace housing {
  namespace {
    constexpr uint32 sold = 0;
    constexpr uint32 failed = 1;
    constexpr uint32 notOwner = 2;
    constexpr uint32 holdsFullCrate = 3;
    constexpr uint32 noCrateWithRoom = 4;
    constexpr uint32 crateHoldsFullHouse = 5;
    constexpr uint32 mayNotRemoveCrate = 6;
    constexpr uint32 needsHouseOfThule = 7;
    constexpr uint32 needsVeilOfAlaris = 8;
    constexpr uint32 notGuildMember = 9;
    constexpr uint32 escrowNeedsWithdraw = 10;
    constexpr uint32 crateNoTrade = 11;
    constexpr uint32 crateLoreHeld = 12;
    constexpr uint32 guildCannotSell = 757;
    constexpr uint32 withdrewAlternateCurrency = 6425;
    constexpr uint32 withdrewCoin = 6426;

    struct SellRefusal : std::runtime_error {
      SellRefusal(const std::string &reason, uint32 code) : std::runtime_error(reason), code(code) {}
      uint32 code;
    };

    void sendResult(Client *client, uint32 code) {
      WireWriter message;
      message.u32(code);
      sendMessage(client, OP_RealEstateSellResult, message);
    }

    std::set<uint32> carriedCrateIds(uint32 characterId) {
      std::set<uint32> crates;
      for (const auto &property : propertiesById(carriedPropertyIds(database, characterId))) {
        if (typeOf(property) == DefinitionType::movingCrate) {
          crates.insert(property.id);
        }
      }
      return crates;
    }

    uint32 itemOfDefinition(uint32 definitionId) {
      auto rows = query(content_db, fmt::format("SELECT id FROM items WHERE real_estate_definition_id = {}", definitionId));
      if (rows.RowCount() != 1) {
        throw std::runtime_error(fmt::format("[{}] items name real estate definition [{}]; a crate needs exactly one", rows.RowCount(), definitionId));
      }
      return Strings::ToUnsignedInt(rows.begin()[0]);
    }

    // A property-carrying item is No Trade and No Drop per instance for as long as it carries it (Live, STT-6).
    std::unique_ptr<EQ::ItemInstance> carrierInstance(uint32 itemId, uint32 propertyId) {
      const EQ::ItemData *item = database.GetItem(itemId);
      if (!item) {
        throw std::runtime_error(fmt::format("item [{}] does not exist", itemId));
      }
      std::unique_ptr<EQ::ItemInstance> instance(database.CreateBaseItem(item));
      if (!instance) {
        throw std::runtime_error(fmt::format("item [{}] could not be created", itemId));
      }
      instance->SetRealEstateID(propertyId);
      instance->SetAttuned(true);
      return instance;
    }

    struct Payout {
      uint64 copper = 0;
      std::map<uint32, uint32> alternate;
    };

    uint32 guildLeaderOf(uint32 guildId) {
      auto rows = query(database, fmt::format("SELECT leader FROM guilds WHERE id = {}", guildId));
      if (rows.RowCount() != 1 || !Strings::ToUnsignedInt(rows.begin()[0])) {
        throw std::runtime_error(fmt::format("guild [{}] has no leader to receive its plot's escrow", guildId));
      }
      return Strings::ToUnsignedInt(rows.begin()[0]);
    }

    // A GM's sale of a plot the GM does not own sends the escrow to the owner (a guild plot's to its guild's leader) as
    // parcels: coin as money, each alternate currency as its item, which the owner reclaims as currency (the owner's
    // decision, AUD-27). Written in the sale's transaction; returns the parcel slots used.
    std::vector<int> sendEscrowParcels(Client *seller, uint32 recipientId, const std::string &note, const Payout &payout) {
      std::vector<int> slots;
      auto parcelFor = [&]() {
        const int slot = CharacterParcelsRepository::GetNextFreeParcelSlot(database, recipientId, RuleI(Parcel, ParcelMaxItems));
        if (slot == INVALID_INDEX) {
          throw SellRefusal(fmt::format("character [{}] has no free parcel slot for the escrow", recipientId), failed);
        }
        auto parcel = CharacterParcelsRepository::NewEntity();
        parcel.char_id = recipientId;
        parcel.slot_id = static_cast<uint32>(slot);
        parcel.from_name = seller->GetCleanName();
        parcel.note = note;
        parcel.sent_date = std::time(nullptr);
        return parcel;
      };
      auto write = [&](const CharacterParcelsRepository::CharacterParcels &parcel) {
        if (!CharacterParcelsRepository::InsertOne(database, parcel).id) {
          throw std::runtime_error(fmt::format("a parcel of the escrow to character [{}] could not be written", recipientId));
        }
        slots.push_back(static_cast<int>(parcel.slot_id));
      };
      for (uint64 left = payout.copper; left > 0;) {
        const uint64 copper = std::min<uint64>(left, std::numeric_limits<uint32>::max());
        auto parcel = parcelFor();
        parcel.money = static_cast<uint32>(copper);
        write(parcel);
        left -= copper;
      }
      for (const auto &[currency, units] : payout.alternate) {
        const uint32 itemId = zone->GetCurrencyItemID(currency);
        const EQ::ItemData *item = itemId ? database.GetItem(itemId) : nullptr;
        if (!item) {
          throw std::runtime_error(fmt::format("currency [{}] has no item to send in a parcel", currency));
        }
        const uint32 stack = item->Stackable && item->StackSize > 1 ? static_cast<uint32>(item->StackSize) : 1;
        for (uint32 left = units; left > 0;) {
          const uint32 count = std::min(left, stack);
          std::unique_ptr<EQ::ItemInstance> instance(database.CreateItem(itemId, static_cast<int16>(count)));
          if (!instance) {
            throw std::runtime_error(fmt::format("item [{}] of currency [{}] could not be made", itemId, currency));
          }
          auto parcel = parcelFor();
          if (!ItemLayer::Save(database, *instance, ItemLocation::InHolder(ItemHolderType::Parcel, recipientId, static_cast<uint16>(parcel.slot_id)), seller->ItemOwner())) {
            throw std::runtime_error(fmt::format("item [{}] of currency [{}] could not be saved to character [{}]'s parcels", itemId, currency, recipientId));
          }
          CharacterParcelsRepository::SetGUID(parcel, instance->GetGUID());
          write(parcel);
          left -= count;
        }
      }
      return slots;
    }

    Payout escrowPayout(uint32 plotId) {
      Payout payout;
      for (const auto &[currency, amount] : readBalances(database, plotId)) {
        const int64 units = flooredUnits(amount);
        if (currency == 0) {
          payout.copper = static_cast<uint64>(units);
        }
        else if (units > 0) {
          payout.alternate[currency] = static_cast<uint32>(units);
        }
      }
      return payout;
    }
  }

  void deliverPendingCrates(Client *client) {
    const auto archived = RealEstatePropertiesRepository::GetWhere(database, fmt::format(
      "owner_character_id = {} AND archived = 1", client->CharacterID()
    ));
    for (const auto &crate : archived) {
      if (typeOf(crate) != DefinitionType::movingCrate) {
        continue;
      }
      // An evicted plot's crate has no item until its owner has room for one.
      std::unique_ptr<EQ::ItemInstance> instance = crate.container_guid
        ? std::unique_ptr<EQ::ItemInstance>(ItemLayer::LoadOne(database, database, *crate.container_guid))
        : carrierInstance(itemOfDefinition(crate.definition_id), crate.id);
      if (!instance) {
        throw std::runtime_error(fmt::format("archived crate [{}]'s item did not load", crate.id));
      }
      const int16 slot = client->GetInv().FindFirstFreeSlotThatFitsItem(instance->GetItem());
      if (slot == INVALID_INDEX) {
        LogHousing("[{}] has no room for archived crate [{}]; it waits for a later zone-in", client->GetCleanName(), crate.id);
        continue;
      }
      Transaction transaction(database);
      query(database, fmt::format("UPDATE real_estate_properties SET archived = 0, revision = revision + 1 WHERE id = {}", crate.id));
      if (!crate.container_guid) {
        if (!ItemLayer::Save(database, *instance, ItemLocation::InRealEstate(crate.id), client->ItemOwner())) {
          throw std::runtime_error(fmt::format("archived crate [{}]'s item could not be issued", crate.id));
        }
        const ItemGUID guid = instance->GetGUID();
        query(database, fmt::format(
          "UPDATE real_estate_properties SET container_guid_first = {}, container_guid_second = {}, container_guid_third = {} WHERE id = {}",
          guid.first, guid.second, guid.third, crate.id
        ));
      }
      giveAndCommit(client, transaction, slot, *instance);
      LogHousing("Delivered archived crate [{}] to [{}] in slot [{}]", crate.id, client->GetCleanName(), slot);
    }
  }

  void handleSell(Client *client, WireReader &reader) {
    PlotRequest request;
    if (!readPlotRequest(reader, request)) {
      LogError("Housing: [{}] sent a sale of [{}] bytes, not [{}]", client->GetCleanName(), reader.remaining(), PlotRequest::size);
      return;
    }
    const uint32 plotId = request.u32At(0x00);
    Session *state = session(client);
    Property plot;
    Payout payout;
    std::unique_ptr<EQ::ItemInstance> crateItem;
    int16 crateSlot = INVALID_INDEX;
    std::string escrowRecipient;
    std::vector<int> escrowParcels;
    uint32 crateId = 0;
    bool crateCarriesProperty = false;
    std::vector<PlacedItem> leaving;
    std::set<uint32> under;
    try {
      if (!state || !state->properties.count(plotId)) {
        throw SellRefusal("the plot is not one this zone sent", failed);
      }
      Transaction transaction(database);
      plot = lockedProperty(database, plotId);
      std::map<uint32, Property> fresh(state->properties);
      fresh[plotId] = plot;
      auto lookup = [&fresh](uint32 id) -> const Property * {
        auto found = fresh.find(id);
        return found == fresh.end() ? nullptr : &found->second;
      };
      auto lists = accessLists({plotId});
      auto listOf = [&lists](uint32 id) -> const AccessList * {
        auto found = lists.find(id);
        return found == lists.end() ? nullptr : &found->second;
      };
      const DefinitionType type = typeOf(plot);
      const bool guildPlot = type == DefinitionType::guildPlot;
      if (type != DefinitionType::plot && !guildPlot) {
        throw SellRefusal("the property is not a plot", failed);
      }
      if (!hostedHere(plot)) {
        throw SellRefusal("the seller is not in the plot's zone", failed);
      }
      if (guildPlot ? !activeGuildId(plot) : !plot.owner_character_id) {
        throw SellRefusal("the plot is not owned", failed);
      }
      const uint32 guild = client->GuildID() == GUILD_NONE ? 0 : client->GuildID();
      const bool gm = client->GetGM();
      if (!gm) {
        if (!guildPlot && plot.owner_character_id != client->CharacterID()) {
          throw SellRefusal("the seller is not the owner", notOwner);
        }
        if (guildPlot && !(client->GetPP().expansions & veilOfAlaris)) {
          throw SellRefusal("the seller lacks Veil of Alaris", needsVeilOfAlaris);
        }
        if (guildPlot) {
          bool escrowHeld = false;
          for (const auto &[currency, amount] : readBalances(database, plotId)) {
            escrowHeld = escrowHeld || amount != 0.0;
          }
          if (escrowHeld && !permitted(client, Permission::withdrawUpkeep, plot, lookup, listOf)) {
            throw SellRefusal("the guild plot holds escrow and the seller may not withdraw it", escrowNeedsWithdraw);
          }
          if (guild != activeGuildId(plot)) {
            throw SellRefusal("the seller is not in the plot's guild", notGuildMember);
          }
        }
        if (!guildPlot && !(client->GetPP().expansions & houseOfThule)) {
          throw SellRefusal("the seller lacks House of Thule", needsHouseOfThule);
        }
      }
      const auto rows = RealEstateItemsRepository::GetWhere(database, fmt::format("property_id = {} ORDER BY id", plotId));
      std::map<uint32, PlacedItem> rowCarrying;
      for (const auto &row : rows) {
        if (row.carried_property_id) {
          rowCarrying[row.carried_property_id] = row;
        }
      }
      const PlacedItem *crateRow = nullptr;
      Property crate;
      for (const auto &child : propertiesChildOf({plotId})) {
        if (typeOf(child) == DefinitionType::movingCrate) {
          auto row = rowCarrying.find(child.id);
          if (row != rowCarrying.end() && row->second.placed) {
            crate = child;
            crateRow = &row->second;
          }
        }
      }
      const size_t movedRows = rows.size() - (crateRow ? 1 : 0);
      // The seller may carry one non-empty crate, so only a sale that hands over another is refused, as the client's gate (0x57ef90 with the plot) lets an empty plot go.
      if (movedRows > 0 || (crateRow && holdsContents(database, crate.id))) {
        for (uint32 carried : carriedCrateIds(client->CharacterID())) {
          if (holdsContents(database, carried)) {
            throw SellRefusal("the seller already carries a non-empty crate", holdsFullCrate);
          }
        }
      }
      if (movedRows > 0) {
        if (!crateRow) {
          throw SellRefusal("the plot holds items and no placed crate", noCrateWithRoom);
        }
        const auto crateItems = RealEstateItemsRepository::GetWhere(database, fmt::format("property_id = {}", crate.id));
        if (crateItems.size() + movedRows > catalog().definition(crate.definition_id)->row.max_items) {
          throw SellRefusal("the placed crate has no room for the plot's items", noCrateWithRoom);
        }
        uint32 fullHouses = 0;
        for (const auto &child : propertiesChildOf({plotId, crate.id})) {
          const DefinitionType childType = typeOf(child);
          if ((childType == DefinitionType::playerHouse || childType == DefinitionType::guildHall) && holdsContents(database, child.id)) {
            fullHouses++;
          }
        }
        if (fullHouses > 1) {
          throw SellRefusal("the crate would hold more than one house with items", crateHoldsFullHouse);
        }
      }
      if (crateRow) {
        fresh[crate.id] = crate;
        if (!gm && !permitted(client, Permission::removeCrate, crate, lookup, listOf)) {
          throw SellRefusal("the seller may not remove the crate", mayNotRemoveCrate);
        }
        const EQ::ItemData *crateData = database.GetItem(crateRow->item_id);
        if (!crateData) {
          throw std::runtime_error(fmt::format("the crate is item [{}], which does not exist", crateRow->item_id));
        }
        // The client's gate (0x5b7fa0) tests the crate as it holds it, where placement's attune makes it No Trade (STT-6's stand-in).
        if ((!crateData->NoDrop || crateRow->attuned) && crateRow->owner_character_id != client->CharacterID()) {
          throw SellRefusal("the crate is No Trade and the seller did not place it", crateNoTrade);
        }
        if (crateData->LoreFlag && client->CheckLoreConflict(crateData)) {
          throw SellRefusal("the crate is Lore and the seller holds one", crateLoreHeld);
        }
      }
      if (guildPlot && !gm && !guild_mgr.CheckPermission(guild, client->GuildRank(), GUILD_ACTION_REAL_ESTATE_GUILD_PLOT_SELL)) {
        client->MessageString(Chat::Red, guildCannotSell);
        throw SellRefusal("the seller's rank may not sell guild plots", sold);
      }

      if (RuleB(Housing, SellRefundsEscrow)) {
        payout = escrowPayout(plotId);
      }
      if (crateRow) {
        crateId = crate.id;
        under = descendants(database, plotId);
        under.erase(crate.id);
        packPlotInto(database, plotId, crate.id);
        crateCarriesProperty = holdsContents(database, crate.id);
        crateItem = itemInstance(*crateRow);
        if (!crateItem) {
          throw std::runtime_error(fmt::format("crate row [{}] holds item [{}], which did not load", crateRow->id, crateRow->item_id));
        }
        queryOneRow(database, fmt::format("DELETE FROM real_estate_items WHERE id = {}", crateRow->id));
        crateSlot = client->GetInv().FindFirstFreeSlotThatFitsItem(crateItem->GetItem());
        if (crateCarriesProperty) {
          crateItem->SetAttuned(true);
          query(database, fmt::format(
            "UPDATE real_estate_properties SET parent_id = 0, owner_character_id = {}, archived = {}, revision = revision + 1 WHERE id = {}",
            client->CharacterID(), crateSlot == INVALID_INDEX ? 1 : 0, crate.id
          ));
        }
        else {
          crateItem->SetAttuned(false);
          crateItem->SetRealEstateID(0);
          query(database, fmt::format("DELETE FROM real_estate_access_groups WHERE property_id = {}", crate.id));
          query(database, fmt::format("DELETE FROM real_estate_access_names WHERE property_id = {}", crate.id));
          query(database, fmt::format("DELETE FROM real_estate_properties WHERE id = {}", crate.id));
        }
        if (crateSlot == INVALID_INDEX) {
          // Without a free slot a crate that carries its property waits on that property for a zone-in with room, and
          // an empty one goes to the cursor: saved there first, so the push after the commit changes memory only.
          const ItemLocation waiting = crateCarriesProperty
            ? ItemLocation::InRealEstate(crate.id)
            : ItemLocation::InHolder(ItemHolderType::Cursor, client->CharacterID(), static_cast<uint16>(client->GetInv().CursorSize()));
          if (!ItemLayer::Update(database, *crateItem, waiting)) {
            throw std::runtime_error(fmt::format("crate row [{}] could not leave property [{}]", crateRow->id, plotId));
          }
        }
      }
      releasePlot(database, plotId);
      leaving = rows;
      PurseGuard purse(client);
      const bool sellerOwns = guildPlot ? guild == activeGuildId(plot) : plot.owner_character_id == client->CharacterID();
      if (sellerOwns) {
        if (payout.copper) {
          client->AddMoneyToPP(payout.copper, false);
        }
        for (const auto &[currency, units] : payout.alternate) {
          purse.addCurrency(currency, static_cast<int64>(units));
        }
      }
      else if (payout.copper || !payout.alternate.empty()) {
        const uint32 recipientId = guildPlot ? guildLeaderOf(activeGuildId(plot)) : plot.owner_character_id;
        const auto recipients = characterIdentities({recipientId});
        if (!recipients.count(recipientId)) {
          throw std::runtime_error(fmt::format("the escrow's recipient [{}] is no character", recipientId));
        }
        escrowRecipient = recipients.at(recipientId).name;
        escrowParcels = sendEscrowParcels(client, recipientId, propertyName(plotId), payout);
      }
      if (crateItem && crateSlot != INVALID_INDEX) {
        giveAndCommit(client, transaction, crateSlot, *crateItem);
      }
      else {
        transaction.commit();
      }
      purse.committed();
    }
    catch (const SellRefusal &refusal) {
      LogHousing("Refused [{}]'s sale of property [{}]: {}", client->GetCleanName(), plotId, refusal.what());
      sendResult(client, refusal.code);
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s sale of property [{}] failed: {}", client->GetCleanName(), plotId, error.what());
      sendResult(client, failed);
      return;
    }

    if (crateItem && crateSlot == INVALID_INDEX) {
      if (crateCarriesProperty) {
        LogHousing("[{}] has no room for crate [{}]; it is archived until a zone-in with room", client->GetCleanName(), crateId);
      }
      else if (!client->PushItemOnCursor(*crateItem, true)) {
        LogError("Housing: the empty crate from property [{}] is on [{}]'s cursor, and the cursor did not save again", plotId, client->GetCleanName());
      }
    }
    if (!escrowParcels.empty()) {
      for (int slot : escrowParcels) {
        auto pack = std::make_unique<ServerPacket>(ServerOP_ParcelDelivery, static_cast<uint32>(sizeof(Parcel_Struct)));
        auto *delivery = reinterpret_cast<Parcel_Struct *>(pack->pBuffer);
        delivery->item_slot = static_cast<uint32>(slot);
        strn0cpy(delivery->send_to, escrowRecipient.c_str(), sizeof(delivery->send_to));
        worldserver.SendPacket(pack.get());
      }
      client->Message(Chat::White, fmt::format("The escrow went to {} in {} parcel(s).", escrowRecipient, escrowParcels.size()).c_str());
      LogHousing("[{}] sold property [{}], which is not theirs; its escrow went to [{}] in [{}] parcels", client->GetCleanName(), plotId,
        escrowRecipient, escrowParcels.size());
    }
    else {
      if (payout.copper) {
        client->MessageString(Chat::White, withdrewCoin, std::to_string(payout.copper).c_str());
      }
      for (const auto &[currency, units] : payout.alternate) {
        client->MessageString(Chat::White, withdrewAlternateCurrency, std::to_string(units).c_str(), std::to_string(currency).c_str());
      }
    }
    sendResult(client, sold);
    LogHousing("[{}] sold property [{}]", client->GetCleanName(), plotId);
    std::vector<ItemListChangedRecord> withdrawn;
    for (const PlacedItem &row : leaving) {
      withdrawRow(row);
      if (row.placed) {
        withdrawn.push_back(ItemListChangedRecord{row.id, row.property_id, row.item_id, 0, row.guid});
      }
    }
    sendPropertyChanged(plotId, PropertyChangedPayload{plot.owner_character_id, 0, plot.guild_id, 0});
    if (crateId) {
      sendPropertyChanged(crateId, PropertyChangedPayload{plot.owner_character_id, crateCarriesProperty ? client->CharacterID() : 0, 0, 0});
      sendItemListChanged(crateId, {});
    }
    // A house packed into the crate is no longer in a yard: its interior sends its occupants to the plot's exit (INT-38).
    for (uint32 id : under) {
      sendPropertyChanged(id, PropertyChangedPayload{plot.owner_character_id, plot.owner_character_id, 0, 0});
    }
    sendItemListChanged(plotId, withdrawn);
  }
}
