# Server Implementation Plan

**Phase 2** of the server work (phase 1: `serverWork.md`): each foundation and subsystem planned against Peridot's EQEmu source (`PhoenixCampfire/peridot`, upstream `ab45d4358`, v23.8.1). Written in chunks; nothing here is code yet. The client research remains the specification.

**Principle: surgical.** Housing lives in its own module; existing EQEmu code is touched only at the points listed below, each for a stated reason. New tables, a few altered fields and data rows are expected.

**Status:** chunk 1 (architecture, touch points, persistence) written; D1 decided; D2-D4 open; chunks 2-7 to do.

## Chunk 1: architecture

### How housing reaches the client

- **Client messages** arrive in `Client` through the opcode table `ConnectedOpcodes[]`, filled by `MapOpcodes()` (`zone/client_packet.cpp:90`); entries are `Client` member functions (`ClientPacketProc`). Emu opcodes are named in `common/emu_oplist.h` and mapped to RoF2 wire values by `utils/patches/patch_RoF2.conf`.
- **Server messages need no translation layer.** Every opcode's encoder and decoder default to `PassEncoder` / `PassDecoder` (`common/struct_strategy.cpp:17-18`): a packet with no registered encoder goes out as built, only its opcode translated. Housing exists only for RoF2 clients, so **the housing module builds RoF2 wire formats itself** and never touches `rof2.cpp`'s message encoders or `rof2_structs.h`, with two exceptions (below).
- **Zone-in** completes in `Client::CompleteConnect()` (`zone/client_packet.cpp:531`): the F5 send sequence starts there.

### Module

