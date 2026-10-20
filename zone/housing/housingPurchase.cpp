#include "housingHandlers.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/guilds.h"
#include "../../common/housingLedger.h"
#include "../../common/rulesys.h"
#include "../client.h"
#include "../guild_mgr.h"
#include "../zone.h"
#include "../zonedb.h"

#include <functional>
#include <stdexcept>

extern Zone *zone;

namespace housing {
  namespace {
    constexpr uint32 alreadyOwned = 6427;
    constexpr uint32 spentAlternateCurrency = 6428;
    constexpr uint32 spentCoin = 6429;
    constexpr uint32 purchased = 6430;
    constexpr uint32 maximumPlots = 728;
    constexpr uint32 maximumGuildPlots = 750;
    constexpr uint32 guildCannotBuy = 754;
    constexpr uint32 notInGuild = 755;
    constexpr uint32 needsHouseOfThule = 5315;
    constexpr uint32 guildNeighborhoodNonMember = 5317;
    constexpr uint32 guildTooSmall = 5426;
    constexpr uint32 needsVeilOfAlaris = 5430;

    struct CostItemPrice {
      uint32 currency;
      uint64 amount;
    };

    const CostOption &optionAt(uint32 costId, uint32 key, const char *use) {
      const Cost *cost = catalog().cost(costId);
      if (!cost || key >= cost->options.size()) {
        throw Refusal(fmt::format("{} option [{}] is not an enabled option of cost [{}]", use, key, costId), nullptr);
      }
      return cost->options[key];
    }

    void sendResult(Client *client, bool success) {
      WireWriter message;
      message.u8(success ? 1 : 0);
      sendMessage(client, OP_RealEstatePurchase, message);
    }
  }

  Refusal::Refusal(const std::string &reason, std::function<void(Client *)> tell) : std::runtime_error(reason), tell(std::move(tell)) {}

  std::function<void(Client *)> say(uint32 stringId, std::vector<std::string> arguments) {
    return [stringId, arguments](Client *client) mutable {
      arguments.resize(2);
      client->MessageString(Chat::Red, stringId, arguments[0].empty() ? nullptr : arguments[0].c_str(), arguments[1].empty() ? nullptr : arguments[1].c_str());
    };
  }

