# Coverage Plan — Full Restart

**Layers:**
1. Research specs (per message, in this folder).
2. **GUI baseline**: every control in every XML file, in [`guiControlInventory.md`](guiControlInventory.md) (generated) with per-control analysis in [`windows/`](windows/).
3. **Flow specs** in [`flows/`](flows/), which double as the test plans.
4. The server data model draft in [`serverDataModel.md`](serverDataModel.md). The implementation plan comes after the flows are stable.

**Rule for conditions:** a condition is only recorded as done when every predicate it calls has been expanded, level by level, down to **data leaves**: a field whose origin is known (which server message and field, which client action, or which initialization step, and when). Sequential checks are recorded in order, with what the client does when each one fails.

Goal: every real-estate code path in the client **read completely** (not sampled) before live testing, so that the server never sends the client something it can't handle. Each item below is marked done only when the function has been read end to end and every field, branch, and failure path is recorded with evidence addresses.

Status legend: `todo`, `reading`, `done`, `blocked (reason)`.

## Window status

| Window | Controls | Status | File |
|---|---|---|---|
| Purchase | 19 / 19 | done | `windows/RealEstatePurchaseWnd.md` |
| Items | 44 / 44 (+ 22 static) | done | `windows/RealEstateItemsWnd.md` |
| Layout Details | 4 / 4 (+ 4 static) | done (Load option labels inferred) | `windows/RealEstateLayoutDetailsWnd.md` |
| Manage | 31 / 31 (30 in the XML + `REMW_EscrowProperty_Combobox`, which the XML lacks; + 16 static) | done | `windows/RealEstateManageWnd.md` |
| Neighborhood | 15 / 15 (+ 11 static) | done | `windows/RealEstateNeighborhoodWnd.md` |
| Plot Search | 10 / 10 (+ 7 static) | done | `windows/RealEstatePlotSearchWnd.md` |
| **World layer** (placed items in the 3D world) | steps 1–6 | done | `worldLayer.md` |
| Trophy Tribute (`TributeTrophyWnd`) | 19 bound | done | `windows/TributeTrophyWnd.md`, `trophyTribute.md` |
| Vote / Polls (`VoteWnd`, `VoteResultsWnd`) | 6 + 5 bound (+ templates and static) | done | `windows/VoteWnd.md`, `windows/VoteResultsWnd.md`, `polls.md` |

Flows: `flows/README.md` (Purchase draft needs a refresh; Place, Edit/Move, Moving crate, Layouts, Escrow and upkeep, Access and permissions, Neighborhoods, Plot Search written). **All six windows are done: 123 / 123 client-used controls.**

## Foundation fix: `HandleWorldMessage`'s second opcode switch (decoded)

Every message reaches the common tail `0x4cd5e0`. If `[0xdd2660]->0x488450()` returns nonzero, a **second switch** on the opcode runs (compare tree from `0x4cd625` on local `[ebp−0x55b0]`, plus 6 jump tables). Its fallback `0x4d3f43` prints "Received unknown message" only when the **first switch also** didn't handle the message (flag `[ebp−0xaa5]`). Decoded with the same simulator, now in `realEstateOpcodeScan.py`:

