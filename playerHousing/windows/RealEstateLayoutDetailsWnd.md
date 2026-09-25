# RealEstateLayoutDetailsWnd — Control Analysis

One row per control the client looks up (from [`../guiControlInventory.md`](../guiControlInventory.md)). A row is **done** only when every condition is expanded to data leaves (a field with a known origin: server message and field, client action, or initialization step) and every message involved is named.

Columns: **Enabled when** / **Shown when** (the conditions feeding `0x866610` and vtable `+0xd8`), **On use** (handler branch, client checks in order, message sent), **Populated by** (server message and field), **Server data** (the stored entity or field this implies), **Status**.

**Purely client-side.** The window shows a dry run of loading a layout: which items the layout needs and whether they're available under the chosen options. **It sends no messages.** It's opened by the Items window's **Show Details** button (`REIW_LayoutDetails_Btn`), which pushes the Use Placed / Use Inventory options into it (`0x5ac460` → `+0x229`, `0x5ac490` → `+0x22a`), resets the Load option (`0x5ac4c0` → `+0x22c` = 0) and runs the analysis `0x73ab10`.

**Code:** binding `0x5ac730`; title "Layout - %s" (`0x5aca60`); notification handler `0x5ad040`; refresh `0x5ad2f0`; row fill `0x5acb40`. The analysis lives in the Items window (`0x73be30` → `0x73ab10`, the same planning as Load; see `RealEstateItemsWnd.md`, *How Load plans*).

| Control | Type | Binding | Enabled when | Shown when | On use | Populated by | Server data | Status |
|---|---|---|---|---|---|---|---|---|
| `RELDW_ItemList` | Listbox | `+0x238` | always | always | header click sorts (text and number comparators in `0x5ad040`) | one row per item the layout needs (`0x5acb40`); columns (icon), **Item Name**, **Transformed** ("Yes" / "No" / "N/A": the layout entry's appearance-modified flag), **Required**, **Available**, **Shortage** (counts from the analysis) | — (the client's own view of placed, stored and inventory items) | done |
| `RELDW_Load_Combobox` | Combobox | `+0x23c` | always | always | selection → `+0x22c` → refill the rows (`0x5acb40`). Tooltip: "For a specific load option, view the layout's required items and their availability." | *inferred:* the three load scopes (All / Yard / House), mirroring the Load buttons; the labels aren't literals in this window's code | — | done (labels inferred) |
| `RELDW_UsePlacedItems_Btn` | Button (checkbox) | `+0x240` | always | always | toggle → `+0x229` → re-run the analysis (`0x73be30`) | set from the Items window's option when opened | — | done |
| `RELDW_UseInventoryItems_Btn` | Button (checkbox) | `+0x244` | always | always | toggle → `+0x22a` → re-run the analysis; the refresh (`0x5ad2f0`) re-applies Load's inventory-permission rule (permission 9 / 6 / 4 on the property you're in, or GM) whenever that property changes (`+0x224`) | set from the Items window's option when opened | permissions | done |

There's no Store Extra option here, since it doesn't affect availability.

Controls the client never looks up (4): the static labels `RELDW_Load_Label`, `RELDW_UsePlacedItems_Label`, `RELDW_UseInventoryItems_Label`, and the screen `RELDW_Options_Screen`. Each still needs a check that it isn't changed indirectly (for example through its parent screen or a template).
