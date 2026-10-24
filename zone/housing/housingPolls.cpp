#include "housingPolls.h"

#include "housingAccess.h"
#include "housingEvents.h"
#include "housingInteriors.h"
#include "housingPlacement.h"
#include "housingSession.h"

#include "../../common/eqemu_logsys.h"
#include "../../common/housingLedger.h"
#include "../../common/housingPollRules.h"
#include "../../common/rulesys.h"
#include "../../common/strings.h"
#include "../../common/timer.h"
#include "../client.h"
#include "../entity.h"
#include "../zonedb.h"

#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

extern EntityList entity_list;

namespace housing {
  namespace {
    constexpr size_t optOutSize = 8;
    constexpr size_t teleportSize = 16;
    // Prompts and the Status column follow each poll's dates, so the zone looks for a poll opening or closing this often.
    Timer stateTimer(30000);

    struct Poll {
      uint32 id;
      PollKind kind;
      std::string question;
      std::string location;
      PollSelection selection;
      uint32 maxSelections;
      int64 opensAt;
      int64 closesAt;
      bool archived;
      bool resultsUnpublished;
      bool prompt;
      std::vector<std::pair<uint32, uint32>> options;
    };

    std::map<uint32, PollState> lastStates;

    std::vector<Poll> publishedPolls() {
      std::vector<Poll> polls;
      std::map<uint32, size_t> index;
      for (auto row : query(database,
        "SELECT id, kind, question, polling_location, selection_type, max_selections, UNIX_TIMESTAMP(opens_at), UNIX_TIMESTAMP(closes_at), archived, "
        "results_unpublished, prompt_players FROM real_estate_polls WHERE published = 1 ORDER BY id")) {
        Poll poll{Strings::ToUnsignedInt(row[0]), static_cast<PollKind>(Strings::ToUnsignedInt(row[1])), row[2], row[3],
                  static_cast<PollSelection>(Strings::ToUnsignedInt(row[4])), Strings::ToUnsignedInt(row[5]), Strings::ToBigInt(row[6]),
                  Strings::ToBigInt(row[7]), Strings::ToUnsignedInt(row[8]) != 0, Strings::ToUnsignedInt(row[9]) != 0, Strings::ToUnsignedInt(row[10]) != 0, {}};
        index[poll.id] = polls.size();
        polls.push_back(poll);
      }
      for (auto row : query(database, "SELECT o.poll_id, o.option_key, o.position FROM real_estate_poll_options o JOIN real_estate_polls p ON p.id = o.poll_id "
        "WHERE p.published = 1 ORDER BY o.poll_id, o.position")) {
        auto found = index.find(Strings::ToUnsignedInt(row[0]));
        if (found != index.end()) {
          polls[found->second].options.emplace_back(Strings::ToUnsignedInt(row[1]), Strings::ToUnsignedInt(row[2]));
        }
      }
      return polls;
    }

    PollState stateOf(const Poll &poll, int64 now) {
      return pollState(true, poll.archived, poll.opensAt, poll.closesAt, now);
    }

    std::map<uint32, std::string> labels(const std::vector<Poll> &polls) {
      std::set<uint32> keys;
      for (const auto &poll : polls) {
        if (poll.kind == PollKind::question) {
          for (const auto &[key, position] : poll.options) {
            keys.insert(key);
          }
        }
      }
      std::map<uint32, std::string> texts;
      if (keys.empty()) {
        return texts;
      }
      std::vector<std::string> ids;
      for (uint32 key : keys) {
        ids.push_back(std::to_string(key));
      }
      for (auto row : query(database, fmt::format("SELECT id, text FROM real_estate_poll_labels WHERE id IN ({})", Strings::Join(ids, ",")))) {
        texts[Strings::ToUnsignedInt(row[0])] = row[1];
      }
      return texts;
    }

    // Every House of Thule character votes on an open poll of either kind; the ballot is the account's (POL-5).
    bool mayVote(Client *client, const Poll &poll, int64 now) {
      return stateOf(poll, now) == PollState::open && (client->GetPP().expansions & houseOfThule) != 0;
    }

    std::map<uint32, std::vector<uint32>> votesOf(uint32 accountId) {
      std::map<uint32, std::vector<uint32>> votes;
      for (auto row : query(database, fmt::format("SELECT poll_id, option_key FROM real_estate_poll_votes WHERE account_id = {} ORDER BY poll_id, option_key", accountId))) {
        votes[Strings::ToUnsignedInt(row[0])].push_back(Strings::ToUnsignedInt(row[1]));
      }
      return votes;
    }

    std::string keyText(const Poll &poll, uint32 key, const std::map<uint32, std::string> &texts) {
      if (poll.kind == PollKind::property) {
        return propertyName(key);
      }
      auto found = texts.find(key);
      if (found == texts.end()) {
        throw std::runtime_error(fmt::format("poll [{}] offers key [{}], which has no label", poll.id, key));
      }
      return found->second;
    }

