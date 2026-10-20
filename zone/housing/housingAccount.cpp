#include "housingHandlers.h"

#include "housingAccess.h"
#include "housingCatalog.h"
#include "housingEvents.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingLedger.h"
#include "../client.h"
#include "../zone.h"
#include "../zonedb.h"

#include <stdexcept>

extern Zone *zone;

namespace housing {
  namespace {
    constexpr uint32 depositFromCursor = 0;
    constexpr uint32 withdrawAmount = 1;
    constexpr uint32 setUpkeepOption = 2;
    constexpr uint32 addDayAll = 3;
    constexpr uint32 addWeekAll = 4;
    constexpr uint32 addDayItem = 5;
    constexpr uint32 addWeekItem = 6;
    constexpr uint32 withdrawAll = 7;
    constexpr uint32 depositedAlternateCurrency = 6423;
    constexpr uint32 depositedCoin = 6424;
    constexpr uint32 withdrewAlternateCurrency = 6425;
    constexpr uint32 withdrewCoin = 6426;
    constexpr uint64 copperPerDenomination[] = {1, 10, 100, 1000};

    struct AccountRequest {
      uint32 type;
      uint32 propertyId;
      uint32 argument;
      uint8 flag;
      std::vector<std::pair<uint32, std::pair<uint32, uint64>>> costItems;
    };

    bool readAccountRequest(WireReader &reader, AccountRequest &request) {
      request.type = reader.u32();
      request.propertyId = reader.u32();
      request.argument = reader.u32();
      reader.cString();
      reader.cString();
      request.flag = reader.u8();
      reader.u32();
      const uint32 count = reader.u32();
      if (count > 16) {
        return false;
      }
      for (uint32 index = 0; index < count && !reader.failed(); index++) {
        const uint32 currency = reader.u32();
        const uint32 subKey = reader.u32();
        const uint64 amount = reader.u64();
        request.costItems.push_back({currency, {subKey, amount}});
      }
      return !reader.failed() && reader.remaining() == 0;
    }

    WireWriter accountMessage(uint32 type, uint32 propertyId, uint32 argument, uint8 flag, const std::vector<std::pair<uint32, std::pair<uint32, uint64>>> &costItems) {
      WireWriter message;
      message.u32(type).u32(propertyId).u32(argument).cString("").cString("").u8(flag).u32(0).u32(static_cast<uint32>(costItems.size()));
      for (const auto &[currency, value] : costItems) {
        message.u32(currency).u32(value.first).u64(value.second);
      }
      return message;
    }

    int32 *cursorCoin(Client *client, uint32 denomination) {
      auto &profile = client->GetPP();
      switch (denomination) {
        case 0: return &profile.copper_cursor;
        case 1: return &profile.silver_cursor;
        case 2: return &profile.gold_cursor;
        case 3: return &profile.platinum_cursor;
      }
      throw std::invalid_argument(fmt::format("coin denomination [{}] is not 0 to 3", denomination));
    }

    std::set<uint32> escrowCurrencies() {
      std::set<uint32> currencies;
      for (const auto &[id, definition] : catalog().allDefinitions()) {
        for (uint32 costId : {definition.row.cost_definition_id, definition.row.upkeep_definition_id}) {
          const Cost *cost = catalog().cost(costId);
          if (!cost) {
            continue;
          }
          for (const auto &option : cost->options) {
            currencies.insert(option.currency);
          }
        }
      }
      return currencies;
    }

    struct Credit {
      std::map<uint32, double> amounts;
    };

    // Live credits a deposit and then charges the time since the last charge or deposit at the current daily total, so the time is
    // paid from the deposit even on an empty escrow (UPK-8); the placed stamps clear and the next charge stays where it was.
    void depositInto(const Property &plot, const Credit &credit) {
      auto balances = readBalances(database, plot.id);
      for (const auto &[currency, amount] : credit.amounts) {
        balances[currency] += amount;
      }
      writeBalances(database, plot.id, balances);
      settleUpkeep(database, content_db, plot, currentTime());
      clearPlacedUpkeepStamps(database, plot, placedUnder(database, content_db, plot));
      bumpRevision(database, plot.id);
    }

