#include "housingHandlers.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingItems.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/guilds.h"
#include "../../common/housingLedger.h"
#include "../../common/housingPropertyItem.h"
#include "../../common/rulesys.h"
#include "../client.h"
#include "../guild_mgr.h"
#include "../zonedb.h"

#include <memory>
#include <stdexcept>

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

    bool hasContents(uint32 propertyId) {
      auto rows = query(database, fmt::format(
        "SELECT (SELECT COUNT(*) FROM real_estate_items WHERE property_id = {0}) + (SELECT COUNT(*) FROM real_estate_properties WHERE parent_id = {0})",
        propertyId
      ));
      return Strings::ToUnsignedInt(rows.begin()[0]) > 0;
    }

    std::set<uint32> carriedCrateIds(uint32 characterId) {
      std::set<uint32> carried;
      for (auto row : query(database, fmt::format("SELECT custom_data FROM inventory WHERE character_id = {} AND custom_data LIKE '%{}^%'", characterId, propertyItemKey))) {
        const uint32 id = carriedPropertyId(row[0]);
        if (id) {
          carried.insert(id);
        }
      }
      std::set<uint32> crates;
      for (const auto &property : propertiesById(carried)) {
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
      instance->SetCustomData(propertyItemKey, std::to_string(propertyId));
      instance->SetAttuned(true);
      return instance;
    }

    struct Payout {
      uint64 copper = 0;
      std::map<uint32, uint32> alternate;
    };

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
      "owner_character_id = {} AND archived = 1 AND contained_in_item = 0", client->CharacterID()
    ));
    for (const auto &crate : archived) {
      if (typeOf(crate) != DefinitionType::movingCrate) {
        continue;
      }
      auto instance = carrierInstance(itemOfDefinition(crate.definition_id), crate.id);
      const int16 slot = client->GetInv().FindFirstFreeSlotThatFitsItem(instance->GetItem());
      if (slot == INVALID_INDEX) {
        LogHousing("[{}] has no room for archived crate [{}]; it waits for a later zone-in", client->GetCleanName(), crate.id);
        continue;
      }
      {
        Transaction transaction(database);
        query(database, fmt::format("UPDATE real_estate_properties SET archived = 0, contained_in_item = 1, revision = revision + 1 WHERE id = {}", crate.id));
        transaction.commit();
      }
      if (!client->PutItemInInventory(slot, *instance, true)) {
        throw std::runtime_error(fmt::format("crate [{}] could not be put in [{}]'s slot [{}]", crate.id, client->GetCleanName(), slot));
      }
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
    uint32 crateId = 0;
    bool crateCarriesProperty = false;
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
      for (uint32 carried : carriedCrateIds(client->CharacterID())) {
        if (hasContents(carried)) {
          throw SellRefusal("the seller already carries a non-empty crate", holdsFullCrate);
        }
      }
      const auto rows = RealEstateItemsRepository::GetWhere(database, fmt::format("property_id = {} ORDER BY id", plotId));
      std::map<uint32, PlacedItem> rowCarrying;
      for (const auto &row : rows) {
        const uint32 carried = carriedPropertyId(row.custom_data);
        if (carried) {
          rowCarrying[carried] = row;
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
          if ((childType == DefinitionType::playerHouse || childType == DefinitionType::guildHall) && hasContents(child.id)) {
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
        if (!crateData->NoDrop && crateRow->owner_character_id != client->CharacterID()) {
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
        packPlotInto(database, plotId, crate.id);
        crateCarriesProperty = hasContents(crate.id);
        auto instance = itemInstance(*crateRow);
        if (!instance) {
          throw std::runtime_error(fmt::format("crate row [{}] holds item [{}], which does not exist", crateRow->id, crateRow->item_id));
        }
        query(database, fmt::format("DELETE FROM real_estate_items WHERE id = {}", crateRow->id));
        if (crateCarriesProperty) {
          instance->SetAttuned(true);
          crateSlot = client->GetInv().FindFirstFreeSlotThatFitsItem(instance->GetItem());
          query(database, fmt::format(
            "UPDATE real_estate_properties SET parent_id = 0, owner_character_id = {}, contained_in_item = {}, archived = {}, revision = revision + 1 WHERE id = {}",
            client->CharacterID(), crateSlot == INVALID_INDEX ? 0 : 1, crateSlot == INVALID_INDEX ? 1 : 0, crate.id
          ));
        }
        else {
          instance->DeleteCustomData(propertyItemKey);
          query(database, fmt::format("DELETE FROM real_estate_access_groups WHERE property_id = {}", crate.id));
          query(database, fmt::format("DELETE FROM real_estate_access_names WHERE property_id = {}", crate.id));
          query(database, fmt::format("DELETE FROM real_estate_properties WHERE id = {}", crate.id));
        }
        crateItem = std::move(instance);
      }
      releasePlot(database, plotId);
      transaction.commit();
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

    if (crateItem) {
      if (!crateCarriesProperty) {
        if (!client->PutItemInInventoryWithStacking(crateItem.get()) && !client->PushItemOnCursor(*crateItem, true)) {
          throw std::runtime_error(fmt::format("the empty crate from property [{}] could not be given to [{}]", plotId, client->GetCleanName()));
        }
      }
      else if (crateSlot != INVALID_INDEX) {
        if (!client->PutItemInInventory(crateSlot, *crateItem, true)) {
          throw std::runtime_error(fmt::format("crate [{}] could not be put in [{}]'s slot [{}]", crateId, client->GetCleanName(), crateSlot));
        }
      }
      else {
        LogHousing("[{}] has no room for crate [{}]; it is archived until a zone-in with room", client->GetCleanName(), crateId);
      }
    }
    if (payout.copper) {
      client->AddMoneyToPP(payout.copper, true);
      client->MessageString(Chat::White, withdrewCoin, std::to_string(payout.copper).c_str());
    }
    for (const auto &[currency, units] : payout.alternate) {
      client->AddAlternateCurrencyValue(currency, static_cast<int>(units));
      client->MessageString(Chat::White, withdrewAlternateCurrency, std::to_string(units).c_str(), std::to_string(currency).c_str());
    }
    sendResult(client, sold);
    LogHousing("[{}] sold property [{}]", client->GetCleanName(), plotId);
    sendPropertyChanged(plotId, PropertyChangedPayload{plot.owner_character_id, 0, plot.guild_id, 0});
    if (crateId) {
      sendPropertyChanged(crateId, PropertyChangedPayload{plot.owner_character_id, crateCarriesProperty ? client->CharacterID() : 0, 0, 0});
      sendEvent(EventType::itemListChanged, EventTarget::property, crateId, crateId, {});
    }
    sendEvent(EventType::itemListChanged, EventTarget::property, plotId, plotId, {});
  }
}