    void writePoll(WireWriter &message, const Poll &poll, int64 now) {
      message.u32(poll.id)
        .u32(static_cast<uint32>(poll.kind))
        .u32(0)
        .lengthString(poll.question)
        .u32(static_cast<uint32>(poll.opensAt))
        .u32(static_cast<uint32>(poll.closesAt))
        .u32(static_cast<uint32>(stateOf(poll, now)))
        .u32(static_cast<uint32>(poll.selection))
        .u32(static_cast<uint32>(poll.options.size()));
      for (const auto &[key, position] : poll.options) {
        message.u32(key).u32(position);
      }
      message.u32(poll.maxSelections)
        .lengthString(poll.location)
        .u8(poll.resultsUnpublished)
        .u8(poll.prompt);
    }

    WireWriter votesMessage(const std::vector<Poll> &polls, uint32 accountId, const std::map<uint32, std::string> &texts) {
      const auto votes = votesOf(accountId);
      WireWriter entries;
      uint16 count = 0;
      for (const auto &poll : polls) {
        auto found = votes.find(poll.id);
        if (found == votes.end()) {
          continue;
        }
        for (uint32 key : found->second) {
          entries.u32(poll.id).u32(0).u32(key).lengthString(poll.kind == PollKind::property ? keyText(poll, key, texts) : std::string()).u8(0);
          count++;
        }
      }
      WireWriter message;
      message.u16(count).bytes(entries.content().data(), entries.content().size());
      return message;
    }

    // Results are counted when sent, running totals while a poll is open as well as after its close, unless it keeps them unpublished (POL-5).
    WireWriter resultsMessage(const std::vector<Poll> &polls, const std::map<uint32, std::string> &texts) {
      WireWriter entries;
      uint16 count = 0;
      for (const auto &poll : polls) {
        if (poll.resultsUnpublished) {
          continue;
        }
        std::map<uint32, uint32> tally;
        for (auto row : query(database, fmt::format("SELECT option_key, COUNT(*) FROM real_estate_poll_votes WHERE poll_id = {} GROUP BY option_key", poll.id))) {
          tally[Strings::ToUnsignedInt(row[0])] = Strings::ToUnsignedInt(row[1]);
        }
        const auto ranked = rankPollResults(tally);
        entries.u32(poll.id).u32(static_cast<uint32>(ranked.size()));
        for (const auto &result : ranked) {
          entries.u32(result.key).lengthString(keyText(poll, result.key, texts)).u32(result.votes).u32(result.rank);
        }
        count++;
      }
      WireWriter message;
      message.u16(count).bytes(entries.content().data(), entries.content().size());
      return message;
    }

    std::optional<Poll> pollById(uint32 pollId) {
      for (const auto &poll : publishedPolls()) {
        if (poll.id == pollId) {
          return poll;
        }
      }
      return std::nullopt;
    }

    const Property *plotOf(const Session &state, uint32 propertyId) {
      auto found = state.properties.find(propertyId);
      if (found == state.properties.end()) {
        return nullptr;
      }
      const DefinitionType type = typeOf(found->second);
      return type == DefinitionType::plot || type == DefinitionType::guildPlot ? &found->second : nullptr;
    }

    void sendPollUpdate(const std::vector<Poll> &changed, int64 now) {
      WireWriter message;
      message.u16(static_cast<uint16>(changed.size()));
      for (const auto &poll : changed) {
        writePoll(message, poll, now);
      }
      forEachSession([&](Client *client, Session &) {
        sendMessage(client, OP_PollUpdate, message);
      });
    }
  }

  // The four lists the client must hold before its vote window may open (0x5a6830), then whether polling is available.
  void sendPolls(Client *client) {
    if (!RuleB(Housing, EnablePolls)) {
      WireWriter none;
      none.u16(0);
      WireWriter noAccess;
      noAccess.u32(0);
      sendMessage(client, OP_PollLabels, none);
      sendMessage(client, OP_PollList, none);
      sendMessage(client, OP_PollAccess, noAccess);
      sendMessage(client, OP_PollVotes, none);
      sendMessage(client, OP_PollResults, none);
      WireWriter available;
      available.u8(0);
      sendMessage(client, OP_PollAvailable, available);
      return;
    }
    try {
      const int64 now = currentTime();
      const auto polls = publishedPolls();
      const auto texts = labels(polls);
      WireWriter labelMessage;
      labelMessage.u16(static_cast<uint16>(texts.size()));
      for (const auto &[key, text] : texts) {
        labelMessage.u32(key).lengthString(text);
      }
      WireWriter list;
      list.u16(static_cast<uint16>(polls.size()));
      WireWriter access;
      access.u32(static_cast<uint32>(polls.size()));
      for (const auto &poll : polls) {
        writePoll(list, poll, now);
        access.u32(poll.id).u8(mayVote(client, poll, now) ? 1 : 0);
      }
      sendMessage(client, OP_PollLabels, labelMessage);
      sendMessage(client, OP_PollList, list);
      sendMessage(client, OP_PollAccess, access);
      sendMessage(client, OP_PollVotes, votesMessage(polls, client->AccountID(), texts));
      sendMessage(client, OP_PollResults, resultsMessage(polls, texts));
      WireWriter available;
      available.u8(1);
      sendMessage(client, OP_PollAvailable, available);
    }
    catch (const std::exception &error) {
      LogError("Housing: the polls could not be sent to [{}]: {}", client->GetCleanName(), error.what());
    }
  }

