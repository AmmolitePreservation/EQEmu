# Polls (Voting)

**Status:** done (messages and the poll manager; `windows/VoteWnd.md`; `windows/VoteResultsWnd.md`; flow `flows/polls.md`). Windows: `EQUI_VoteWnd.xml` ("Vote") and `EQUI_VoteResultsWnd.xml` ("Polls"); usage maps in `windows/VoteWnd.usage.md` and `windows/VoteResultsWnd.usage.md`.

**The server pushes the poll data; the client asks for it only after a UI reload**, with a bare `0x15e2` (`0x49d990`), which the server answers with `0x15e2`, `0x6860`, `0x7497`, `0x52b5` and `0x593a`. All seven server messages are routed by the second dispatcher switch to one handler (`0x4d36bb` / `0x4d3e44`) and on to the **poll manager** (singleton `[0xe643ec]`, 0x294 bytes, getter `0x5a72a0`, constructor `0x5a7020`), whose dispatcher `0x5a7a20` switches on the opcode. Peridot names them `OP_Poll*` (`../serverPlan.md`, the opcode map).

## Server -> client

| Opcode | Method | Payload | Effect |
|---|---|---|---|
| **`0x15e2`** | `0x5a7440` | u16 count, then that many **poll records** | **replaces the poll list** (manager `+0x00` count, `+0x04` array of 0x60-byte records); sets `+0x28d` |
| **`0x39bb`** | `0x5a70c0` | u16 count, then poll records | **updates polls by id** (replace the matching record, append new ones) |
| **`0x3311`** | `0x7fb780` -> `0x7fb660` / `0x7fb3a0` | u16 count, then per poll: **u32 poll id, u32 count N, N × {u32 key, string label, u32 votes, u32 rank}** | **the poll results** (manager `+0x20`, 0x18-byte entries by poll id, rows of 0x14 bytes; an existing poll is replaced): the results window's Rank, label and vote count (`windows/VoteResultsWnd.md`) |
| **`0x6860`** | `0x7fb020` | u16 count, then N × **{u32 key, string}** | the global **key -> text table** (manager `+0x10`, 12-byte entries): **the option labels** (`0x7fa630`); an option whose key has no entry isn't shown; sets `+0x28e` |
| **`0x7497`** | `0x5a7840` | u32 count N, then N × **{u32 poll id, u8 flag}** | clears and refills the manager's two **poll-id sets** (`+0x26c` and `+0x27c`, chosen by the flag); **set A (`+0x26c`) = the polls the player may vote in** ("can vote" `0x5a6e20` = state Open **and** in set A, `0x5a6d70`); **set B (`+0x27c`) = polls the player may see but not vote in**; the results window lists polls in either set; sets `+0x28f` |
| **`0x52b5`** | `0x5a75f0` | u16 count, then N × **{u32 poll id, u32, u32, string, u8}** (reader `0x7fa3a0`) | the player's **votes**, in the table at `+0x25c` (0x18-byte entries: `+0x04` poll id, **`+0x0c` the chosen option key**, `+0x10` string (for a property poll, the text shown as "Your Vote", e.g. the plot's name); one entry per chosen option, so a multiple-choice poll (`+0x2c` = 3) can have several); used to pre-select your choices (`0x5a6890`, `0x5a68f0`); refreshes that poll's "Your Vote:" in the vote window (`0x7941f0`); sets `+0x290` |
| **`0x593a`** | `0x5a7420` | u8 | **polling available** flag (`+0x258`); going from 0 to non-zero runs the refresh below |

Every loader ends by refreshing the vote window (`0x795f30` / `0x795f90`), running `0x5a7360`, and refreshing the results window (`0x7931f0` / `0x793e50`).

**Server-opened:** message **`0x7a4d`** (first dispatcher switch, `0x4c5bb8`) = **{u32 poll id, string}**: opens the vote window on that poll (`0x795b00`, mode 3) if the player can vote on it, otherwise prints `0x1de2` "There are no active polls to vote on."

**Auto-open (`0x5a7360`):** if polling is available (`+0x258`), the vote window exists and **hasn't been auto-shown yet** (`VoteWnd +0x266`), and there are open polls needing the player's vote (`0x5a6830`), the client collects them (`0x5a6f00`), fills the window (`0x796130`) and **shows it once** (`+0x266` = 1).

## The poll record (0x60 bytes; reader `0x5a69a0`, wire order)

