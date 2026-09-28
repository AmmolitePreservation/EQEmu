# RealEstatePlotSearchWnd — Control Analysis

One row per control the client looks up (from [`../guiControlInventory.md`](../guiControlInventory.md)). A row is **done** only when every condition is expanded to data leaves (a field with a known origin: server message and field, client action, or initialization step) and every message involved is named.

Columns: **Enabled when** / **Shown when** (the conditions feeding `0x866610` and vtable `+0xd8`), **On use** (handler branch, client checks in order, message sent), **Populated by** (server message and field), **Server data** (the stored entity or field this implies), **Status**.

## Search, results, filters and Go (chunk 1)

**Opening** (activate, vtable `+0xdc` = `0x74bb00`): requires **manager `+0x90`**, the **definition type of the server's current property** (set with `+0x8c` by `0x6fd0`; default 10), to be a **neighborhood**: `0x7f5f70` tests type -> category 4, i.e. **definition type 5**. Otherwise it prints **`0x1f8e` "You must be in a neighborhood to use that."** and doesn't open. On opening it clears the search box (`0x74ba70`). **It sends nothing.** Vtable `+0x98` (`0x74b8d0`) closes the window's text prompt and clears `+0x24c`.

**Searches** (all `0x2691`, the 224-byte request built inline; `+0x04` = **manager `+0x8c`**, the current property, i.e. the neighborhood you're in):

| Control | Type (`+0x00`) | `+0x5c` (player name) | `+0xdc` |
|---|---|---|---|
| **Search** (`0x74b920`) | **4** | the typed owner name (empty text is sent as-is) | **1** |
| **My Plots** (`0x74b920`) | **4** | your own name (also put in the box) | **0** |
| **My Items** (`0x74be00`) | **5** | your own name | 0 |

The byte after `+0xdc` is 1 in all three. **Type 4 = plots by owner name** (`+0xdc` 1 = a typed name, 0 = your own plots); **type 5 = plots containing items you own**; both within the current neighborhood.

**`REPSW_PlotList`** (the reply's modes 4/5, fill `0x74c4c0`): columns (the plot id), **Address** (string `a`), **Type** ("Player" / "Guild"; other kinds dropped), **Owner** (string `b`); each row stores the reply's **`rowValue`**. A click enables Go while a row is selected and disables it otherwise. Code `0x10` is the sort comparator (numeric and text columns).

**Filters** (`0x74bb50`, re-run on each click and after each fill): the constructor **checks all four**, so every row shows by default. The filter tests **Type** (column 2) for Player / Guild and **Owner** (column 3) for Owned / Vacant and hides rows (`0x8578e0`); *inferred by analogy with the Neighborhood window:* an unchecked box hides its category (Player rows, Guild rows, rows with an owner, rows without one).

**Go** (`REPSW_Go_Btn`): enabled only while a row is selected. It starts the client's **/find wisp to the plot**: `0x59c2c0`(rowValue, 1) sets find target **type 4** with **`+0x34` = rowValue**, and `0x59a420` sends **`OP_FindPersonRequest` (`0x5cea`)**: `{type 4, npc_id -1, rowValue, -1, player position, 0}`. **The server must answer with `OP_FindPersonReply` (`0x7e58`), a path to the plot's center.** So **`rowValue` is the find target** the server wants back (for example the plot's property id).

**`0x796c` is unreachable in this build:** Go sends `0x796c {u32 rowValue}` instead of the /find request when window flag `+0x24c` is set, then clears it (as do the constructor and `0x74b8d0`), but **nothing in this client sets the flag**: no code in the window's range, none of its 95 virtual methods, and no other code using its global (`0xd1fcc4`) or its name writes it, and no packet handler reaches it. **Live sets it from the server** (eqgame 2026-09-17, window `+0x330`): the teleflinger's message `0x8a99 {u8}` opens Plot Search and stores its byte in the flag; Go then sends `0x1708 {u32 rowValue}` once, answered with a fling, and clears it. This build has the sending half but no handler for the arming message.

## Close-out

**Every control, notification and send is accounted for.**

**Notifications** (handler `0x74c110`): **1** clicks (Search, My Plots, My Items, Go, the four filters, the plot list); **`0x10`** the plot list's sort comparator. The search box has no branch of its own (Enter doesn't search). **No dialogs.** The text prompt id `0x4f` is only ever closed (`0x74b8f1`), never opened.

**Static controls (7)**, none referenced by the client: `REPSW_Main_Layout`, `REPSW_Show_Layout`, `REPSW_ShowLabel`, `REPSW_ShowPlayer_Label`, `REPSW_ShowGuild_Label`, `REPSW_ShowOwned_Label`, `REPSW_ShowVacant_Label`.

**All sends:**

| Message | From | Payload |
|---|---|---|
| `0x2691` type 4 | Search (`+0xdc` = 1), My Plots (`+0xdc` = 0) | the owner name at `+0x5c`; `+0x04` = the current neighborhood |
| `0x2691` type 5 | My Items | your own name at `+0x5c` |
| `0x5cea` (`OP_FindPersonRequest`) | Go | find target type 4 with the row's `rowValue` |
| `0x796c` | Go (unreachable branch) | never sent by this build |

(`0x792c` at `0x74c91d` belongs to the Purchase window's dialog-answer function, not this window.)

See `../flows/plotSearch.md`.

| Control | Type | Binding | Enabled when | Shown when | On use | Populated by | Server data | Status |
|---|---|---|---|---|---|---|---|---|
| `REPSW_Search_EditBox` | Editbox | `+0x224` | always | always | the owner name for Search (empty sent as-is); cleared on opening; set to your name by My Plots / My Items | — | — | done |
| `REPSW_Search_Btn` | Button | `+0x228` | always | always | **`0x2691` type 4**, the text as the owner name, `+0xdc` = 1 | — | plots by owner name in the neighborhood | done |
| `REPSW_MyPlots_Btn` | Button | `+0x22c` | always | always | **`0x2691` type 4**, your own name, `+0xdc` = 0 | — | your plots in the neighborhood | done |
| `REPSW_MyItems_Btn` | Button | `+0x230` | always | always | **`0x2691` type 5**, your own name | — | plots containing your items | done |
| `REPSW_Go_Btn` | Button | `+0x234` | **only while a row is selected** | always | **/find wisp: `OP_FindPersonRequest` type 4 with the row's `rowValue`** (the `0x796c` branch is unreachable) | — | `OP_FindPersonReply` path to the plot | done |
| `REPSW_PlotList` | Listbox | `+0x238` | always | always | select a row (enables Go); header sort | `0x2691` modes 4/5: id, Address, Player/Guild, Owner; row value = `rowValue` | plots: address, kind, owner, find target | done |
| `REPSW_ShowPlayer_Btn` | Button (checkbox) | `+0x23c` | always | always | re-run the filter; *inferred:* unchecked hides Player rows | checked by the constructor | — (client-side view filter) | done |
| `REPSW_ShowGuild_Btn` | Button (checkbox) | `+0x240` | always | always | re-run the filter; *inferred:* unchecked hides Guild rows | checked by the constructor | — | done |
| `REPSW_ShowOwned_Btn` | Button (checkbox) | `+0x244` | always | always | re-run the filter; *inferred:* unchecked hides rows with an owner | checked by the constructor | — | done |
| `REPSW_ShowVacant_Btn` | Button (checkbox) | `+0x248` | always | always | re-run the filter; *inferred:* unchecked hides rows without an owner | checked by the constructor | — | done |

Controls the client never looks up (8): static labels, layout boxes, and screens. Each still needs a check that it isn't changed indirectly (for example through its parent screen or a template).
