# RealEstatePurchaseWnd — Control Analysis

One row per control the client looks up (from [`../guiControlInventory.md`](../guiControlInventory.md)). A row is **done** only when every condition is expanded to data leaves (a field with a known origin: server message and field, client action, or initialization step) and every message involved is named.

Columns: **Enabled when** / **Shown when** (the conditions feeding `0x866610` and vtable `+0xd8`), **On use** (handler branch, client checks in order, message sent), **Populated by** (server message and field), **Server data** (the stored entity or field this implies), **Status**.

| Control | Type | Binding | Enabled when | Shown when | On use | Populated by | Server data | Status |
|---|---|---|---|---|---|---|---|---|
| `REPW_OwnerVal_Label` | Label | `+0x264` | — | — | — | guild-type definition: guild name for record `+0x24` (via `0x425670`, "None" if -1); otherwise record `+0x2c` owner name ("None" if empty) | property owner name, owning guild id | done (fill) |
| `REPW_GuildVal_Label` | Label | `+0x26c` | — | inside `REPW_Guild_Screen` | — | guild name of the nearest Neighborhood ancestor's guild (`0x4d6790(record, 0x10, 0)`, then `+0x24`); text color **red** (`+0x12c` = `0xffff0000`) when that guild ≠ the character's guild (`+0x204c`); emptied (color -1) when there is none | neighborhood guild (seen on Live: a vacant guild plot in a guild village shows Owner "None" and Guild the village's guild) | done |
| `REPW_Guild_Screen` | Screen | `+0x268` | — | `+0x230` = the nearest **Neighborhood** ancestor (`0x4d6790(record, 0x10, 0)`) exists and its guild `+0x24` ≠ -1 | — | — | property parent chain (`+0x1c`), neighborhood guild | done |
| `REPW_TypeVal_Label` | Label | `+0x260` | — | — | — | "Guild" if the definition type == 8 (Guild Plot), otherwise "Player" | definition type | done |
| `REPW_Price_Combobox` | Combobox | `+0x234` | always (the client never disables it) | always | no click path of its own; the Purchase click reads its selection | cleared, then `0x74d240(definition +0x20, combobox, -1)`: one entry per option of the definition's **purchase** cost element (`0x2698`); the return value feeds `pricesLoaded` | definition purchase cost id, cost options | done (fill); no click path of its own (read by Purchase) |
| `REPW_Upkeep_Combobox` | Combobox | `+0x238` | always (the client never disables it) | always | see *Upkeep combobox sequence* below: acts on manager `+0x98` (the plot you are on); GM or permission `0xc` → apply locally (record `+0x3c`) and send `0x2aa6` type 2 (property, option key); vacant → silently nothing; otherwise a message (`0x14c7` / `0x1523`) and the selection reverts | cleared, then `0x74d240(definition +0x24, combobox, record +0x3c)`: the **upkeep** cost element, with the property's current upkeep option selected | definition upkeep cost id, property upkeep option | done |
| `REPW_CapacityVal_Label` | Label | `+0x23c` | — | — | — | definition `+0x28` (max items) | definition | done (fill) |
| `REPW_Vault_CapacityVal_Label` | Label | `+0x240` | — | — | — | definition `+0x2c` (max non-real-estate items, i.e. vault) | definition | done (fill) |
| `REPW_Npc_CapacityVal_Label` | Label | `+0x244` | — | — | — | definition `+0x48` (max pets), or `+0x28` if negative | definition | done (fill) |
| `REPW_Category_Label` | Label | `+0x248` | — | — | — | dbstr category **27** text for definition `+0x40`; empty if not found | definition category id | done (fill) |
| `REPW_Manage_Button` | Button | `+0x258` | fill: `occupied(window record)`; **then live** (`0x74d380`): the plot **you're on** (manager `+0x98`) is occupied | always | toggles the Manage window through subcommand `manage` (`0x4e4e60`); **no preconditions**, and the window has no show gate | — | — | done |
| `REPW_Items_Button` | Button | `+0x254` | always (the client never disables it) | always | toggles the Items window through subcommand `items` (`0x4e4e60`); **no preconditions**, and the window has no show gate | — | — | done |
| `REPW_Purchase_Button` | Button | `+0x24c` | `+0x22c` = `!occupied && pricesLoaded`. `occupied` = `0x5b41d0(record)` (any owner or guild); `pricesLoaded` = the price fill (`0x74d240`) returned true | `+0x22e == 0` | notification 1: purchase gate `0x5b1890(propertyID, 1)` (see *Purchase gate* below, 11 checks) → price combobox must exist → 188-byte struct: `+0x00` property id, `+0xac` price option key, `+0xb0` upkeep option key, `+0xb4` `[0xdd5d00]` (unknown) → **send `0x45be`** → hide the window | — | — | done |
| `REPW_Sell_Button` | Button | `+0x250` | always (only shown or hidden) | `+0x22e` = `!canPurchase && (isOwner \| sell gate `0x5b3430` (see *Sell gate* below; acts on the property the player is **standing in**) → dialog `0x64` "Are you sure you wish to sell this plot?…" (`0x1dd2`) → yes: send `0x792c` | isGuildMemberOfGuildPlot)` | the property the player is standing in (manager `+0x94`) and its record | done (sub-checks in Q20) | — | done (sub-checks in Q20) |
| `REPW_Transfer_Button` | Button | `+0x25c` | always (only shown or hidden) | `+0x22f` = `+0x22e && !0x7f6280(definition type)` (no transfer for guild plots) | transfer gate `0x5b1380` (see *transfer gate* below; the standing-in property; target must be a living player) → dialog `0x65`, text `0x1dd3` with the target's name → yes: send `0x507f` | — | — | done (sub-check `0x57bfb0` in Q20) |
| `REPW_VotingOptOut_Label` | Label | `+0x274` | — | `+0x22d` = `!canPurchase && (0x4ff6c0(0xc, record, character) \|\| isOwner)` | — | done | — | done |
| `REPW_VotingOptOut_Checkbox` | Button (checkbox) | `+0x278` | **per frame** (`0x74d380`): `+0x22d` AND the player is on this plot (manager `+0x98`/`+0x9c` == window `+0x228`) | `+0x22d` | property = manager `+0x98` (the plot you are on); record must exist; new state = checkbox `+0x1e4`; `0x5b09d0(property, state)` **sets record `+0x44` locally** (true if the record exists) → send **`0x6573`** (8 bytes: `u32 propertyID`, then a u32 whose **low byte** is the opt-out flag; the upper 3 bytes are **uninitialized stack**) → the checkbox is refreshed from record `+0x44` (now the new value) | checked = property record `+0x44` | property voting opt-out flag | done |
| `REPW_Vote_Button` | Button | `+0x270` | **live** (`0x74d380`, every frame while visible): **onThisPlot** (window `+0x228` == manager `+0x98` or `+0x9c`) AND the record for manager `+0x98` exists AND is **occupied** AND its voting opt-out `+0x44` == 0 | always (no visibility logic) | requires the poll window `[0xd1fd50]`; `0x795fc0(manager +0x98)` (poll window mode 1) → the poll manager has polls → show the poll window; none: `0x1de2` "There are no active polls to vote on." | poll data: the poll manager, loaded by `0x15e2`, `0x3311`, `0x39bb`, `0x52b5`, `0x6860`, `0x7497` (`0x593a` separately); see *Poll window* | polls per property (poll subsystem, Q24) | done (control); the poll payload formats are tracked as the separate poll subsystem (Q24) |
| `REPW_Eviction_Label` | Label | `+0x27c` | — | — | — | only if the control exists: definition type Guild Plot → `0x3416` "Plots will pack up all items, send them to the owner, and become available for purchase again if no member of [the guild…]"; otherwise → `0x3415` "…if the owner of [the plot has not logged in within 90 days]" (`0x74e3f4`–`0x74e4fc`); the patcher replaces both with Live's escrow wording (Q24) | the 90-day reclaim after the escrow expired (RCL-1) | done |

## Fill routine `0x74dcb0(record)`

Called to populate the window from a property record (for example from `0x2691` single-property with `openPurchase`). Checks run in this order; **any failure skips the whole fill and leaves the previous contents and control states in place**:

1. The record must be non-null.
2. The manager's definitions table (manager `+0x74`) must exist, i.e. `0x2698` has been received.
3. The definition for record `+0x20` must be found (`0x7f4f70`).

Then:
- window `+0x228` = record `+0x14` (the property id)
- title = record `+0x38`, with `" (%d)"` and the property id appended when the character's virtual `+0x90` is true (**the GM flag**, spawn `+0x25c`; Q7)
- both comboboxes are rebuilt
- the state bytes are computed (`+0x22c` canPurchase, `+0x22d` canManageVoting, `+0x22e` ownerView, `+0x22f` canTransfer, `+0x230` hasGuildNeighborhood)
- labels are set, and enabled and visible states are applied

### Expanded predicates

| Predicate | Meaning (verified) |
|---|---|
| `0x5b41d0(record)` **occupied** (was misnamed "owned") | the definition type (`0x7f4fa0(record +0x20)`) is **Plot (4)** and owner name `+0x2c` is non-empty, **or** the type is **Guild Plot (8)** and guild `+0x24` ≠ -1. So Purchase is enabled only on a **vacant** plot, and Manage only on an occupied one. |
| `0x7f6280(type)` | type == 8 (Guild Plot) |
| `0x7f6270(type)` | type == 4 (Plot) |
| `0x7f60f0` / `0x7f61b0` / `0x7f6030` (type) | all use the same type → category map (1 Regular Zone; 2 Guild Hall and Player House; 3 Plot and Guild Plot; 4 Neighborhood; 5 Town; 6 Moving Crate; type 10 → 8; type 11 → 9) and test category 2 / 6 / 3 respectively |
| `0x7f5e30(type)` | type → mask bit `1 << (type-1)` (Plot `0x08`, Neighborhood `0x10`, Moving Crate `0x40`, Guild Plot `0x80`, type 10 `0x200`, type 11 all bits) |
| `0x4d6790(record, typeMask, skipLevels)` | walks up through **record `+0x1c` (parent property id)** in the manager's property map (at most 10 levels); returns the first ancestor past `skipLevels` whose type bit is in `typeMask`, or null |
| `0x7f62a0()` = `0x10`, `0x7f62b0()` = `0x88` | masks: Neighborhood; Plot \| Guild Plot |
| `0x5ad900(character)` | character virtual `+0x90` (the same call that adds the id to the title; **the GM flag** (spawn `+0x25c`; Q7)) |
| `0x4d6b10(characterName, record)` **isOwner** | if the record is a house (category 2) or crate (category 6), move to its containing plot (`0x4d6790(record, 0x88, 1)`); then require type == **Plot (4)**, a non-empty owner name, and `0x8068c0(ownerName, characterName)` (**not read yet**; presumably a name comparison). `characterName` is at character profile `+0x44` (not verified). |

Consequences:
- `isOwner` only applies to **player** plots; guild plots use the guild id comparison instead.
- `REPW_Guild_Screen` shows when the nearest **Neighborhood** ancestor has a guild (a guild neighborhood).
- Property record `+0x1c` is the **parent property id**, which gives the hierarchy used by every "containing plot" and "neighborhood" lookup.

| `0x74d240(costID, combobox, selectKey)` **cost combobox fill** | looks up the cost element in the manager's costs table (`+0x78`); **not found → false and the combobox stays empty**. For each option, the price text comes from `0x5aa760`; **options whose text is empty are skipped**; the rest are added with their option key, and the one matching `selectKey` is selected. Returns **false if the combobox ends up empty**; otherwise it selects the first entry if nothing matched and returns true. So `pricesLoaded` needs the purchase cost element from `0x2698` with at least one option that renders non-empty text (when `0x5aa760` renders empty is still to read). |
| `0x8068c0` | exact, byte-by-byte, **case-sensitive** CXStr equality; null and empty compare equal. The owner name must match the character name exactly. |
| `0x4ff6c0(0xc, record, character)` | the permission evaluator; see [`../permissions.md`](../permissions.md). Permission `0xc` requires the House of Thule bit, then ownership or the access list. |

| `0x5aa760(out, costElement, optionIndex)` **price text** | out = ""; `itemCount = 0x7fb970(optionIndex)`; **itemCount ≤ 0 → the text stays empty → `0x74d240` skips the option**. Otherwise each cost item `(currency, subKey, amount)` (`0x7fb9b0`) is rendered by `0x5aa5e0` (coin → formatted amount; alt currency → name via the character's alt-currency table `0x7f7e70`) and joined with a separator (`0x9c45dc` between items). **Rule: every cost option needs at least one cost item**, or it disappears from the combobox. Whether an alt currency missing from the character's table renders empty is not verified. |

Character virtual `+0x90` is **the GM flag** (spawn `+0x25c`, the spawn bitfield `gm`; Q7).

### Purchase gate `0x5b1890(propertyID, showMessages)`

Checks in order. Messages print (chat, color `0xd`) only when `showMessages` ≠ 0; the Purchase button passes 1.

| # | Check | On failure |
|---|---|---|
| 1 | character `[0xdd261c]` present | deny (silent) |
| 2 | manager request latch `+0xa4` clear | deny (silent) |
| 3 | property record in the manager's map (`+4`/`+8`) | deny (silent) |
| 4 | definition type is the plot category (`0x7f6030`: Plot or Guild Plot) | deny (silent) |
| 5 | **GM override** `0x5ad900(character)` (virtual `+0x90`) | **allow**, skipping 6–10 |
| 6 | `0x5b5b60(record, expansionFlags)`: under a Guild Plot → needs **`0x20000`**; otherwise → needs **`0x10000`** | `0x10000` missing: `0x14c3` "You must purchase the House of Thule expansion before you can own a plot."; `0x20000` missing: `0x1536` "You must purchase the Veil of Alaris expansion before you can buy a guild plot." |
| 7 | guild plot only: `0x5b5b90`: membership **feature 10** allowed (character virtual slot 0, argument 10) | membership message from `0x7dbd10(…, 10, 1, 1, 1)`: "Your access to a number of guild hall functions has been disabled because you do not have at least a %s membership" (or "…until you purchase this feature") |
| 8 | NOT occupied (`0x5b41d0`) | `0x191b` "You may not purchase a plot that is already owned." |
| 9 | `0x5b5130(record, character guild)`: the nearest **Neighborhood** has no guild, or its guild == the character's guild | `0x14c5` "…this is a guild neighborhood and you are not a member of the guild." |
| 10 | guild plot only: the character has a guild (`+0x204c` ≠ -1), and **guild rank permission 27** (`[0xdd5cf8]->0x4241f0(0x1b)`) | `0x2f3` "…you are not in a guild!"; `0x2f2` "…your guild has not granted you the ability to buy guild plots." |
| 11 | — | **allow** |

**New dependencies this exposes:**
- **Account membership:** membership feature 10 must be allowed for guild plots. The feature table is at `0xa0aa28` (22 entries, for example 10 = "access to a number of guild hall functions"). It's evaluated by the membership manager (`0x7dbc50` → `0x7db9c0`); its data comes from **`OP_SendMembership`** (the player's values, entry 10) and **`OP_SendMembershipDetails`** (the tier table); **stock EQEmu already passes** (`permissions.md`, Q21).
- **Guild rank permissions:** guild permission 27 ("buy guild plots"), from the static guild object `0xdd5cf8`'s rank table, filled by **`OP_GuildUpdate` action 5**; every rank starts without it (`permissions.md`, Q22).

### Sell gate `0x5b3430(unused, showMessages)` and transfer gate `0x5b1380(target, showMessages)`

**Both act on manager `+0x94`, the property the player is standing in** (position lookup, falling back to the `0x6fd0` current property), **not** on the property the Purchase window shows (`+0x228`); the sell gate ignores its first argument. **Selling and transferring therefore require standing in the plot.**

**Sell gate:**
1. Character present, request latch clear, and a record for `+0x94` exists; otherwise deny silently.
2. `code = 0x5b7fa0(character, record, GM)`; if ≠ 0 → print the **sell result text** for that code (`0x5ad930`, the same table as server `0x66bf`; see `resultCodes.md`) → deny.
3. Guild Plot: needs **guild rank permission 28** (`0x4241f0(0x1c)`), otherwise `0x2f5` "…your guild has not granted you the ability to sell…" → deny.
4. Allow → dialog `0x64` → yes → `0x792c`.

**Local sell check `0x5b7fa0`** (0 = OK; the codes are `0x66bf`'s):

| Condition (in order) | Code | Text |
|---|---|---|
| GM | 0 | — |
| player Plot, character not the owner (`0x4d6b10`) | 2 | `0x149d` |
| Guild Plot, Veil of Alaris bit `0x20000` missing | 8 | `0x1537` |
| Guild Plot, membership feature 10 missing | 13 | membership message (`0x7dbd10`) |
| Guild Plot, **escrow not empty** (`0x5b5ca0`: any non-zero amount in the record's balance map) and **no permission 2 "Withdraw Upkeep"** | 10 | `0x2f0` "You may not sell this guild plot because it has funds in escrow and you do not have withdraw upkeep access." |
| Guild Plot, character guild ≠ plot guild | 9 | `0x2ef` |
| **the plot's item container isn't loaded** (`0x57c3c0`: manager container map `+0x64`/`+0x68`, and the container reports data; i.e. the client hasn't received the plot's `0x1cef` list) | 1 | default |
| player Plot, House of Thule bit missing | 7 | "You must either empty all of the items from this plot or purchase the House of Thule…" |
| **a non-empty moving crate in the character's inventory or bank** (`0x57ef90`: item search `0x5b7f10` / `0x5b68a0` over character stores `+0x1ea0`, `+0x1ed8` and a third) | 3 | "You already have a non-empty moving crate in your inventory or bank…" |
| **no placed moving crate on the plot** (`0x5b47d0`: the plot's container, a crate-kind definition `0x7f62c0`, `0x5b45b0`) | 4 | "…the plot does not contain a placed [crate]…" |
| **the placed crate has no room** (`0x5b65b0`) | 5 | "…the placed crate cannot add another…" |
| **no permission 3 "Remove Crate"** (`0x4ff6c0(3, …)`) | 6 | "…you do not have permission to remove…" |
| active moving crate conditions (`0x5b7e10(6, …)` -> `0x5b7930`, `0x4234e0`, `0x7b4b70`; internals not expanded) | 11 / 12 | "You may not sell this plot currently because the active moving crate on it…" |

Selling depends on **moving-crate state** (a placed crate on the plot with room, and no non-empty crate already held), **the plot's item list being loaded**, **permission 3 (Remove Crate)**, and on guild plots an **empty escrow or permission 2**. The server enforces the same rules (Live: codes 3, 4, 5, 6 and 10, then the confirmation 7634), packs the placed crate with the plot's items into the seller's inventory, pays out the whole escrow and refunds no price (STT-1), and answers with `0x66bf` codes.

**Transfer gate:**
1. Local player present, request latch clear, and a record for `+0x94` exists.
2. The target must be non-null, `+0x125` == 0, and not rejected by `0x8cfcd0`; otherwise `0xcce` "You must first target a living Player Character." → deny.
3. `code = 0x5b50b0(me, record, GM, target)`; if ≠ 0 → print the **transfer result text** (`0x5adad0`, the same table as server `0x68ba`) → deny.
4. Allow → dialog `0x65` with the target's name → yes → `0x507f`.

**Local transfer check `0x5b50b0`:** requires a target and a record, a **player Plot** (type 4; **guild plots cannot be transferred**), `0x57bfb0(…)` ≠ 0 (**the plot has an item container on the client**, a manager map lookup), and the character to be the owner (`0x4d6b10`); otherwise code 1.

**The server's transfer (Live, STT-7 and STT-8):** the target is a client in the owner's zone and the owner stands in the plot; the offer (`0x5a63`, eqstr 7636) waits 300 s in the zone; refusals are eqstr 5279, 7637, 761, 5316, 5318, 8081 and 5280; the escrow, the items, the upkeep option and the access list stay with the plot.

### Manage and Items buttons: real-estate subcommands `0x4e4e60`

`0x4e4e60(character +0x2dd0, subcommand)` is the real-estate subcommand dispatcher (strings `items`, `manage`, `trophies`, `plot`). **It has no preconditions:** each subcommand toggles its window's visibility (`vtable +0xd8(!visible, 1, 1)`, where visible = window `+0x196`) if the window exists:

| Subcommand | Window |
|---|---|
| `items` | Items `[0xd1fcb8]` |
| `manage` | Manage `[0xd1fcbc]` |
| `trophies` | Trophy window `[0xf75ad8]` |
| `plot` | Plot Search `[0xd1fcc4]` |

Unrecognized subcommands take a message path (`0x4e4f45`, not read).

**Window show gates:** Items, Neighborhood, Plot Search and Purchase use the base show (`0x865290`) unchanged. Manage overrides it (`0x7409d0`) only to clear global `[0xdd264c]` when hiding. **None of the real-estate windows gates its own display on expansion or membership** (unlike the Trophy Tribute window, `0x483b10`). They always open; the checks live on their controls and actions, which matches the "window opens but its controls are dark" reports.

### Upkeep combobox sequence (notification on `+0x238`)

1. Selected index (`0x86a780`) = -1 → nothing.
2. Property = **manager `+0x98`** (the plot the player is on; see below), **not** the window's property. No record → nothing.
3. GM (`0x5ad900`) → apply (step 7).
4. Permission `0xc` (manage access, `0x4ff6c0`) → apply.
5. Not occupied (`0x5b41d0` false) → **nothing, silently** (the combobox keeps the new selection; nothing is applied or sent).
6. Occupied without permission → the House of Thule bit is required unless the plot is under a Guild Plot. Missing → `0x14c7` "This activity is disabled until you own the House of Thule expansion."; otherwise → `0x1523` "You cannot do that because you do not have manage access." Then the combobox is refilled from the definition's upkeep cost (`+0x24`) with record `+0x3c` selected, reverting the choice. **The Veil of Alaris branch (`0x1535`) can never fire here**: the required mask is only ever 0 or `0x10000`.
7. Apply: `0x5aee30(property, optionKey)` sets record `+0x3c`; if the record exists → send `0x2aa6` type 2 (`M+0x04` = property, `M+0x08` = option key).

### Live enable updater `0x74d380`

Runs **every frame while the window is visible** (vtable slot `+0xc4`: calls it when `+0x196` (visible) is set, then the base per-frame routine `0x865f20`) and also from slot `+0xdc` when global `[0xdcee30]` is set. It **overrides** the fill routine's Manage rule and adds the Vote and opt-out rules:

- `onThisPlot` = window property `+0x228` == manager `+0x98` (the plot you're on) **or** == manager `+0x9c` (a second tick-computed id, written by `0x5b0000`; meaning **open**).
- **`REPW_Manage_Button` enabled** = the record for manager `+0x98` is **occupied** (false if there's no record).
- **`REPW_Vote_Button` enabled** = `onThisPlot` AND that record exists AND is occupied AND its voting opt-out `+0x44` == 0.
- **`REPW_VotingOptOut_Checkbox` enabled** = `+0x22d` (may manage voting) AND `onThisPlot`.

So these three states **change as the player walks on or off the plot**. The Manage rule set in the fill routine only holds until the next frame.

### Manager property ids (context for the actions above)

| Field | Meaning | Set by |
|---|---|---|
| `+0x8c` | current property from the server | `0x6fd0` |
| `+0x94` | property the player is **standing in** | tick `0x5b0a10`, once per second: position lookup `0x4b3040`, falling back to `+0x8c` |
| `+0x98` | **the plot the player is on**: the nearest Plot or Guild Plot ancestor of `+0x94` (itself included), or -1 | tick → `0x5b00e0` |

Sell and Transfer use `+0x94`; the upkeep combobox, the voting opt-out and Vote use `+0x98`. **None of them uses the property the Purchase window is showing (`+0x228`)**; only the Purchase button does.

### Character interface and expansion flags (resolved)

**Layout.** The character object (global `[0xdd261c]`, constructor `0x57a2a0`) has a virtual-base table **`0x9d6d38`** at `+8`, whose `[+4]` is `0x31e4`. The interface the gates call (`ecx = char + 8 + [[char+8]+4]`) therefore sits at **character `+0x31ec`**, with vtable **`0x9d6ba8`** (written at `0x57a326` as `[ecx+esi+8]`).

| Slot | Function | Meaning |
|---|---|---|
| 0 | `0x57d250` → `0x5784e0` | membership feature check (argument = feature id); not read |
| `+0x5c` | `0x4608c0`: `mov eax, [ecx+0x45c]` | **expansion flags** = interface `+0x45c` = **character `+0x3648`** |
| `+0x90` | `0x57d280` → `0x580300` | true iff the member object at (adjusted this − `0x3ec`) exists and its virtual `+0xdc` returns true. **Probably the GM flag; not confirmed.** |

(The vtables `0x9d6ba0`/`0x9d6ba4` belong to a separate 0x20-byte helper object stored at character `+0x31b8`, not to the interface.)

**Where the expansion flags come from (Q1).** Every real-estate gate reads interface `+0x45c` (character `+0x3648`) through virtual `+0x5c`. **Every writer of that field:**

| # | Source | Field | Evidence | Status |
|---|---|---|---|---|
| 1 | **`OP_PlayerProfile`** (`0x6506`) | the u32 right after the profile's name (length + 64 bytes), last name (length + 32 bytes) and five u32s (birthday, account start, last login, time played, time entitled). **Exactly EQEmu RoF2 encoder field `emu->expansions`** (`common/patches/rof2.cpp`, `WriteUInt32(emu->expansions)`) | handler `0x4cc34b` → `0x57ab20` (checksum, parse) → parser `0x57e320` reads it into parsed profile `+0x5074` (`0x57e405`) → apply `0x579740` copies it: `mov [edx+ebp+0x464], eax` (`0x579933`), i.e. interface `+0x45c` | **verified**, client code and EQEmu encoder agree |
| 2 | **opcode `0x0767`** (not named in EQEmu's conf): a full **expansion update**, with no text | payload **`+0x08`** (u32) | case `0x4ccaf6`: `setter 0x4b3970([payload+8])` at `0x4ccb2c`, then the same value's bits are copied into the global per-bit bytes `[0xde09c0 + bit]` (the ones `OP_ExpansionInfo` sets) | **verified**: sets the character flags and the globals; the other payload fields aren't read here |
| 3 | **opcode `0x0e48`**: **"Your expansion settings are: …"** (a report about your own expansions; handler `0x4c64c7`, shared with `0x2b10`) | payload **`+0x44`** u32 flags (`0x4c6547`), `+0x48` byte | `0x4c6c4d` → `0x4b3970` **writes your character's flags**, then prints each expansion's name ("None" if 0). **`0x2b10`** = "The expansion settings for %s are: " about **another player** (name at payload `+0x04`, flags `+0x44`, byte `+0x48`); it only prints and **doesn't** write | **verified** |
| 4 | named key/value store, key **`"ExpansionFlags"`** | text value converted with atoi (`0x80e1b0` → `0x80e0e0`) | see *Source 4* below | **trigger not determined**; no network handler reaches it through direct calls |

Not a writer: **`OP_ExpansionInfo`** (`0x590d`, `0x4b6e50`) copies its payload `+0x40` only into globals (`[0xde09d4]` and per-bit bytes `[0xde09c0+bit]`; bit 16 = `[0xde09d0]`). Those are used by `0x483b10` only when no character is loaded.

**Source 4: the named character archive.** Established so far:
- It's a **complete named save/load format for the character**: 9 loader functions read **282 keys** through the archive's typed getters (`0x80e1b0` int; siblings `0x80e1e0`, `0x80e220`, `0x80e250`, `0x80e280`, `0x80e2c0`, all built on the lookup `0x80e0e0`/`0x80e030`). Keys include AAs, achievements, `PDSFaction`, tribute, shared-vault coin, mercenary save strings, `ProfileCount`/`Profile%d`, `ExpansionFlags`. The archive is an abstract interface (its own virtual `+0x10`, and so on).
- The loader chain is entirely virtual. `0x7c5bf0` is **character interface virtual `+0x1c`**: the final vtable `0x9d6ba8` slot `+0x1c` → `0x57d270` → `0x57d100` (`add ecx, -0x3f0`) → `0x7c5bf0`; the base-class vtable `0xa04e44` (installed by constructor `0x7cce20`) routes the same slot through `0x7cd2f0`. `0x7e7be0` is referenced from vtables `0x9c5a34` and `0xa0e968`.
- **No direct caller of interface slot `+0x1c` was found.** A pattern scan found two candidates; both were false positives (`0x538b5d` calls an item's `+0x1c`, and `0x744876` is a `+0x5c` call).
- Working hypothesis (unverified): a save/restore path for the character (for example a local cache or a transfer format) rather than a live server message. **To settle during testing:** after the profile loads, read character `+0x3648` and confirm it matches the profile's `expansions`.

**Profile field `+0x340c` (the other condition in the Trophy-window gate `0x483b10`, which requires it to be 0).** It lives in the profile object returned by `0x7db210` (the same accessor used for cursor coins).
- It's a small **enum (0/1/2)** read by many systems: the expansion gates (`0x483a90`, `0x483b10`, `0x483b70`), guild creation (`0x4e3350`), ability sets (`0x607d50`, `0x60a8a0`), class animations (`0x693d90`), a timer display (`0x6d6660`), and more (52 reads in all).
- **Writers:** `0x7edaa0` (a sequential stream u32 read, called from `0x801060`; not reached from the `OP_PlayerProfile` parser by direct calls), `0x7ebb70` (the named character archive; the key string is built before `0x7ebc4b` and isn't identified yet), the setter `0x7bf270` (called only from the character method `0x5789b0`), the constructor/reset `0x7ef0e0`, and the copy `0x7ef330`.
- **Meaning not identified.** Next steps: the `0x7ebb70` key name, the callers of `0x5789b0` and `0x801060` up to their message, and the values 1/2 as the ability-set code uses them.

**Bit meanings** match EQEmu's expansion bitmask (House of Thule `0x10000`, Veil of Alaris `0x20000`).

**Consequence:** the value the gates see is whichever of sources 1–4 wrote last. Sources 2 and 3 are server messages that **overwrite** the profile value (fields known); source 4 (the named key/value store) still needs its trigger found. To control the flags deliberately, every source that can be sent or triggered must agree.

**Client requirement:** every real-estate gate and permission reads these character flags. The client accepts real-estate actions only when the character's `ExpansionFlags` includes bit `0x10000`, and for guild plots bit `0x20000`.

### Purchase struct `+0xb4` (global `[0xdd5d00]`)

`0xdd5d00` = **guild manager object `0xdd5cf8` + 8** (the static object behind the guild-name lookup `0x425670` and the rank-permission check `0x4241f0`; 171 `mov ecx, 0xdd5cf8` sites). The only direct access is this read. Across 42 guild-manager methods, following `this` transitively, the only write to `+8` is a **reset to 0** in `0x423ba0`, which frees a linked list first. So it behaves like the **element count of a guild list**. For the server this is a field to log, not to rely on.

### Server replies the client handles for this window's actions

What the client does with each possible reply (the order a server sends them in is a design choice, recorded in `flows/purchase.md`):

| Reply (server → client) | Client handling | Evidence |
|---|---|---|
| **`0x45be`** purchase result | payload byte `[0]` ≠ 0 and the Manage window exists → **show the Manage window**; 0 → nothing | case `0x5b29f8` |
| **`0x66bf`** sell result code | chat line from the sell table (`resultCodes.md`) | `0x5ad930` |
| **`0x68ba`** transfer result code | chat line from the transfer table | `0x5adad0` |
| **`0x1228`** item request result | chat line; code 0 ends placement mode | `0x5ae1f0` |
| **`0x63fd`** property update | merges into the known record (compares owner name and `+0x40`); marks the Trophy window `[0xf75ad8] +0x28c` dirty when the owner matches the local character or ownership changes; refreshes the Items, Manage and Purchase windows | case `0x5b2dd8` |
| **`0x4f09`** / **`0x6fd0`** property record | refresh Manage (owner-dirty flag) and **remove the property**, so the server sends it only for a property that is gone / set the current property | `placedItemsAndProperties.md` |
| **`0x2aa7`, `0x3e00`, `0x66df`, `0x0c13`, `0x76c6`** character property lists | update the character's lists | `placedItemsAndProperties.md` |
| **`0x5a63`** transfer offer (to the recipient) | the accept dialog, whose answer is `0x5a63` | README |

After a purchase the client itself **doesn't** change the record's owner. The Purchase window's buttons only change once the server sends an updated property record whose owner name (or guild) makes `occupied` / `isOwner` true.

### Poll window ("Open Polls") used by `REPW_Vote_Button`

- Window global `[0xd1fd50]` (created in UI setup `0x4973f0`); code in `0x792d50`–`0x795fc0`, control prefix `VW_`; title string `0x1de3` "Open Polls".
- **Display modes** (window `+0x228`), both built by `0x7957c0`:
  - **1**: the polls for a property; set by the Vote button through `0x795fc0(property)` (property → window `+0x22c`).
  - **3**: server-pushed; set by `0x795b00(u32, string)` (u32 → window `+0x230`).
- `0x7957c0` reads the **poll manager**, a lazily created singleton (`0x5a72a0`, a 0x294-byte object at `[0xe643ec]`). An empty list → returns false → the Vote button prints `0x1de2` "There are no active polls to vote on."
- **Server → client `0x7a4d`** (`OP_PollOpen`): `u32`, then a length-prefixed string. Requires the poll window to exist; calls `0x795b00(u32, string)` (mode 3), and a true result shows the window.
- **Poll data loading** (second dispatcher switch):
  - handler `0x4d36bb` for **`0x15e2`, `0x3311`, `0x39bb`, `0x52b5`, `0x6860`, `0x7497`** → poll manager `0x5a7a20(opcode, stream)`, which has its own sub-switch: `0x39bb` → `0x5a70c0`, `0x3311` → `0x7fb780`, `0x15e2` → `0x5a7440`, `0x52b5` → `0x5a75f0`, `0x6860` → `0x7fb020` (then sets flag `+0x28e` and refreshes via `0x5a7360`), `0x7497` → `0x5a7840`
  - handler `0x4d3e44` for **`0x593a`** → `0x5a7420(payload byte 0)`
  - The payload formats are the **poll subsystem**, tracked as **Q24**.

### Placement / edit mode (cross-cutting)

No Purchase predicate, gate or updater calls into the placement state (checked 4 levels deep). **Couplings (resolved, `placementMode.md`, *Global effects*):** while placing, the window is **hidden** if `ItemPlacementHideUI` is on (UI mode 3) and otherwise **gets no mouse-over updates and (to confirm) no clicks**; its gates **refuse while a place request or commit is pending** (latch, up to 5 s); the location ids it depends on are **unchanged** by placement.

### Still open for this window

Manager `+0x9c` meaning (`0x5b0000`), and the poll subsystem payloads (Q24).

Controls the client never looks up (16), **checked**. All are static caption labels and layout screens: `REPW_Owner_Label` "Owner:", `REPW_Guild_Label` "Guild:", `REPW_Type_Label` "Type:", `REPW_Price_Label` "Price:", `REPW_Upkeep_Label` "Upkeep:", `REPW_Capacity_Label` "Capacity:", `REPW_Vault_Capacity_Label` "Vault:", `REPW_Npc_Capacity_Label` "Pets:", the screens `REPW_Owner_Screen`, `REPW_Spacer_Screen`, `REPW_Type_Screen`, `REPW_Price_Screen`, `REPW_Upkeep_Screen`, `REPW_Capacity_Screen`, `REPW_Purchase_Screen`, and `REPW_Main_Layout`. **Only `REPW_Guild_Label` changes indirectly:** it sits in `REPW_Guild_Screen` (bound), so it's visible only when that screen is, i.e. in a guild neighborhood. None of the other parent screens is bound, so the client never toggles them.
