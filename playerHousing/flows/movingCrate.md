# Flow: Stow All, Moving Crate, House Swap

**Status:** client side decoded (`../windows/RealEstateItemsWnd.md`, *Counts, title and container buttons*; `../placedItemsAndProperties.md`, menu type `0x11`).

## 1. Goal

**Store All** moves every placed item at the player's location into storage. **Pack** moves the location's items into the active moving crate; **Unpack** moves the crate's items out. **House swap** replaces the placed house with another.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| C1 | Standing in a property (`+0x94`) with a known record and a placed-item container | `0x49df` + records + `0x1cef` | all three buttons |
| C2 | **May-manage gate** passes: House of Thule, Veil of Alaris (guild plot), membership, permission (manager vtable `+0x14`, `0x5b71a0`); **any non-zero disables** | expansions, membership, access lists | all three buttons |
| C3 | Store All: at least one placed item passes the stored-item rules | entries | enable |
| C4 | Pack: the location's container is non-empty; **the moving crate** (`+0xa0`: a crate child of the plot) exists and **isn't full**; at least one item passes the move rules into it | plot child list, crate record + list | enable |
| C5 | Unpack: the location isn't full; the crate exists; at least one crate item passes the move rules out | as above | enable |
| C6 | House swap: a house is already placed (`0x5b3170`) and the menu offered a type `0x11` entry | entries, `0x21ec` | the swap dialog |

## 3. Sequence

| Action | Client → server | Dialog | Payload |
|---|---|---|---|
| Store All | **`0x6917`** | `0x64` ("…move all of the placed items at your current location into storage?") | none |
| Pack | **`0x613d`** | `0x65` ("…pack the active moving crate with all of your real estate items at…") | u8 **1** |
| Unpack | **`0x613d`** | `0x66` ("…unpack all of the items in your active moving crate that are allowed…") | u8 **0** |
| House swap | **`0x21ba`** | `0x67` ("Are you sure you want to swap this house…", 30 s) | 24 bytes: -1, 0, current house property/record, new house property/record |

Each send sets the request latch. The server then sends the resulting entry changes (`0x1097`/`0x1baf`/`0x2cf7`, or a `0x1cef` reload per property).

## 6. Failure branches

| Symptom | Cause |
|---|---|
| All three buttons dark | not standing in a property, no container, or the may-manage gate fails (e.g. no House of Thule) |
| Pack dark | no crate placed on the plot, crate full, or nothing movable |
| Swap fails | server result codes `0x14d3`–`0x14da` ("Your house swap failed because…") |

## 7. Server obligations

Implement the three bulk actions (per item, the same rules as individual moves); keep the crate as a child of the plot (plot record child list); the swap; result codes; entry updates. The crate also matters for **selling** (a plot sells into a placed crate; `../resultCodes.md`).

## 8. Test cases

Store All with mixed stackable/unstackable items; Pack into an empty / full crate; Unpack into a full yard; no crate placed; swap houses with items inside (server-defined behaviour; the failure texts suggest capacity checks).
