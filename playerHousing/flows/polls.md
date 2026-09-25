# Flow: Polls, Property Contests and the Winner Teleport

**Status:** client side decoded (`../polls.md`, `../windows/VoteWnd.md`, `../windows/VoteResultsWnd.md`).

## 1. Goal

The server runs polls: **question polls** (single or multiple choice) and **property polls** (players vote for the plot they stand on, from the Purchase window). Players vote, see results, and can teleport to a ranked property.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| P1 | Option labels (key -> text) | `0x6860` | **options without a label are not shown** |
| P2 | The poll list (id, kind, selection type, question, dates, state, options, max ticks, location, flags) | `0x15e2` / `0x39bb` | everything |
| P3 | Which polls the player may vote in (set A) and may see (set B) | `0x7497` | the vote window shows only Open polls in set A; the Polls window lists set A and B |
| P4 | The player's votes | `0x52b5` | "Your Vote", pre-selected choices, auto-open |
| P5 | Results | `0x3311` | the results list and the teleport |
| P6 | Polling available | `0x593a` | the one-time auto-open |
| P7 | For property polls: the player standing on a plot | real-estate data (manager `+0x98`) | the vote key |

## 3. Sequence

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | S -> C | `0x6860`, `0x15e2`, `0x7497`, `0x52b5`, `0x593a` | labels, polls, eligibility, votes, availability; the client may pop the Vote window once for polls with the "prompt" flag the player hasn't voted on |
| 2 | (client) | open the Vote window | Purchase window **Vote** (property polls, key = the plot), Polls **Open Polls** (question polls), a "pollid=" chat link, or server `0x7a4d` |
| 3 | C -> S | **`0x15a8`** on every click | poll id, the clicked key, selected byte, all ticked keys |
| 4 | S -> C | `0x52b5` | the updated votes |
| 5 | S -> C | `0x39bb` (state changes), `0x3311` (results) | |
| 6 | C -> S | **`0x499a`** {0, poll id, result key, -1} | Teleport to Selected (property polls) after the confirmation |

## 7. Server obligations

Send the labels (`0x6860`) for every option key; keep sets A/B, votes and results current; **validate `0x15a8`** (the poll is Open, the player is in set A, the key belongs to the poll (or, for property polls, is a valid plot), at most record `+0x34` choices); answer with `0x52b5`; for property polls, use property ids as option/result keys and the property's name as the result text; handle `0x499a` by moving the player to that property.
