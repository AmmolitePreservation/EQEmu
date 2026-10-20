# RealEstateManageWnd — Control Analysis

One row per control the client looks up (from [`../guiControlInventory.md`](../guiControlInventory.md)). A row is **done** only when every condition is expanded to data leaves (a field with a known origin: server message and field, client action, or initialization step) and every message involved is named.

Columns: **Enabled when** / **Shown when** (the conditions feeding `0x866610` and vtable `+0xd8`), **On use** (handler branch, client checks in order, message sent), **Populated by** (server message and field), **Server data** (the stored entity or field this implies), **Status**.

## Lifecycle (chunk 1)

**Which property:** the window manages **manager `+0x98`, the plot the player is on** (not the property they're standing in). The house on it (`+0x9c`) is tracked too.

**Opening** (activate, vtable `+0xdc` = `0x747e50`): if `+0x98` is -1, or its record is unknown or empty, print **`0x2d7` "You must be inside a plot or house to use that."** and don't open; otherwise run the full refresh `0x747740`. Opened from the Purchase window's Manage button.

**While open** (per frame, vtable `+0xc4` = `0x748180`):
- **`+0x98` becomes -1 → the window hides** (leaving the plot closes it).
- **The plot isn't an owned plot → the window closes** (`0x5b41d0`: a player plot with an owner name (record `+0x2c` non-empty) or a guild plot with a guild (record `+0x24` ≠ -1)). **Any viewer may keep it open on an owned plot**; the pages gate their own actions.
- **`+0x98` changed** (vs cached `+0x22c`) → full refresh.
- The **Access page gate** (permission 12 or GM) is re-evaluated.
- **The plot or house changed** (cached `+0x22c`/`+0x230`) → reset the selection (`0x7410f0(-1)`) and call manager **`0x5aff80`** for the plot and for the house (*inferred:* request each property's account data; chunk 2).
- Upkeep page dirty (`+0x234`) and no request pending → refresh the upkeep page (`0x742cd0`).
- **Every 3 s** (`+0x228` timestamp): **`0x747ed0` flushes the queued access-list changes as `0x3146`**: for each queued **property id**, the id and **that property's full access list** (manager map `+0x54`, the map `0x485d` fills and the permission evaluator reads), serialized by `0x749420`. The queue (`+0x2c4`–`+0x2cc`, insert `0x748c70`) is fed only by access-list edits, which queue the plot id (`+0x98`): **`0x5b0b10`** set a name's/club's group (Add, club group change), **`0x5b0bb0`** remove an entry (Remove, club group change), **`0x5b0ad0`** set a group's permission bit (grid click). The `0x485d` handler also flushes it before applying a server list. *(Validated: an earlier note here called this upkeep prepayment; that was an assumption with no support. Upkeep adds are `0x2aa6` types 3–6, sent immediately.)*

**Full refresh** (`0x747740`): rebuilds the **per-currency daily-cost totals** at `+0x248`/`+0x24c`: one 0x18-byte element for **coin `{0, 0}`** and one for **each entry of the character's alternate-currency table** (`[0xdd261c] + 0x298c`, copying its currency id and subKey), amounts zeroed; the upkeep page adds costs into them (`+0x08`, u64) and stores each currency's days left (`+0x10`, f64) (chunk 3); the Access page gate; the **escrow property selector** (below); Property_Label; Transfer visibility; the upkeep dropdown (`0x741470`), balances (`0x741930`) and upkeep page (`0x742cd0`); the **plot-type defaults** of the Access page; the eviction text; the access lists (`0x744200`).

| Plot type | Middle view label | Checked view | List shown | Eviction text |
|---|---|---|---|---|
| **Guild plot** | "Guild Members" | Owner's Clubs / Guild Members | the clubs/members list | `0x3416` (30 days, guild) |
| **Player plot** | "Owner's Clubs" | Individuals | the names list | `0x3415` (90 days, owner) |

**Hide** (vtable `+0xd8` = `0x7409d0`): also clears the global `[0xdd264c]` (chunk 2). Vtable `+0x98` (`0x740960`): if visible, closes this window's text prompt (`TextEntryWnd` id `0x4c`); otherwise resets the cached ids.

**Messages sent by the window's code:** `0x2aa6` (escrow and upkeep account requests, chunks 2–3), `0x3146` (the batched access-list changes, chunks 4–5); Transfer's `0x507f` goes through the manager (`0x5b1470`).

### A control your XML doesn't define: `REMW_EscrowProperty_Combobox`

The constructor also looks up **`REMW_EscrowProperty_Combobox`** (binding `+0x2f0`), which **isn't in `EQUI_RealEstateManageWnd.xml`**, so the lookup returns null and every use is skipped. The refresh would fill it with the properties from `0x7490b0` (each property's name `+0x38` and id `+0x14`) and select `+0x98`: **a selector for whose escrow and upkeep the page shows** (the plot, its house, …). **Resolved (close-out):** it's display-only. The balances and upkeep pages read the plot directly, the notification handler has no branch for it, and nothing reads its selection; the manager only keeps its entries in sync (`0x743fd0` add, `0x744070` remove). Its absence from the XML costs nothing. (This was the only client-referenced name missing from any real-estate XML; the other five windows have none.)

## Escrow balances (chunk 2)

**Whose balances:** always the **plot** (manager `+0x98`): the balance list reads the plot record's **balance map** (`{u32 currency → f64}`, key 0 = coin) directly. It does **not** consult the escrow property selector, so the selector missing from the XML doesn't hide any balances.

**`REMW_Balance_List`** (fill `0x741930`; argument 1 = clear and rebuild, otherwise update the amounts in place): columns **(icon), Name, Quantity**. Rows: **coin** first, then **each alternate currency the character has** (`[0xdd261c] + 0x298c`) **that the real-estate cost table uses**: the filter `0x5adb50` → `0x5aa720` checks a currency list at manager `+0x78` that `0x5aaad0` derives from the **`0x2698` costs** right after they're read. **So only currencies that can pay for real estate can be escrowed.** Quantities come from the plot's balance map (coin shown per denomination via `0x542800`).

**Deposit and withdraw** (`0x741ee0`, called by a click on a balance row (`0x7459ed`, the row's currency) and by the quantity window's answer (`0x746581`, notification `0x24`, currency `+0x340` + quantity)):
- **The only client check: the plot's record is known and the plot is owned** (`0x5b41d0`). **No permission check and no pending-request check.**
- **Something on the cursor** (coin, cursor coin kind `[0xdd2660] + 0x11`; or an alt-currency item in cursor slot `0x21`) → **deposit: `0x2aa6` type 0** ("*Drop Currency Below to Deposit"; the XML hint).
- **Otherwise → withdraw:** the quantity window opens (capped by the balance, converted per denomination; alt currencies also by the character's data; at most 1,000,000,000) and its answer sends **`0x2aa6` type 1**.
- **Withdraw All** (`0x7459f7`): sends **`0x2aa6` type 7** for `+0x98` immediately, **no confirmation, no client check**.

**So any viewer standing on an owned plot can send deposits and withdrawals; the server alone decides who may withdraw** (and where Withdraw All's money goes).

**Balance updates** come from the server's property-record updates (`0x63fd`, which carry the balance map); the list refreshes on the window's full refresh and upkeep-page refresh. Nothing is requested by the window when the plot or house changes: manager `0x5aff80` only re-notifies the windows from the locally held item containers (`0x5ab1c0`).

`[0xdd264c]` (cleared on hide) is general client state also used by inventory, world entry and death; not real-estate data.

## Upkeep (chunk 3)

**Whose upkeep:** the plot (`+0x98`), read directly by the page fill `0x742cd0`, which doesn't consult the escrow property selector either.

**Which items have upkeep:** each row is a placed item whose **upkeep cost id** (item definition **`+0x5c4`**, `0x7b0b80`) matches a **cost element in the `0x2698` cost table** (manager `+0x78`, element key `+0x14`) that has at least one option. **Items without a matching cost element get no row** (no upkeep). The row's price is that element's cost option (count `0x7fb970`, items `0x7fb9b0`), formatted by `0x5aa5e0`.

**Where `+0x5c4` comes from** (the item deserializer `0x7f6bf0`): after the Heirloom byte the client reads u32 `+0x5ac` (placeable), u8 `+0x5b0`, u32 `+0x5b4`, `+0x5b8`, `+0x5bc`, `+0x5c0`, **u32 `+0x5c4` (upkeep cost id, bytes 21–24 after Heirloom)**, u32 `+0x5c8`, a string (`+0x5cc`), u8 `+0x608`, … **EQEmu's `ItemQuaternaryBodyStruct` doesn't match this layout after Heirloom**: bytes 21–24 straddle its `unknown27` and `unknown_RoF_6`, and `rof2.cpp` zeroes the struct and never sets them. **So with stock EQEmu no item has upkeep.** The encoder must write the upkeep cost id there (best by correcting the struct to the client's read order from Heirloom on).

**The plot's own upkeep ("Plot Upkeep"):** record `+0x20` → its definition → **definition `+0x24` (upkeep cost id)** → that cost element → **the option selected by record `+0x3c`** (`0x7413f0`) is added to the totals first.

**Daily cost = the plot's selected upkeep option + each item's upkeep option, net.** For every cost item of an option: **a positive amount is a cost** (shown in **Cost**), **a negative amount is a reduction** (shown in **Reduction**); both are added into the per-currency totals (`0x7411c0`, window `+0x248`). So **an item's upkeep option can reduce the plot's daily upkeep** with negative amounts. **The totals only have entries for coin and the character's alternate-currency table**, and `0x7411c0`'s result is ignored: **a cost in any other currency is left out of the totals and the time left** (it still shows in the Cost text).

**Time left** (second pass): per currency, **days = the plot's balance in that currency (balance map) ÷ the net daily total** (-1.0 = no limit). A row's **Time Left** is the minimum over its currencies and **Limited By** names the currency that runs out first. **There's no per-item paid-until data on the client:** time left is purely escrow ÷ daily cost.

**Summaries:** `REMW_TimeRemainingVal_Label` = the overall time (formatted **"%dw %dd"**, weeks and days, `0x740a50`) + " by " + a second formatted value (`0x486f30`); `REMW_LimitedItemVal_Label` = the name of the item whose upkeep runs out first, or **"Plot Upkeep"**; `REMW_DailyCharge_Label` = **"Daily Cost:"** + the net per-currency totals.

**Actions** (all `0x2aa6`, sent immediately, **no client-side checks**):

| Control | Type | Argument |
|---|---|---|
| `REMW_Upkeep_Combobox` change | **2** (set upkeep option) | the option key (applied locally first) |
| `REMW_AddDay_Btn` | **3** | — (property id) |
| `REMW_AddWeek_Btn` | **4** | — |
| a row's `REMW_AddDayTemplate_Btn` | **5** | **the row item's upkeep cost id** (definition `+0x5c4`), not an item instance id |
| a row's `REMW_AddWeekTemplate_Btn` | **6** | as type 5 |

**"Add a day/week" pays exactly one day's or seven days' cost into escrow from the clicker's purse**, as a deposit (Live, UPK-10); the client keeps no per-item paid time. Types 5/6 can't distinguish two placed items with the same upkeep cost id.

**Setup** (`0x747570`, once): list colors and column behavior; the two template buttons (cloned into every row, columns 0 and 1) and Transfer start hidden.

## Access: views and lists (chunk 4)

**Fill** (`0x744200`; runs on the full refresh and after the server sets (`0x485d`) or removes (`0x3968`) the list; **it empties the `0x3146` queue at the end**, which is safe after `0x485d` (flushed first) but **drops an edit made less than 3 s before the plot changes**). It reads the plot's access list (`0x4ff100`, map `+0x54`), computes **may edit** (GM, owner, or the viewer's group holds Manage 12) and the **viewer's group** (`0x4ff250` with the character's name and club ids), tallies each entry's group into `+0x2a8[group]`, and fills:

| Control | Contents |
|---|---|
| `REMW_Name_List` (Individuals) | one row per name → group entry: (icon), **Name**, **Group** (a clone of `REMW_Groups_Combobox` as the row's selector, `0x740cf0`), (button) |
| `REMW_OwnerClubs_List` (Owner's Clubs / Guild Members) | player plot: **"Fellowship"** and **"Guild - \<name\>"** rows (**Type**, **Group**); keys encoded by `0x4feff0` (`"%c%s%d"`); on a guild plot the label reads "Guild Members" |
| `REMW_GroupTotals_List` | one row per group: **Name**, **#** (count, `+0x2a8`) |
| `REMW_Permissions_List` | **13 rows** (permission ids 0–12, names in `permissions.md`) × **7 checkbox columns** (clones of `REMW_Permissions_Checkbox`, tagged `+0x34` = group 6 … 0, so column 1 = Owner … column 7 = Everyone): **checked = the group's mask bit**; Owner column always checked and disabled; **enabled = may edit AND (the group is below the viewer's group and the viewer's group holds that permission, or the viewer is the Owner or a GM)** |

**Views** (the three radio buttons): each shows its own list and hides the other two. Defaults by plot type: see *Lifecycle*.

**Edits** (all local, then queue the plot id for **`0x3146`**, sent within 3 s):

| Action | Manager call |
|---|---|
| **grid cell click** (`0x745ec1`) | **`0x5b0ad0`**(property, group, permission, …): set the group's permission bit |
| a club row's Group selector (`0x745d80`) | `0x5b0bb0` (remove) + **`0x5b0b10`** (set the new group) |
| Add / Remove (chunk 5) | `0x5b0b10` / `0x5b0bb0` |

## Access: editing (chunk 5)

**Add** (`0x7451c6`), in order:
1. No name box / access dropdown, or an **empty name** → nothing. The name is normalized (`0x8077d0`, `0x8076f0`).
2. **Group** = the access dropdown's text matched against the seven group names (`0x740c70`, table `0xb414e4`, 0–6); **group 0 (Everyone) is never added**.
3. **Owner mode** = GM, or the viewer's name equals the plot's owner name (record `+0x2c`, case-insensitive).
4. **Already on the list** (`0x749370`) **or the owner's own name** → **`0x192b` "%1 cannot be added because they already have access."**
5. **Not in owner mode:** the viewer's group (`0x4ff250`, name + club ids) must be at least the chosen group, else **`0x192c` "You may not add a player to an access group higher than the one you belong to."**
6. **`0x5b0b10`(plot, group, name)**; if it fails (group above 6, or the plot has no access list yet) → **`0x628` "Name is not valid."** and the name box is cleared. **The client never checks that the name is a real character**; the server must.
7. Success → queue the plot id → **`0x3146`** within 3 s.

**Remove** (`0x74554f`): needs a **selected `Name_List` row** → **`0x5b0bb0`**(plot, that name) → queue → **`0x3146`**; the row is deselected. No rank check on click; the rank rule is in Remove's **enable** state, re-evaluated on each selection (`0x745690`–`0x74585b`):
- **owner mode → enabled**;
- otherwise **with Manage (`0x4ff6c0`)** → enabled only if **the viewer's group is higher than the selected entry's group**;
- otherwise disabled;
- a name comparison (`0x806660`) disables it for one name (*inferred:* the viewer's own, "you can't remove yourself"; the owner can never be on the list).

Remove starts disabled (constructor) and is disabled again by every access-list fill. **Add and the name box have no enable logic** (always enabled); **the access dropdown** is enabled by **may edit** (chunk 4).

## Close-out

**Every control, notification and send is accounted for.**

**Notifications** (handler `0x745090`): **1** click (buttons, list rows, grid cells by their `+0x34` tag, template buttons in upkeep rows); **3** right click on an **upkeep row → the client's item display** for that item (`0x6fff10` / `0x6ae610`; local); **`0x10`** sort comparators (the 7 bound in the switch is a column count, not a code); **`0x24`** the quantity window's answer (withdraw). **One dialog** in the whole window: Transfer's (`0x65`, answered by `0x740c10`). The text prompt id `0x4c` is only ever closed, never opened.

**Static controls (16)** — none of these names appears anywhere in the client, so nothing changes them: labels `REMW_DefaultUpkeep_Label`, `REMW_TimeRemaining_Label`, `REMW_LimitedItem_Label`, `REMW_DepositHint_Label`, `REMW_View_Label`, `REMW_ViewIndividualPermissions_Label`, `REMW_ViewGroupTotals_Label`; layouts `REMW_Balance_Layout`, `REMW_Upkeep_Layout`, `REMW_Escrow_Main_Layout`, `REMW_View_Layout`, `REMW_Edit_Layout`, `REMW_Access_Layout`, `REMW_Permissions_Layout`, `RENW_Access_Main_Layout` (the XML's prefix typo); page `REMW_EscrowPage` (switched by the tab box).

**All sends:**

| Message | From | Payload / notes |
|---|---|---|
| `0x2aa6` type 0 | a balance row with coin / an alt-currency item on the cursor | deposit |
| `0x2aa6` type 1 | a balance row (empty cursor) → quantity window | withdraw an amount |
| `0x2aa6` type 7 | Withdraw All | withdraw everything, no confirmation |
| `0x2aa6` type 2 | the upkeep dropdown | set the plot's upkeep option |
| `0x2aa6` types 3 / 4 | Add Day / Add Week | all upkeep |
| `0x2aa6` types 5 / 6 | a row's Add Day / Add Week | argument = that item's upkeep cost id |
| `0x3146` | any access-list edit, batched every 3 s | property id + full access list |
| `0x507f` | Transfer (dialog `0x65`) → manager `0x5b1470` | the same as the Purchase window's Transfer |

**No client-side permission checks** on any `0x2aa6` action or on Transfer beyond its gate; the access-list edits are gated by the rank rules in chunks 4–5. See `../flows/escrowAndUpkeep.md` and `../flows/accessAndPermissions.md`.

| Control | Type | Binding | Enabled when | Shown when | On use | Populated by | Server data | Status |
|---|---|---|---|---|---|---|---|---|
| `REMW_EscrowProperty_Combobox` | Combobox (**not in the XML**) | `+0x2f0` | — | never (null with this XML) | **nothing**: no handler branch, and no code reads its selection | every property record the client knows (`0x7490b0` over the manager's record map: name `+0x38`, id `+0x14`), current = the plot; kept in sync as records arrive (`0x743fd0`) and are removed (`0x744070`) | — | done (display-only; its absence costs nothing) |
| `REMW_Property_Label` | Label | `+0x2f4` | always | always | — | the plot record's **name** (`+0x38`) for manager `+0x98`; empty if the record is unknown (refresh `0x747740`) | property record name | done |
| `REMW_Transfer_Btn` | Button | `+0x2f8` | always | **`0x5afcb0(character, 1, 1)`: a GM, or the owner (`0x4d6b10`, by name) of the property the player is physically standing in** (position lookup `0x4b3040`) | re-runs the transfer gate `0x5b1380` (acts on the standing-in property; player plots only; target must be a living player, `[0xdd2648]`) → confirmation **dialog `0x65`** owned by this window (`0x1dd3`: "Are you sure you wish to transfer your plot to %1?  All items and money in the escrow will remain with the plot.", 10 s) → answer `0x740c10` → manager `0x5b1470` → **`0x507f`** (the same send as the Purchase window's Transfer) | — | property owner, target | done |
| `REMW_Upkeep_Combobox` | Combobox | `+0x2fc` | always | always | **`0x2aa6` type 2** (option key), applied locally first; no client check | the plot definition's upkeep cost element (definition `+0x24`) options (`0x74d240`); selected = record `+0x3c` (server `0x2aa6` type 2 updates it) | upkeep option per plot | done |
| `REMW_TimeRemainingVal_Label` | Label | `+0x300` | — | always | — | overall time left ("%dw %dd") + " by " + `0x486f30`'s value | balances, costs | done |
| `REMW_LimitedItemVal_Label` | Label | `+0x308` | — | always | — | the item whose upkeep runs out first, or "Plot Upkeep" | as above | done |
| `REMW_WithdrawAll_Btn` | Button | `+0x318` | always | always | **`0x2aa6` type 7** for the plot (`+0x98`) immediately; no confirmation, no client check | — | who may withdraw; where the money goes | done |
| `REMW_Balance_List` | Listbox | `+0x310` | always | always | click a row: **cursor holds coin / an alt-currency item → deposit (`0x2aa6` type 0)**; otherwise the quantity window → **withdraw (`0x2aa6` type 1)**; no client permission check (see *Escrow balances*) | coin + the character's alt currencies that the `0x2698` cost table uses; quantities from the plot record's balance map | the plot's balance per currency; the set of escrowable currencies = cost-table currencies | done |
| `REMW_AddDay_Btn` | Button | `+0x31c` | always | always | **`0x2aa6` type 3** (no check) | — | *inferred:* pay one day of all upkeep into escrow | done |
| `REMW_AddWeek_Btn` | Button | `+0x320` | always | always | **`0x2aa6` type 4** (no check) | — | as above, one week | done |
| `REMW_DailyCharge_Label` | Label | `+0x30c` | — | always | — | "Daily Cost:" + net per-currency totals | as above | done |
| `REMW_Upkeep_List` | Listbox | `+0x314` | always | always | row buttons: types 5/6 | one row per placed item with an upkeep cost (definition `+0x5c4` → cost element): [day btn], [week btn], icon, Name, **Cost** (positive cost items), **Reduction** (negative), **Time Left** ("%dw %dd"), **Limited By** (currency) | item upkeep cost ids; the cost table; the plot's balances | done |
| `REMW_AddDayTemplate_Btn` | Button (template) | `+0x324` | always | hidden itself; cloned into column 0 of every upkeep row | **`0x2aa6` type 5**, argument = the row item's upkeep cost id | — | as above, per upkeep kind | done |
| `REMW_AddWeekTemplate_Btn` | Button (template) | `+0x328` | always | hidden itself; cloned into column 1 | **`0x2aa6` type 6** | — | as above | done |
| `REMW_ViewIndividualPermissions_Btn` | Button (radio) | `+0x260` | always | Access page | shows `Name_List`, hides the other two | checked by default on player plots | — | done |
| `REMW_ViewOwnerClubsPermissions_Btn` | Button (radio) | `+0x264` | always | Access page | shows `OwnerClubs_List`, hides the other two | checked by default on guild plots | — | done |
| `REMW_ViewOwnerClubsPermissions_Label` | Label | `+0x26c` | — | always | — | "Owner's Clubs" (player plot) / "Guild Members" (guild plot) | plot type | done |
| `REMW_ViewGroupTotals_Btn` | Button (radio) | `+0x268` | always | Access page | shows `GroupTotals_List`, hides the other two | — | — | done |
| `REMW_Name_List` | Listbox | `+0x270` | always | the Individuals view | select a row (Remove); the row's Group selector | name → group entries of the plot's access list | access list | done |
| `REMW_OwnerClubs_List` | Listbox | `+0x274` | always | the Owner's Clubs view | a row's Group selector → `0x5b0bb0` + `0x5b0b10` → `0x3146` | Fellowship / Guild rows (encoded keys) | access list; owner's clubs | done |
| `REMW_GroupTotals_List` | Listbox | `+0x278` | always | the Group Totals view | — | group name + count | access list | done |
| `REMW_Name_EditBox` | Editbox | `+0x288` | always | Access page | the name for Add (normalized); cleared after an invalid add | — | — | done |
| `REMW_Access_Combobox` | Combobox | `+0x28c` | **may edit** (GM, owner, or Manage 12) | Access page | the group for Add (text → group via `0x740c70`) | XML choices Visitor … Co-Owner (groups 1–5) | — | done |
| `REMW_Add_Btn` | Button | `+0x290` | always | Access page | Add checks (see chunk 5) → `0x5b0b10` → **`0x3146`**; messages `0x192b`, `0x192c`, `0x628` | — | access list; name validity (server) | done |
| `REMW_Remove_Btn` | Button | `+0x294` | on a `Name_List` selection: owner mode, or Manage and a lower-group entry (not one excluded name) | Access page | `0x5b0bb0` → **`0x3146`** | — | access list | done |
| `REMW_Permissions_List` | Listbox | `+0x27c` | always | Access page | cell click → `0x5b0ad0` → `0x3146`; column sort | 13 permissions × 7 groups (see chunk 4 section) | the 7 group masks | done |
| `REMW_Groups_Combobox` | Combobox (template) | `+0x284` | always | cloned into each `Name_List` row's Group column (`0x740cf0`) | changes that entry's group | choices Everyone … Owner (groups 0–6) | access list | done |
| `REMW_Permissions_Checkbox` | Button (template) | `+0x280` | per cell (see `Permissions_List`) | cloned into every grid cell (tag `+0x34` = group) | as `Permissions_List` | checked = mask bit | as above | done |
| `REMW_AccessPage` | Page | `+0x25c` | — | as `REMW_Subwindows` (permission 12 or GM) | — | — | access lists | done |
| `REMW_Subwindows` | TabBox | `+0x258` | always | always | tab switching (UI framework) | the Access page is **removed** (`0x88d650`) or **re-added** (`0x88db60`) on refresh and **every frame**: kept only with **permission 12 (manage access, `0xc`)** on the plot (`0x4ff6c0`) or GM (`0x5ad900`) | access lists, GM flag | done |
| `REMW_Eviction_Label` | Label | `+0x344` | always | always | — | **player plot:** `0x3415` "Plots will pack up all items, send them to the owner, and become available for purchase again if the owner of the plot has not logged in within 90 days."; **guild plot** (`0x7f6280` on the plot's definition type): `0x3416` "…if no member of the guild has logged in within 30 days." | none: the strings describe a login rule Live no longer has; the server reclaims a plot 90 days after its escrow expired, player or guild (RCL-1), and the patcher replaces both strings with Live's escrow wording (Q24) | done |

Controls the client never looks up (16): static labels, layout boxes, and screens. Each still needs a check that it isn't changed indirectly (for example through its parent screen or a template).
