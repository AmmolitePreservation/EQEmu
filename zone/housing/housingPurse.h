#ifndef EQEMU_ZONE_HOUSING_HOUSING_PURSE_H
#define EQEMU_ZONE_HOUSING_HOUSING_PURSE_H

#include "../../common/types.h"

#include <map>

class Client;

namespace housing {
  // The coin and alternate currency an act moves with its transaction (AUD-25). Coin changes in the character's memory and
  // is saved inside the transaction (TakeMoneyFromPP and AddMoneyToPP without a client update, the cursor fields with
  // SaveCurrency); alternate currency is written to its rows inside the transaction and reaches memory only through
  // committed(), which then shows both to the client. A guard destroyed before committed() puts the coin back in memory,
  // so an act whose transaction rolls back leaves the purse as the database holds it. Declare it inside the act's try
  // block: it must restore before a catch block reads the purse.
  class PurseGuard {
  public:
    explicit PurseGuard(Client *client);
    ~PurseGuard();
    PurseGuard(const PurseGuard &) = delete;
    PurseGuard &operator=(const PurseGuard &) = delete;

    void addCurrency(uint32 currencyId, int64 amount);
    void committed();

  private:
    struct Coin {
      int32 platinum;
      int32 gold;
      int32 silver;
      int32 copper;
      int32 platinumCursor;
      int32 goldCursor;
      int32 silverCursor;
      int32 copperCursor;
    };
    static Coin coinOf(Client *client);

    Client *client;
    const Coin before;
    std::map<uint32, int64> pendingCurrency;
    bool done = false;
  };
}

#endif
