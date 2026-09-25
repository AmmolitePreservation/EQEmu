# Server Implementation Plan

**Phase 2** of the server work (phase 1: `serverWork.md`): each foundation and subsystem planned against Peridot's EQEmu source (`PhoenixCampfire/peridot`, upstream `ab45d4358`, v23.8.1). Written in chunks; nothing here is code yet. The client research remains the specification.

**Principle: surgical.** Housing lives in its own module; existing EQEmu code is touched only at the points listed below, each for a stated reason. New tables, a few altered fields and data rows are expected.

**Status:** chunk 1 (architecture, touch points, persistence) written; chunks 2-7 to do.

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
| T9 | item data (decision D1) | the housing tail's values | F1 needs per-item values |

**The custom migration track ships with an upstream test entry at version 1** (it creates a table named `new_table`). It has never run (the running server reports `custom_database_version` 0), but it would run as soon as the binary version reaches 1, so **housing's first migration replaces it** rather than numbering after it.

### Persistence

- **Housing tables** follow EQEmu's repository pattern: generated base repositories (`utils/scripts/generators/repository-generator.pl`, the same generator as the 250 existing ones) plus thin extended repositories for queries the module needs.
- **The database is the source of truth.** Zones load what the players present need, when they need it (at zone-in, and on request), and write every change through before replying.
- Schema design (tables and fields for definitions, cost elements, properties, balances, placed items, access lists, neighborhoods, regions, polls, trophies) is part of chunk 3, where each table's shape comes straight from its messages.

## Chunk 1: decisions

**D1. Where the housing tail's item values live.** The `items` table already has a `placeable` column, but the in-memory item (`ItemData`, loaded into shared memory) doesn't carry it, and none of the other tail fields (upkeep id, trophy id, real-estate definition id, scale range, housepet file, flags, defaults) exist anywhere.
- **(a)** Add columns to `items` and fields to `ItemData`: EQEmu's conventional path, but it changes the shared memory layout and the item loader (`item_data.h`, the shared-memory item loading, the items repository).
- **(b)** A separate **`item_housing`** table keyed by item id, loaded by the housing module, and **a small provider hook** that the RoF2 serializer calls for an item's tail (default: zeros, as now, with the trophy id -1). Items without a row serialize exactly as today.
- **Recommended: (b).** Only items that matter to housing get rows; shared memory and the item loader stay untouched; the hook is the only item-side change.

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