  // RoF2 asks with a bare 0x15e2 after a UI reload (0x49d990, POL-1).
  void handlePollRequest(Client *client, WireReader &reader) {
    if (reader.remaining()) {
      LogError("Housing: [{}] sent a [{}]-byte poll request; it has no body", client->GetCleanName(), reader.remaining());
      return;
    }
    sendPolls(client);
  }

  void handleVote(Client *client, WireReader &reader) {
    const uint32 pollId = reader.u32();
    const uint32 clicked = reader.u32();
    const uint8 selected = reader.u8();
    const uint32 count = reader.u32();
    std::vector<uint32> keys;
    for (uint32 index = 0; index < count && !reader.failed(); index++) {
      keys.push_back(reader.u32());
    }
    if (reader.failed() || reader.remaining()) {
      LogError("Housing: [{}] sent a malformed vote", client->GetCleanName());
      return;
    }
    Session *state = session(client);
    if (!state || !RuleB(Housing, EnablePolls)) {
      return;
    }
    try {
      const auto poll = pollById(pollId);
      const int64 now = currentTime();
      if (!poll || !mayVote(client, *poll, now)) {
        LogHousing("[{}] voted in poll [{}], which they may not vote in now", client->GetCleanName(), pollId);
        return;
      }
      std::set<uint32> options;
      for (const auto &[key, position] : poll->options) {
        options.insert(key);
      }
      const auto ballot = ballotOf(poll->selection, clicked, selected != 0, keys);
      if (!ballot || !voteKeysAllowed(poll->kind, poll->selection, poll->maxSelections, options, *ballot)) {
        LogHousing("[{}]'s vote in poll [{}] (clicked [{}], selected [{}], [{}] others ticked) is no ballot the poll allows", client->GetCleanName(), pollId,
          clicked, selected, keys.size());
        return;
      }
      if (poll->kind == PollKind::property) {
        for (uint32 key : *ballot) {
          const Property *plot = plotOf(*state, key);
          const uint32 standing = standingProperty(client, *state);
          const Property *standingPlot = standing ? ancestorOfType(state->properties.at(standing), typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, state->lookup()) : nullptr;
          const bool occupied = plot && (typeOf(*plot) == DefinitionType::guildPlot ? activeGuildId(*plot) != 0 : plot->owner_character_id != 0);
          if (!occupied || plot->voting_opt_out || !standingPlot || standingPlot->id != key) {
            LogHousing("[{}] voted in poll [{}] for property [{}], which is no occupied plot they stand on that takes votes", client->GetCleanName(), pollId, key);
            return;
          }
        }
      }
      Transaction transaction(database);
      query(database, fmt::format("DELETE FROM real_estate_poll_votes WHERE poll_id = {} AND account_id = {}", pollId, client->AccountID()));
      for (uint32 key : *ballot) {
        query(database, fmt::format("INSERT INTO real_estate_poll_votes (poll_id, account_id, option_key) VALUES ({}, {}, {})", pollId, client->AccountID(), key));
      }
      transaction.commit();
      // Every zone sends its lists again: the new totals to everyone, the ballot to the account's other characters (POL-5).
      sendEvent(EventType::pollsChanged, EventTarget::allZones, 0, 0, {});
      std::vector<std::string> ticked;
      for (uint32 key : *ballot) {
        ticked.push_back(std::to_string(key));
      }
      LogHousing("[{}] voted [{}] in poll [{}]", client->GetCleanName(), Strings::Join(ticked, ","), pollId);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s vote in poll [{}] failed: {}", client->GetCleanName(), pollId, error.what());
    }
  }

