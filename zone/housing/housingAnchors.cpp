#include "housing.h"

#include "housingAccess.h"
#include "housingAnchors.h"
#include "housingInteriors.h"
#include "housingItems.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/spdat.h"
#include "../client.h"
#include "../mob.h"
#include "../zonedb.h"

#include <map>
#include <stdexcept>

namespace housing {
  namespace {
    constexpr uint32 primaryAnchor = 52584;
    constexpr uint32 secondaryAnchor = 52585;
    constexpr uint32 guildHallAnchor = 50874;
    constexpr size_t answerSize = 0x54;
    constexpr size_t nameLength = 64;
    constexpr uint32 none = 0xFFFFFFFF;
    // Live landed 2.9 to 3.8 above the anchor (INT-7).
    constexpr float landingHeight = 3.5f;
    // The client's anchor dialog closes after 30 s (0x4d3aa0).
    constexpr int64 offerSeconds = 30;

    constexpr uint32 selfNoAccess = 5346;
    constexpr uint32 selfGone = 5349;
    constexpr uint32 selfNotInYard = 5350;
    constexpr uint32 selfUpkeepExpired = 5351;
    constexpr uint32 offerNoAccess = 5330;
    constexpr uint32 offerNotInYard = 5347;
    constexpr uint32 offerUpkeepExpired = 5348;
    constexpr uint32 noAnchor = 5453;

    struct Offer {
      uint32 propertyId;
      uint32 recordId;
      int64 expiresAt;
    };

    std::map<uint32, Offer> offers;

    // A personal anchor is the one its owner placed (Lore: one of a kind, INT-7); a Guild Hall Anchor is the guild's, in its
    // guild plot or the hall on it. Of several Guild Hall Anchors the first placed is used (INT-21).
    std::optional<PlacedItem> anchorFor(Client *owner, uint32 anchorItemId) {
      std::string where;
      if (anchorItemId == guildHallAnchor) {
        if (!owner->IsInAGuild()) {
          return std::nullopt;
        }
        where = fmt::format(
          "property_id IN (SELECT id FROM real_estate_properties WHERE guild_id = {0} UNION SELECT c.id FROM real_estate_properties c "
          "JOIN real_estate_properties p ON p.id = c.parent_id WHERE p.guild_id = {0})", owner->GuildID()
        );
      }
      else if (anchorItemId == primaryAnchor || anchorItemId == secondaryAnchor) {
        where = fmt::format("owner_character_id = {}", owner->CharacterID());
      }
      else {
        throw std::runtime_error(fmt::format("anchor effect names item [{}], which is no anchor", anchorItemId));
      }
      auto rows = RealEstateItemsRepository::GetWhere(database, fmt::format("item_id = {} AND placed = 1 AND {} ORDER BY id LIMIT 1", anchorItemId, where));
      return rows.empty() ? std::nullopt : std::optional<PlacedItem>(rows.front());
    }

    glm::vec4 landingOf(const PlacedItem &anchor, const Client *client) {
      return glm::vec4(anchor.x, anchor.y, anchor.z + landingHeight, client->GetHeading());
    }

    void sendOffer(Client *target, Mob *caster, const PlacedItem &anchor) {
      const std::string location = propertyName(anchor.property_id);
      WireWriter message;
      // Guild id -1 always: Live's Guild Hall Anchor prompt was 5329, which RoF2 shows only for a player anchor (0x4d37c8).
      message.cString(std::string(caster->GetCleanName()).substr(0, nameLength - 1))
        .u32(0)
        .u32(anchor.property_id)
        .u32(anchor.id)
        .u32(none)
        .u32(none)
        .lengthString(location);
      sendMessage(target, OP_RealEstateAnchorTeleport, message);
      offers[target->CharacterID()] = Offer{anchor.property_id, anchor.id, currentTime() + offerSeconds};
      LogHousing("Offered [{}] the anchor of record [{}] in property [{}]", target->GetCleanName(), anchor.id, anchor.property_id);
    }

