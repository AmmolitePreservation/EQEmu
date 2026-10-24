# Placed Items and Properties — Message Specs (RoF2 client)

Decoded by reading `RealEstateManagerClient::HandleMessage` (`0x5b2500`) case by case, then each reader it calls. The stream cursor is `{buffer, length, position}` (the same layout `OP_ItemPacket` builds with `0x410c40`), and every integer is little-endian. The `string` type means a `u32 length` followed by that many raw bytes, as in the [`0x2698` spec](opcode2698DefinitionsAndCosts.md).

Confidence labels: **wire** means the field order and type were read directly from the reader code. **Meaning** is labeled as either *verified* (from how the code uses the field) or *inferred*.

## How placed items exist in the world

1. The **property item list (`0x1cef`)** gives the client every placed-item record in a property: an id, a flag, seven floats (placement transform), two strings, and a **full serialized item**.
2. **Placed items exist in the world in two ways** (see `worldLayer.md`): as **world objects** (added by `0x1045`, removed by `0x2a9e`) and as **NPC items** (spawns bound by `0x21ea`). The original note follows: **Placed items render as spawns.** `0x21ea` takes a spawn id and tags that spawn with a placed item's 16-hex-character GUID (spawn `+0x378`) plus two dwords. So the server spawns each placed item as an ordinary world spawn and then binds the spawn to the item record. The client never builds world actors from the list itself; no path from the manager to actor creation exists.
3. **Placement** is `0x626e` (request) → placement mode → `0x711a` (commit). See the README. When the spawn for the item currently being placed arrives, `0x21ea` records its spawn id in the placement state singleton (`0x558540`, `+0x24` = 1, `+0x28` = spawn id).

This supersedes the earlier NPC-only reading of the commit's spawn-relative branch. Any placed item that exists in the world is a spawn, and NPC items (placed items backed by an NPC spawn) are one case of that.

## `0x1cef` — Property item list (server → client)

Case `0x5b2654` → `0x5b0d00` → container reader `0x5ac050` → entry reader `0x5abd20`.

```
u32   realEstateID          find or create this property's container (manager map +0x64/+0x68)
u32   entryCount
Entry[entryCount]
```

The container is cleared (`0x5ab3a0`) before reading. **If any entry fails, the whole list fails**: the property's container is dropped (`0x5b4680`) and the Items and Manage windows refresh. An entry fails when it contains no item.

### `Entry` (client object 0x3c bytes; wire order below)

