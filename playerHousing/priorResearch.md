# Prior Research Cross-Check

**Status:** final. Every open question, capture and TODO item on the page is answered or classified below.

Source: the EQEmu docs page "Player Housing" (docs.eqemu.io/server/player/player-housing; read via a public GitHub fork of the docs, xXxWarAngelxXx/Fork_EQEmu_docs-main, `categories/player/player-housing.md`). It holds Live packet captures from 2018–2019, some RoF captures, and open questions. **The RoF2 client code is the authority here**: the Live captures come from later clients and have drifted, so mismatches with them are recorded, not pursued.

## Their open questions, answered

### 1. "The placement window is disabled"

> The primary issue facing Player Housing is that the placement window is disabled. I don't know if there is a missing packet on client initialization, one that appears only when entering a zone that supports placing objects, a client status (Paid or not) level, or if the copy of RoF2 … simply doesn't support it. … The Place button … remains disabled in the Guild Lobby. Repeat the process in a neighborhood or inside a residence and the button becomes enabled.

**RoF2 supports it; the Place button is decided entirely by the client's real-estate manager**, never by `OP_NewZone` (which is why their NewZone flag changed nothing). It needs, together (`flows/placeItem.md`, `windows/RealEstateItemsWnd.md`, `placementMode.md`):

1. **A current property:** the plot whose `0x49df` region contains the player, falling back to `0x6fd0`'s current property (README critical findings 1–3). Live sent these in neighborhoods and residences and not in the Guild Lobby, which is exactly the pattern they saw.
2. **The definitions and property records** (`0x2698`, `0x6fd0` / `0x4f09` / `0x63fd` / `0x2691`) and **the property's item list** (`0x1cef`): Place is evaluated for the **selected row** of the Items list, so with no item list there is nothing to select.
3. **A placeable item.** **Even with every message right, stock EQEmu can't light Place up:** its RoF2 item encoder writes `Placeable = 0`, and the client reads that field as a **u32 "placeable in" bitmask** (open zones / guild halls / houses / yards / guild yards; critical finding 11).
4. **The placement rules** (`0x5b69b0`, 13 ordered checks), which include the **House of Thule** expansion bit through the permission system, and no pending real-estate request (5-second latch).

### 2. "A client status (Paid or not) level"

**Mostly expansion flags, not payment:**

- **Expansion bits** from `OP_PlayerProfile` (overwritable by `0x0767` / `0x0e48`): every real-estate permission needs **House of Thule `0x10000`**; guild plots and the guild trophy opt-in also need **Veil of Alaris `0x20000`**. **A server that limits expansions by era (Peridot is PoP-era) must still send these bits** (critical finding 6).
- **Account membership** (feature 10, "Create Guilds?") gates **guild plots only**, and **stock EQEmu already passes it** (it sends Gold with feature 10 on).
- `profileType` must be 0 for the trophy windows; EQEmu sends 0.

### 3. The multi-purpose opcode (Live 0x125C / RoF 0x67C9)

**RoF2 `0x2691`, fully decoded** (`opcode2691PropertySearch.md`): the server's neighborhood list, neighborhood details and single-property replies, and the client's searches (types 0, 1, 2, 4, 5). Their captures match (table below).

### 4. "Adding house interior zones (first attempt results in a client crash)"

**Reviewed statically (`robustness.md`):** no housing message handler has a crash path (every lookup is null-checked; the one unchecked index is `OP_GuildUpdate` action 5), and the client has no code specific to the housing zones. The likely causes are zone-level, and **the page's zone range is part of it**: the real house interiors are **714–720, 723, 739–750 and 766**; **721 and 722 aren't registered** in this client; and 724–736 and 752–765 are ordinary zones. Test plan and the list of interior zone files to check are in `robustness.md`.

## Their captures against RoF2