  void handlePurchase(Client *client, WireReader &reader) {
    PlotRequest request;
    if (!readPlotRequest(reader, request)) {
      LogError("Housing: [{}] sent a purchase of [{}] bytes, not [{}]", client->GetCleanName(), reader.remaining(), PlotRequest::size);
      return;
    }
    const uint32 propertyId = request.u32At(0x00);
    const uint32 priceKey = request.u32At(0xac);
    const uint32 upkeepKey = request.u32At(0xb0);
    Session *state = session(client);
    Property bought;
    CostItemPrice price{0, 0};
    try {
      if (!state || !state->properties.count(propertyId)) {
        throw Refusal("the property is not one this zone sent", nullptr);
      }
      Transaction transaction(database);
      bought = lockedProperty(database, propertyId);
      std::map<uint32, Property> fresh(state->properties);
      fresh[propertyId] = bought;
      auto lookup = [&fresh](uint32 id) -> const Property * {
        auto found = fresh.find(id);
        return found == fresh.end() ? nullptr : &found->second;
      };
      const DefinitionType type = typeOf(bought);
      const bool guildPlot = type == DefinitionType::guildPlot;
      if (type != DefinitionType::plot && !guildPlot) {
        throw Refusal("the property is not a plot", nullptr);
      }
      if (!hostedHere(bought)) {
        throw Refusal("the buyer is not in the plot's zone", nullptr);
      }
      if (guildPlot ? activeGuildId(bought) != 0 : bought.owner_character_id != 0) {
        throw Refusal("the plot is owned", say(alreadyOwned));
      }
      const uint32 guild = client->GuildID() == GUILD_NONE ? 0 : client->GuildID();
      if (!client->GetGM()) {
        if (guildPlot && !(client->GetPP().expansions & veilOfAlaris)) {
          throw Refusal("the buyer lacks Veil of Alaris", say(needsVeilOfAlaris));
        }
        if (!guildPlot && !(client->GetPP().expansions & houseOfThule)) {
          throw Refusal("the buyer lacks House of Thule", say(needsHouseOfThule));
        }
        const Property *neighborhood = ancestorOfType(bought, typeBit(DefinitionType::neighborhood), true, lookup);
        const uint32 neighborhoodGuild = neighborhood ? activeGuildId(*neighborhood) : 0;
        if (neighborhoodGuild && neighborhoodGuild != guild) {
          throw Refusal("the plot is in another guild's neighborhood", say(guildNeighborhoodNonMember));
        }
        if (guildPlot && guild) {
          if (!guild_mgr.CheckPermission(guild, client->GuildRank(), GUILD_ACTION_REAL_ESTATE_GUILD_PLOT_BUY)) {
            throw Refusal("the buyer's rank may not buy guild plots", say(guildCannotBuy));
          }
        }
      }
      if (guildPlot) {
        if (!guild) {
          throw Refusal("the buyer is in no guild", say(notInGuild));
        }
        const int limit = RuleI(Housing, MaxGuildPlotsPerGuild);
        if (guildPlotCount(database, content_db, guild) >= static_cast<uint32>(std::max(0, limit))) {
          throw Refusal("the guild holds its limit of guild plots", say(maximumGuildPlots, {std::to_string(limit)}));
        }
        const int minimum = RuleI(Housing, GuildPlotMinimumMembers);
        const uint32 members = static_cast<uint32>(guild_mgr.GetGuildMembers(guild).size());
        if (minimum > 0 && members < static_cast<uint32>(minimum) && !guildOwnsNeighborhood(guild)) {
          throw Refusal("the guild is too small", say(guildTooSmall, {std::to_string(members), std::to_string(minimum)}));
        }
      }
      else {
        const int limit = RuleI(Housing, MaxPlotsPerCharacter);
        if (ownedPlotCount(database, content_db, client->CharacterID()) >= static_cast<uint32>(std::max(0, limit))) {
          throw Refusal("the buyer holds the plot limit", say(maximumPlots, {std::to_string(limit)}));
        }
      }
      const Definition &definition = *catalog().definition(bought.definition_id);
      const CostOption &priceOption = optionAt(definition.row.cost_definition_id, priceKey, "price");
      if (priceOption.items.front().amount <= 0) {
        throw std::runtime_error(fmt::format("cost [{}] option [{}] is not a price", definition.row.cost_definition_id, priceKey));
      }
      price = CostItemPrice{priceOption.currency, static_cast<uint64>(priceOption.items.front().amount)};
      const uint32 upkeepCurrency = definition.row.upkeep_definition_id ? optionAt(definition.row.upkeep_definition_id, upkeepKey, "upkeep").currency : 0;
      const bool affordable = price.currency == 0
        ? client->HasMoney(price.amount)
        : client->GetAlternateCurrencyValue(price.currency) >= price.amount;
      if (!affordable) {
        throw Refusal("the buyer cannot pay the price", nullptr);
      }
      const int64 now = currentTime();
      const int jitter = RuleI(Housing, UpkeepJitterSeconds);
      const int64 nextCharge = now + nextChargeDelay(RuleI(Housing, UpkeepIntervalSeconds), jitter, zone->random.Int(-jitter, jitter));
      query(database, fmt::format(
        "UPDATE real_estate_properties SET owner_character_id = {}, guild_id = {}, upkeep_currency = {}, upkeep_expired_time = 0, "
        "upkeep_settled_at = FROM_UNIXTIME({}), next_upkeep_at = FROM_UNIXTIME({}), voting_opt_out = 0, revision = revision + 1 WHERE id = {}",
        guildPlot ? 0 : client->CharacterID(), guildPlot ? guild : 0, upkeepCurrency, now, nextCharge, propertyId
      ));
      writeDefaultAccessList(database, propertyId, guildPlot);
      transaction.commit();
      if (guildPlot) {
        bought.guild_id = guild;
      }
      else {
        bought.owner_character_id = client->CharacterID();
      }
    }
    catch (const Refusal &refusal) {
      LogHousing("Refused [{}]'s purchase of property [{}]: {}", client->GetCleanName(), propertyId, refusal.what());
      if (refusal.tell) {
        refusal.tell(client);
      }
      sendResult(client, false);
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s purchase of property [{}] failed: {}", client->GetCleanName(), propertyId, error.what());
      sendResult(client, false);
      return;
    }
    if (price.currency == 0) {
      if (!client->TakeMoneyFromPP(price.amount, true)) {
        throw std::logic_error(fmt::format("[{}] could pay [{}] copper a moment ago and cannot now", client->GetCleanName(), price.amount));
      }
      client->MessageString(Chat::White, spentCoin, std::to_string(price.amount).c_str());
    }
    else {
      client->AddAlternateCurrencyValue(price.currency, -static_cast<int>(price.amount));
      client->MessageString(Chat::White, spentAlternateCurrency, std::to_string(price.amount).c_str(), std::to_string(price.currency).c_str());
    }
    client->MessageString(Chat::White, purchased);
    sendResult(client, true);
    LogHousing("[{}] bought property [{}]", client->GetCleanName(), propertyId);
    sendPropertyChanged(propertyId, PropertyChangedPayload{0, bought.owner_character_id, 0, bought.guild_id});
  }
}