  // The Purchase window's opt-out box (0x6573): the plot the player stands on, a flag in the low byte, stack bytes above it.
  void handleVotingOptOut(Client *client, WireReader &reader) {
    if (reader.remaining() != optOutSize) {
      LogError("Housing: [{}] sent a [{}]-byte voting opt-out; it is [{}] bytes", client->GetCleanName(), reader.remaining(), optOutSize);
      return;
    }
    const uint32 propertyId = reader.u32();
    const uint8 optOut = static_cast<uint8>(reader.u32() & 0xFF);
    Session *state = session(client);
    if (!state) {
      return;
    }
    try {
      const Property *plot = plotOf(*state, propertyId);
      const uint32 standing = standingProperty(client, *state);
      const Property *standingPlot = standing ? ancestorOfType(state->properties.at(standing), typeBit(DefinitionType::plot) | typeBit(DefinitionType::guildPlot), false, state->lookup()) : nullptr;
      if (!plot || !standingPlot || standingPlot->id != propertyId || !permitted(client, Permission::manage, *plot, state->lookup(), state->accessLookup())) {
        LogHousing("[{}] may not set property [{}]'s voting opt-out", client->GetCleanName(), propertyId);
        return;
      }
      Transaction transaction(database);
      const Property locked = lockedProperty(database, propertyId);
      query(database, fmt::format("UPDATE real_estate_properties SET voting_opt_out = {}, revision = revision + 1 WHERE id = {}", optOut ? 1 : 0, propertyId));
      transaction.commit();
      sendPropertyChanged(propertyId, PropertyChangedPayload{locked.owner_character_id, locked.owner_character_id, locked.guild_id, locked.guild_id});
      LogHousing("[{}] set property [{}]'s voting opt-out to [{}]", client->GetCleanName(), propertyId, optOut ? 1 : 0);
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s voting opt-out on property [{}] failed: {}", client->GetCleanName(), propertyId, error.what());
    }
  }

  // Teleport to Selected (dialog 0x8b): {0, poll id, the ranked property, -1}; property polls whose results are out.
  void handlePollTeleport(Client *client, WireReader &reader) {
    if (reader.remaining() != teleportSize) {
      LogError("Housing: [{}] sent a [{}]-byte poll teleport; it is [{}] bytes", client->GetCleanName(), reader.remaining(), teleportSize);
      return;
    }
    reader.u32();
    const uint32 pollId = reader.u32();
    const uint32 key = reader.u32();
    reader.u32();
    try {
      const auto poll = pollById(pollId);
      if (!poll || poll->kind != PollKind::property || poll->resultsUnpublished) {
        LogHousing("[{}] asked to go to property [{}] of poll [{}], which shows no property results", client->GetCleanName(), key, pollId);
        return;
      }
      auto votes = query(database, fmt::format("SELECT COUNT(*) FROM real_estate_poll_votes WHERE poll_id = {} AND option_key = {}", pollId, key));
      auto plots = propertiesById({key});
      if (!Strings::ToUnsignedInt(votes.begin()[0]) || plots.size() != 1) {
        LogHousing("[{}] asked to go to property [{}], which poll [{}] does not rank", client->GetCleanName(), key, pollId);
        return;
      }
      moveToPlot(client, plots.front());
    }
    catch (const std::exception &error) {
      LogError("Housing: [{}]'s teleport to property [{}] of poll [{}] failed: {}", client->GetCleanName(), key, pollId, error.what());
    }
  }

  void refreshPolls() {
    forEachSession([](Client *client, Session &) {
      sendPolls(client);
    });
  }

  // A poll opening or closing updates the list (0x39bb); one that opens with its prompt flag opens the vote window (0x7a4d) for
  // those who may vote and haven't.
  void processPolls() {
    if (!RuleB(Housing, EnablePolls) || !stateTimer.Check()) {
      return;
    }
    try {
      const int64 now = currentTime();
      const auto polls = publishedPolls();
      std::vector<Poll> changed;
      std::vector<Poll> opened;
      for (const auto &poll : polls) {
        const PollState current = stateOf(poll, now);
        auto last = lastStates.find(poll.id);
        if (last != lastStates.end() && last->second != current) {
          changed.push_back(poll);
          if (current == PollState::open && poll.prompt) {
            opened.push_back(poll);
          }
        }
        lastStates[poll.id] = current;
      }
      if (changed.empty()) {
        return;
      }
      sendPollUpdate(changed, now);
      forEachSession([&](Client *client, Session &) {
        WireWriter access;
        access.u32(static_cast<uint32>(polls.size()));
        for (const auto &poll : polls) {
          access.u32(poll.id).u8(mayVote(client, poll, now) ? 1 : 0);
        }
        sendMessage(client, OP_PollAccess, access);
        const auto votes = votesOf(client->AccountID());
        for (const auto &poll : opened) {
          if (poll.kind == PollKind::question && mayVote(client, poll, now) && !votes.count(poll.id)) {
            WireWriter prompt;
            prompt.u32(poll.id).lengthString(poll.question);
            sendMessage(client, OP_PollOpen, prompt);
          }
        }
      });
    }
    catch (const std::exception &error) {
      LogError("Housing: the polls' states could not be checked: {}", error.what());
    }
  }

  void resetPolls() {
    lastStates.clear();
  }
}
