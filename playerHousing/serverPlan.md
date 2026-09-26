# Server Implementation Plan

**Phase 2** of the server work (phase 1: `serverWork.md`): each foundation and subsystem planned against Peridot's EQEmu source (`PhoenixCampfire/peridot`, upstream `ab45d4358`, v23.8.1). Written in chunks; nothing here is code yet. The client research remains the specification.

**Principle: surgical.** Housing lives in its own module; existing EQEmu code is touched only at the points listed below, each for a stated reason. New tables, a few altered fields and data rows are expected.

**Scope:** the standard housing system (`serverWork.md`) **and all three non-standard extensions** (X1 private zones as interiors, X2 custom neighborhood maps, X3 plots in the live world; section below), designed together.

**Development model:** everything is designed first, then written in full, and the owner builds once. **Database work is tested for real** before delivery: migrations and queries run in a sandbox MariaDB created from the server's real schema (`pok-schema-all.json`), including the `items` rename and decode against the exported rows. **Code is verified by careful reading** against Peridot's source and the research specifications; EQEmu isn't built in the sandbox. After the owner's build, validation happens in game with the diagnostics plugin and the Lua harness.

**Status:** chunks 1-6 complete (the item tail's migration, the schema and the region conversion are tested). Remaining chunks, each covering the standard system and X1-X3 together:

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
| T6 | `common/patches/rof2.cpp` (+ `rof2.h`) | **the item tail fix (F1)**, **one exported wrapper around the internal RoF2 item serializer**, `OP_ClickObjectAction`'s encoder copying the container ids, `OP_FindPersonRequest`'s decoder keeping the type and ids | F1 is a bug fix in the existing encoder; item-list entries embed a fully serialized item |
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
| T17 | `zone/command.cpp`, `zone/command.h` | one `#realestate` GM command registration | plot and catalog authoring |
| T18 | `zone/object.h`, `zone/object.cpp` | a transient flag (`Save` / `VarSave` skip the database); the click builder sets the new id fields to -1 | housing tradeskill containers reuse EQEmu's world containers |
| T19 | `common/eq_packet_structs.h` | property and record ids appended to `ClickObjectAction_Struct`; target type, property and record ids appended to `FindPersonRequest_Struct` | the RoF2 encoder / decoder (T6) carry them |
| T20 | `zone/client_packet.cpp` (`Handle_OP_FindPersonRequest`) | types 3 and 4 go to housing | Locate and plot finding |

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
- **The migration** (`sql/itemHousingTail.sql`, generated by `tools/makeItemHousingTailSql.py`) keeps the raw capture in **`items_housing_capture`**, decodes from it (aligned or shifted, detected per row), retypes the 18 columns in place and renames them in one atomic `ALTER`, so every column keeps its position. **Non-placeable items** get max per real estate -1, trophy id -1 and zeros: the full table has only three non-placeable patterns (83,732 never captured, 23,977 shifted, 4,658 aligned), and the captured ones already decode to exactly those values. **Guards** (named check constraints) stop it before any change if a placeable row has no single plausible layout, a housepet capture isn't empty, or a non-placeable row is outside those patterns. It runs while `UNK224` exists (check `SHOW COLUMNS FROM items LIKE 'UNK224'`, condition `not_empty`), and every statement before the final `ALTER` can be repeated.
- **Tested** (`tools/testItemHousingTail.py`, MariaDB in strict mode, the real `items` definition and the exported rows): every value matches the decoder bit for bit, the column order is unchanged, a re-run after stopping mid-way gives the same result, and each guard stops it cleanly.
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

## Chunk 2: the opcode map (F2)

Every housing message, **76 opcode values** (the register's sections 2 and 3, `serverWork.md` and the robustness review's handler list, reconciled). Checked against Peridot's `patch_RoF2.conf`: **only two are mapped today, both reused as they are**, `OP_ClickObjectAction` (`0x0c1e`, a housing container closed) and `OP_FindPersonRequest` (`0x5cea`, Locate and plot finding); `OP_TradeSkillCombine` and `OP_GuildUpdate` (rank permissions, action 5) are reused by name. **The other 74 are new**: one `N(OP_...)` each in `common/emu_oplist.h` (T1) and one line each in `utils/patches/patch_RoF2.conf` (T2), none clashing with an existing name. Each maps to the forwarding handler `Client::Handle_OP_Housing` (T3) for its client direction; server-direction opcodes are only sent.

**Not housing, excluded:** `0x0ecb` (the respawn window), `0x65c7` and `0xcadf` (movement reports), `0x4936` (an item sync), `0x2c4e`, and `0x796c` (Plot Search's alternate branch, unreachable in this build). **`0x89` is not an opcode:** it's the game dialog the transfer offer is shown in; the offer and its answer travel in `0x5a63`.

| Value | Direction | EQEmu name | Role | Subsystem |
|---|---|---|---|---|
| `0x2698` | S→C | `OP_RealEstateDefinitions` | definitions, then cost elements with options and items | S1 |
| `0x6fd0` | S→C | `OP_RealEstateCurrentProperty` | a property record that also sets the current property | S2 |
| `0x4f09` | S→C | `OP_RealEstateProperty` | a property record: add or refresh | S2 |
| `0x63fd` | S→C | `OP_RealEstatePropertyUpdate` | a property record merged into a known property | S2 |
| `0x2aa7` | S→C | `OP_RealEstateOwnedProperties` | the character's owned properties (list A) | S2 |
| `0x3e00` | S→C | `OP_RealEstateItemOwnedProperties` | properties owned through items (list B) | S2 |
| `0x66df` | S→C | `OP_RealEstateArchivedProperties` | archived properties (list C) | S2 |
| `0x76c6` | S→C | `OP_RealEstateOwnedPropertyChange` | add or remove one id in list A | S2 |
| `0x0c13` | S→C | `OP_RealEstateItemOwnedPropertyChange` | add or remove one id in list B | S2 |
| `0x1228` | S→C | `OP_RealEstateItemResult` | item request result code | S2, S5 |
| `0x66bf` | S→C | `OP_RealEstateSellResult` | sell plot result code | S7 |
| `0x68ba` | S→C | `OP_RealEstateTransferResult` | transfer plot result code | S7 |
| `0x49df` | S→C | `OP_RealEstateRegions` | region volumes (id and matrix); clears the permission map | S3 |
| `0x1cef` | S→C | `OP_RealEstateItemList` | a property's whole item list | S4 |
| `0x087f` | S→C | `OP_RealEstateItemsUpdate` | add or update several entries | S4 |
| `0x2cf7` | S→C | `OP_RealEstateItemUpdate` | add or update one entry | S4 |
| `0x1097` | S→C | `OP_RealEstateItemReplace` | replace one entry | S4 |
| `0x1baf` | S→C | `OP_RealEstateItemRemove` | remove one entry (cancels its placement) | S4 |
| `0x1967` | S→C | `OP_RealEstateItemsRemoveByOwner` | remove every entry of an owner | S4 |
| `0x7d28` | S→C | `OP_RealEstateItemListDrop` | drop a property's whole list | S4 |
| `0x626e` | both | `OP_RealEstatePlace` | client: place request; server: begin placement | S5 |
| `0x711a` | C→S | `OP_RealEstatePlaceCommit` | commit a placement | S5 |
| `0x2036` | C→S | `OP_RealEstateMoveToInventory` | move an entry to inventory | S5 |
| `0x3424` | C→S | `OP_RealEstateMoveToStorage` | move an entry to a closet, shed or crate | S5 |
| `0x1045` | S→C | `OP_RealEstateWorldObject` | a placed item's world object | S5 |
| `0x2a9e` | S→C | `OP_RealEstateWorldObjectRemove` | remove a placed item's world object (by property and record id) | S5 |
| `0x2523` | S→C | `OP_RealEstateWorldObjectMoved` | the item being placed moved (its storage location and ids) | S5 |
| `0x21ea` | S→C | `OP_RealEstateNpcItemBinding` | binds an NPC item's entry to its spawn | S5 |
| `0x21ec` | both | `OP_RealEstateInteractionMenu` | client: menu request; server: the menu | S5 |
| `0x218c` | C→S | `OP_RealEstateInteract` | interact (menu type 1); record id -1 enters a neighborhood | S5, S6 |
| `0x050c` | C→S | `OP_RealEstateEnter` | enter (menu type 8) | S5, S10 |
| `0x51e3` | C→S | `OP_RealEstateMenuType9` | menu entry type 9 | S5 |
| `0x64a3` | C→S | `OP_RealEstateMenuType19` | menu entry type 0x13 | S5 |
| `0x7ad1` | C→S | `OP_RealEstateMenuType5` | menu entry type 5 | S5 |
| `0x3c8b` | C→S | `OP_RealEstateMenuText` | a menu entry's text answer | S5 |
| `0x58c7` | both | `OP_RealEstateInspect` | inspect a placed item, and the reply | S5 |
| `0x06c8` | C→S | `OP_RealEstateContainerOpen` | a housing tradeskill container was opened | S5 |
| `0x2691` | both | `OP_RealEstateSearch` | neighborhood and plot searches; replies by mode | S6 |
| `0x1d3c` | S→C | `OP_RealEstateGuildNeighborhoodCreated` | a new neighborhood for your guild | S6 |
| `0x45be` | both | `OP_RealEstatePurchase` | purchase, and its success byte | S7 |
| `0x792c` | C→S | `OP_RealEstateSell` | sell | S7 |
| `0x507f` | C→S | `OP_RealEstateTransfer` | transfer | S7 |
| `0x5a63` | both | `OP_RealEstateTransferOffer` | server: the offer; client: the answer | S7 |
| `0x2aa6` | both | `OP_RealEstateAccount` | deposit, withdraw, set the upkeep option, and the replies | S8 |
| `0x3146` | C→S | `OP_RealEstateSetAccessList` | set a property's access list | S9 |
| `0x485d` | S→C | `OP_RealEstateAccessList` | a property's access list | S9 |
| `0x3968` | S→C | `OP_RealEstateAccessListRemove` | remove a property's access list | S9 |
| `0x21ba` | C→S | `OP_RealEstateHouseSwap` | swap houses | S10 |
| `0x6917` | C→S | `OP_RealEstateStoreAll` | store everything | S10 |
| `0x613d` | C→S | `OP_RealEstatePack` | pack or unpack | S10 |
| `0x5b41` | C→S | `OP_RealEstateLoadLayout` | load a layout | S10 |
| `0x1da9` | C→S | `OP_RealEstateLeave` | leave real estate (/leaverealestate) | S10 |
| `0x7bb1` | both | `OP_RealEstateAnchorTeleport` | server: the offer; client: accept | S10 |
| `0x15a8` | C→S | `OP_PollVote` | vote | S11 |
| `0x499a` | C→S | `OP_PollTeleport` | teleport to a ranked property | S11 |
| `0x6573` | C→S | `OP_PollPropertyVote` | the Purchase window's Vote button | S11 |
| `0x15e2` | S→C | `OP_PollList` | replace the poll list | S11 |
| `0x39bb` | S→C | `OP_PollUpdate` | update polls by id | S11 |
| `0x3311` | S→C | `OP_PollResults` | results | S11 |
| `0x6860` | S→C | `OP_PollLabels` | option labels | S11 |
| `0x7497` | S→C | `OP_PollAccess` | may vote, may see | S11 |
| `0x52b5` | S→C | `OP_PollVotes` | your votes | S11 |
| `0x593a` | S→C | `OP_PollAvailable` | polls available | S11 |
| `0x7a4d` | S→C | `OP_PollOpen` | open the vote window on a poll | S11 |
| `0x51a9` | C→S | `OP_TrophyWindowOpened` | the trophy window opened | S12 |
| `0x4b65` | both | `OP_TrophyPersonalSlot` | client: upgrade or downgrade; server: set one slot | S12 |
| `0x7697` | S→C | `OP_TrophyPersonalSlots` | set all 10 personal slots and the active flag | S12 |
| `0x08bf` | S→C | `OP_TrophyPersonalClear` | clear all 10 personal slots | S12 |
| `0x51ae` | S→C | `OP_TrophyPersonalTimer` | the personal trophy timer | S12 |
| `0x7dd2` | S→C | `OP_TrophyGuildSlots` | the guild's 4 active slots, favor and active flag | S12 |
| `0x7db5` | S→C | `OP_TrophyGuildAvailable` | the guild's available benefits | S12 |
| `0x7a41` | both | `OP_TrophyGuildEditLock` | the guild edit lock | S12 |
| `0x7c1d` | C→S | `OP_TrophyGuildSave` | save the guild set | S12 |
| `0x02bc` | C→S | `OP_TrophyGuildOptIn` | the guild opt-in checkbox | S12 |

## Chunk 3: the schema

**`sql/housingSchema.sql`**: 20 tables, EQEmu's conventions (`snake_case`, `id int(10) unsigned`, `character_id` / `guild_id` / `zone_id` + `instance_id`, `datetime` for times). **Every column exists for a message or rule; anything derivable is derived when sent:** owner names (from `character_data`), children (from `parent_id`), the owner's fellowship, the character's three property lists, neighborhood plot counts, poll states (from dates and flags), poll results (from votes), region matrices (from the box), trophy flags (below). **Tested** (`tools/testHousingSchema.py`, MariaDB, the real `inventory` definition): created twice, a neighborhood → plot → house hierarchy with a crate carried as an item, and the module's queries.

| Table | Kind | Holds | Wire |
|---|---|---|---|
| `real_estate_definitions` (+ `_children`) | content | every definition field (the five unknown ones as `unknown_*`, default 336 kept), a name for content work, **`instance_zone_id` / `_version`**: the zone instanced for properties of this definition (a house or hall interior, a neighborhood's template, a private zone for X1) | `0x2698` |
| `real_estate_costs`, `_cost_options`, `_cost_items` | content | cost elements, their options in order, and the items per option: currency, sub-key, **signed** amount (negative = reduction) | `0x2698` |
| `real_estate_regions` | content | a box: center, size, heading; the client's matrix is computed from it (chunk 4) | `0x49df` |
| `real_estate_properties` | state | the hierarchy, definition, guild, owner, name, **host** (zone + instance; D2 routing, X2, X3), **interior instance** (D3, X1), **the item guid that contains it** (list B), upkeep option and state, **next upkeep** (indexed, D4), voting opt-out, archived (list C), **revision** (D2) | `0x6fd0` / `0x4f09` / `0x63fd`, lists |
| `real_estate_property_balances` | state | per currency, exact integers (sent as f64) | the record's balance map |
| `real_estate_items` | state | placed and stored items: property, owner, placed flag, transform, and **the item instance exactly as `inventory` keeps it** (charges, color, augments, custom data, ornaments, guid); the record id is the row id | `0x1cef` and updates |
| `real_estate_access_groups`, `_access_names` | state | 7 group masks per property; names with their group | `0x485d`, `0x3146` |
| `real_estate_transfer_offers` | state | one pending offer per property, with its expiry (D2) | `0x5a63` |
| `real_estate_polls`, `_poll_options`, `_poll_labels`, `_poll_votes` | state (labels: content) | polls (published and archived flags; the state is derived), options in order, the global label table, votes | `0x15e2` ... `0x7a4d` |
| `character_trophy_tribute` (+ `_state`), `guild_trophy_tribute` (+ `_state`) | state | 10 personal / 4 guild slots; active flag and timer; the guild opt-in; **the guild edit lock** (who, when; global under D2) | `0x7697` ... `0x02bc` |

**Constraints for what the client can't check:** definition type 1-8, item caps (non-real-estate and pet items within max items), access groups 0-6, personal slots 0-9, guild slots 0-3, poll kinds, selection types and dates, region volume, one pending offer per property.

**Migrations** (custom track, T7): version 1, the item tail (chunk 2); version 2, this schema. **Repositories:** generated base repositories for every table, like EQEmu's own.

**Findings from the full-table queries** (they shape later chunks):
- **The trophy benefits aren't in `tributes`**: it holds personal tribute 0-59 and guild tribute 60-82, none of the ids trophy items name (100, 111-121, 201-206, ...). They're content to add (`tributes`, `tribute_levels`). EQEmu's `tributes.unknown` is the tier count, not the client's trophy flag; the flag is **derived**: a tribute is a trophy benefit if a placeable item names it, personal or guild by the item's placeable bits.
- **Only zone 712 (Sunrise Hills) exists in `zone`**: the house interiors and guild halls need zone rows (chunk 7).
- **House definitions are keyed by size and interior, six exterior styles each** (House 5, 7, 8; Hut 9-11; Residence 12-14; Domicile 15-17; the Cobbler and Behroe sets 18-29; Manor 32-34), so **the definition decides the interior zone**, as X1 needs; guild halls 38-41; the Crate of Eviction 31; **eleven storage crates 174-184**.
- **`guild_permissions`** holds a value per permission (24, 25 and 29: 224; 27 and 28: 128, one guild): its rank encoding is settled with purchase in chunk 6.
- **The character's property lists are named by MacroQuest** (`OwnedRealEstates`, `OwnedItemRealEstates`, `ArchivedRealEstates`): archived properties are a subset of the owned ones, and list B is properties contained in items the character carries (here, by item guid).

## Chunk 4: catalog, properties, regions, item lists, zone-in, plot authoring

**Wire primitives** (every housing stream): little-endian integers; **`string` = `u32` length + raw bytes** (records, entries, the catalog); the exceptions are `0x1967` (a raw NUL-terminated name) and the item tail's housepet name (chunk 2).

**S1, the catalog (`OP_RealEstateDefinitions`, `0x2698`):** every definition in wire order (id, region id, purchase and upkeep cost ids, the three caps' first two, `unknown_30`/`_34`/`_38`, type, category, `unknown_string`, `unknown_44`, max pet items, layout origin, layout heading, child count and ids from `real_estate_definition_children`), then every cost element (options in `position` order, each with its items). The catalog is small, so **the whole catalog is sent** at zone-in and again after a GM change.

**S2, properties:** a record per property (`0x4f09` to add or refresh, `0x63fd` for updates the client compares, `0x6fd0` to set the current property): id, 0 (`+0x18`), parent, definition, guild (-1 for none), 0 (fellowship), an empty string (`+0x30`, never read), **the owner's name** (from `character_data`), 0 (`+0x34`), the children's count, **the property name**, the children's ids (from `parent_id`), the balances (`u8` count, then currency and amount as f64), **the upkeep option**, the upkeep-current flag, the voting opt-out. The character's lists: A = owned, C = owned and archived, B = contained in an item the character carries (`contained_in_item_guid` = an inventory row's `guid`).

**S3, regions (`OP_RealEstateRegions`, `0x49df`):** `u32` count, then per region its id, the 16 floats (row 0 first) and five zero `u32`s. **The matrix comes from the region's box** (`tools/regionMatrix.py`, tested on 40,000 points): the client tests the player's `LastCollision` **in the order Y, X, Z**, through the inverted matrix, against [-1, 1]; in that order the rows are the across axis `half_x * (-sin, cos, 0)`, the along axis `half_y * (cos, sin, 0)`, `(0, 0, half_z)` and the center `(y, x, z)`, with the heading in EQ units (512 per turn; heading 0 faces +y, EQEmu's own convention). **A region belongs to a property through its definition, in the context of the current property**, so every neighborhood instance shares its plot definitions and regions, the neighborhood property being each instance's context. **Interiors need no regions:** with none, "standing in" falls back to the current property, so a house (or an X1 private zone) sent as the current property takes every placement inside it.

**S4, item lists (`0x1cef`, updates `0x087f` / `0x2cf7` / `0x1097`, removals `0x1baf` / `0x1967`, drop `0x7d28`):** each entry: property id, record id (the row id), placed flag, 0, the transform (heading, pitch, roll, scale, x, y, z), the owner's name, an empty string, then **the item serialized exactly as `OP_ItemPacket` carries it after its packet type**, through the exported RoF2 serializer (T6) from the row's item instance.

**The zone-in sequence (F5, from `Client::CompleteConnect`, T4)**, in dependency order:
1. the catalog;
2. property records for **the properties hosted here** (this zone and instance), **the character's properties** (lists A and B) and, for each, **its ancestors and descendants** (the context lookup and the personal trophy walk need the trees);
3. the current property: the host's top property (a neighborhood or town) where properties are hosted; **the house or hall inside its interior instance**;
4. the character's lists A, B and C;
5. **the regions** of the hosted plots (an empty `0x49df` elsewhere, which also clears any left over);
6. item lists for the hosted properties and the character's properties (personal trophies are computed from the owned plots' items wherever the character is);
7. access lists for the same properties;
8. **world objects** (`0x1045`) for the placed items of the properties hosted here, **NPC items** as spawns with their bindings (`0x21ea`);
9. trophy slots and timers, polls (chunk 7).

**To verify in game:** whether the client keeps its real-estate state across zoning (the probe shows it: the manager's current property and records after zoning out of a neighborhood); if it does, leaving a housing zone must clear it (an empty item list and region set, and records for the properties no longer relevant).

**Plot authoring (T17: one registration in `zone/command.cpp` / `command.h`; the implementation is in the housing module):** a `#realestate` GM command for the spatial content the other tables need:
- `region create | move | show`: a box at the GM's position and heading (sizes as arguments, its bottom at the GM's feet); `show` sends the zone's regions and reports which contain the GM, by the same math as the client;
- `definition` and `cost` editing for content without SQL;
- `neighborhood create`: a neighborhood property and its instance of the definition's template zone (X2), with **its plots created from the definition's children**; `town create` for plots hosted in a public zone (X3);
- `property info`: the properties and regions at the GM's position; `reload`: re-send the catalog and state to the zone.

Its status level is an account status like EQEmu's other content commands; `Housing:Enabled` gates it with the rest.

## Chunk 5: placement, editing and the world layer (S5)

**A placement session** (per client, in memory; dropped on zone-out and disconnect, T5): new or edit, the source (an inventory location, or a property and record), the item's GUID, the context property, and the request latch the client keeps until the result arrives.

**Requests and the rules the server replays** (the client doesn't re-check a commit):
- **`0x626e` place request** (60 bytes: the common header with the GUID, property and record ids, the item location and the click's screen X / Y): pick the rule set by source: **from inventory, the client's 13 rules in order** (House of Thule `0x2f`; Veil of Alaris under a guild plot `0x52`; membership `0x53`; plot upkeep current `0x18`; `4`; placeable `0x24`; **destination inside the current context** `0x1e`; **allowed in this real estate** `0x11`: the item carries the bit `1 << (type - 1)`, any item for a crate; house and guild-hall counts per yard `0x0f` / `0x44`; moving-crate rules `0x27` / `0x51` / `0x20`; **capacity** `0x0a` / `0x0c` / `0x26` from the definition's caps, **waived by a stack merge**; empty containers `0x13`; item and house upkeep `0x19`; **permissions** `0x4b` / `0x4d` / `0x47`), **from storage** the stored-item set, **between properties** the move set; then **the trophy rule** (a trophy under the player's guild plot needs rank permission 29, `0x46`). A refusal is `0x1228` with the code.
- **The reply, `0x626e` begin placement:** the placement object (below), the location, A = 1 when editing a placed item, B = 1 (preview), C = 0, D / E = the request's click position, F = the NPC item's spawn id when editing one, then **the permission map: a code per region in the context and the extra final pair `{current property id, code}`**, each computed by the same rules for this player and item, **0 where placing is allowed**. The client's green / red preview then matches the server's judgment.
- **`0x711a` commit:** the rules again, for the property the item lands in: the region containing **the item's position**, otherwise the current property (the server has no model data, so it checks the position, not the client's box of 8 corners), plus the scale within the item's range. Then, in a transaction with the property's revision: a **new** placement moves the item instance from inventory into `real_estate_items` (guid, charges, augments and all) or **merges into a stack** (same owner, same item, room; rule 10b); an **edit** updates the transform. Then `0x1228`, the entry (`0x2cf7`), the world object (`0x1045`) to everyone in the host zone and instance, the NPC spawn for an NPC item, and the item-list event (D2).
- **`0x2036` pick up** (removal permission, inventory space) and **`0x3424` store** (destination 2 closet, 3 shed, 6 crate, resolved to the property's own storage): a transaction, then `0x1baf` (removed) or the updated entry, and **`0x2a9e`** (the world object removed).

**World objects (`0x1045` add or update, `0x2a9e` remove, `0x2523` the item being placed moved)**, the placement object's fields from their sources: **model name** = the item's model (its `idfile`), or for an NPC item the NPC's race tag; NPC flag and size; 0 (`+0x0c`, unused); **the GUID** (the instance's `guid`, 16 hex digits); property and record ids; **scale, heading, pitch, roll, position** (note: the entry's order is heading, pitch, roll, scale, position); the NPC flag again (`+0x98`) and the allowed sub-modes (from the free-placement and rotation flags); **surface type = `placement_type`** (1 = wall: the weapons and shields); **minimum, maximum and default scale**; heading offset = the default heading, the two wall tilts = the default pitch and roll (**inferred from the names; to verify with the probe**, which reads every world object's fields); **light type = `items.light`**. **World objects don't survive zoning**: zone-in sends them all again (chunk 4).

**NPC items** (vendors, bankers, familiars): **a server NPC** of the item's type (`real_estate_npc_items`: item id to NPC type, content) spawned at the transform and **bound to its entry with `0x21ea`** (the spawn's property id, record id and GUID); the menu carries its spawn id (`+0x2fc`); merchants and bankers then work through EQEmu's own paths. If one is killed it returns after the item's `npc_respawn_interval` (**its unit is to verify**: the data holds 4500 for familiars and 750 for merchants).

**Housing tradeskill containers** (a placed forge): opened by a menu entry or a world click, the server builds **a transient `Object`** holding the container item, hands it to the player (`SetTradeskillObject`), and sends **`OP_ClickObjectAction` with the property and record ids**; the client confirms with `0x06c8`; **combines run through EQEmu's own code** unchanged (`Object::HandleCombine`); closing returns anything left inside to the player (EQEmu's `Object::Close`), so contents never persist. Two changes keep it safe: **T18** (`zone/object.h` / `.cpp`): a transient flag that makes `Save()` and `VarSave()` skip the database (otherwise the first item taken out would **insert a permanent ground object**, `Object::Save` with no id), and the click builder setting the new id fields to -1 for ordinary objects; **T19** (`common/eq_packet_structs.h`): the two id fields appended to `ClickObjectAction_Struct`, copied by the RoF2 encoder (T6), which hardcodes -1 today.

**The interaction menu (`0x21ec`)**: entries `{type, b, label, submenu}`; labels are **`dbstr_us.txt` category 28** ids (132 interaction actions in the client's own file). **Types fix the client action**: 2 edit, 3 store, 4 pick up, 7 show the item, `0xc` / `0xd` an NPC item's merchant, `0xe` text entry (answered by `0x3c8b`), `0x11` swap houses (`0x21ba`); **1, 8, 9 and `0x13` send `{property, record, b}`**, so `b` is the server's action code. **Menus are built in code per item kind**, since each action is code: every item (for players allowed to): edit, store, pick up, show; a house: **enter** (to its interior instance: D3, X1) and swap; a container: open; a vendor or banker: its merchant entry; a door item: open or close. **The label ids are content**: chosen from the client's category 28 list.

**Inspect (`0x58c7`)** replies with the entry's item; **Locate (`OP_FindPersonRequest` type 3) and plot finding (type 4)** reply with a path to the item or plot. **T20:** EQEmu's RoF2 decoder keeps only `npc_id` and the position of this 40-byte request, dropping the type, property id and record id (the RoF2 struct's `unknown00`, `unknown08`, `unknown12`): the three fields are appended to `FindPersonRequest_Struct`, copied by the decoder (T6), and `Handle_OP_FindPersonRequest` sends types 3 and 4 to housing.

**Content this needs:** the NPC type of each NPC item; NPC items' model names (the capture left every `housepet_file_name` empty); the menu label ids.

**To verify in game:** the inferred world-object fields; the respawn interval's unit; that the server's position check agrees with the client's box check at plot edges.

## Chunk 6: neighborhoods and search, purchase / sell / transfer, escrow and upkeep, access (S6-S9)

### Permissions: one evaluator for every check

**The server's check is the client's evaluator (`0x4ff570`)**, so the client's disabled buttons and the server's refusals always agree: House of Thule for every real-estate permission (Veil of Alaris and membership feature 10 under a guild plot); **the owner of a player plot passes, and so does the guild's rank 1 (Leader) on its guild plot**; anyone else gets a group from the property's access list (a house or crate uses its plot's): their name's entry, then `_Fellowship` if their fellowship is the owner's, then `_Guild<rank>` if they're in the owning guild; **the group's mask decides** (the second u32 of each pair). EQEmu has no fellowships, so the owner's fellowship is sent as 0 and `_Fellowship` entries never match.

| Permission | Enforced at |
|---|---|
| 1 Enter | entering a house or hall interior (a menu entry, a door); an expired house refuses with `0x1920` |
| 2 Withdraw Upkeep | `0x2aa6` types 1 and 7 (the client checks nothing) |
| 3-6, 8-11 remove / add crate and house, remove, add, move, interact with items | chunk 5's handlers, with the client's codes |
| 12 Manage | `0x3146` and the Manage window's actions |
| 0 Receive Buffs, 7 Mail Item | no housing buff source or item mail exists on this server; the bits are stored and sent, not enforced |

**Guild rank permissions are EQEmu's own:** `CheckPermission` reads bit `8 - rank` of the permission's value (rank 1, Leader, is 128), and EQEmu names the housing actions (`GUILD_ACTION_REAL_ESTATE_GUILD_PLOT_BUY` 27, `_SELL` 28, `_MODIFY_TROPHIES` 29). Peridot's current values: guild plot buy and sell 128 (Leader only); trophy permissions 24, 25 and 29 at 224 (ranks 1-3).

### S6, neighborhoods and search (`0x2691`)

Requests (224 bytes): **type 0** name search (neighborhood, player or guild name; "My Plots" is your own name); **1** My Items (plots holding items you own, from `real_estate_items.owner_character_id` up to their plot); **2** a neighborhood's details; **4** plot search by owner, **5** by your items (both scoped to the current neighborhood). Replies by kind and mode: **a plot or guild plot as one property** (with "open the Purchase window" when it's vacant), **the neighborhood list** (id, name, guild, player plots and used, guild plots and used: counted, chunk 3), **a neighborhood's plot lists**, **plot search rows** (property id, kind, the `/find` target, two columns). **Go** (`0x218c` with the neighborhood's id and record -1) moves the player into the neighborhood's instance at its zone version's safe point. **A new guild neighborhood** is announced to the guild (`0x1d3c`, a cross-zone guild event). Guild names come from the client's cache, which EQEmu fills at login; **which message answers a cache miss is to verify**.

### S7, purchase, sell and transfer

- **Purchase (`0x45be`)**: a vacant Plot or Guild Plot; the price and upkeep option keys within their cost elements (**option keys are indexes**, below); funds in every currency of the price; the client's gate (House of Thule; for a guild plot Veil of Alaris, membership feature 10, `GUILD_ACTION_REAL_ESTATE_GUILD_PLOT_BUY`, `Housing:GuildPlotMinimumMembers`); `Housing:MaxPlotsPerCharacter`. In a transaction: the currencies taken, owner or guild set, the upkeep option, the first charge scheduled; then `0x45be` success (the client opens Manage), the record, list A, and the events.
- **Sell (`0x792c`)**: the owner, or `_SELL` for a guild plot, and the client's sell gate (`0x66bf` codes). **Everything on the plot goes into a new moving crate** (definition 31, the Crate of Eviction, item 52147) **carried as an item by the seller** (list B, through its guid; refused if the seller has no room for it); the escrow balance returns to the seller; the plot is vacated.
- **Transfer (`0x507f`, the target's name)**: a pending offer (D2) sent to the target wherever they are (`0x5a63`, the 188-byte struct with the property id and the owner's name); the answer completes it in the target's zone, checked against the offer and the property's revision; the owner gets `0x68ba`. **An offer nobody answers expires** (`Housing:TransferOfferExpirySeconds`), cleared by the world tick, which then tells the owner.

### S8, escrow and upkeep (`0x2aa6`)

- **Types:** 0 deposit from the cursor (a failed deposit is refunded: type 0, flag 0); 1 withdraw an amount and 7 withdraw all (permission 2; coin pays out as type 1, flag 1); 2 set the upkeep option (reply type 2); **3 / 4 add a day / week for all items, 5 / 6 for one item** (argument: its upkeep cost id). The client keeps no per-item paid time and computes time left from the balances, so **types 3-6 deposit the equivalent amount into escrow**: the same state, no per-item timers.
- **Option keys are indexes** (`0x7fb970` indexes the options by the key it's given): each cost's keys are 0, 1, 2, ... in order. **A placed item's upkeep is option 0 of its upkeep cost element; the plot's is the option its record selects** (`+0x3c`).
- **The world tick (D4)**, per property due: the daily cost per currency, net of reductions; **the plot's own upkeep first, then its houses, then the other items**; what escrow covers is charged; **an item escrow can't cover is warned about first** (`0x191f` to the owner, a cross-zone event) **and stored at the next charge** (entry update, `0x2a9e`; a player editing it gets `0x1baf`, which ends the edit cleanly); **a house or plot whose own upkeep isn't covered becomes expired** (`upkeep_current` 0: no placing, `0x18` / `0x19`; no entering, `0x1920`).

### S9, access (`0x3146`, `0x485d`, `0x3968`)

The client sends a property's whole list (every 3 s while it's being edited). The server checks **may edit** (GM, owner, or Manage) and **the client's grid rule** (a viewer only edits groups below their own), validates the names (existing characters, or the club keys), replaces the list in a transaction, sends `0x485d` to everyone who knows the property, and raises the access event; **a player inside an interior who loses Enter is moved out to the plot**.

### Decisions (recommended)

- **D6. Selling** returns the escrow balance, not the purchase price (`Housing:SellRefundPercent`, default 0).
- **D7. Upkeep failure** follows the client's strings (items warned, then stored; an expired house can't be entered); **a plot whose own upkeep stays expired for `Housing:AbandonDays`** is reclaimed like a sale: everything into a Crate of Eviction carried by the owner, the plot vacated.
- **D8. An empty name search** lists every neighborhood.
- **D9. The first upkeep charge** comes one day after purchase.
- **D10. Transfers need no presence check:** an unanswered offer expires and the owner is told.

**Rules added:** `Housing:MaxPlotsPerCharacter`, `Housing:SellRefundPercent`, `Housing:AbandonDays`.