| # | Wire | Field | Meaning |
|---|---|---|---|
| 1 | u32 | `+0x04` | **poll id** (updates and the player tables match on it) |
| 2 | u32 | `+0x08` | **kind**: **1 = property poll** (voted from the Purchase window: "This One" votes for the plot you're on), **2 = question poll** (radio or checkbox); the vote window shows only polls whose kind equals its mode |
| 3 | u32 | `+0x1c` | **not used by the client** (only read and copied); a server value |
| 4 | string | `+0x20` | **the question** (the "Poll" column) |
| 5 | u32 | `+0x10` | **open date** (formatted as a date, `0x8dd670`) |
| 6 | u32 | `+0x14` | **close date** |
| 7 | u32 | `+0x28` | **state**: **0 Initialized, 1 Upcoming, 2 Open, 3 Closed, 4 Archived** (`0x7f9d10`, the "Status" column) |
| 8 | u32 | `+0x2c` | **selection type**: **1 = the property poll's button**, **2 = single choice (radio)**, **3 = multiple choice (checkbox)** |
| 9 | u32 count N, N × {u32, u32} | options (`+0x50` count, `+0x54` data) | **the options**: the first u32 is the **option key** (a global label id, text from `0x6860`; what votes send back); the second is its **display order** (options are sorted by it after loading, `0x5a8bc0` / `0x5a87b0`) |
| 10 | u32 | `+0x34` | **the most options a player may tick** (multiple-choice polls) |
| 11 | string | `+0x38` | **polling location** (the "Polling Location" column) |
| 12 | u8 | `+0x4c` | **results won't be published**: when a poll has no results, 0 shows "Results for this poll are not available. Check back later.", non-zero drops "Check back later." |
| 13 | u8 | `+0x3c` | **prompt players**: only polls with this set make the Vote window pop up automatically for players who haven't voted (`0x5a6f00`) |

`0x5a6db0(poll id)` answers whether a poll id is in either `0x7497` set.

## Client -> server

| Opcode | From | Meaning |
|---|---|---|
| **`0x15a8`** | the vote window's handler (`0x794e90`, send at `0x7951a3`) | **cast a vote** (sent on every click): **u32 poll id, u32 option key** (the clicked control's), **u8 selected** (0 when un-ticking a checkbox), **u32 N, N × u32** every option key currently ticked |
| **`0x6573`** | the Purchase window's opt-out checkbox (`windows/RealEstatePurchaseWnd.md`) | **the plot's voting opt-out**: **{u32 property id (the plot one stands on), u32 whose low byte is the flag}**; the record's `+0x44` keeps it, and the Vote button is dark for an opted-out plot |
| **`0x499a`** | game dialog `0x8b` ("Do you wish to be teleported to the selected real estate: %1?", from `VoteResultsWnd`'s Teleport to Selected) | **teleport to a ranked property**: **{u32 0, u32 poll id, u32 result key (the property), u32 -1}** (16 bytes); property polls only; on no, nothing is sent |

## Close-out

**All poll messages:**

| Direction | Opcodes |
|---|---|
| server -> client | `0x15e2` (poll list), `0x39bb` (poll updates), `0x3311` (results), `0x6860` (option labels), `0x7497` (may vote / may see), `0x52b5` (your votes), `0x593a` (polling available), `0x7a4d` (open on a poll) |
| client -> server | `0x15e2` (bare request), `0x15a8` (vote), `0x6573` (voting opt-out), `0x499a` (teleport to a ranked property) |

Peridot names them `OP_Poll*`; `0x15e2` (`OP_PollList`) goes both ways, the client's being the bare request after a UI reload.

**Sweep:** the results window handles clicks (1) and column sorts (`0x10`); the vote window only clicks. The only dialog is `0x8b` (the teleport). The "pollid=" chat link (`0x7961c0`) parses the id up to the next `&` and opens that poll, or prints `0x1de2`.

**Static controls:** never referenced: `VW_VerticalLayout`, `VW_HorizontalLayout`, `VW_Btn_Spacer`, `VW_Radio_Spacer`, `VW_Checkbox_Spacer`, `VRW_Selection_Label`, `VRW_VotingResults_Label`, `VRW_Top_Screen`, `VRW_Bottom_Screen`, `VRW_Main_Layout`. Looked up by name inside clones or screens (not bound by the window): `VW_Btn`, `VW_Btn_Label`, `VW_Radio`, `VW_Radio_Label`, `VW_Checkbox`, `VW_Checkbox_Label`, `VRW_SelectionValue_Label`.
