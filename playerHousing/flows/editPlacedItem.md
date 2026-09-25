# Flow: Edit, Move, Store or Locate a Placed Item

**Status:** client side decoded (`../windows/RealEstateItemsWnd.md`, `../placedItemsAndProperties.md`, `../placementMode.md`).

## 1. Goal

A player moves an already-placed item (reposition it in place mode), stores it (closet / shed / crate), picks it up into inventory, or locates it. Entry points: **clicking the item in the world**, **right-clicking its row** in the Items list, or the **Items window buttons**.

## 2. Client state checklist

As `placeItem.md` S1–S8, plus:

| # | State | Origin | Why |
|---|---|---|---|
| E1 | The item's **entry** (property id `+0x34`, record id `+0x38`, placed flag, owner name, item) | `0x1cef` / `0x2cf7` / `0x1097` | list rows, menu ids, requests |
| E2 | The item's **spawn** bound to its entry | spawn + `0x21ea` | world click picking, editing (the real item is hidden while its preview moves) |
| E3 | For Locate: the item's property is the server's current property (`0x6fd0`) or under it | `0x6fd0` | Locate enable (`0x5b0cc0`) |

## 3. Sequences

**Edit in place (reposition):**

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | C → S | world click on the item: **`0x218c`** `{property, record, 0}` if the click ray hits it, else **`0x050c`** `{property, record, -1}` if within 20 units; or right-click on its row: **`0x21ec`** menu request (0x300 bytes, empty entries, mouse X/Y) | |
| 2 | S → C | **`0x21ec`** interaction menu (same struct, entries filled: types and labels) | the popup appears at the given X/Y |
| 3 | C → S | the player picks a **type 2** entry → **`0x626e`** (header with the menu's property/record ids) | |
| 4 | S → C | **`0x626e`** begin placement with **A = 1 (editing)**, B = 1, the item's **spawn id**, its current transform and the region permission list | the client hides the real item (or spawn) and moves a preview |
| 5–7 | | commit `0x711a` → `0x1228` code 0 → the updated entry (**`0x1097`**) and the spawn moved | as in `placeItem.md` |

**Store / pick up (buttons or menu types 3 / 4):** `0x3424` (destination 2 closet / 3 shed / 6 crate, or the property's own storage for menu type 3) or `0x2036`. **Stacks** first open the quantity window; its answer sends the request with the quantity at `+0x28` (0 = the whole item). Server → client: the updated entry (`0x1097`, placed flag 0 / new property) or its removal (`0x1baf`), plus the inventory item for a pick-up.

**Locate:** a toggle. Start sends **`OP_FindPersonRequest` (`0x5cea`)** with target type 3 + property/record ids + the player's position; the server answers **`OP_FindPersonReply` (`0x7e58`)** with a path. Clicking again stops locally.

**Other menu types** (server-defined labels): 1 → `0x218c`; 5 → `0x7ad1`; 7 → show the item locally; 8 / 9 / `0x13` → `0x050c` / `0x51e3` / `0x64a3`; `0xc`/`0xd` → the NPC-item (vendor/banker) interaction path; `0xe` → text entry → `0x3c8b`; `0x11` → house swap (`movingCrate.md`).

## 4. Decision trees

`../windows/RealEstateItemsWnd.md`: *Action buttons*, *Rules for placing a stored item*, *Rules for moving between properties*, *Move buttons and Locate*, *Request structs*; `../placedItemsAndProperties.md`: the menu table.

## 6. Failure branches

| Symptom | Cause |
|---|---|
| Clicking a placed item does nothing | no `0x21ec` reply from the server, or the click missed and the player is more than 20 units away |
| Menu appears but "place" does nothing | no `0x626e` begin reply with A = 1 and the spawn id |
| Real item stays visible while editing | the spawn isn't bound (`0x21ea`) or the spawn id in `0x626e` is wrong |
| Move buttons dark | the destination (closet/shed/crate) doesn't exist, the rules fail, or a request is pending |
| Locate dark | the item isn't placed, or its property isn't the current property (`0x6fd0`) |
| Locate starts but no path | no `OP_FindPersonReply` for target type 3 |

## 7. Server obligations

Answer `0x218c`/`0x050c`/`0x21ec` with a `0x21ec` menu (types decide the client action); answer place requests for placed items with `0x626e` A = 1 + spawn id; validate and apply moves/stores/pick-ups (same rules as the client) and send `0x1097`/`0x1baf` + inventory updates; implement `OP_FindPersonRequest` type 3 → `OP_FindPersonReply`. **Upkeep:** when storing an item for unpaid upkeep, send `0x1baf` to end any edit cleanly (`0x1097` alone leaves the editor working on a stored item; `../placementMode.md`, *Upkeep*).

## 8. Test cases

World click (hit / near / far); right-click menu; edit and commit; edit and cancel; store single / stack (quantity window); pick up with one non-empty house+crate already in inventory (`0x21`); Locate toggle; server removes the item mid-edit (`0x1baf` → placement cancelled with `0x1468`).