    std::map<uint32, int64> dailyTotal(const Property &plot, int32 upkeepCostId) {
      if (upkeepCostId < 0) {
        const auto contents = placedUnder(database, content_db, plot);
        return netDailyCost(upkeepCostItems(database, content_db, plot, contents));
      }
      const auto contents = placedUnder(database, content_db, plot);
      const auto upkeepIds = itemUpkeepIds(content_db, [&]() {
        std::set<uint32> ids;
        for (const auto &row : contents.placedItems) {
          ids.insert(row.item_id);
        }
        return ids;
      }());
      bool placed = false;
      for (const auto &[itemId, id] : upkeepIds) {
        placed = placed || id == upkeepCostId;
      }
      if (!placed) {
        throw Refusal(fmt::format("no item placed on the plot pays upkeep [{}]", upkeepCostId), nullptr);
      }
      const Cost *cost = catalog().cost(static_cast<uint32>(upkeepCostId));
      if (!cost || cost->options.empty()) {
        throw std::runtime_error(fmt::format("upkeep [{}] has no enabled option", upkeepCostId));
      }
      return netDailyCost({{cost->options.front().currency, cost->options.front().items.front().amount}});
    }

    // The client clears its cursor coin when it sends a deposit; a refused one is put back with type 0, flag 0.
    void returnCursorCoin(Client *client, const AccountRequest &request) {
      if (request.type != depositFromCursor || request.costItems.size() != 1 || request.costItems.front().first != 0) {
        return;
      }
      const uint32 denomination = request.costItems.front().second.first;
      if (denomination > 3) {
        return;
      }
      const int32 held = *cursorCoin(client, denomination);
      sendMessage(client, OP_RealEstateAccount, accountMessage(depositFromCursor, request.propertyId, 0, 0, {{0, {denomination, static_cast<uint64>(std::max(0, held))}}}));
    }

    void tellDeposit(Client *client, uint32 currency, double amount) {
      const std::string units = std::to_string(flooredUnits(amount));
      if (currency == 0) {
        client->MessageString(Chat::White, depositedCoin, units.c_str());
      }
      else {
        client->MessageString(Chat::White, depositedAlternateCurrency, units.c_str(), std::to_string(currency).c_str());
      }
    }

    void tellWithdrawal(Client *client, uint32 currency, uint64 units) {
      if (currency == 0) {
        client->MessageString(Chat::White, withdrewCoin, std::to_string(units).c_str());
      }
      else {
        client->MessageString(Chat::White, withdrewAlternateCurrency, std::to_string(units).c_str(), std::to_string(currency).c_str());
      }
    }
  }