- switch 1: **431** opcodes; switch 2: **271** opcodes (186 handlers; 132 named by EQEmu); **only one opcode in both**; **union 701**.
- **Real-estate reachability re-check** (each handler bounded by the next handler's start, then 3 call levels): new real-estate paths the "30 forwarded opcodes" conclusion had missed:

| Opcode (S → C, switch 2) | Handler | Reaches | Status |
|---|---|---|---|
| **`0x626e`** | `0x4d058a` | manager `0x5adb60` (the request-latch clear used by the 5-second timeout) and the manager constructor path | structured reply (object `0x504460`, reads `0x4d5510`, `0x4d6780`, 3 × u8, 2 × u32, …); **likely the server's reply to the client's `0x626e` place request**; to decode |
| **`0x7bb1`** | `0x4d3798` | manager singleton | **decoded: real-estate anchor teleport offer** (see README, *Message roles*); the client answers dialog `0x8a` by sending `0x7bb1` back |
| **`0x15e2`, `0x3311`, `0x39bb`, `0x52b5`, `0x6860`, `0x7497`** | `0x4d36bb` | poll manager dispatcher `0x5a7a20` | **decoded** (`polls.md`) |
| **`0x593a`** | `0x4d3e44` | poll manager `0x5a7420` | **decoded**: polling-available flag (`polls.md`) |
| `0x40e5` | `0x4d33e9` | — | **not real estate: the account-kick exchange** ("Another player logging in with this account has requested to kick you off the server…", dialog `0x88`, answered with `0x40e5`); the `0x73fd70` link was a false positive |
| guild member messages (`OP_GuildMemberList` and others) | `0x4d1d6f` | guild manager `0x425d30` -> Manage `0x7414f0` | **UI refresh only**: when a guild's data or name changes, the guild manager refreshes that guild's "Guild - %s" entry in the Manage window's owner-clubs list; no real-estate data in these messages |
| `0x3e7a` | `0x4d3262` | — | **not real estate: a guild-bank message** ("There is no more room in the Guild Bank."); `0x73f8f0` is a generic 13-instruction helper with 14 callers |

## Cross-cutting mode: in-world placement / edit mode

Players can enter an in-world mode to place a new item, or to move, rotate or scale one already placed. Like "standing on the plot", this mode is **client state that can change any control's behavior**, so every window is re-checked against it.

**What's known:**
- A **placement-state singleton** (getter `0x558540`: `+0x08` active flag, `+0x14` the item being placed, `+0x18..+0x28` location/record/spawn fields) with helpers `0x556d70` (is active), `0x556d80` (is this GUID the placed item), `0x556dc0` (is this property/record), `0x556e10` (update location), and `0x557b60` (end/cancel placement).
- **It reaches well beyond real-estate code:** 83 call sites use the getter (real-estate manager 5 functions; Items window `0x730510`; second-switch handlers `0x4d0050`, `0x4d0a30`; about 21 functions elsewhere, including `0x5384d0`, which sends the `0x626e` place request). The "is active" helper has 40 call sites, 37 of them outside real-estate code (input, camera, UI).
- The commit is `0x711a` (placement callback interface `0x9d90f0` entry `[2]`); overlay/keybind text is in the display layer (`0x48a5b0`); `CMD_REAL_ESTATE_ITEM_PLACEMENT_*` keybinds exist; the options include `ItemPlacementHideUI`, `ItemPlacementShowOverlay` and `ItemPlacementDefaultModeCursor`.

**Purchase window vs. this mode (checked):**
- **No call path** from any Purchase predicate or gate to the placement state (4 levels; roots: the fill `0x74dcb0`, the live updater `0x74d380`, notification `0x74d500`, gates `0x5b1890`/`0x5b3430`/`0x5b1380`, local checks `0x5b7fa0`/`0x5b50b0`, `0x5b41d0`, `0x4ff6c0`, `0x74d240`, dialog callbacks `0x74c8d0`).
- **Data couplings (to verify while mapping the mode):**
  1. **Request latch:** place requests set manager `+0xa4` (`0x5b1d40`), so the purchase/sell/transfer gates refuse while a placement request is pending (up to 5 s or until the reply).
  2. **`ItemPlacementHideUI`:** may hide windows (Purchase included) during placement.
  3. **Location ids** `+0x94`/`+0x98` come from position; confirm placement doesn't alter them.

**Plan:** map this mode (entry points: Items Place → `0x626e` request/reply, interaction menu, keybinds, clicking a placed spawn; the sub-modes cursor/collision/wall/free; exit, commit and cancel; everything the 37 external "is active" callers change) **before** the Items action buttons, then re-check each window's controls against it, including the Purchase window's couplings above.

## Open questions register

Every open question in one place, with a pointer to its details. The window-by-window pass closes these as it reaches them; a question is removed only when its answer is recorded in the file it points to. Numbers are stable, so don't renumber.

| # | Question | Why it matters | Details |
|---|---|---|---|
| Q1 | **Main source verified:** `OP_PlayerProfile` field `expansions` (the client parser and EQEmu's RoF2 encoder agree), copied to character `+0x3648`. **Other writers found:** opcode `0x0767` (payload `+0x08`; also sets the globals), opcode `0x0e48` (payload `+0x44`), and the named `"ExpansionFlags"` key/value path. **Open:** what triggers the key/value path (timeboxed: entirely virtual, no direct network caller found; the entry points and a test-time check are recorded), `0x0767` (expansion update), `0x0e48` ("Your expansion settings are", writes the flags) and `0x2b10` (another player's settings, no write) are now identified. **Profile `+0x340c` = `profileType`** (the key=value profile key, `0x7ebb70`; `+0x3410` = `profileListType`): in `OP_PlayerProfile` it is the u32 right after the checksum fields, followed by `profileListType`, gender, race and class (reader `0x7edaa0`). **EQEmu's RoF2 encoder writes 0 to both (its two "Unknown" u32s), so the Trophies gate's profile check passes**; 0 is the normal character profile, 1 and 2 switch other systems (ability sets, class animations) and disable the housing UI. | whether any real-estate action works (a precondition of every test) | `windows/RealEstatePurchaseWnd.md`, *Character interface and expansion flags* |
| Q2 | **Answered:** `0x4b3040` resolves "the player is inside a property" from the **`0x49df` region volumes** (oriented boxes) in the world region object `[0xdcee30]`, matched by definition `+0x1c` under `0x6fd0`'s current property (`placementMode.md`, *Region volumes*) | sets manager `+0x94` | `placementMode.md` |
| Q3 | **Answered:** manager vtable `+0xc` → rules `0x5b69b0` (13 ordered checks with codes, `windows/RealEstateItemsWnd.md`); **`0x23` is server-only** (no client check produces it) | final gate for Place | `windows/RealEstateItemsWnd.md`, *Action buttons* |
| Q4 | **Answered:** items already in a property use vtable `+0x10` → `0x5b6eb0` (stored) or `+0x1c` → `0x5b5ff0` (between properties); both rule sets recorded. Some helper-returned capacity codes are still generic | Move and re-place | `windows/RealEstateItemsWnd.md`, *Can-place* |
| Q5 | **Answered:** definition `+0x5ac` = EQEmu's `ItemQuaternaryBodyStruct.Placeable` (read with the next three bytes as a u32; non-zero = placeable). **EQEmu's RoF2 encoder hardcodes `Placeable = 0`** | Place button, placement | `windows/RealEstateItemsWnd.md`, *What makes an item placeable* |
| Q6 | **Answered:** the real-estate windows have no show gate (they always open); gates sit on controls and actions. Every real-estate permission needs the House of Thule bit `0x10000` (guild plots also `0x20000`), including the Items window's Place path (Q3 answered earlier); the Trophies window's `profileType` check passes with EQEmu (Q1) | test preconditions | `priorResearch.md` Q2, `permissions.md` |
| Q7 | **Answered:** character virtual base vtable `+0x90` (`0x9d6ba8`, thunk `0x57d280` -> `0x580300`) -> **the player's spawn** (`this - 0x3ec`) vtable `+0xdc` (spawn vtable `0x9d71f0`, `0x597f40`) -> **spawn `+0x25c`, the GM flag** (the spawn bitfield `gm`, bit 6, set by the spawn parser `0x590dd0`). Every "GM bypass" in the housing code is this flag | Purchase and Manage button states | `windows/RealEstatePurchaseWnd.md` |
| Q8 | Purchase window click paths: **done** apart from the poll window's data source and Q20's sub-checks. History: **Decoded:** Purchase (gate `0x5b1890`, 11 checks; sends `0x45be`), Sell (gate `0x5b3430` + local check `0x5b7fa0`; sends `0x792c`), Transfer (gate `0x5b1380` + local check `0x5b50b0`; sends `0x507f`), Manage/Items (subcommand toggles, no preconditions), the upkeep combobox's effects. Vote (poll window), Opt-out (`0x6573`), the upkeep combobox sequence. **Still to read:** the poll window data source (`[0xdd5d00]` resolved as a guild-list count) (see Q20 for the sell/transfer sub-checks) | Purchase flow | `windows/RealEstatePurchaseWnd.md` |
| Q9 | **Answered** (see Q21, Q22; `permissions.md`, *Guild rank permissions and account membership*): the membership gate passes with stock EQEmu; **rank permissions 27/28 default to no rank and must be granted with `OP_GuildUpdate` action 5** | guild housing | `windows/RealEstatePurchaseWnd.md`, `flows/purchase.md` |
| Q10 | **Answered:** "inside a plot or house" (`0x2d7`) = the Manage window needs a known record for manager `+0x98`; "in a neighborhood" (`0x1f8e`) = Plot Search needs manager `+0x90` (the current property's definition type, from `0x6fd0`) to be a neighborhood (type 5) | Manage and Plot Search | `windows/RealEstateManageWnd.md`, `windows/RealEstatePlotSearchWnd.md` |
| Q11 | **Reviewed statically** (`robustness.md`): no crash path in the housing handlers and no housing-zone-specific client code; likely zone-level (zone files, wrong zone ids, zone-in data). Remaining: a **test item** with a test plan and the zone list | crash avoidance | `worldLayer.md`, `priorResearch.md` Q4 |
| Q12 | **Answered:** none of the 14 computed-opcode send sites is real estate (reward selection, GM summon / corpse drag, the game-wide dialog's case `0x90`, the merchant path, marketplace windows). **`0x1ff4` is game dialog `0x87`'s yes-answer** (shared send tail `0x52da12`); **no code opens dialog `0x87`** (searched constant ids, register ids and the generic helper `0x565320`), so it is unreachable in this build and unrelated to real estate | send completeness | this file, section 1 |
| Q13 | **Answered:** `0x45be` from the server is `{u8 success}`; non-zero opens the Manage window. The manager virtuals (vtable `0x9d910c`) are record bookkeeping: `+0x24` indexes a record (by id, guild and others), `+0x08` moves it in the guild index, `+0x28` removes it (indexes, access list, `0x5b4680`) | receive completeness | this file, section 2 |
| Q14 | **Answered:** section 3 rebuilt from the send inventory; every real-estate client message is documented; the sweep's four extra sends are not real estate | server handlers | this file, section 3 |
| Q15 | Placed-item entry: **resolved:** first u32 (`+0x34`) = owning property id; first string (`+0x24`) = owner name; the u8 flag = placed/stored; counters vs caps = real estate (`0x0a`), vault (`0x0c`), pets (`0x26`); **the seven floats are heading, pitch, roll, scale, x, y, z** (verified via the layout writer); the u32 at `+0x04` and the second string are not read by the client | Items list, placement rules | `placedItemsAndProperties.md`, `windows/RealEstateItemsWnd.md` |
| Q16 | `0x21ec` interaction menu. **Resolved:** the client's request is the same 0x300-byte struct with empty entries; header `+0x20`/`+0x24` = property id / record id; **every entry type's client action and request payload is decoded** (types 1–5, 7–9, `0xc`/`0xd`, `0xe` → `TextEntryWnd` → `0x3c8b`, `0x11` → house swap `0x21ba`, `0x13`); the labels and meaning are server-defined. **Open:** what the entry's `b` value means per type (server-defined) | interaction menu | `placedItemsAndProperties.md` |
| Q17 | **Answered:** the client never sets the flag; the reply handler honors flag ≠ 0 only with type 1 and flag 0 with type 0 or type 2. Server usage: refund a failed deposit (type 0, flag 0), pay a withdrawal (type 1, flag 1), confirm an upkeep change (type 2, flag 0) (`opcode2aa6PropertyAccount.md`) | escrow flow | `opcode2aa6PropertyAccount.md` |
| Q19 | **Answered** (`permissions.md`, *Group computation*): reserved keys `_Fellowship` and `_Guild<rank>`; name lookup `0x4ff130`; **character `+0x2054` = fellowship id**, `+0x205c` = guild rank, `+0x204c` = guild id; **record `+0x28` = the owner's fellowship id**; guild rank 1 = Owner on guild plots; permission names and feature 10 done earlier | every non-owner action | `permissions.md` |
| Q20 | **Answered at the rule level** (`windows/RealEstatePurchaseWnd.md`, *Sell gate*): code 1 = the plot's item list not loaded; 3 = a non-empty crate in inventory or bank; 4 = no placed crate; 5 = the crate is full; 6 = no permission 3 (Remove Crate); 10 = guild-plot escrow not empty without permission 2; 11/12 = active-crate conditions (internals not expanded); transfer `0x57bfb0` = the plot has an item container | Sell and Transfer preconditions; moving-crate model | `windows/RealEstatePurchaseWnd.md` |
| Q21 | **Answered:** feature 10 comes from `OP_SendMembership` (`0x7acc`, the player's values) and `OP_SendMembershipDetails` (`0x057b`, the tier table); EQEmu already sends values that pass | guild-plot purchase, sell and permissions | `permissions.md`, `windows/RealEstatePurchaseWnd.md` |
| Q22 | **Answered:** **`OP_GuildUpdate` (`0x2958`) action 5** {action 5, `+0x50` rank, `+0x54` permission, `+0x58` value} fills the rank table of the static guild object `0xdd5cf8`; all ranks default to no permissions; a new struct for EQEmu | guild-plot purchase and sell | `windows/RealEstatePurchaseWnd.md` |
| Q23 | **Answered.** `0x7bb1` = real-estate anchor teleport (offer and accept); `0x40e5` = account kick (not real estate); the guild messages only refresh the Manage window's guild entry; `0x3e7a` = guild bank (not real estate). The poll messages and the vote-results teleport (`0x499a`, dialog `0x8b`, `VoteResultsWnd`) moved to Q24 | inventory completeness | this file |
| Q24 | **Answered** (`polls.md`, `windows/VoteWnd.md`, `windows/VoteResultsWnd.md`, `flows/polls.md`): 8 server messages (`0x15e2`, `0x39bb`, `0x3311`, `0x6860`, `0x7497`, `0x52b5`, `0x593a`, `0x7a4d`), 2 client messages (`0x15a8` vote, `0x499a` winner teleport); property polls are voted from the Purchase window for the plot you stand on | poll subsystem | `polls.md` |
| Q25 | **Analysis done** (`placementMode.md`); what remains is a **test item**: confirm at runtime that windows receive no clicks while placing | every window; the edit flow | this file, *Cross-cutting mode* |
| Q26 | **Answered:** the region's five trailing values aren't used by any housing code (containment, standing-in, drawing); the server can send zeros. Region format and message order were answered earlier | placement validity, Place button | `placementMode.md` |
| Q27 | What is character `+0x22c4` (MacroQuest `PcClient::RealEstateZoneID`)? | **Answered: real, but inert in RoF2.** The name comes from the client's own archive key `RealEstateZoneId`. Every access is bookkeeping: the character reset `0x7c9ca0` zeroes it; the text archive writer `0x7c1060` and loader `0x7c5bf0` and the binary writer `0x7ce150` and reader (`0x7ca5a0`) copy it, next to `PreviousZoneId` (`+0x22c0`) and `NewZoneId` (`+0x22b4`), through the character's sub-object at `+0x2dcc` (offset `-0xb08`). **No game logic reads it and no network message writes it** (the loaders are reachable only through vtable slots with no callers, as in Q1's source 4). *Inferred:* on Live it remembers the real-estate zone to return a character to. |
| Q18 | **Answered:** `0x613d`'s byte is written in the send buffer: **1 = pack** (dialog `0x65`), **0 = unpack** (dialog `0x66`) | Pack/Unpack Moving Crate | `windows/RealEstateItemsWnd.md`, *Container buttons* |

## 1. Inventory foundations

| Item | Status | Notes |
|---|---|---|
| Receive dispatch: every opcode that reaches `RealEstateManagerClient::HandleMessage` or real-estate windows | done | 30 forwarded; manager case map verified by hand |
| Receive dispatch: main-dispatcher opcodes that touch real-estate state directly | done | none found beyond the 30 |
| Send inventory with shared tails | done | `sendInventory.json` |
| Send inventory: sends reached through indirect calls or virtual message objects | done | the 14 computed sites: none are real estate (Q12) |
| Dialog responses: every dialog opened by real-estate code, per target | done | README *Confirmation dialogs* |
| Dialog `0x87` opener (`0x1ff4`) | done | no opener in this build; `0x1ff4` unreachable (Q12) |

## 2. Server → client handlers (one row per handler; mark done only after a complete read)

| Opcode | Handler | Status |
|---|---|---|
| `0x2698` | `0x7f5b60`, `0x7fc6b0` | done |
| `0x1cef` | `0x5b0d00` | done |
| `0x087f` | `0x5b0e30` | done |
| `0x1097` | `0x5ab4b0` | done |
| `0x2cf7` | `0x5abd20` | done |
| `0x1baf` | `0x5afe80` | done |
| `0x1967` | `0x5ab030` | done |
| `0x21ea` | inline `0x5b26fa` | done |
| `0x21ec` | `0x7375f0` | done (entry `type`/`b` meanings open) |
| `0x58c7` | inline `0x5b2c7a` | done |
| `0x6fd0`, `0x4f09`, `0x63fd` | `0x7fdf00` + cases | done |
| `0x2aa7`, `0x3e00`, `0x66df`, `0x0c13`, `0x76c6` | character lists | done |
| `0x2aa6` | `0x741670` | done |
| `0x2691` | `0x5b0f80`, `0x74ab50`, `0x74c4c0` | done |
| `0x1228`, `0x66bf`, `0x68ba` | result codes | done |
| `0x7d28` | `0x5b4680` | done |
| `0x5a63` | inline | done |
| `0x45be` | inline `0x5b29f8` | done: `{u8 success}`, non-zero opens the Manage window |
| `0x3968`, `0x485d` | `0x5b5250`, `0x5b7a90` | done (`permissions.md`) |
| Manager virtuals reached from handlers (vtable `+0x08`, `+0x24`, `+0x28`) | `0x5b5a10`, `0x5b5a90`, `0x5b7c70` | done (record indexing; Q13) |

## 3. Client → server messages

Rebuilt from `sendInventory.json` (every send site in the real-estate, placement, world-object, container and vote code, plus the world click and game-dialog sends) and checked against these docs. **All real-estate client messages are documented:**

| Opcode | Meaning | Where |
|---|---|---|
| `0x626e` | place (from inventory or the cursor) | `flows/placeItem.md`, `placementMode.md` |
| `0x711a` | commit a placement | `placementMode.md` |
| `0x2036`, `0x3424` | move to inventory / to closet, shed or crate | `windows/RealEstateItemsWnd.md` |
| `0x21ec` | interaction-menu request | `placedItemsAndProperties.md` |
| `0x218c`, `0x050c` | interact / enter (world click; Neighborhood Go) | `worldLayer.md`, `flows/editPlacedItem.md` |
| `0x51e3`, `0x64a3`, `0x7ad1` | interaction-menu entries (types 9, 0x13, 5) | `windows/RealEstateItemsWnd.md` |
| `0x3c8b` | text-entry answer (menu type 0xe) | `windows/RealEstateItemsWnd.md` |
| `0x21ba`, `0x6917`, `0x613d` | house swap, store all, pack / unpack | `flows/movingCrate.md` |
| `0x5b41` | load a layout | `flows/layouts.md` |
| `0x58c7` | inspect a placed item | `windows/RealEstateItemsWnd.md` |
| `0x45be`, `0x792c`, `0x507f` | purchase, sell, transfer (188-byte struct) | `windows/RealEstatePurchaseWnd.md` |
| `0x5a63` | transfer-offer reply | README, *Confirmation dialogs* |
| `0x2aa6` | escrow and upkeep | `opcode2aa6PropertyAccount.md` |
| `0x3146` | set an access list | `permissions.md` |
| `0x2691` | searches (types 0, 1, 2, 4, 5) | `opcode2691PropertySearch.md` |
| `0x6573` | the Purchase window's Vote button (opens the vote window) | `windows/RealEstatePurchaseWnd.md` |
| `0x15a8`, `0x499a` | vote, teleport to a ranked property | `polls.md` |
| `0x7bb1` | accept an anchor teleport | README, *Message roles* |
| **`0x1da9`** | **leave real estate** (`/leaverealestate`, key binding 446; bare request) | `commandsAndKeybindings.md` |
| `0x5cea` | /find (Locate type 3, Plot Search Go type 4) | `worldLayer.md`, `windows/RealEstatePlotSearchWnd.md` |
| `0x51a9`, `0x4b65`, `0x7a41`, `0x7c1d`, `0x2bc` | trophy tribute | `trophyTribute.md` |
| `OP_ClickObjectAction` (`0x0c1e`, close), `0x06c8`, `OP_TradeSkillCombine` | housing tradeskill containers | `worldLayer.md`, step 4 |
| `0x796c` | Plot Search Go's alternate branch, **unreachable** in this build | `windows/RealEstatePlotSearchWnd.md` |

**Found in the sweep and not real estate:** `0x0ecb` (`OP_RespawnWindow`, the respawn window next to the Purchase window's code); `0x65c7` and `0xcadf` (movement and collision reports); **`0x4936`** (the server's `0x4936` asks which of a list of item-instance ids the client holds; the client answers with the subset from an item-instance set that server `0x2c4e` updates; generic item syncing near the placement code, not housing).

## 3a. Client state model (preconditions set outside the handler)

A handler or window can be decoded perfectly and still do nothing if client state that another code path sets isn't in place. Examples: an expansion flag, the character profile, the current property, a window that hasn't been opened, or a pending-request latch. For every path in sections 2 and 3, record:

- **Every global or object field it reads** that it doesn't set itself (character `[0xdd261c]`, local player `[0xdd2630]`, windows `[0xd1fcXX]`, manager fields, placement state `0x558540`, UI globals).
- **What sets each one**: which server message (and which field of it), which client action, or which initialization step, and **when** relative to login, zone-in, and window open.
- **What the path does when a precondition fails**: silent no-op, a chat refusal (with its eqstr id), or a request sent to the server.
- **Sequencing:** what must have arrived first (for example `0x2698` definitions before anything that looks up a definition, and `0x6fd0` current property before location-gated actions), and what the client itself requests when a window opens.

The result is a per-test **state checklist**: the exact client state and message order a live test must set up before sending the message under test.

### Known client-side gates so far

Found by listing real-estate refusal strings in `eqstr_us.txt` and locating the code that shows each one.

| Gate | Text | Where shown | Check |
|---|---|---|---|
| House of Thule for UI | "You must own the House of Thule expansion to display this window." (`0x152e`) | `0x788fe0` (Trophy Tribute window show) | `0x483b10`: character `[0xdd261c]` must exist, its virtual `+0x5c` flags must include **`0x10000`**, and profile `+0x340c` must be 0. Without a character it falls back to global `[0xde09d0]`. When the gate passes, opening the window **sends `0x51a9`** (opcode only). `0x483b10` has 2 other callers, not yet traced. |
| House of Thule for activities | "This activity is disabled until you own the House of Thule expansion." (`0x14c7`) | `0x5ae1f0` (`0x1228` result-code path), `0x74d904` (Purchase window) | done: the House of Thule bit `0x10000` in the character's expansion flags (the Purchase window tests it just before) |
| House of Thule for plot ownership | `0x14c3` | `0x5b1890` (purchase gate), `0x5f7f70` ×2 | `0x5b1890` also requires: character present, manager request latch (`+0xa4`) clear, property record known, the definition type known (`0x7f4fa0`) and single-property (`0x7f6030`), plus `0x5ad900(character)` and the character's `+0x5c` flags. Full read todo. |
| Veil of Alaris for guild plots | `0x1536` | `0x5b1890` | done (`windows/RealEstatePurchaseWnd.md`) |
| Inside a plot or house | "You must be inside a plot or house to use that." (`0x2d7`) | `0x747e50` (Manage window), `0x50d780`, `0x7e0de0`, `0x938fc0` | done (Q10: a known record for manager `+0x98`) |
| In a neighborhood | "You must be in a neighborhood to use that." (`0x1f8e`) | `0x74bb00` (Plot Search window) | done (Q10: manager `+0x90` is a neighborhood type) |
| Gold membership (bag slots) | `0x1558` | no direct push found | not real estate: "If you wish to gain access to %1 bag slots, you must purchase House of Thule and be a Gold member." (the expansion's inventory slots) |

### Control enabled state ("dark" controls)

A window can be visible while its controls stay disabled. The client's enable primitive is **`0x866610(enabled, depth)`**: it writes the control's enabled byte (`CXWnd +0x1a`) and applies the same value to child controls down to `depth`. Visibility is separate, through vtable `+0xd8`. For every real-estate window, the conditions feeding each control's `0x866610` and `+0xd8` calls go into the state model.

**Worked example: Purchase window** (state computed in `0x74dcb0` when the window is filled from a property record, applied at `0x74e124`):

| Window byte | Drives | Condition |
|---|---|---|
| `+0x22c` | `REPW_Purchase_Button` **enabled** | `!occupied && pricesLoaded`. `occupied` = `0x5b41d0(record)` (the plot has any owner or guild; see `windows/RealEstatePurchaseWnd.md`). `pricesLoaded` = return of `0x74d240(definition +0x20 purchase costID, priceCombobox, …)`, true only when the definition's cost element from **`0x2698`** has options. |
| `+0x22d` | (second control; to identify) | `!canPurchase && (0x4ff6c0(0xc, record, character) \|\| isOwner)` |
| `+0x22e` | `REPW_Purchase_Button` **hidden** | `!canPurchase && (isOwner \|\| isGuildMemberOfGuildPlot)` |

- `isOwner` = `0x4d6b10(character, record) || 0x5ad900(character)`
- `isGuildMemberOfGuildPlot` = definition type is a guild type (`0x7f6280`) && property record **`+0x24`** (the owning guild id) ≠ -1 && `+0x24` == the character's guild id (`[0xdd261c]+0x204c`)

**Test preconditions this implies:**
- `0x2698` must have delivered the property's definition **and** its cost element with at least one option before the Purchase window is filled (from `0x2691` single-property, `openPurchase`). Otherwise the Purchase button is disabled.
- The property record must not look owned by the character.
- For guild plots, the record's guild id and the character's guild id must line up.

Still to identify: **what sets the character's `+0x5c` expansion flags** (which server message and field), and what profile `+0x340c` is. These decide whether any real-estate window will open at all, so they are next.

## 3b. Prior research

**Done** ([`priorResearch.md`](priorResearch.md)): the EQEmu docs page's four open questions answered (Place: the manager's current property, the item lists, and the placeable bitmask EQEmu zeroes; "paid status": expansion bits; the multi-purpose opcode is `0x2691`; the interior crash: zone-level, with corrected zone ids), every capture classified against RoF2, their TODO list closed, and the next walls an implementation hits listed.

## 4. Robustness review (crash avoidance)

For every server → client handler: what malformed or unexpected input does to the client (short reads, counts, null lookups, missing windows, ordering assumptions). This is the section that most directly prevents crashes during live testing.

| Handler | Status |
|---|---|
| House interior zone-in crash reported by prior research (see `priorResearch.md`) | reviewed: no housing-handler crash path; zone-level causes and a test plan in `robustness.md` (Q11) |
| all 31 housing handlers | done (`robustness.md`): lookups all null-checked, allocations overflow-safe, window pointers guarded; **one real risk: `OP_GuildUpdate` action 5 indexes the rank table without bounds** |

## GUI folder review

The full default UI folder (190 XML files) was checked for housing: every `EQUI_RealEstate*`, vote and trophy window is analyzed; housing controls in other windows are the inventory's **Plot Items** button, the three **placement options**, and the guild window's (unbound) **Trophy Tribute Opt Out**, all recorded. `EQUI_LayoutCopyWnd` is UI layouts, not housing layouts.

## Slash commands and key bindings

All 366 + 23 slash commands and 484 key bindings checked; housing: `/realestate`, `/leaverealestate` (sends `0x1da9`), key bindings 429–447 and 356 (`commandsAndKeybindings.md`).