| Page capture | RoF2 | Result |
|---|---|---|
| **Opcode table**: RoF2 `OP_HouseAddress` 0x48AE, `OP_HouseContents` 0x6419 (ShowEQ); RoF `OP_HouseAddress` 0x6786 | none of them is handled or sent by this client | **wrong for RoF2** (the page itself doubted them). *Inferred from the names:* "house contents" is `0x1cef` (the property item list) and its updates; "house address" is the property record. The real set is in `README.md`. |
| **Door click → neighborhood list** (`OP_ClickDoor`, door 77 in the Guild Lobby) | the server answers the click with `0x2691` (kind 6, mode 0, `showWindow` 1) | **consistent**; the client never requests the list itself |
| **Neighborhood list** (S → C) | `0x2691`: skipped u32, **kind 6**, **mode 0**, u16 count, u8 showWindow, u32 selected, rows of id, name, guild id, 4 plot counts | **exact for RoF**; Live added a u32 after the guild id |
| **Neighborhood search / click** (C → S) | `0x2691` request, 224 bytes | **exact**; type 2 is the row click, "1,1" is the constructor's constant, "132,0" is uninitialized padding |
| **Neighborhood details** (S → C) | `0x2691` **kind 10**, **mode 2**: player plots, then guild plots | **consistent**; the per-guild "terminator" is Live-only |
| **Property marker** (S → C) | `0x2691` kind 4 (Plot) single-property: ids, **parent id** (their "Neighborhood ID"), definition id, guild id, strings, 4 × u32 (skipped by RoF2), u8 `openPurchase` | **start and end match**; their street address is RoF2's **property name** (`str2`, the Purchase window title); the "unknown name" (`str3`) is never read by RoF2; RoF2 takes Capacity / Vault / Pets from the definition; Live has more fields in the middle |
| **Trophies button** (C → S): Live 0xA02F, 342 bytes | the Items window's Trophies button toggles the Trophy Tribute window, which sends **`0x51a9` with no payload** on opening | **different**: RoF2's request is bare; Live's later client sends a 342-byte form |
| **`OP_NewZone`** extra bytes; "UnknownFlag = 1 in neighborhood zones … did not enable Place" | no real-estate code reads `OP_NewZone` | **explains their result** (question 1) |
| **Zone table** (neighborhood 712, interiors 714–766, guild halls 737 / 738 / 751) | client zone table `0x7e0de0` | **the interior range is wrong** (question 4) |

## Their TODO list, final status

| Page TODO | Status |
|---|---|
| In-zone clicks for the spring | **decoded**: left-clicking a placed item sends `0x218c` (and `0x050c` within 20 units), right-clicking opens the `0x21ec` menu, and placed housing containers use `OP_ClickObjectAction` with property and record ids (`worldLayer.md`, steps 3–4) |
| Purchasing a property | **decoded**: gate, request `0x45be`, reply `{u8 success}` that opens the Manage window, guild rank grants (`flows/purchase.md`) |
| Placing items in a property | **decoded**: `0x626e` → placement mode → `0x711a`, world objects `0x1045`, NPC items (`flows/placeItem.md`, `placementMode.md`, `worldLayer.md`) |
| Identifying RoF opcodes | **done** for RoF2 (`README.md`, `COVERAGE.md` sections 2–3) |
| House interior zones (crash) | **reviewed** (question 4); runtime test remains |
| Trophy data | **decoded**: trophies are placeable items with a benefit id, and personal benefits come from the trophies placed in your plots (`trophyTribute.md`, `flows/trophies.md`) |

## What they would have hit next

The page stopped at the Place button. With the client decoded, these are the next walls an EQEmu implementation meets (README *Critical findings*):

- **Item data:** the placeable bitmask, the upkeep cost id and the trophy benefit id all sit in the item tail that EQEmu's RoF2 struct misaligns and zeroes (findings 11, 12, 19).
- **The Neighborhood window looks empty with the stock XML**: all six filter checkboxes start unchecked (finding 13).
- **Placed items vanish on zoning** unless the server re-sends `0x1045` after every zone-in (finding 14), and the two placement float orders differ (finding 15).
- **Nobody can buy a guild plot** until the server grants rank permissions 27 / 28 with `OP_GuildUpdate` action 5, which EQEmu can't express (finding 17).
- **NPC items** need the server to fill the spawn id at `0x21ec` `+0x2fc`; housing tradeskill containers need property and record ids in `OP_ClickObjectAction` (finding 16).