  void handleAccount(Client *client, WireReader &reader) {
    AccountRequest request;
    if (!readAccountRequest(reader, request)) {
      LogError("Housing: [{}] sent a malformed account request", client->GetCleanName());
      return;
    }
    Session *state = session(client);
    Property plot;
    std::function<void()> afterCommit;
    try {
      if (!state || !state->properties.count(request.propertyId)) {
        throw Refusal("the property is not one this zone sent", nullptr);
      }
      Transaction transaction(database);
      plot = lockedProperty(database, request.propertyId);
      const DefinitionType type = typeOf(plot);
      if (type != DefinitionType::plot && type != DefinitionType::guildPlot) {
        throw Refusal("the property is not a plot", nullptr);
      }
      if (type == DefinitionType::plot ? !plot.owner_character_id : !activeGuildId(plot)) {
        throw Refusal("the plot is not owned", nullptr);
      }
      std::map<uint32, Property> fresh(state->properties);
      fresh[plot.id] = plot;
      auto lookup = [&fresh](uint32 id) -> const Property * {
        auto found = fresh.find(id);
        return found == fresh.end() ? nullptr : &found->second;
      };
      auto lists = accessLists({plot.id});
      auto listOf = [&lists](uint32 id) -> const AccessList * {
        auto found = lists.find(id);
        return found == lists.end() ? nullptr : &found->second;
      };
      switch (request.type) {
        case depositFromCursor: {
          if (request.costItems.size() != 1) {
            throw Refusal("a deposit carries one cost item", nullptr);
          }
          const auto &[currency, value] = request.costItems.front();
          const auto &[subKey, amount] = value;
          if (!escrowCurrencies().count(currency)) {
            throw Refusal(fmt::format("currency [{}] is not one the cost table uses", currency), nullptr);
          }
          if (amount == 0 || amount > 0x7FFFFFFF) {
            throw Refusal(fmt::format("deposit amount [{}] is out of range", amount), nullptr);
          }
          Credit credit;
          if (currency == 0) {
            int32 *coin = cursorCoin(client, subKey);
            if (static_cast<uint64>(std::max(0, *coin)) < amount) {
              throw Refusal(fmt::format("the cursor holds [{}] of denomination [{}], not [{}]", *coin, subKey, amount), nullptr);
            }
            credit.amounts[0] = static_cast<double>(amount * copperPerDenomination[subKey]);
            depositInto(plot, credit);
            transaction.commit();
            *coin -= static_cast<int32>(amount);
            client->SaveCurrency();
          }
          else {
            const EQ::ItemInstance *cursor = client->GetInv().GetItem(EQ::invslot::slotCursor);
            const uint32 currencyItem = zone->GetCurrencyItemID(currency);
            if (!cursor || !currencyItem || cursor->GetID() != currencyItem || static_cast<uint64>(std::max<int16>(1, cursor->GetCharges())) < amount) {
              throw Refusal(fmt::format("the cursor does not hold [{}] of currency [{}]", amount, currency), nullptr);
            }
            credit.amounts[currency] = static_cast<double>(amount);
            depositInto(plot, credit);
            transaction.commit();
            client->DeleteItemInInventory(EQ::invslot::slotCursor, static_cast<int16>(amount), true);
          }
          tellDeposit(client, currency, credit.amounts.begin()->second);
          break;
        }
        case withdrawAmount:
        case withdrawAll: {
          if (!permitted(client, Permission::withdrawUpkeep, plot, lookup, listOf)) {
            throw Refusal("the player may not withdraw upkeep", nullptr);
          }
          auto balances = readBalances(database, plot.id);
          std::map<uint32, uint64> units;
          if (request.type == withdrawAmount) {
            if (request.costItems.size() != 1) {
              throw Refusal("a withdrawal carries one cost item", nullptr);
            }
            const auto &[currency, value] = request.costItems.front();
            const auto &[subKey, amount] = value;
            if (currency == 0 && subKey > 3) {
              throw Refusal(fmt::format("coin denomination [{}] is not 0 to 3", subKey), nullptr);
            }
            const uint64 wanted = currency == 0 ? amount * copperPerDenomination[subKey] : amount;
            if (wanted == 0 || static_cast<int64>(wanted) > flooredUnits(balances[currency])) {
              throw Refusal(fmt::format("the escrow holds less than [{}] of currency [{}]", wanted, currency), nullptr);
            }
            units[currency] = wanted;
          }
          else {
            for (const auto &[currency, amount] : balances) {
              if (flooredUnits(amount) > 0) {
                units[currency] = static_cast<uint64>(flooredUnits(amount));
              }
            }
          }
          for (const auto &[currency, count] : units) {
            balances[currency] -= static_cast<double>(count);
          }
          writeBalances(database, plot.id, balances);
          bumpRevision(database, plot.id);
          transaction.commit();
          for (const auto &[currency, count] : units) {
            if (currency != 0) {
              client->AddAlternateCurrencyValue(currency, static_cast<int>(count));
            }
            else {
              uint64 left = count;
              for (int denomination = 3; denomination >= 0; denomination--) {
                const uint64 coins = left / copperPerDenomination[denomination];
                left %= copperPerDenomination[denomination];
                if (!coins) {
                  continue;
                }
                int32 *coin = cursorCoin(client, static_cast<uint32>(denomination));
                *coin += static_cast<int32>(coins);
                sendMessage(client, OP_RealEstateAccount, accountMessage(withdrawAmount, plot.id, 0, 1, {{0, {static_cast<uint32>(denomination), static_cast<uint64>(*coin)}}}));
              }
              client->SaveCurrency();
            }
            tellWithdrawal(client, currency, count);
          }
          break;
        }
        case setUpkeepOption: {
          if (!permitted(client, Permission::manage, plot, lookup, listOf)) {
            throw Refusal("the player has no Manage permission", nullptr);
          }
          const uint32 costId = catalog().definition(plot.definition_id)->row.upkeep_definition_id;
          const Cost *cost = catalog().cost(costId);
          if (!cost || request.argument >= cost->options.size()) {
            throw Refusal(fmt::format("upkeep option [{}] is not an enabled option of cost [{}]", request.argument, costId), nullptr);
          }
          query(database, fmt::format("UPDATE real_estate_properties SET upkeep_currency = {}, revision = revision + 1 WHERE id = {}", cost->options[request.argument].currency, plot.id));
          transaction.commit();
          sendMessage(client, OP_RealEstateAccount, accountMessage(setUpkeepOption, plot.id, request.argument, 0, {}));
          break;
        }
        case addDayAll:
        case addWeekAll:
        case addDayItem:
        case addWeekItem: {
          const int64 days = request.type == addWeekAll || request.type == addWeekItem ? 7 : 1;
          const bool oneItem = request.type == addDayItem || request.type == addWeekItem;
          const auto daily = dailyTotal(plot, oneItem ? static_cast<int32>(request.argument) : -1);
          const int percent = upkeepPercent(database, content_db, plot.host_zone_id);
          Credit credit;
          for (const auto &[currency, amount] : daily) {
            const int64 owed = flooredUnits(proratedCharge(amount, percent, days * secondsPerDay));
            if (owed > 0) {
              credit.amounts[currency] = static_cast<double>(owed);
            }
          }
          if (credit.amounts.empty()) {
            throw Refusal("there is no upkeep to pay ahead", nullptr);
          }
          for (const auto &[currency, amount] : credit.amounts) {
            const bool affordable = currency == 0
              ? client->HasMoney(static_cast<uint64>(amount))
              : client->GetAlternateCurrencyValue(currency) >= static_cast<uint32>(amount);
            if (!affordable) {
              throw Refusal(fmt::format("the player cannot pay [{}] of currency [{}]", amount, currency), nullptr);
            }
          }
          depositInto(plot, credit);
          transaction.commit();
          for (const auto &[currency, amount] : credit.amounts) {
            if (currency == 0) {
              if (!client->TakeMoneyFromPP(static_cast<uint64>(amount), true)) {
                throw std::logic_error(fmt::format("[{}] could pay [{}] copper a moment ago and cannot now", client->GetCleanName(), amount));
              }
            }
            else {
              client->AddAlternateCurrencyValue(currency, -static_cast<int>(amount));
            }
            tellDeposit(client, currency, amount);
          }
          break;
        }
        default:
          throw Refusal(fmt::format("account request type [{}] is not one the client sends", request.type), nullptr);
      }
    }
    catch (const Refusal &refusal) {
      LogHousing("Refused [{}]'s account request [{}] on property [{}]: {}", client->GetCleanName(), request.type, request.propertyId, refusal.what());
      returnCursorCoin(client, request);
      return;
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s account request [{}] on property [{}] failed: {}", client->GetCleanName(), request.type, request.propertyId, error.what());
      returnCursorCoin(client, request);
      return;
    }
    LogHousing("[{}] made account request [{}] on property [{}]", client->GetCleanName(), request.type, request.propertyId);
    sendPropertyChanged(plot.id, PropertyChangedPayload{plot.owner_character_id, plot.owner_character_id, plot.guild_id, plot.guild_id});
  }
}
