# Testing Notes: Client Behaviors That Look Like Server Bugs

Behaviors of the RoF2 client (with the stock XML files) that are easy to mistake for server problems during testing. Each is verified in the client code; the linked page has the details.

## Before testing anything

| Behavior | Why | Source |
|---|---|---|
| **No item can be placed, and no item has upkeep, with stock EQEmu** | EQEmu's RoF2 encoder writes `Placeable = 0` and zeroes the item's upkeep cost id (bytes 21–24 after Heirloom) | README critical findings 11–12 |
| **Real-estate windows open but stay empty or dark** | they need definitions (`0x2698`), property records, `0x6fd0`, regions (`0x49df`), item lists (`0x1cef`) in the right order | `flows/placeItem.md` |

## Neighborhood window

| Behavior | Why | Source |
|---|---|---|
| **The neighborhood list is empty even though the server sent neighborhoods** | all six filter checkboxes start **unchecked** in stock RoF2 (no `<Checked>` in the XML), and an unchecked box hides its category; tick them (the owner ships no UI patch, PAT-2; Live's client starts them ticked) | `windows/RealEstateNeighborhoodWnd.md`, chunk 1 |
| **Opening the window shows nothing and sends nothing** | the list is only filled by a `0x2691` reply (search results, or a server push with `showWindow`) | chunk 1 |
| **Go is greyed out** | it's enabled only while a neighborhood row is selected | chunk 1–2 |
| **Pressing Enter in the search box does nothing** | only the Search button searches | chunk 2 |
| **A search with an empty box still goes out** | the client sends empty text as-is; the server lists every neighborhood, as Live does | chunk 2 |
| **The search text disappears** | changing the search type clears the box | chunk 2 |
| **Clicking a Player Plots / Guild Plots result does nothing** | both result lists are display-only | chunk 2 |
| **Guild names show "Unknown Guild"** | the client's guild-name cache missed; it requests the name itself, and the server must answer | `opcode2691PropertySearch.md` |

## Items window and placement

| Behavior | Why | Source |
|---|---|---|
| **Place is enabled but the placement is refused** | several rule failures (House of Thule, permissions, upkeep) leave Place enabled and the click doesn't re-check, so the server must refuse with the right code | `windows/RealEstateItemsWnd.md`, *Can-place* |
| **Place stays dark** | not placeable (see above), a pending request (up to 5 s), no selected row, or a disabling code | *Can-place* |
| **The list is empty** | it only shows items of the plot you're on, its house and crate, and only after `0x1cef`/`0x2cf7`; filters all start on | chunk 1 |
| **"Cannot place that item in this zone" / "limit for that item" / lore** | these codes (`0x23`, `0x1f`, `0x41`) are server-only; the client never produces them | *Rules* |
| **A full yard still accepts an item** | an item that can merge into an existing stack waives the capacity limit | *Rules*, 10b |
| **Move buttons open a "how many?" box** | stacks go through the quantity window; single items send immediately | *Request structs* |
| **Everything disappears during placement** | `ItemPlacementHideUI` hides the whole UI while placing | `placementMode.md`, *Global effects* |
| **Windows can't be clicked during placement** | placement captures the mouse like mouse-look | *Global effects* |
| **A long press doesn't commit** | only a quick click (< 500 ms) commits | `placementMode.md` |
| **The preview is always red** | no `0x49df` regions, region ids ≠ definition `+0x1c`, or no code-0 region in `0x626e` | `placementMode.md`, *Validation* |
| **A fresh preview floats at eye height and is refused locally (code 29, 'not flat enough', or 17 when its box leaves the plot)** | begin starts the preview at the camera and pushes it out along the view; until the mouse moves it in Cursor mode it isn't on a surface. Move it with the mouse; with no mouse (automation), look down first (`/look -60`), which puts it on the floor | `placementMode.md`, *Begin's distance handling* |
| **Walking off the plot doesn't cancel placement** | player movement never cancels it; validity follows the item, not the player | *Scenario* |
| **Left-clicking a placed item does nothing** | a left click sends `0x218c`/`0x050c`, which Peridot only logs; a house enters only from its door or the menu's Enter; the menu comes from a right click (`0x21ec`), which the server answers with the `0x21ec` menu | `flows/editPlacedItem.md` |
| **MacroQuest shows no item for a placed entry** | a placed entry's item has only the shared definition, which the client reads at `+0x144` (getter `0x554fb0`); MacroQuest's emu `ItemClient` reads it at `+0x148` and returns null. The probe (mqperidotemu) reads the client's offset | mqperidotemu `probeSpec.md` |
| **Unpack does nothing and says nothing** | Unpack moves only the rows that pass the individual move rules and is silent about the rest (Live, INT-9); a house never passes the stored-item rule. Place a house straight from the crate's row instead | `windows/RealEstateItemsWnd.md` |
| **Layouts appear for every character** | layout files (`.\userdata\RE_Layout_*.xml`) have no character in the name | *Layouts* |
| **A loaded layout lands in the wrong place** | positions are relative to the plot definition's layout origin/heading (definition fields 15–18) | `flows/layouts.md` |
| **Use Inventory has no effect when loading a layout** | it's silently turned off without permission 9, 6 or 4 (or GM) | *How Load plans* |

## Manage window

| Behavior | Why | Source |
|---|---|---|
| **"You must be inside a plot or house to use that."** | Manage only opens on a plot the client can resolve (regions + records) | `windows/RealEstateManageWnd.md`, chunk 1 |
| **The window closes by itself** | leaving the plot, or the plot not being owned, closes it | chunk 1 |
| **Anyone can open Manage on an owned plot** | only the Access tab is gated (permission 12 or GM) | chunk 1 |
| **Deposits/withdrawals by non-owners reach the server** | the client checks no permissions on `0x2aa6`; withdrawals need permission 2 server-side | chunk 2 |
| **Withdraw All happens without asking** | it has no confirmation | chunk 2 |
| **Only coin can be deposited** | escrowable currencies are the ones used in the `0x2698` cost table | chunk 2 |
| **The upkeep list is empty** | items need an upkeep cost id matching a cost element (EQEmu writes 0) | chunk 3 |
| **Time Left ignores a currency** | currencies not in the character's alt-currency table are left out of the totals | chunk 3 |
| **An access-list change was lost** | edits are batched every 3 s; leaving the plot within 3 s drops them | chunk 4 |
| **"Name is not valid." for a real character** | that message only means the local update failed (bad group, no access list yet); the client never checks names | chunk 5 |
| **Transfer isn't shown** | only to a GM or the owner of the property you're **standing in** | chunk 1 |

## Live comparisons

Behavior the client leaves entirely to the server, so it can only be learned by watching Live. These are features that have probably not drifted since 2013; opcode numbers on Live's current client differ, so match by behavior and sequence.

| Feature | What the RoF2 client does | What to capture on Live |
|---|---|---|