New code in **`zone/housing/`**, one file per concern: the manager (per-zone state and the zone-in sequence), wire builders (RoF2 formats from the research), handlers per subsystem, rule checks (the client's rules, reproduced server-side), and persistence through repositories. Handlers dispatch internally on the emu opcode.

### Touch points in existing EQEmu code

| # | File | Change | Why |
|---|---|---|---|
| T1 | `common/emu_oplist.h` | one `N(OP_...)` per housing message | emu opcode names; none exist today |
| T2 | `utils/patches/patch_RoF2.conf` | the RoF2 wire value for each | the client's opcodes |
| T3 | `zone/client.h`, `zone/client_packet.cpp` (`MapOpcodes`) | **one** handler, `Client::Handle_OP_Housing`, mapped for every housing opcode, forwarding to the module | one declaration and one definition instead of one per message |
| T4 | `zone/client_packet.cpp` (`CompleteConnect`) | one call: start the housing zone-in sequence | F5 |
| T5 | `zone/client.cpp` (disconnect / zone-out) | one call: drop per-client housing state (a placement in progress, the edit lock) | no stale state across zone-ins |
| T6 | `common/patches/rof2.cpp` (+ `rof2.h`) | **the item tail fix (F1)**, and **one exported wrapper around the internal RoF2 item serializer** | F1 is a bug fix in the existing encoder; item-list entries embed a fully serialized item |
| T7 | `common/database/database_update_manifest_custom.cpp`, `common/version.h` | housing migrations on the custom schema track; `CUSTOM_BINARY_DATABASE_VERSION` | the custom track can't collide with upstream's versions |
| T8 | `zone/CMakeLists.txt` | the module's sources | build |
| T9 | the `items` table (migration, decision D1) | rename `UNK223`-`UNK241` and `placeable` to their true names; re-slice the shifted rows | the housing tail's values already exist there, unlabeled |
| T10 | `common/repositories/base/base_items_repository.h` | regenerated for the renamed columns | the only EQEmu code that names them (used by `#find item`) |

**The custom migration track ships with an upstream test entry at version 1** (it creates a table named `new_table`). It has never run (the running server reports `custom_database_version` 0), but it would run as soon as the binary version reaches 1, so **housing's first migration replaces it** rather than numbering after it.

### Persistence

- **Housing tables** follow EQEmu's repository pattern: generated base repositories (`utils/scripts/generators/repository-generator.pl`, the same generator as the 250 existing ones) plus thin extended repositories for queries the module needs.
- **The database is the source of truth.** Zones load what the players present need, when they need it (at zone-in, and on request), and write every change through before replying.
- Schema design (tables and fields for definitions, cost elements, properties, balances, placed items, access lists, neighborhoods, regions, polls, trophies) is part of chunk 3, where each table's shape comes straight from its messages.

## Chunk 1: decisions

**D1. Where the housing tail's item values live. Decided: rename `items` columns in place and fix their data.**

The PEQ-derived `items` table already holds the housing tail, captured from Live in unlabeled columns: **`UNK223`-`UNK241` are exactly `SubClass` plus the 18 housing-tail fields, in the client's order** (proved field by field on rows captured aligned; the values match the client's semantics: bitmask 8 on houses, 128 on guild halls; house definition ids 5-34, guild halls 38-41, the crate 31; upkeep ids 17-24). Most rows were captured **shifted by one byte** after the housepet string (an empty string's terminator), so from `UNK232` on their values must be re-sliced from the bytes; a few rows were captured aligned.

| Column | Becomes | Client field |
|---|---|---|
| `UNK223` | `subclass` | `SubClass` (`+0x5a4`) |
| `UNK224` | `placeable` | the "placeable in" bitmask (`+0x5ac`) |
| `UNK225` | `placeable_ignore_collisions` | `+0x5b0` |
| `UNK226` | `placement_type` | `+0x5b4` |
| `UNK227` | `real_estate_definition_id` | `+0x5b8` |
| `UNK228`, `UNK229` | `placeable_scale_min`, `placeable_scale_max` (float) | `+0x5bc`, `+0x5c0` |
| `UNK230` | `real_estate_upkeep_id` | `+0x5c4` |
| `UNK231` | `max_per_real_estate` | `+0x5c8` |
| `UNK232` | `housepet_file_name` (string) | `+0x5cc` |
| `UNK233` | `trophy_benefit_id` (-1 = none) | `+0x5ec` |
| `UNK234` | `disable_placement_rotation` | `+0x5f0` |
| `placeable` | `disable_free_placement` | `+0x5f1` (EQEmu's 2015 migration named `UNK235` `placeable`; it isn't) |
| `UNK236` | `npc_respawn_interval` | `+0x5f4` |
| `UNK237`-`UNK240` | `placeable_default_scale`, `_heading`, `_pitch`, `_roll` (float) | `+0x5f8`-`+0x604` |
| `UNK241` | `interactive_object` | `+0x608` |

- **Why rename rather than add:** EQEmu's server never reads these columns (the shared-memory loader's `item_fieldlist.h` skips the block) and EQEmu itself renamed unknowns in place before (migration 9093); renaming keeps one source of truth, no second "placeable", and the column count (dumps keyed by position still load).
- **Cost, accepted by the owner:** Spire's item model maps these columns by name, so **Spire's item editor, item endpoints and eqtraders item import break** for them. Spire's server control doesn't use the item model (its dashboard only counts items), so it keeps working.
- **The migration** renames the 19 columns, converts the float and string ones to their types, and **re-slices the shifted rows** (detected per row) into the right columns; items without a placeable bitmask get the defaults (trophy id -1).
- **The decoder is validated** (`tools/itemTailDecode.py`) against all **5,577** items with a placeable bitmask: **every row has exactly one plausible layout** (5,205 shifted, 372 aligned; none ambiguous, none implausible both ways), and every decoded field is sane: trophy ids decode to exactly the trophy items (100 "Idol of Severilous", 111-120 the "Empowered Trophy of ..." series, 121 "Trophy of Expanding Life"; 88 in all, the rest -1); default headings and pitches fall on EverQuest's 512-unit scale (0, 128, 256, 384); respawn intervals 4500 (familiar NPC items), 750 (NPC merchants) or 0; default scales 0.25-5.0; scale ranges 0.05-25; disable free placement on 170 items (houses, halls), disable rotation on 455. **Shifted rule** (per 32-bit value, little-endian): each field from the trophy id on is the high three bytes of its column plus the low byte of the next (`UNK233`/`UNK234` form the trophy id; `placeable` is the rotation flag; `UNK236`'s low byte is the free-placement flag). **One oddity kept as captured:** the Candy Dish (55446) has bitmask -1 (placeable anywhere).
- **Content found along the way:** Live's real-estate catalog ids: house definitions 5-34 (by style, size and interior), guild halls 38-41, the moving crate 31; upkeep cost ids 17-24 for houses by size.
- **Reaching the serializer:** the in-memory item (`ItemData`) still doesn't carry these fields, so the housing module loads them for items with a non-zero `placeable` and the RoF2 serializer asks it through the provider hook (T6); items without them serialize as today.

**D2. Cross-zone updates.** Zones are separate processes; ownership and the character's property lists are global.
- **(a)** Start with **no cross-zone messaging**: the database is the truth and every zone-in re-sends housing state (F5 requires that anyway), so a change made in another zone appears at the next zone-in.
- **(b)** Add one server op now (definition, world forwarding, zone handling: three more touch points) for real-time refreshes.
- **Recommended: (a) first**, adding (b) only where a real case needs it (a transfer to a player in another zone is the likely first one).

**D3. House interiors.** An interior is an ordinary zone to the client (it needs the S1-S4 data there too).
- **Recommended:** each house's interior is an **EQEmu instance** of its interior zone (`database_instances.cpp`), with the instance id kept on the property; entering routes the player into it. To be confirmed against the zone-in path in chunk 6, and against Q11 (the interior-zone crash) in testing.

**D4. When upkeep is charged.** No client rule decides this.
- **(a)** **Lazily**: due charges are computed from the last-charged time whenever a property is loaded or touched (zone-in, the Manage window, a purchase), so no scheduler or new process is needed.
- **(b)** A daily job in the world server.
- **Recommended: (a)**; it's deterministic, needs no timer, and fits D2 (a).
