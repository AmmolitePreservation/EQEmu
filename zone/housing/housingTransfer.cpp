#include "housingHandlers.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingLedger.h"
#include "../../common/housingPropertyItem.h"
#include "../../common/housingTransferOffers.h"
#include "../../common/rulesys.h"
#include "../client.h"
#include "../entity.h"
#include "../zone.h"
#include "../zonedb.h"

#include <stdexcept>

extern EntityList entity_list;

namespace housing {
  namespace {
    constexpr uint32 failed = 1;
    constexpr uint32 notOwner = 2;
    constexpr uint32 noTradeContents = 3;
    constexpr uint32 recipientAtLimit = 761;
    constexpr uint32 recipientOffline = 782;
    constexpr uint32 recipientNeedsHouseOfThule = 5316;
    constexpr uint32 recipientNotGuildMember = 5318;
    constexpr uint32 noTradeCurrency = 8081;
    constexpr uint32 recipientReceived = 735;
    constexpr uint32 ownerTransferred = 736;
    constexpr uint32 declined = 7638;
    constexpr size_t ownerNameOffset = 0x0c;
    constexpr size_t targetNameOffset = 0x68;
    constexpr size_t acceptedOffset = 0xb9;

    TransferOffers offers;

    struct TransferRefusal : std::runtime_error {
      TransferRefusal(const std::string &reason, uint32 code, uint32 stringId = 0) : std::runtime_error(reason), code(code), stringId(stringId) {}
      uint32 code;
      uint32 stringId;
    };

    void sendResult(Client *client, uint32 code) {
      WireWriter message;
      message.u32(code);
      sendMessage(client, OP_RealEstateTransferResult, message);
    }

    bool noTradeItem(uint32 itemId) {
      const EQ::ItemData *item = database.GetItem(itemId);
      if (!item) {
        throw std::runtime_error(fmt::format("item [{}] does not exist", itemId));
      }
      return !item->NoDrop;
    }

    void checkPlotContents(uint32 plotId) {
      for (const auto &row : RealEstateItemsRepository::GetWhere(database, fmt::format("property_id = {}", plotId))) {
        if (!carriedPropertyId(row.custom_data)) {
          continue;
        }
        if (row.instnodrop || noTradeItem(row.item_id)) {
          throw TransferRefusal("the plot holds a NO TRADE house or crate", noTradeContents);
        }
      }
    }

    void checkEscrowCurrencies(uint32 plotId) {
      for (const auto &[currency, amount] : readBalances(database, plotId)) {
        if (currency == 0 || amount <= 0.0) {
          continue;
        }
        const uint32 itemId = zone->GetCurrencyItemID(currency);
        if (!itemId) {
          throw std::runtime_error(fmt::format("escrow currency [{}] has no alternate currency item", currency));
        }
        if (noTradeItem(itemId)) {
          throw TransferRefusal("the escrow holds a NO TRADE currency", failed, noTradeCurrency);
        }
      }
    }

    void checkRecipient(Client *recipient, const Property &plot, const PropertyLookup &lookup) {
      const int limit = RuleI(Housing, MaxPlotsPerCharacter);
      if (ownedPlotCount(database, content_db, recipient->CharacterID()) >= static_cast<uint32>(std::max(0, limit))) {
        throw TransferRefusal("the recipient holds the plot limit", failed, recipientAtLimit);
      }
      if (!(recipient->GetPP().expansions & houseOfThule)) {
        throw TransferRefusal("the recipient lacks House of Thule", failed, recipientNeedsHouseOfThule);
      }
      const Property *neighborhood = ancestorOfType(plot, typeBit(DefinitionType::neighborhood), true, lookup);
      const uint32 neighborhoodGuild = neighborhood ? activeGuildId(*neighborhood) : 0;
      if (neighborhoodGuild && recipient->GuildID() != neighborhoodGuild) {
        throw TransferRefusal("the plot is in a guild neighborhood the recipient is not a member of", failed, recipientNotGuildMember);
      }
    }

    PropertyLookup lookupWith(const Session &state, std::map<uint32, Property> &fresh, const Property &plot) {
      fresh = state.properties;
      fresh[plot.id] = plot;
      return [&fresh](uint32 id) -> const Property * {
        auto found = fresh.find(id);
        return found == fresh.end() ? nullptr : &found->second;
      };
    }
  }