    bool refuseOn(Client *client, EntryState state, uint32 propertyId, bool self, Mob *caster) {
      if (state == EntryState::open) {
        return false;
      }
      const std::string name = state == EntryState::gone ? std::string() : propertyName(propertyId);
      if (self) {
        const uint32 message = state == EntryState::gone ? selfGone : state == EntryState::notInYard ? selfNotInYard
          : state == EntryState::upkeepExpired ? selfUpkeepExpired : selfNoAccess;
        client->MessageString(Chat::Red, message, name.c_str());
      }
      else {
        const uint32 message = state == EntryState::notInYard ? offerNotInYard : state == EntryState::upkeepExpired ? offerUpkeepExpired : offerNoAccess;
        client->MessageString(Chat::Red, message, caster->GetCleanName(), name.c_str());
      }
      return true;
    }

    void moveToAnchor(Client *client, const PlacedItem &anchor, EntryCheck check) {
      moveInto(client, *check.property, landingOf(anchor, client));
    }
  }

  // 437 sends the target to the caster's anchor, 438 to the target's own; a self Gate moves at once, every other cast asks (INT-7).
  void handleAnchorEffect(Mob *caster, Mob *target, uint16 spellEffect, uint16 spellId, int32 anchorItemId) {
    if (!housingActive() || !caster || !target || !target->IsClient()) {
      return;
    }
    Client *client = target->CastToClient();
    try {
      Client *owner = spellEffect == SE_TranslocatetoAnchor ? client : caster->IsClient() ? caster->CastToClient() : nullptr;
      if (!owner) {
        LogHousing("Anchor spell [{}] cast by a non-player on [{}]; nothing happens", spellId, client->GetCleanName());
        return;
      }
      const auto anchor = anchorFor(owner, static_cast<uint32>(anchorItemId));
      if (!anchor) {
        client->MessageString(Chat::Red, noAnchor, caster->GetCleanName());
        return;
      }
      const bool selfGate = caster == target && spells[spellId].target_type == ST_Self;
      const EntryCheck check = checkEntry(client, anchor->property_id);
      if (refuseOn(client, check.state, anchor->property_id, selfGate, caster)) {
        return;
      }
      if (selfGate) {
        moveToAnchor(client, *anchor, check);
        return;
      }
      sendOffer(client, caster, *anchor);
    }
    catch (const std::exception &error) {
      LogError("Housing: anchor spell [{}] on [{}] failed: {}", spellId, client->GetCleanName(), error.what());
    }
  }

  // The client echoes the offer's 0x54 bytes on Yes and sends nothing on No (README, 0x7bb1).
  void handleAnchorAnswer(Client *client, WireReader &reader) {
    if (reader.remaining() != answerSize) {
      LogError("Housing: [{}] sent a [{}]-byte anchor answer; it is [{}] bytes", client->GetCleanName(), reader.remaining(), answerSize);
      return;
    }
    reader.skip(nameLength);
    reader.u32();
    const uint32 propertyId = reader.u32();
    const uint32 recordId = reader.u32();
    auto offer = offers.find(client->CharacterID());
    if (offer == offers.end() || offer->second.propertyId != propertyId || offer->second.recordId != recordId || currentTime() > offer->second.expiresAt) {
      LogHousing("[{}] accepted an anchor teleport to record [{}] of property [{}] they were not offered", client->GetCleanName(), recordId, propertyId);
      return;
    }
    offers.erase(offer);
    try {
      const auto anchor = placedItem(propertyId, recordId);
      const EntryCheck check = checkEntry(client, propertyId);
      if (!anchor || !anchor->placed) {
        client->MessageString(Chat::Red, selfGone, check.property ? propertyName(propertyId).c_str() : "");
        return;
      }
      if (refuseOn(client, check.state, propertyId, true, client)) {
        return;
      }
      moveToAnchor(client, *anchor, check);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s anchor teleport to record [{}] failed: {}", client->GetCleanName(), recordId, error.what());
    }
  }

  void forgetAnchorOffer(Client *client) {
    offers.erase(client->CharacterID());
  }
}
