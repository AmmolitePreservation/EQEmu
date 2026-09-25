# RealEstateNeighborhoodWnd — Control Analysis

One row per control the client looks up (from [`../guiControlInventory.md`](../guiControlInventory.md)). A row is **done** only when every condition is expanded to data leaves (a field with a known origin: server message and field, client action, or initialization step) and every message involved is named.

Columns: **Enabled when** / **Shown when** (the conditions feeding `0x866610` and vtable `+0xd8`), **On use** (handler branch, client checks in order, message sent), **Populated by** (server message and field), **Server data** (the stored entity or field this implies), **Status**.

## Lifecycle and neighborhood browsing (chunk 1)

**Opening:** activate (vtable `+0xdc` -> `0x749bd0`) only resets the selection (`+0x224` = 0, `+0x228` = -1) and clears the search box; **it sends nothing**. The list is filled by the server's **`0x2691`** reply (handled by manager `0x5b0f80` -> window `0x74ab50`; format in `../opcode2691PropertySearch.md`): **modes 0/1** fill `RENW_NeighborhoodList`, and their **`showWindow`** byte lets **the server open the window itself** with the results. Other overrides: `+0x98` closes this window's text prompt (`TextEntryWnd` id `0x4d`); `+0x168` hides the window.

**`RENW_NeighborhoodList`** (rows from `0x2691` modes 0/1): **Neighborhood** (name), **Guild** (the guild id's name from the client's guild-name cache, `0x425670`, "Unknown Guild" on a miss, empty for -1), **PPlots**, **Used**, **GPlots**, **Used** (player and guild plot totals and how many are in use), plus hidden per-row values. **Clicking a row** stores its neighborhood id (`+0x228`) and sends **`0x2691` request type 2** (neighborhood details, `0x74a876`); the reply's **mode 2** fills `RENW_PlayerList` / `RENW_GuildList` (chunk 2).

**Filters** (`0x749cd0`, re-run on every checkbox click and after every fill): each **unchecked** box hides one kind of row, comparing the displayed cells as text (`0x806660`); a missing checkbox counts as checked:

| Unchecked | Hides rows where |
|---|---|
| `RENW_ShowNonGuild_Btn` | the Guild cell is empty |
| `RENW_ShowGuild_Btn` | the Guild cell isn't empty |
| `RENW_ShowFullPPlots_Btn` | Used == PPlots |
| `RENW_ShowOpenPPlots_Btn` | Used ≠ PPlots |
| `RENW_ShowFullGPlots_Btn` | Used == GPlots |
| `RENW_ShowOpenGPlots_Btn` | Used ≠ GPlots |

**No code sets the checkboxes' initial state, and the XML gives none of them `<Checked>`, so all six start unchecked and every row is hidden** (each row is either guild or non-guild, and one of those two boxes is always unchecked). **With this XML the list looks empty until the player ticks boxes**; a row shows only when its guild/non-guild, player-plot full/open and guild-plot full/open categories are all checked. Setting `<Checked>true</Checked>` on the six buttons in the XML would show everything by default.

**Go** (`RENW_Go_Button`): **enabled only while a neighborhood row is selected**; needs the selection (`+0x228` ≠ -1) and sends **`0x218c` `{u32 neighborhood id, u32 -1, u32 0}`** (12 bytes). **`0x218c` is the client's general "interact / enter" request**: the same layout the Items window sends for a placed item (`{property id, record id, b}`); **record id -1 means the property itself**, here "take me to this neighborhood". The server should move the player there.

## Search and results (chunk 2)

**All searches send `0x2691`** (224-byte request, constructor `0x749500`; shared send tail, no pending-request check) with `+0x04` = window `+0x22c` (the "selected" id from the last neighborhood-list reply):

| Control | Request type (`+0x00`) | Criterion |
|---|---|---|
| **Search** with the type dropdown on **Player** | **0** | the typed text in the **player name** field (`+0x5c`, 64) |
| **Search** on **Guild** | **0** | the text in the **guild name** field (`+0x9c`, 64) |
| **Search** on **Neighborhood** | **0** | the text in the **neighborhood name** field (`+0x0c`, 80) |
| **My Plots** | **0** | **the character's own name** as the player (also sets the dropdown to Player and the box to the name) |
| **My Items** | **1** | the character's own name as the player: plots containing items you own |
| a click on a neighborhood row | **2** | that neighborhood's id at `+0x04` (only when the selection changes) |

**Search** only checks that the edit box exists: **empty text is sent as-is**. **Pressing Enter doesn't search** (the edit box has no handler branch; only the button searches). **Changing the search type clears the search box** (`0x74a946`).

**Results** (`0x2691` reply, `../opcode2691PropertySearch.md`): modes 0/1 refill the neighborhood list (the searches above); **mode 2** (the details request) fills **`RENW_PlayerList`** ("Player Plots": one string per row) and **`RENW_GuildList`** ("Guild Plots": guild ids shown as names from the guild-name cache). **Both result lists are display-only**: the handler has no branch for them, so clicking a result does nothing.

## Close-out

**Every control, notification and send is accounted for.**