  void handleTransfer(Client *client, WireReader &reader) {
    PlotRequest request;
    if (!readPlotRequest(reader, request)) {
      LogError("Housing: [{}] sent a transfer of [{}] bytes, not [{}]", client->GetCleanName(), reader.remaining(), PlotRequest::size);
      return;
    }
    const uint32 plotId = request.u32At(0x00);
    const std::string targetName = request.textAt(targetNameOffset, 64);
    Session *state = session(client);
    Client *recipient = nullptr;
    Property plot;
    try {
      if (!state || !state->properties.count(plotId)) {
        throw TransferRefusal("the plot is not one this zone sent", failed);
      }
      plot = propertiesById({plotId}).at(0);
      std::map<uint32, Property> fresh;
      const PropertyLookup lookup = lookupWith(*state, fresh, plot);
      if (typeOf(plot) != DefinitionType::plot) {
        throw TransferRefusal("only player plots are transferred", failed);
      }
      if (!hostedHere(plot)) {
        throw TransferRefusal("the owner is not in the plot's zone", failed);
      }
      if (plot.owner_character_id != client->CharacterID()) {
        throw TransferRefusal("the sender is not the owner", notOwner);
      }
      recipient = entity_list.GetClientByName(targetName.c_str());
      if (!recipient) {
        throw TransferRefusal(fmt::format("[{}] is not in this zone", targetName), failed, recipientOffline);
      }
      if (recipient == client) {
        throw TransferRefusal("the owner targeted themselves", failed);
      }
      checkPlotContents(plotId);
      checkRecipient(recipient, plot, lookup);
      checkEscrowCurrencies(plotId);
      offers.expire(currentTime());
      if (offers.pending(plotId, currentTime())) {
        throw TransferRefusal("an offer for the plot is already waiting", failed);
      }
      offers.offer(plotId, client->CharacterID(), recipient->CharacterID(), plot.revision, currentTime(), RuleI(Housing, TransferOfferExpirySeconds));
    }
    catch (const TransferRefusal &refusal) {
      LogHousing("Refused [{}]'s transfer of property [{}] to [{}]: {}", client->GetCleanName(), plotId, targetName, refusal.what());
      if (refusal.stringId) {
        client->MessageString(Chat::Red, refusal.stringId);
      }
      sendResult(client, refusal.code);
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s transfer of property [{}] failed: {}", client->GetCleanName(), plotId, error.what());
      sendResult(client, failed);
      return;
    }
    WireWriter offer;
    offer.u32(plotId).u32(0xFFFFFFFF).u32(0xFFFFFFFF).fixedString(client->GetName(), 64);
    std::string rest(PlotRequest::size - 0x4c, '\0');
    offer.bytes(rest.data(), rest.size());
    sendMessage(recipient, OP_RealEstateTransferOffer, offer);
    sendResult(client, 0);
    LogHousing("[{}] offered property [{}] to [{}]", client->GetCleanName(), plotId, recipient->GetCleanName());
  }

  void handleTransferAnswer(Client *client, WireReader &reader) {
    PlotRequest request;
    if (!readPlotRequest(reader, request)) {
      LogError("Housing: [{}] answered a transfer with [{}] bytes, not [{}]", client->GetCleanName(), reader.remaining(), PlotRequest::size);
      return;
    }
    const uint32 plotId = request.u32At(0x00);
    const bool accepted = request.u8At(acceptedOffset) != 0;
    const auto offer = offers.answer(plotId, client->CharacterID(), currentTime());
    if (!offer) {
      LogHousing("[{}] answered a transfer of property [{}] that is not waiting for them", client->GetCleanName(), plotId);
      return;
    }
    Client *owner = entity_list.GetClientByCharID(offer->ownerCharacterId);
    if (!accepted) {
      if (owner) {
        owner->MessageString(Chat::White, declined);
      }
      LogHousing("[{}] declined property [{}]", client->GetCleanName(), plotId);
      return;
    }
    std::string name;
    try {
      Session *state = session(client);
      if (!state) {
        throw std::runtime_error("the recipient has no housing session here");
      }
      Transaction transaction(database);
      const Property plot = lockedProperty(database, plotId);
      if (plot.revision != offer->revision || plot.owner_character_id != offer->ownerCharacterId) {
        throw TransferRefusal("the plot changed since the offer", failed);
      }
      std::map<uint32, Property> fresh;
      checkPlotContents(plotId);
      checkRecipient(client, plot, lookupWith(*state, fresh, plot));
      checkEscrowCurrencies(plotId);
      query(database, fmt::format("UPDATE real_estate_properties SET owner_character_id = {}, revision = revision + 1 WHERE id = {}", client->CharacterID(), plotId));
      transaction.commit();
      name = propertyName(plotId);
    }
    catch (const TransferRefusal &refusal) {
      LogHousing("Refused [{}]'s acceptance of property [{}]: {}", client->GetCleanName(), plotId, refusal.what());
      if (refusal.stringId && owner) {
        owner->MessageString(Chat::Red, refusal.stringId);
      }
      if (owner) {
        sendResult(owner, refusal.code);
      }
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s acceptance of property [{}] failed: {}", client->GetCleanName(), plotId, error.what());
      if (owner) {
        sendResult(owner, failed);
      }
      return;
    }
    const auto identities = characterIdentities({offer->ownerCharacterId});
    const std::string ownerName = identities.count(offer->ownerCharacterId) ? identities.at(offer->ownerCharacterId).name : std::string();
    client->MessageString(Chat::White, recipientReceived, ownerName.c_str(), name.c_str());
    if (owner) {
      owner->MessageString(Chat::White, ownerTransferred, name.c_str(), client->GetName());
    }
    LogHousing("Property [{}] passed from character [{}] to [{}]", plotId, offer->ownerCharacterId, client->GetCleanName());
    sendPropertyChanged(plotId, PropertyChangedPayload{offer->ownerCharacterId, client->CharacterID(), 0, 0});
  }

  void expireTransferOffers() {
    for (const auto &offer : offers.expire(currentTime())) {
      for (uint32 characterId : {offer.ownerCharacterId, offer.recipientCharacterId}) {
        Client *party = entity_list.GetClientByCharID(characterId);
        if (party) {
          party->MessageString(Chat::White, declined);
        }
      }
      LogHousing("The transfer offer of property [{}] expired", offer.propertyId);
    }
  }
}
