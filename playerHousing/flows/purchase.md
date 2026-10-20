# Flow: Purchase a Plot

**Status:** client side decoded. Two small items remain: the purchase request's `+0xb4` value (the guild object's `+0x08`; not identified, and the server can ignore it), and confirming in testing that clicking a plot's sign sends `0x218c`.

## 1. Goal

A player standing in a neighborhood views a vacant plot, chooses a price option and an upkeep option, confirms, and becomes the owner. The Purchase window then turns into the owner's view (Sell, Transfer, Manage).

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| S1 | Character expansion flags include **`0x10000`** (House of Thule), and **`0x20000`** (Veil of Alaris) for guild plots | **`OP_PlayerProfile` `expansions`** (character `+0x3648`); `0x0767` / `0x0e48` can overwrite them | every real-estate permission requires it (`permissions.md`); the purchase gate checks it. **Send these bits even on an era-limited server.** |
| S2 | `profileType` (profile `+0x340c`) == 0 | `OP_PlayerProfile` (the u32 after the checksum; **EQEmu sends 0**) | only the Trophies window's gate uses it; listed so a non-zero value isn't a surprise |
| S3 | Definitions and costs received | server `0x2698` | the Purchase fill aborts without the definitions table; the price combobox needs the definition's purchase cost element (definition `+0x20`) with at least one option that renders non-empty text |
| S4 | The plot's property record known, plus its parent chain (`+0x1c`) up to the Neighborhood | any record source: `0x6fd0` / `0x4f09` / `0x63fd`, or the `0x2691` single-property reply itself (it creates the record) | the fill, "occupied", "is owner" and guild-neighborhood checks all walk records |
| S5 | The plot is **vacant**: player Plot with empty owner name, or Guild Plot with guild id -1 | property record | `0x5b41d0` occupied → Purchase disabled |
| S6 | No real-estate request pending (manager `+0xa4` clear) | client; clears on a reply or after 5 s | the purchase gate checks the latch |
| S7 | Local character present (`[0xdd261c]`, `[0xdd2630]`) | login | gates return false without it |
| S8 | Guild plots: the character is in a guild (`+0x204c` ≠ -1); **guild rank permission 27** "buy guild plots" (`0x4241f0(0x1b)`); **membership feature 10** | rank permissions: **`OP_GuildUpdate` action 5** (every rank starts with none; **EQEmu 23.8.1 sends them on zone-in from `guild_permissions`**); membership: `OP_SendMembership` / `OP_SendMembershipDetails` (**stock EQEmu passes**) | purchase gate steps 7 and 10 |
| S9 | Guild neighborhoods: the character's guild == the neighborhood's guild | property records (Neighborhood `+0x24`) | purchase gate step 9 |
| S10 | **The character is standing on the plot the window shows** (manager `+0x98`/`+0x9c` == window `+0x228`), and its position resolves through property records (or `0x6fd0`'s current property) | tick `0x5b0a10` (position lookup `0x4b3040`, falling back to `0x6fd0`) | Manage, Vote and Opt-out enabled states are recomputed **every frame** from this |

## 3. Sequence

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | S → C | `0x2698` definitions and costs | before any window fill (S3) |
| 2 | S → C | property records for the neighborhood and plots | any of the sources in S4; `0x6fd0` also sets the current property on zone-in |
| 3 | C → S | the player opens the plot | clicking the plot's sign (a placed world object) sends **`0x218c`** with its property and record ids (`worldLayer.md`; *inferred* that the sign is a world object, confirm in testing); the Neighborhood window (`0x2691` type 2) and Plot Search reach plots too |
| 4 | S → C | `0x2691` kind 4 (Plot) or 8 (Guild Plot), single-property, `W` > 0, `openPurchase` = 1 | creates or updates the record, fills the Purchase window (`0x74dcb0`) and shows it |
| 5 | client | fill → enabled states | Purchase enabled = vacant && prices loaded (§4) |
| 6 | player | picks price and upkeep options, clicks Purchase | an upkeep combobox change here may send `0x2aa6` type 2, only once its checks pass |
| 7 | client | purchase gate `0x5b1890(propertyID, 1)` | 11 ordered checks (`windows/RealEstatePurchaseWnd.md`): character, latch, record, plot category, **GM flag override** (spawn `+0x25c`), expansion bit, membership feature 10 (guild), vacant, guild neighborhood, guild membership and rank permission 27 (guild) |
| 8 | C → S | **`0x45be`** purchase request (188-byte struct): property id `+0x00`, **price option key `+0xac`**, **upkeep option key `+0xb0`**, `+0xb4` = the guild object's `+0x08` (not identified; ignore) | the window hides |
| 9 | S → C | **`0x45be` `{u8 success}`**: non-zero **opens the Manage window**; plus the **updated property record** (`0x63fd`: owner = buyer, or the guild id for a guild plot) and the character's property lists (`0x76c6` / `0x2aa7`) | a refusal: `0x45be` with 0 and a result code (`resultCodes.md`) |

## 4. Decision trees (fill and click)

Fill `0x74dcb0(record)`, aborting on any failure: record ≠ null → definitions table exists → definition found. Then:

- **Purchase enabled** (`+0x22c`) = NOT occupied AND pricesLoaded
  - occupied = (type == Plot && owner name non-empty) OR (type == Guild Plot && guild id ≠ -1)
  - pricesLoaded = the cost element `definition +0x20` exists AND at least one option has **≥ 1 cost item** (an option with no items renders empty and is skipped, `0x5aa760`)
- **Purchase visible** = NOT ownerView (`+0x22e`)
  - ownerView = NOT canPurchase AND (isOwner OR guildMemberOfGuildPlot)
  - isOwner = **the GM flag** (character virtual `+0x90` = spawn `+0x25c`) OR (containing Plot's owner name == character name, case-sensitive)
  - guildMemberOfGuildPlot = type == Guild Plot AND record guild id ≠ -1 AND == character guild id

Click (Purchase button, notification 1): purchase gate `0x5b1890` → price combobox exists → send `0x45be`.

## 5. Expected result per step

- After step 4: the window's title is the plot's name; Type = Player or Guild; Owner = "None"; capacity labels from the definition; price and upkeep lists filled; **Purchase enabled**; Sell and Transfer hidden.
- After step 8: the window hides.
- After step 9: **the Manage window opens** (success byte), and once the record update arrives the Purchase window shows the owner's view.

## 6. Failure branches (symptom → missing precondition)

| Symptom | Cause |
|---|---|
| The Purchase window never appears | step 4 missing, `openPurchase` = 0 or `W` = 0, or the fill aborted (S3/S4 missing: no definitions table or unknown definition) |
| The window appears with stale or blank data | the fill aborted; same causes as above |
| Purchase button dark (disabled) | the plot looks occupied (S5), or the price list is empty (S3: missing cost element or no renderable option) |
| Purchase button hidden, Sell/Transfer shown | the character looks like the owner (name match or the GM flag), or a guild member of this guild plot |
| Clicking Purchase does nothing, silently | the gate's silent checks: no character (S7), request pending (S6), the record missing from the manager map (S4), or the definition isn't a plot type |
| Clicking Purchase prints "…House of Thule…before you can own a plot" / "…Veil of Alaris…guild plot" | S1 (often: era-limited expansion flags) |
| "…because you do not have at least a … membership" | S8, membership feature 10 |
| "You may not purchase a plot that is already owned" | the record shows an owner (S5) |
| "…guild neighborhood and you are not a member…" | S9 |
| "…not in a guild!" | S8, guild membership |
| "…has not granted you the ability to buy guild plots", **even for the guild leader** | S8: the leader's rank lacks permission 27 in `guild_permissions` (EQEmu sends it with `OP_GuildUpdate` action 5) |
| Purchase succeeds on the server but the Manage window doesn't open | the `0x45be` reply is missing or its byte is 0 |
| Manage / Vote / Opt-out dark even though the window filled correctly | S10: the character isn't standing on that plot (or the plot isn't occupied, or it has opted out); recomputed every frame |
| Sell or Transfer act on a different plot than the window shows, or do nothing | S10: they use the plot the character is **standing in** (manager `+0x94`) |
| Changing the upkeep option snaps back, with "…House of Thule…" / "…Veil of Alaris…" / "…manage access" | S1, or no manage permission |

**Note:** the real-estate windows open without any expansion or membership check; every gate sits on controls and actions.

## 7. Server obligations

- Send the House of Thule (and Veil of Alaris) expansion bits in `OP_PlayerProfile`, whatever the server's era.
- Send `0x2698` early, with every definition referenced by properties and each definition's purchase and upkeep cost elements.
- Send property records (with parent chains) before or with the `0x2691` single-property reply.
- For guild plots, grant permissions 27 and 28 in `guild_permissions` for the right ranks; EQEmu sends them with `OP_GuildUpdate` action 5 (ranks 1–8, permissions 1–30, inside the bounds the client doesn't check; `robustness.md`).
- Handle `0x45be`: validate the option keys against the definition's cost elements, the vacancy, funds, and the same rules as the gate, plus Live's server-only rules: the player in the plot's zone instance; at most 3 plots per character (eqstr 728) and 1 guild plot per guild (750); a guild plot needs a guild of at least 10 members (5426); a guild neighborhood sells only to its guild's members. Reply with `0x45be {u8 success}`, then `0x63fd` (new owner) and the character's property lists; on refusal, `0x45be` 0 and a result code. A new plot starts paid (`+0x40` = 0), settled now, its first charge one interval plus jitter away, with Live's default access list (`../permissions.md`).
- Handle `0x2aa6` type 2 (upkeep option) and confirm with type 2, flag 0 (`opcode2aa6PropertyAccount.md`).
- Answer guild-name requests for any guild id sent.

## 8. Test cases

1. Happy path: vacant player Plot, all state present → Purchase enabled → `0x45be` received with the right option keys → reply `0x45be` 1 → the Manage window opens → the record update shows the owner's view.
2. Missing `0x2698` → the window doesn't fill.
3. Cost element with no options → Purchase disabled.
4. Owner name set (occupied) → Purchase disabled; Manage enabled.
5. Owner name == character name → owner view (Sell, Transfer visible).
6. Guild Plot with the character's guild id → owner view; without it → Purchase (if vacant).
7. Clear `0x10000` in `OP_PlayerProfile`'s expansions → the purchase gate refuses with the House of Thule message.
8. Request pending (click twice within 5 s without a reply) → the second click is blocked.
9. Guild plot, leader's rank without permission 27 in `guild_permissions` → "…has not granted you…"; after granting it (and zoning, so EQEmu re-sends the ranks) → Purchase proceeds.

## 9. Open questions

The purchase request's `+0xb4` (guild object `+0x08`; ignorable); confirm in testing that a plot's sign sends `0x218c`.
