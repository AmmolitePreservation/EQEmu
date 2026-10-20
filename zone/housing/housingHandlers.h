#ifndef EQEMU_ZONE_HOUSING_HOUSING_HANDLERS_H
#define EQEMU_ZONE_HOUSING_HOUSING_HANDLERS_H

#include "housingStore.h"
#include "housingWire.h"

#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

class Client;

namespace housing {
  // The client's 188-byte real estate request (0x45be, 0x792c, 0x507f, 0x5a63; constructor 0x5ad6f0).
  struct PlotRequest {
    static constexpr size_t size = 188;
    std::string raw;

    uint32 u32At(size_t offset) const {
      uint32 value;
      memcpy(&value, raw.data() + offset, sizeof(value));
      return value;
    }

    uint8 u8At(size_t offset) const {
      return static_cast<uint8>(raw[offset]);
    }

    std::string textAt(size_t offset, size_t length) const {
      const char *start = raw.data() + offset;
      return std::string(start, strnlen(start, length));
    }
  };

  // A request the server turns down, with the chat line the client shows for it, if any.
  struct Refusal : std::runtime_error {
    Refusal(const std::string &reason, std::function<void(Client *)> tell);
    std::function<void(Client *)> tell;
  };

  std::function<void(Client *)> say(uint32 stringId, std::vector<std::string> arguments = {});

  inline bool readPlotRequest(WireReader &reader, PlotRequest &request) {
    if (reader.remaining() != PlotRequest::size) {
      return false;
    }
    request.raw.resize(PlotRequest::size);
    for (size_t index = 0; index < PlotRequest::size; index++) {
      request.raw[index] = static_cast<char>(reader.u8());
    }
    return !reader.failed();
  }

  void handleSearch(Client *client, WireReader &reader);
  void sendPropertyWindow(Client *client, const Property &property, bool openPurchase);
  void handlePurchase(Client *client, WireReader &reader);
  void handleSell(Client *client, WireReader &reader);
  void deliverPendingCrates(Client *client);
  void handleTransfer(Client *client, WireReader &reader);
  void handleTransferAnswer(Client *client, WireReader &reader);
  void expireTransferOffers();
  void handleAccount(Client *client, WireReader &reader);
}

#endif
