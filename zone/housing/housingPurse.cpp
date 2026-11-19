#include "housingPurse.h"

#include "../../common/housingLedger.h"
#include "../client.h"
#include "../zonedb.h"

#include <limits>
#include <stdexcept>

namespace housing {
  PurseGuard::Coin PurseGuard::coinOf(Client *client) {
    const auto &profile = client->GetPP();
    return Coin{profile.platinum, profile.gold, profile.silver, profile.copper, profile.platinum_cursor, profile.gold_cursor,
      profile.silver_cursor, profile.copper_cursor};
  }

  PurseGuard::PurseGuard(Client *client) : client(client), before(coinOf(client)) {
  }

  PurseGuard::~PurseGuard() {
    if (done) {
      return;
    }
    auto &profile = client->GetPP();
    profile.platinum = before.platinum;
    profile.gold = before.gold;
    profile.silver = before.silver;
    profile.copper = before.copper;
    profile.platinum_cursor = before.platinumCursor;
    profile.gold_cursor = before.goldCursor;
    profile.silver_cursor = before.silverCursor;
    profile.copper_cursor = before.copperCursor;
  }

  void PurseGuard::addCurrency(uint32 currencyId, int64 amount) {
    if (!client->IsAlternateCurrencyLoaded()) {
      throw std::runtime_error(fmt::format("[{}]'s alternate currencies are not loaded yet", client->GetCleanName()));
    }
    const int64 value = static_cast<int64>(client->GetAlternateCurrencyValue(currencyId)) + pendingCurrency[currencyId] + amount;
    if (value < 0 || value > std::numeric_limits<int32>::max()) {
      throw std::runtime_error(fmt::format("[{}]'s currency [{}] would become [{}]", client->GetCleanName(), currencyId, value));
    }
    query(database, fmt::format("REPLACE INTO character_alt_currency (char_id, currency_id, amount) VALUES ({}, {}, {})",
      client->CharacterID(), currencyId, value));
    pendingCurrency[currencyId] += amount;
  }

  void PurseGuard::committed() {
    done = true;
    const Coin after = coinOf(client);
    if (after.platinum != before.platinum || after.gold != before.gold || after.silver != before.silver || after.copper != before.copper) {
      client->SendMoneyUpdate();
    }
    for (const auto &[currencyId, amount] : pendingCurrency) {
      if (amount) {
        client->AddAlternateCurrencyValue(currencyId, static_cast<int>(amount));
      }
    }
  }
}