| # | Wire | Client field | Meaning | Evidence |
|---|---|---|---|---|
| 1 | u32 | +0x34 | **the id of the property this item belongs to** (verified: the Items list keys rows by it, only shows items of the tracked properties, and removes a property's rows by it; `0x735a60`, `0x730880`); also copied to the item's `+0x24` | `0x7fca90`; `0x556e10`; `0x735a60` |
| 2 | u32 | +0x38 | **placed-item record id**, unique within the property; a re-sent id replaces the old entry (*verified*); also copied to the item's `+0x28` | `0x7fca90`; key of `0x5ab910`; `0x556e10` |
| 3 | u8 | +0x00 | **placed flag**: 1 = placed in the world, 0 = stored (verified: the Items list's Status column and Show Placed/Stored filters); also gates a UI-dirty update after the read | `0x7fc9a0`; `0x5abfe7`; `0x735a60` |
| 4 | u32 | +0x04 | **not read by any client code** found; Live's field here is `upkeepExpiredTime`, the item's upkeep expiry (0 while paid) | `0x7fc9a0` |
| 5–11 | f32 ×7 | +0x08..+0x20 | **placement transform: heading, pitch, roll, scale, x, y, z** (verified: the layout writer `0x5ae900` reads them in this order to save layouts, heading and position relative to the yard). The same order is used by the layout file and `0x5b41`. **The placement object (`0x626e`, `0x1045`) uses a different order: scale, heading, pitch, roll, position** (`worldLayer.md`); the server must convert between them. The placement commit (`0x711a`) uses this entry's order. | `0x7fc9e0`; `0x5ae900` |
| 12 | string | +0x24 | **owner name** (verified: shown in the Items list's Owner Name column; the Owned/Unowned filters compare it with the player's name). The client stores `CRC32(string)` at `+0x2c` for fast lookups, and `0x1967` removes every entry with a given owner | `0x7fc960`; CRC32 `0x808cb0`; `0x735a60`, `0x736bc0` |
| 13 | string | +0x28 | **not read by any client code**; Live's field here is `ownerHandle`, the owner's per-account handle as text (only assigned at entry creation, `0x5acb40`) | `0x7fc960` |
| 14 | Item | +0x30 | **the full item, in the standard RoF2 serialized form**: exactly the bytes `OP_ItemPacket` carries after its leading `u32` packet type (*verified*: same deserializer `0x5559a0`, same cursor, no prefix) | `0x5ac0b0` → `0x5aad80` → `0x5559a0` |

**For the server:** EQEmu already emits the item portion for RoF2 `OP_ItemPacket`. Reuse its item serializer and leave off the leading packet type.

## Incremental item-list messages (server → client)

These all use the `Entry` format above and act on a property's existing container, looked up by `propertyID` (manager map `+0x64`/`+0x68`).

| Opcode | Payload | Effect | Evidence |
|---|---|---|---|
| `0x087f` | `u32 propertyID`, `string` (read and discarded), `u32 count`, `Entry × count` | **Add or update entries without clearing.** The container is created if needed. Entries are read with flag 0, which allows the "item placed" notice below. If the container is **new**, the first bad entry deletes it and stops the read; if it **already existed**, bad entries are skipped. A new container is inserted afterwards (`0x5b5170`). | `0x5b0e30` |
| `0x2cf7` | `u32 propertyID`, one `Entry` | **Add or update one entry** (flag 0). Ignored if the property has no container. | `0x5afe00` → `0x5abd20` |
| `0x1097` | `u32 propertyID`, one `Entry` | **Replace an existing entry**, matched by record id. If there's no match, or the matched entry has no item, the update is dropped. Otherwise all fields and a clone of the new item are copied in (`0x7fcb50`), including `+0x34`/`+0x38`. If the item is the one currently being placed, the placement state is updated. The Items and Manage windows refresh according to what changed: flag, the u32 after it, the item's stack count (item `+0x98`), and the first string. | `0x5afe00` → `0x5ab4b0` |
| `0x1baf` | raw 8 bytes: `u32 propertyID`, `u32 recordID` | **Remove one entry.** If it is the item currently being placed, placement is cancelled first (`0x557b60`) and chat string id `0x1468` is shown. | `0x5afe80` → `0x5ab910` |
| `0x1967` | raw: `u32 propertyID`, then a **NUL-terminated string** at `+0x04` | **Remove every item owned by that player** (entries whose owner name `+0x24` equals it), matched by CRC32 and then by text. Each removal decrements the container counters `+0x0c`, `+0x10`, and `+0x14` according to item predicates (`0x7fcad0` = placeable, i.e. a real-estate item; `0x7fcb00` = pet/NPC item (`0x7b1640`); `0x7fcb40`), i.e. the real-estate/vault/pet counts behind the definition caps. | `0x5afd50` → `0x5ab030` |

**"The item you are placing has been moved."** (string id `0x1469`): shown in chat and on screen when an accepted entry (`0x087f`, `0x2cf7` with flag 0) or a `0x1097` replacement is the item currently being placed. It is suppressed while the manager has a request pending (manager `+0xa4`). `0x1baf` shows `0x1468` instead: "Item placement has been cancelled because the item being placed has been removed from the real estate."

*Inferred:*
- The entry's first string is a grouping key, possibly the name of the character who placed the item; `0x1967` would then remove everything one character placed.
- The container counters `+0x0c`, `+0x10`, and `+0x14` are the per-category counts that the definition's three caps (max items, max non-real-estate items, max pet items) apply to. Which counter corresponds to which cap is unverified.

## `0x21ea` — Bind spawn to placed item (server → client)

Case `0x5b26fa` (inline).

| Offset | Type | Meaning | Evidence |
|---|---|---|---|
| +0x00 | u32 | **spawn id**; the message is ignored if the spawn isn't known | `GetSpawnByID` `0x5996e0` |
| +0x04 | char[16] + NUL | **placed item GUID**, 16 hex characters; the client validates it by round-tripping through hex decode and encode | copied to spawn `+0x378` by `0x80c920` |
| +0x18 | u32 | **property id** of the item; copied to spawn `+0x198` | verified: the world right-click builds the `0x21ec` request from spawn `+0x198`/`+0x420` (`0x537d30`) |
| +0x1c | u32 | **record id** of the item; copied to spawn `+0x420` | as above |

Total length is 32 bytes (`+0x14`–`+0x17` is padding after the terminator). If the GUID matches the item currently being placed, the placement state records the spawn id, and the spawn's actor is then refreshed (spawn vtable `+0x4c`, then `+0x08` with argument 1).

## `0x58c7` — Inspect item (both directions)

- **Server → client** (case `0x5b2c7a`): the payload **is** a standard serialized item, with no leading type. The client hands it to the item display manager (`[0xf73a28]`, method `0x6ae610`), which opens an item display window. The same manager serves merchant previews and Krono displays.
- **Client → server:** sent by the Items window (site `0x73ece8`). Request layout still to be decoded.

*Not* a list of placed items.

## Property record (`0x6fd0`, `0x4f09`, `0x63fd`; server → client)

All three deserialize the same class (vtable `0xa1052c`, reader `0x7fdf00`).

```
u32 id                 client +0x14
u32                    +0x18  (Live: staticId, -1; 0 on a vacant player plot)
u32 parentID           +0x1c  (parent property id; walked by 0x4d6790)
u32 definitionID       +0x20  (0x6fd0 looks up its type)
u32 guildID            +0x24  (owning guild, -1 = none; see the Purchase window checks)
u32                    +0x28
string                 +0x30  (Live: primaryHandle, the owner's account id as text; empty with no owner)
string                 +0x2c  (owner name: compared against the local character's name at player +0xa4)
u32                    +0x34  (Live: defaultPermissionLevel, 0)
u32 idCount            (count comes before the next string)
string                 +0x38
u32 × idCount          list at +0x48/+0x4c
u8  pairCount
{u32 key, f64 value} × pairCount   keyed map at +0x04  (balance per currency; key 0 = coin)
u32                    +0x3c
u32 upkeepExpiredTime  +0x40  (**upkeepExpiredTime**: 0 while paid; non-zero, the second a charge found the escrow empty, = expired → codes `0x18` (plot, `0x5b4780`) / `0x19` (house or crate); `0x63fd` compares it between updates)
u8 votingOptOut        +0x44  (REPW_VotingOptOut_Checkbox checked state)
```

- **`0x6fd0`** sets the **current property**: manager `+0x8c` = id and `+0x90` = definition type, then calls manager vtable `+0x24`.
- **`0x4f09`** refreshes the Manage window, raises a UI-dirty flag if the owner name matches the local character, then calls manager vtable `+0x28(id, 0, 0, 0)`, which **removes the property** (`0x5b7c70`; Live's client the same), so the server sends it only for a property that is gone.
- **`0x63fd`** merges an update into a known property, and inserts a property it doesn't know (vtable `+0x24`, `0x5b2f19`), so every record goes out as `0x63fd`. It compares the owner name and the `+0x40` flag to decide what to refresh, and updates the Items window (for the three ids it tracks), the Manage window, and the Purchase window.

## Character property lists (server → client)

Each of these first requires the local character (`[0xdd2630]` → `0x58c9a0`).

| Opcode | Shape | Action |
|---|---|---|
| `0x2aa7` | `u32 count`, `u32 × count` | clear list A (`0x5b4060`), then add each id (`0x7c9aa0`) |
| `0x3e00` | `u32 count`, `u32 × count` | clear list B (`0x5b40a0`), then add each id (`0x7c9b10`) |
| `0x66df` | `u32 count`, `u32 × count` | clear list C (`0x5b40e0`), then add each id (`0x7c9b80`) |
| `0x0c13` | `u32 id` at +0x00, byte flag at +0x68 | flag set: add to list B (`0x7c9b10`); clear: `0x7bfb90` |
| `0x76c6` | `u32 id` at +0x00, byte flag at +0x68 | flag set: add to list A (`0x7c9aa0`); clear: `0x7bfb10`; then raise the UI-dirty flag |

**Named by MacroQuest** (`PcClient`): list A is **`OwnedRealEstates`** (`+0x29ac`), list B **`OwnedItemRealEstates`** (`+0x29bc`: properties contained in items the character carries; the client's `GetItemContainedRealEstateIds`), list C **`ArchivedRealEstates`** (`+0x29cc`: a subset of the owned properties; the client's `GetNonArchivedOwnedRealEstates` excludes them). `0x0c13` and `0x76c6` are single-id updates to the same lists. The payload is at least 0x69 bytes; the bytes between +0x04 and +0x68 aren't read here.

## Result codes, container drop, and prompt (server → client)

### `0x66bf`, `0x68ba` — plot result codes

`0x66bf` is the **sell plot** result and `0x68ba` the **transfer plot** result; each is a single `u32 code` printed as a chat line. The full code tables with texts, along with `0x1228` (item request result), are in [`resultCodes.md`](resultCodes.md).

#**Property record child list (`+0x48`, the `u32 × count` list):** the **child property ids**, e.g. the house and moving crate placed on a plot (verified: the manager tick finds the plot's house by scanning it, `0x5b0000`).

## `0x7d28` — drop a property's item container (`0x5b4680`)

Payload is `u32 propertyID`. The client frees that property's whole placed-item container and removes it from the manager map (`+0x64`). This is the same routine `0x1cef` uses when a list fails to parse.

### `0x5a63` — plot transfer offer (inline in case `0x5b2b87`)

The payload is the **188-byte fixed struct** described below. The client copies it (`0x5ad760`), looks up the property named by its first u32 (`0x466030`, using the record's `+0x38` string), and formats string id `0x1dd4`, **"Do you wish to accept %1 from %2?"**, with `%1` = the property record's `+0x38` name and `%2` = the struct's `+0x0c` string (the offering player). Together with `0x68ba` (transfer plot result), this makes `0x5a63` the **plot transfer offer**. It then opens a confirmation dialog through `[0xf74338]->0x71eb20`: dialog type `0x89`, a 300,000 ms (5-minute) timeout, and the struct copy as its data.

**Reply (verified):** the dialog's target is the game object's dialog interface (`[0xe67ccc]+8` → vtable `0x9d1c08` → `0x52d540`). Its case `0x89` (`0x52e6eb`) sets struct `+0xb9` = 1 on yes or 0 on no (and on no prints `0x1dd6`, "The real estate transfer has been declined."), then **sends the whole 188-byte struct back as client → server `0x5a63`**. The offer itself is triggered by the sender's `0x507f` (see the README's *Confirmation dialogs*).

### The 188-byte struct (`0x5a63`, `0x507f`, `0x792c`)

Copy routine `0x5ad760`, constructor `0x5ad6f0`:

| Offset | Type | Default |
|---|---|---|
| +0x00 | u32 (property id in `0x5a63`) | 0 |
| +0x04 | u32 | -1 |
| +0x08 | u32 | -1 |
| +0x0c | char[64] | empty |
| +0x4c | char[32] | empty |
| +0x6c | char[64] | empty |
| +0xac | u32 | 0 |
| +0xb0 | u32 | 0 |
| +0xb4 | u32 | 0 |
| +0xb8 | u8 | 0 |
| +0xb9 | u8 | 0 |
| +0xba | u8 | 0 |

Total size is 0xbc (188) bytes. **Five opcodes carry this struct**:

| Opcode | Direction | Built by | Fields set |
|---|---|---|---|
| **`0x45be`** | C → S | Purchase button (`0x74d563`) | **purchase request**: `+0x00` property id (Purchase window `+0x228`), `+0xac` selected **price option key**, `+0xb0` selected **upkeep option key** (0 without the combobox), `+0xb4` global `[0xdd5d00]` (unknown) |
| **`0x792c`** | C → S | Purchase dialog `0x64` "sell this plot?" yes | **sell request**: `+0x00` property id (window `+0x228`) |
| **`0x507f`** | C → S | Purchase/Manage dialog `0x65` "transfer … to %1?" yes (`0x5b1470`) | **transfer request**: `+0x00` = manager `+0x94`; 64 characters from `[0xdd2648]+0xa4` (the target's name) copied to `+0x68`. `+0x68` falls inside the `+0x4c` string's range, so field boundaries there are unconfirmed. |
| **`0x5a63`** | S → C | server | **transfer offer** to the recipient: `+0x00` property id, `+0x0c` the offering player's name |
| **`0x5a63`** | C → S | game dialog `0x89` answer | **transfer reply**: the same struct echoed, with `+0xb9` = 1 (accept) or 0 (decline) |

## `0x21ec` — Placed-item interaction menu (server → client)

Handled by the Items window (`[0xd1fcb8]->0x7375f0`). This is a **raw fixed-size struct** (at least 0x300 bytes): the handler indexes the buffer directly. It builds and **pops up a context menu of interaction actions for one placed item**. The client also sends `0x21ec` (Items window); that request hasn't been decoded.

Verified as a context menu from the client's own menu code: `[0x15d391c]` is the context-menu manager; `0x85b590`, instantiated for each new submenu, uses the menu-template strings "Item", "SubMenu", and "Check"; `0x85b7b0` (233 callers) adds a menu item; and `0x862220` pops up the menu with the window's menu (window `+0x358`) at a point.

| Offset | Type | Use | Evidence |
|---|---|---|---|
| +0x00 | char[18] | **item GUID** (16 hex characters + NUL, padded), copied into window `+0x370` | `0x80c920` at `0x737cf4` |
| +0x12..+0x1f | — | not read | |
| +0x20 | u32 | **property id** of the item (window `+0x384`; also passed to `0x736a00`) | the client's request sets it from the list row (`0x542a90`) |
| +0x24 | u32 | **record id** of the item (window `+0x388`; also passed to `0x736a00`) | as above |
| +0x28 | — | not read by the handler | |
| +0x2c | char[64] | **menu title**, added first in green (`0xff00ff00`) | `0x85b7b0` at `0x7377a5` |
| +0x6c | entry[40] | **menu entries**, 16 bytes each (below); also copied into window `+0x390 + i*16` | loop to `0x28` |
| +0x2ec | u8 | **filter flag** (below) | |
| +0x2f0 | i32 | **popup point X** | `0x862220` |
| +0x2f4 | i32 | **popup point Y** | `0x862220` |
| +0x2f8 | u32 | window `+0x36c` (no use identified) | `0x737cad` |
| +0x2fc | u32 | **the NPC's spawn id** for an NPC item (window `+0x38c`; menu entry types `0xc`/`0xd` look it up and run the NPC interaction). The client's request doesn't carry it; **the server must fill it** | `0x737d18`, `0x73ee1a` |

### Menu entry `{u32 type, u32 b, u32 labelID, u32 d}`

- **`labelID`:** for a **type 9** entry or a **type `0xa`** submenu header, a **`dbstr_us.txt` category 28 id**, looked up with `[0xd1f380]->0x4866c0(id, 0x1c, 0)`; for **every other type**, an **`eqstr_us.txt` id**, looked up with `[0xdd25c4]->0x7d0660(id, 0)` (the type 9 test at `0x737b2b`; Live's client makes the same split at `0x1404f3a20`, and its own menus label Enter, Inspect, Show Items and Help, types 1, 7, `0xb` and `0xe`, with eqstr 723, 720, 5285 and 13300). Category 28 in the owner's file holds 132 interaction actions, for example "Stay", "Follow Me", "Open the Door", "Pose: Sit / Stand", "Activate/Deactivate Idol", "Fly / Stop Flying", "Pet the Tiny Bear".
- **`type`:** -1 means an empty slot, which is skipped. Type `0xa` entries go into a **submenu** keyed by `d`: an existing submenu with that key is reused, otherwise a new one is created. Types choose the text color:
  - `0x12`: `0xffc04080`
  - 1, 8, 9, `0xa`, `0xc`, `0xd`: `0xff40c0c0`
  - 2, 3, 4, 5, 6, `0x11`: `0xff8aa3ff`
  - 0, `0xb`, `0xe`: `0xffc08080`
  - anything else: `0xff80c080`
- **Filter flag `+0x2ec`:** when set, types 3, 4, `0xb`, `0xe`, and type 2 with `b` = 1 are left out.

### Picking a menu entry (client → server)

Menu command ids are `0x65 + entryIndex`. The Items window notification handler (switch at `0x73e6fa`) reads the picked entry's `type` (window `+0x390 + index*16`) and dispatches through table `0x73f5dc`:

| type | action | evidence |
|---|---|---|
| 1 | send **`0x218c`**, 12 bytes `{u32 property id (+0x384), u32 record id (+0x388), u32 entry.b}` (the same message a world click on a placed item sends) | `0x73e730` |
| 2 | **place / edit**: send **`0x626e`** (60-byte place request: the common header with the menu's property/record ids, location -1); the server's `0x626e` reply starts placement (editing for an already-placed item) | `0x73e7e8` |
| 3 | **store**: send **`0x3424`** (48 bytes: the header with the menu's ids + destination **`0x5b5660(property id)`**, the property's own storage; quantity 0) | `0x73e903` |
| 4 | **pick up**: send **`0x2036`** (44 bytes: the header with the menu's ids, constructor `0x542470`) | `0x73ea10` |
| 5 | send **`0x7ad1`**, 148 bytes `{u32 property id, u32 record id, 140 zero bytes}`; the zero tail is unused (the client never receives `0x7ad1`), so the action is whatever the server's menu entry says | `0x73eae8` |
| 7 | local: look up the placed entry (`0x5b01c0`) and show its item in an item display window | `0x73ec25` |
| 8 | send **`0x050c`** | `0x73ebbf` |
| 9 | send **`0x51e3`** | `0x73ed1d` |
| `0x13` | send **`0x64a3`** | `0x73ed5e` |
| `0xc`, `0xd` | local: `GetSpawnByID` then `0x536d80` (the merchant path), which fits interacting with an **NPC item** (a placed item backed by an NPC spawn, such as a vendor or banker) | `0x73ee1a` |
| `0xe` | **ask for text**: opens the client's **`TextEntryWnd`** (`[0xd1fcf4]`, `0x776ef0` with id `0x4b`); its answer (notification `0x26`, second handler `0x73ecc0`) sends **`0x3c8b`** (below) | `0x73edfe` |
| `0x11` | **swap house**: the house-swap confirmation (dialog `0x67`, "Are you sure you want to swap this house with the house that is currently placed?…", `0x14d9`, 30 s; the currently placed house is found by `0x5b3170`), and on yes **`0x21ba`** (below) | `0x73e78d` |
| 0, `0xb` | local UI actions only (`0xb` re-adds the row, `0x736a00`) | |
| 6, `0xa`, `0xf`, `0x10`, `0x12` | nothing (`0xa` entries are submenu headers) | |

Types 8, 9, and `0x13` share one send tail (`0x73ebe4`) with a 12-byte payload `{u32 window+0x384, u32 window+0x388, u32 entry.b}`: the **property id and record id** from the `0x21ec` header (`+0x20`/`+0x24`) plus the entry's `b`.

**`0x21ba` (swap house, client → server, 24 bytes)** after the swap confirmation (menu type `0x11` → dialog `0x67`, answer handler `0x735480`): `{u32 -1, u32 0, u32 current house property id, u32 current house record id, u32 new house property id, u32 new house record id}` (the new house's ids are the menu's `+0x384`/`+0x388`, carried in the dialog). The client has the failure texts `0x14d3`–`0x14da` ("Your house swap failed because…") for the server's result codes.

**The meaning of each entry type is defined by the server:** it supplies the entries (with their labels) in `0x21ec`, and the client only maps the type to the request above. None of these requests (`0x218c`, `0x21ba`, `0x7ad1`, `0x050c`, `0x51e3`, `0x64a3`, `0x3c8b`) is also received by the client; only `0x58c7` (inspect) has a server → client handler (`0x4cc1d7`).

**Related sends (client → server):**
- **`0x58c7`**, inspect request: 8 bytes, `{u32 window+0x384, u32 window+0x388}` (`0x73ecc6`).
- **`0x3c8b`** (the answer to menu type `0xe`): a 352-byte struct (constructor `0x73f630`): `+0x00` u32 = 2, `+0x08` u32 **property id**, `+0x0c` u32 **record id**, `+0x28` **the entered text** (up to 128 characters) from **`TextEntryWnd`** (`[0xd1fcf4]`, class `0x774d00`), sent on notification `0x26` (`0x73f445`).

**`b`** is a value the server chooses per entry: types 1, 8, 9 and `0x13` send it back, and the client's only own use is the filter flag (a type-2 entry with `b` = 1 is hidden when `+0x2ec` is set). **`d`** keys type `0xa` submenus. What each type *means* is the server's choice; the client only maps it to the request above.

## Other cases

| Opcode | Handling | Notes |
|---|---|---|
| `0x1228` | `0x5ae1f0(u32 code)` | **decoded**: item request result; see [`resultCodes.md`](resultCodes.md) |
| `0x087f` | stream → `0x5b0e30` | **decoded**, incremental list (above) |
| `0x2691` | stream → `0x5b0f80` | **decoded**, see [`opcode2691PropertySearch.md`](opcode2691PropertySearch.md) |
| `0x1097`, `0x2cf7` | `0x5afe00(opcode, stream)` | **decoded**, replace / add one entry (above) |
| `0x1baf` | `0x5afe80(opcode, buffer)` | **decoded**, remove one entry (above) |
| `0x1967` | `0x5afd50(buffer)` | **decoded**, remove entries by string (above) |
| `0x21ec` | Items window `0x7375f0(buffer)` | **decoded** (above): raw struct, item tree |
| `0x2aa6` | Manage window `0x741670(stream)` | **decoded**, see [`opcode2aa6PropertyAccount.md`](opcode2aa6PropertyAccount.md) |
| `0x3968`, `0x485d` | access-list object (`0x5b5250`) read by `0x5b7a90` | **decoded**: remove / set a property's access list; see [`permissions.md`](permissions.md) |
| `0x45be` | byte flag; if set, Manage window virtual `+0xd8(1, 1, 1)` | purchase result? |
| `0x5a63` | 188-byte object from the buffer; timed (300 s) confirmation dialog, string id `0x1dd4` | **decoded** (above); the reply is open |
| `0x66bf`, `0x68ba`, `0x7d28` | `u32` → `0x5ad930` / `0x5adad0` / `0x5b4680` | **decoded** (above): two result codes and a container drop |
| `0x0351`, `0x3d94` | ignored by the manager | |
