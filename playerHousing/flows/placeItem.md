# Flow: Place a New Item

**Status:** client side decoded (`../windows/RealEstateItemsWnd.md`, `../placementMode.md`). Open items are marked **OPEN**.

## 1. Goal

A player standing on their plot (yard) or in their house takes a placeable item from inventory, clicks **Place** (or clicks in the world with the item on the cursor), positions it in placement mode, and commits. The item then appears in the world and in the Items list as "Placed".

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| S1 | **The item is placeable**: `ItemQuaternaryBodyStruct.Placeable` (+ the next three bytes) non-zero | the item serialization (Peridot's item tail writes Live's placeable mask; stock EQEmu's `rof2.cpp` hardcodes `Placeable = 0`) | every rule starts with it (`0x24`); the Place button is dark without it |
| S2 | Character expansion flags include **House of Thule `0x10000`** (and **Veil of Alaris `0x20000`** under a guild plot) | `OP_PlayerProfile` `expansions` | rule 1/2 (`0x2f`/`0x52`); **Place stays enabled without it**, so the server must refuse |
| S3 | Definitions received, including each property's **region id** (`+0x1c`), caps (`+0x28`/`+0x2c`/`+0x48`) and layout frame | `0x2698` | region → property lookup, capacity rules |
| S4 | Property records for the plot, its house/crate (plot child list `+0x48`) and ancestors; **`0x6fd0` current property** = an ancestor (e.g. the neighborhood) | `0x2691`/`0x63fd`/`0x6fd0` | "standing in" (`+0x94`) and "the plot you're on" (`+0x98`) |
| S5 | **Region volumes** whose ids equal the plots' definition `+0x1c` | `0x49df` (oriented boxes, 4×4 matrices) | "standing in" resolution; placement validity |
| S6 | Plot upkeep current (record `+0x40` = 0) and permissions (add item / add house / add crate) | `0x63fd`; access lists `0x485d` | rules 4 and 13 (Place stays enabled; server must refuse) |
| S7 | Capacity: below the location's cap, or the item can merge into an existing stack | item lists (`0x1cef`) + definition caps | `0x0a`/`0x0c`/`0x26` |
| S8 | No request pending (manager `+0xa4`) | client (reply or 5 s) | Place is dark while pending |

## 3. Sequence

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | player | select the inventory item (selection kind 0) or put it on the cursor | Place enabled = `0x72fc60(0x5af0a0(-1, +0x94, …))` |
| 2 | C → S | **`0x626e`** place request (60 bytes: header with the item's inventory location, `+0x2c` = 1, `+0x30` = 1, **`+0x34`/`+0x38` mouse or click X/Y**) | latch set |
| 3 | S → C | (**`0x49df`** regions, if not already sent) | clears the permission map |
| 4 | S → C | **`0x626e`** begin placement: placement object (transform, GUID, flags `+0x99`/`+0x9a`/`+0x9c`/`+0x2c`, property/record ids `+0x24`/`+0x28`), the item's location, A = 0 (new), B = 1 (create preview), **D/E = echo request X/Y**, spawn id, **the region permission list** (count + 1 `{region id, code}` pairs; code 0 = allowed) | clears the latch; placement begins; default sub-mode Wall/Cursor/Collision |
| 5 | player | move, rotate, change sub-mode; the preview turns green/red | validity = item box inside a region with code 0 (`0x4b2bf0`), surface (`0x1d`), not on a player (`0x1c`) |
| 6 | C → S | **`0x711a`** commit (quick click < 500 ms, valid position): the transform | latch set; an invalid position is refused locally |
| 7 | S → C | **`0x1228`** code 0 | placement ends (finalize) |
| 8 | S → C | the new entry (**`0x2cf7`** add, placed flag 1; or `0x1cef` reload), the item's world object (**`0x1045`**) or, for an NPC item, its spawn carrying the binding, and the inventory update removing the item | the list shows the item as Placed; counts update |

## 4. Decision trees

See `../windows/RealEstateItemsWnd.md`: *Action buttons*, *Can-place*, *Rules for placing from inventory* (13 checks), the trophy rule; `../placementMode.md`: *Validation*, *Boundary check*, *Sub-modes*.

## 5. Expected result per step

After 2: Place and the other gated buttons go dark (latch). After 4: the UI is hidden if `ItemPlacementHideUI` is on, otherwise it gets no mouse input; the preview appears under the click point. After 7: placement mode ends. After 8: Items list row "Placed", yard/house count incremented.

## 6. Failure branches

| Symptom | Cause |
|---|---|
| Place dark with an item selected | not placeable (S1), no local character, request pending, or a disabling code (`0x53` membership, …) |
| Request sent, server refuses with a message | server result code: `0x2f`/`0x52` (expansion), `0x18`/`0x19` (upkeep), `0x47`/`0x4b`/`0x4d` (permission), `0x0a`/`0x0c`/`0x26` (full), **`0x23` (zone) / `0x1f` (item limit) / `0x41` (lore): server-only** |
| No placement mode after the request | no `0x626e` reply; the latch clears after 5 s |
| Preview always red | no `0x49df` regions, region ids ≠ definition `+0x1c`, or no code-0 pair in the `0x626e` list |
| Only some sub-modes available | placement object flags (`+0x9c` wall, `+0x9a`/`+0x2c` block Free) |
| Commit ignored | invalid (red) position, on top of a player, or the press was longer than 500 ms |
| Placement continues after a commit | `0x1228` code ≠ 0 (message printed) |

## 7. Server obligations

Set `Placeable` in the item serialization; send `0x2698`, property records, `0x6fd0`, `0x49df` in that dependency order; answer `0x626e` with the begin-placement `0x626e` (echo X/Y, region permission list); validate the commit with the same rules (the client doesn't re-check); reply `0x1228`; create the entry, the world object (`0x1045`) or an NPC item's bound spawn, and the inventory removal.

## 8. Test cases

Happy path; not placeable (an item without Live's placeable mask); missing House of Thule (request sent, server refuses `0x2f`); drag outside the regions (red, commit refused); full yard (`0x0a`) vs mergeable stack (allowed); no reply to the request (5 s latch); commit rejected (placement continues).

## 9. Status and open questions

**OPEN:** the exact `0x2cf7`/spawn/inventory order Live used after a commit (a server design choice; the client handles each independently).
