# Server Implementation Plan

**Phase 2** of the server work (phase 1: `serverWork.md`): each foundation and subsystem planned against Peridot's EQEmu source (`PhoenixCampfire/peridot`, upstream `ab45d4358`, v23.8.1). Written in chunks; nothing here is code yet. The client research remains the specification.

**Principle: surgical.** Housing lives in its own module; existing EQEmu code is touched only at the points listed below, each for a stated reason. New tables, a few altered fields and data rows are expected.

**Scope:** the standard housing system (`serverWork.md`) **and all three non-standard extensions** (X1 private zones as interiors, X2 custom neighborhood maps, X3 plots in the live world; section below), designed together.

**Development model:** everything is designed first, then written in full, and the owner builds once. **Database work is tested for real** before delivery: migrations and queries run in a sandbox MariaDB created from the server's real schema (`pok-schema-all.json`), including the `items` rename and decode against the exported rows. **Code is verified by careful reading** against Peridot's source and the research specifications; EQEmu isn't built in the sandbox. After the owner's build, validation happens in game with the diagnostics plugin and the Lua harness.

**Status:** chunk 1 complete. Remaining chunks, each covering the standard system and X1-X3 together:

| Chunk | Covers |
|---|---|
| 2 | F1 (the item tail: byte-exact wire order, the rename migration's SQL, the serializer hook) and F2 (every housing opcode with its RoF2 value) |
| 3 | the schema: every housing table (definitions with their zones, costs, properties with hosts and revisions, balances, placed items, access lists, neighborhoods with template zones, regions, polls, trophies, transfer offers), migrations, repositories; and the housing zone versions and instances |
| 4 | S1-S4 and the zone-in sequence (F5) for any host zone; region authoring (the GM plot tool) |
| 5 | S5 placement and editing; the world layer (world objects, NPC items, containers) |
| 6 | S6-S9 (neighborhoods and search, purchase / sell / transfer across zones, escrow and the upkeep tick, access and its enforcement) |
| 7 | S10-S12 (houses and private-zone interiors, the crate, layouts, leaving; polls; trophies) |
| 8 | the database test setup, the probe checks and Lua suites for the owner's validation, and the order to validate in |

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
| T9 | the `items` table (migration, decision D1) | rename `UNK224`-`UNK241` and `placeable` to their true names; re-slice the shifted rows | the housing tail's values already exist there, unlabeled |
| T10 | `common/repositories/base/base_items_repository.h` | regenerated for the renamed columns | the only EQEmu code that names them (used by `#find item`) |
| T11 | `common/servertalk.h` | `ServerOP_Housing` and its envelope | cross-zone housing events (D2) |
| T12 | `world/zoneserver.cpp` | one `case` in the list world forwards to every zone | delivery |
| T13 | `zone/worldserver.cpp` | one `case` handing the envelope to the housing module | receiving |
| T14 | `common/ruletypes.h` | the `Housing` rule category | configuration through EQEmu rules |
| T15 | `world/main.cpp` | the upkeep `Timer`, a startup run, one call | the upkeep tick (D4) |
| T16 | `world/CMakeLists.txt` | the world-side housing sources | build |

**The custom migration track ships with an upstream test entry at version 1** (it creates a table named `new_table`). It has never run (the running server reports `custom_database_version` 0), but it would run as soon as the binary version reaches 1, so **housing's first migration replaces it** rather than numbering after it.

### Persistence

- **Housing tables** follow EQEmu's repository pattern: generated base repositories (`utils/scripts/generators/repository-generator.pl`, the same generator as the 250 existing ones) plus thin extended repositories for queries the module needs.
- **The database is the source of truth.** Zones load what the players present need, when they need it (at zone-in, and on request), and write every change through before replying.
- Schema design (tables and fields for definitions, cost elements, properties, balances, placed items, access lists, neighborhoods, regions, polls, trophies) is part of chunk 3, where each table's shape comes straight from its messages.

## Chunk 1: decisions

**D1. Where the housing tail's item values live. Decided: rename `items` columns in place and fix their data.**

The PEQ-derived `items` table already holds the housing tail, captured from Live in unlabeled columns: **`UNK224`-`UNK241` (with `placeable` at 235) are the 18 captured housing-tail fields, in the client's memory order** (proved field by field on rows captured aligned; the values match the client's semantics: bitmask 8 on houses, 128 on guild halls; house definition ids 5-34, guild halls 38-41, the crate 31; upkeep ids 17-24). Most rows were captured **shifted by one byte**: on the wire `bInteractiveObject` follows the housepet string, and that capture didn't know it, so from the trophy id on each column holds the next field's bytes one position early (and `UNK233`'s low byte is the interactive flag); a few rows were captured aligned. **`UNK223` is not part of this and is left untouched:** on the wire the client reads `Placeable` immediately after the `Heirloom` byte, so no tail field sits between them (the capture's field numbering isn't wire order), and it isn't a copy of EQEmu's `subtype` either (they differ on 2,710 of 117,944 items).

| Column | Becomes | Client field |
|---|---|---|
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
- **The migration** renames the 18 columns, converts the float and string ones to their types, and **re-slices the shifted rows** (detected per row) into the right columns; items without a placeable bitmask get the defaults (trophy id -1).
- **The decoder is validated** (`tools/itemTailDecode.py`) against all **5,577** items with a placeable bitmask: **every row has exactly one plausible layout** (5,205 shifted, 372 aligned; none ambiguous, none implausible both ways), and every decoded field is sane: trophy ids decode to exactly the trophy items (100 "Idol of Severilous", 111-120 the "Empowered Trophy of ..." series, 121 "Trophy of Expanding Life"; 88 in all, the rest -1); default headings and pitches fall on EverQuest's 512-unit scale (0, 128, 256, 384); respawn intervals 4500 (familiar NPC items), 750 (NPC merchants) or 0; default scales 0.25-5.0; scale ranges 0.05-25; disable free placement on 170 items (houses, halls), disable rotation on 455; **exactly one interactive object, the Guild Regeneration Pool**. **Shifted rule** (per 32-bit value, little-endian): `UNK233`'s low byte is the interactive flag; each field from the trophy id on is the high three bytes of its column plus the low byte of the next (`UNK233`/`UNK234` form the trophy id; `placeable` is the rotation flag; `UNK236`'s low byte is the free-placement flag). **One oddity kept as captured:** the Candy Dish (55446) has bitmask -1 (placeable anywhere).
- **Content found along the way:** Live's real-estate catalog ids: house definitions 5-34 (by style, size and interior), guild halls 38-41, the moving crate 31; upkeep cost ids 17-24 for houses by size.
- **Reaching the serializer:** the in-memory item (`ItemData`) still doesn't carry these fields, so the housing module loads them for items with a non-zero `placeable` and the RoF2 serializer asks it through the provider hook (T6); items without them serialize as today.

**D2. Cross-zone updates. Decided: first-class, fully implemented** (owner's decision). Zones and instances are separate processes; the client hears only from the zone it's in, so anything that changes in one process and matters to a player in another needs a server-to-server message.

**Delivery** follows EQEmu's existing cross-zone pattern (the quest system's `ServerOP_CZ*`): world forwards the op to every zone (`ZSList::Instance()->SendPacket`, `world/zoneserver.cpp`), and each zone filters locally for its targets. Housing adds **one server op, `ServerOP_Housing`**, carrying an envelope:

| Field | Meaning |
|---|---|
| event | which housing event (catalog below) |
| target kind, target id | character (character id), guild (guild id), property (the zones hosting it: its neighborhood's instance and its house interior's instance), zone instance, or all zones |
| origin zone, instance | the sender, for replies |
| property id, revision | the property the event is about, and its revision after the change |
| payload | event-specific data, when needed |

**Events say what changed, not the new data.** A receiving zone re-reads the database and sends its own clients the updates (records, lists, access lists, item lists, world objects). The database stays the single source of truth, and events can arrive in any order.

| Event | Target | Receiving zone does |
|---|---|---|
| Transfer offer | the recipient character | shows the offer (game dialog `0x89`); the answer (`0x5a63`) completes the transfer there |
| Transfer resolved | the offering character | tells the owner the result |
| Property changed (owner, balances, upkeep, name, children) | the property; the owner and previous owner | re-sends the records (`0x6fd0` / `0x4f09` / `0x63fd`) and the owners' property lists |
| Item list changed | the property | re-sends the changed entries and world objects |
| Access list changed | the property | re-sends `0x485d`; applies the new permissions to players present |
| Character property lists changed | the character | re-sends `0x2aa7` / `0x3e00` / `0x66df` |
| Personal trophy benefits changed | the owner | recomputes the available benefits (they come from trophies placed in the owner's plots) |
| Guild neighborhood created | the guild | `0x1d3c` |
| Polls changed | all zones | re-sends the affected poll messages to players with the vote windows' data |

**Consistency:** every change runs inside a database **transaction** (`TransactionBegin` / `Commit` / `Rollback`, `common/dbcore.h`), and each property carries a **revision** checked and incremented by every change (`UPDATE ... WHERE id = ? AND revision = ?`). When two zones change the same property at once, the second change fails cleanly, and that player gets a result code and the fresh state. Transfers use a **pending-offer** record with an expiry, so an offer survives the hop between zones and can't be accepted twice.

**Touch points added:** T11 `common/servertalk.h` (the op and the envelope), T12 `world/zoneserver.cpp` (one `case` in the forward-to-all-zones list), T13 `zone/worldserver.cpp` (one `case` handing the envelope to the housing module).

**D3. House interiors. Decided: one EQEmu instance per house.** An interior is an ordinary zone to the client (it needs the S1-S4 data there too). Each house's interior is an **instance** of its interior zone (`common/database_instances.cpp`), with the instance id kept on the property; entering routes the player into it. **World purges expired instances every 7.5 minutes** (`PurgeInstanceTimer`, `database.PurgeExpiredInstances()`), so house instances are created **without expiry** and removed only when the house is. Details in chunk 6, and Q11 (the interior-zone crash) in testing.

**D4. Upkeep. Decided: a scheduled tick in the world server, on the model of parcel pruning.** World's main loop runs every 32 ms, and its periodic jobs are `Timer` objects whose `Check()` is only a clock comparison (`world/main.cpp`): the instance purge every 450 s, and **`parcel_prune_timer` once a day**, which is gated by rules (`Parcel:EnablePruning`, `Parcel:ParcelPruneDelay`), tells the zones (`ServerOP_ParcelPrune` to every booted zone) and does its database work. The upkeep tick follows it exactly: a `Timer` in the loop calling a small world-side housing routine that charges only the properties that are due (an indexed next-charge time), inside transactions, and sends the housing "property changed" event for each change. Two refinements: **it also runs once at startup** (upkeep that came due while the server was down is caught up), and **its interval is a rule** (hourly by default, so charges land near their due time; a pass with nothing due is one indexed query). The charging rules themselves are chunk 5.

## Configuration: EQEmu rules

**Every housing behavior an operator might tune is an EQEmu rule**, in a new **`Housing`** rule category, the way parcels and merchants are configured (`common/ruletypes.h`: `RULE_CATEGORY(Parcel)` with `RULE_BOOL`, `RULE_INT` and `RULE_REAL`, each with a default and a description). **No SQL is needed for rules**: world injects every declared rule missing from `rule_values` at boot (`RuleManager::UpdateInjectedRules`, `world/world_boot.cpp:318`) and removes orphaned ones. Rule names follow EQEmu's PascalCase.

**Each chunk declares its subsystem's rules** with defaults and descriptions. Known now:

| Rule | Purpose |
|---|---|
| `Housing:Enabled` | master switch: when off, no housing data is sent and housing requests are refused with a result code |
| `Housing:EnableUpkeep`, `Housing:UpkeepTickSeconds` | the upkeep tick (D4) and its interval |
| `Housing:TransferOfferExpirySeconds` | how long a pending transfer offer lives (D2) |
| `Housing:GuildPlotMinimumMembers` | the server-only minimum guild size for guild plots (the client shows string 5426 but never checks it) |
| `Housing:EnablePolls`, `Housing:EnableTrophyTribute` | the poll and trophy subsystems |

**Touch points added:** T14 `common/ruletypes.h` (the `Housing` category), T15 `world/main.cpp` (the upkeep `Timer`, its startup run and one call, mirroring the parcel block); the upkeep routine itself is new code in world, and `world/CMakeLists.txt` lists it (T16).

## Non-standard housing

Three extensions the owner wants, all in scope: **private zones as house and guild-hall interiors** (a house door leading to your own Plane of Hate or Qeynos; the owner's priority), **custom neighborhood maps** (East Commons as a neighborhood), and **plots in the live world** (plots in the public East Commons).

**What the client allows** (verified):
- **Nothing in the client is tied to the housing zones.** No real-estate code reads `OP_NewZone`, and nothing checks for the housing zone ids (`robustness.md`): plots exist wherever the server sends regions (volumes placed anywhere by a matrix) and property records.
- **What an item may be placed in is decided by the property's type, not the zone.** The placement rule "allowed in this real estate" (`0x5b4d10`) requires the item's placeable bit **`1 << (definition type - 1)`**: Regular Zone `0x01`, Guild Hall `0x02`, Player House `0x04`, Plot `0x08` (yards), Neighborhood `0x10`, Town `0x20`, Guild Plot `0x80` (guild yards); only a Moving Crate (`0x7f61b0`: category 6) accepts any placeable item.
- **So non-standard housing reuses the standard property types.** The captured item data carries the Regular Zone bit on one item only (the Candy Dish), so a "Regular Zone" property would accept almost nothing; a private zone is a **Player House** or **Guild Hall** property (most furniture carries both bits: 14 and 142), and a plot anywhere is a **Plot** or **Guild Plot**.

**What EQEmu provides** (the schema): spawns, doors, objects, ground spawns, traps and zone points are keyed by **zone version** (`spawn2`, `doors`, `object`, `ground_spawns`, `traps`, `zone_points` all have `version`), `zone` has a row per version with its own `ruleset`, and instances carry a version and `never_expires` (`instance_list`). A private copy of a zone is **an instance of a housing version of that zone**, which has no spawns, doors or zone lines unless content adds them.

| Extension | Design | Additive or binary | Rules and tuning |
|---|---|---|---|
| **X1. Private zones as interiors** | Each house or guild-hall definition names its **interior zone and version**, generalizing D3 (standard houses name their `phinterior*` zones; a "Plane of Hate Estate" names `hateplane` at the housing version). The property covering the interior is a Player House or Guild Hall with **one region enclosing the zone**. Entering the house (`0x218c` / `0x050c` on the house) creates or reuses the never-expiring instance and zones the player in; `/leaverealestate` (`0x1da9`) returns them | **Additive**: pure content beside the standard interiors | item caps per definition (content); the housing zone version number (a rule) |
| **X2. Custom neighborhood maps** | Each neighborhood names its **template zone and version** (standard: 712; custom: East Commons at the housing version); plots are authored region boxes and plot definitions on that map | **Additive** | per neighborhood (content) |
| **X3. Plots in the live world** | The same plots and regions, hosted in the **public zone** (instance 0) | **Additive, with tuning**: nothing else must be switched off, but public plots raise the stakes on upkeep and abandonment, griefing (yards are open ground; only house interiors enforce Enter), NPC pathing (mobs walk through placed objects; the server's path maps don't include them), and zone-in traffic (every visitor receives the zone's placed objects) | `Housing:EnableWorldPlots`; **per-zone tuning through the zone's `ruleset`** (stricter upkeep or smaller plots in public zones) |

**Architecture consequences, adopted now** (cheap now, expensive later):
1. **A property's host is data**: `(zone id, instance id)`, never an assumed neighborhood zone; cross-zone "property" targets (D2) route by it.
2. **Definitions carry zones**: a house or guild hall's interior zone and version (X1); a neighborhood's template zone and version (X2).
3. **Plot authoring is a first-class tool**: an in-game GM command that defines a plot's region box (position, size, heading) and its definition on any map, used by all three extensions and by standard neighborhoods alike.
4. **Housing rules are read in the zone's context**, so a zone's `ruleset` can tune housing per zone (X3).

**To verify:** that placed objects render outside the housing zones (house exteriors and furniture use global models, but it needs a test; the diagnostics plugin shows it directly: a world object whose actor stays 0 has no body); plot authoring accuracy against the client's own containment (the plugin recomputes it); zone-in traffic in a busy public zone (X3).

## Chunk 2: the item tail (F1)

**Wire layout** (client reader `0x7f6bf0`; `windows/RealEstateItemsWnd.md`): after `Heirloom`, the 18 housing fields in wire order with a **NUL-terminated housepet string** and **`bInteractiveObject` right after it**, a u8 at `+0x609`, then the reader's last 9 bytes (`+0x110`-`+0x118`); what follows belongs to the caller.

**Today** `SerializeItem` (`common/patches/rof2.cpp:6850-6892`) fills `ItemQuaternaryBodyStruct` and writes it as one block; the client reads its `unknown28` / `unknown29` as a 3-character housepet name `"\xff\xff\xff"`, which absorbs the struct's 6 extra bytes, and `unknown30` (-1) as the trophy id.

**The change** (T6), for every packet type except `ItemPacketInvalid`:
1. write the struct's bytes up to and including `Heirloom` (`offsetof(ItemQuaternaryBodyStruct, Placeable)` bytes), unchanged;
2. write the 18 tail fields in wire order from the item's housing tail (the housepet name as a C string, at most 31 characters), then the `+0x609` byte (0);
3. write the struct's **last 14 bytes unchanged** (from `unknown37`'s second byte to the end: `+0x110`-`+0x118` for this reader, then 5 bytes the caller reads), so everything after the tail stays byte-identical.

**`ItemPacketInvalid` keeps today's bytes** (it sets `unknown29` to `0xff`, which changes how the client reads the name); nothing housing sends uses it.

**The housing tail's values** come from a provider the serializer calls with the item id: a function registered by the zone's housing module (the only item-side hook), returning the renamed `items` columns for items with a non-zero `placeable`, and for every other item the values the client reads today apart from the name: all zero, an empty name, trophy id -1. `common/` holds only the provider's declaration (a struct of the 18 fields and a setter), so world, which never registers one, keeps sending defaults.
