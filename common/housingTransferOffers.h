#ifndef EQEMU_COMMON_HOUSING_TRANSFER_OFFERS_H
#define EQEMU_COMMON_HOUSING_TRANSFER_OFFERS_H

#include "types.h"

#include <fmt/format.h>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

namespace housing {
  struct TransferOffer {
    uint32 propertyId;
    uint32 ownerCharacterId;
    uint32 recipientCharacterId;
    uint32 revision;
    int64 expiresAt;
  };

  // Offers wait in the owner's zone, where the recipient is (Live: same zone, 300 s, eqstr 7638 to both on expiry).
  class TransferOffers {
  public:
    bool pending(uint32 propertyId, int64 now) const {
      auto found = offers.find(propertyId);
      return found != offers.end() && found->second.expiresAt > now;
    }

    const TransferOffer &offer(uint32 propertyId, uint32 ownerCharacterId, uint32 recipientCharacterId, uint32 revision, int64 now, int expirySeconds) {
      if (expirySeconds <= 0) {
        throw std::invalid_argument(fmt::format("Housing:TransferOfferExpirySeconds is [{}]; it must be above 0", expirySeconds));
      }
      if (pending(propertyId, now)) {
        throw std::logic_error(fmt::format("property [{}] already has a transfer offer waiting", propertyId));
      }
      offers[propertyId] = TransferOffer{propertyId, ownerCharacterId, recipientCharacterId, revision, now + expirySeconds};
      return offers[propertyId];
    }

    std::optional<TransferOffer> answer(uint32 propertyId, uint32 recipientCharacterId, int64 now) {
      auto found = offers.find(propertyId);
      if (found == offers.end() || found->second.recipientCharacterId != recipientCharacterId || found->second.expiresAt <= now) {
        return std::nullopt;
      }
      TransferOffer offer = found->second;
      offers.erase(found);
      return offer;
    }

    std::vector<TransferOffer> expire(int64 now) {
      std::vector<TransferOffer> expired;
      for (auto offer = offers.begin(); offer != offers.end();) {
        if (offer->second.expiresAt <= now) {
          expired.push_back(offer->second);
          offer = offers.erase(offer);
        }
        else {
          ++offer;
        }
      }
      return expired;
    }

  private:
    std::map<uint32, TransferOffer> offers;
  };
}

#endif