**Notifications** (handler `0x74a3a0`): **1** clicks (the three search buttons, Go, the type dropdown, the six filters, the neighborhood list); **`0x10`** the neighborhood list's sort comparator (5 columns). The search box and the two result lists have no branches. **No dialogs.** The text prompt id `0x4d` is only ever closed (`0x749c9a`), never opened.

**Static controls (11)**, none referenced by the client: `RENW_List_LayoutScreen`, `RENW_List_Layout`, `RENW_Main_Layout`, `RENW_Show_Layout`, `RENW_ShowLabel`, and the six filter labels (`RENW_ShowNonGuild_Label`, `RENW_ShowGuild_Label`, `RENW_ShowFullPPlots_Label`, `RENW_ShowOpenPPlots_Label`, `RENW_ShowFullGPlots_Label`, `RENW_ShowOpenGPlots_Label`).

**All sends:**

| Message | From | Payload |
|---|---|---|
| `0x2691` type 0 | Search (Player / Guild / Neighborhood), My Plots | 224 bytes: the name criterion in the chosen field; `+0x04` = `+0x22c` |
| `0x2691` type 1 | My Items | your own name as the player |
| `0x2691` type 2 | a neighborhood row click (selection changed) | the neighborhood id at `+0x04` |
| `0x218c` | Go | `{u32 neighborhood id, u32 -1, u32 0}` |

See `../flows/neighborhoods.md`.

| Control | Type | Binding | Enabled when | Shown when | On use | Populated by | Server data | Status |
|---|---|---|---|---|---|---|---|---|
| `RENW_Search_EditBox` | Editbox | `+0x230` | always | always | the search text (no Enter handling; cleared when the search type changes; set to your own name by My Plots / My Items) | — | — | done |
| `RENW_SearchType_Combobox` | Combobox | `+0x234` | always | always | change: clears the search box; selects the criterion field for Search | XML choices Player / Guild / Neighborhood (index 0 / 1 / 2) | — | done |
| `RENW_Search_Button` | Button | `+0x240` | always | always | **`0x2691` type 0** with the text in the chosen name field | — | name search over neighborhoods, players, guilds | done |
| `RENW_MyPlots_Button` | Button | `+0x238` | always | always | **`0x2691` type 0** with your own name as the player | — | plots you own | done |
| `RENW_MyItems_Button` | Button | `+0x23c` | always | always | **`0x2691` type 1** with your own name | — | plots containing items you own | done |
| `RENW_Go_Button` | Button | `+0x244` | **only while a neighborhood row is selected** (the row click enables it; a click with no selection disables it) | always | **`0x218c` {id, -1, 0}** (enter) | — | moving the player into the neighborhood | done |
| `RENW_NeighborhoodList` | Listbox | `+0x248` | always | always | click a row: **`0x2691` type 2** (details) and select it for Go | `0x2691` modes 0/1: Neighborhood, Guild, PPlots, Used, GPlots, Used | neighborhoods: name, guild, plot counts | done |
| `RENW_GuildList` | Listbox | `+0x250` | always | always | none (display-only) | `0x2691` mode 2: guild ids as names | guild names (the client asks for unknown ones) | done |
| `RENW_PlayerList` | Listbox | `+0x24c` | always | always | none (display-only) | `0x2691` mode 2: one string per player plot | — | done |
| `RENW_ShowNonGuild_Btn` | Button (checkbox) | `+0x254` | always | always | toggles; re-runs the filter (`0x749cd0`): **unchecked hides rows where the Guild cell is empty** | starts unchecked (no `<Checked>` in the XML) | — (client-side view filter) | done |
| `RENW_ShowGuild_Btn` | Button (checkbox) | `+0x258` | always | always | toggles; re-runs the filter (`0x749cd0`): **unchecked hides rows where the Guild cell isn't empty** | starts unchecked (no `<Checked>` in the XML) | — (client-side view filter) | done |
| `RENW_ShowFullPPlots_Btn` | Button (checkbox) | `+0x25c` | always | always | toggles; re-runs the filter (`0x749cd0`): **unchecked hides rows where Used == PPlots** | starts unchecked (no `<Checked>` in the XML) | — (client-side view filter) | done |
| `RENW_ShowOpenPPlots_Btn` | Button (checkbox) | `+0x260` | always | always | toggles; re-runs the filter (`0x749cd0`): **unchecked hides rows where Used ≠ PPlots** | starts unchecked (no `<Checked>` in the XML) | — (client-side view filter) | done |
| `RENW_ShowFullGPlots_Btn` | Button (checkbox) | `+0x264` | always | always | toggles; re-runs the filter (`0x749cd0`): **unchecked hides rows where Used == GPlots** | starts unchecked (no `<Checked>` in the XML) | — (client-side view filter) | done |
| `RENW_ShowOpenGPlots_Btn` | Button (checkbox) | `+0x268` | always | always | toggles; re-runs the filter (`0x749cd0`): **unchecked hides rows where Used ≠ GPlots** | starts unchecked (no `<Checked>` in the XML) | — (client-side view filter) | done |

Controls the client never looks up (12): static labels, layout boxes, and screens. Each still needs a check that it isn't changed indirectly (for example through its parent screen or a template).
