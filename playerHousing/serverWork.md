# Server Work Inventory

Every piece of server-side work the client research specifies, grouped into subsystems in dependency order. This is **phase 1** of the server plan; **phase 2** turns each item into concrete changes in Peridot's EQEmu source (`serverPlan.md`). The research documents each item cites are the specification; this page doesn't repeat their formats.

**Readiness** of each item:
- **Specified**: the client research fixes what the server must send and accept.
- **Design**: the client leaves a policy open (how much upkeep costs, what a search returns); it needs our decision, not more research.
- **Content**: data that has to be authored (which neighborhoods and plots exist, region volumes).
- **Verify**: can only be settled by a test or a Live comparison (`testingNotes.md`).

## Already satisfied by Peridot

Verified live with the diagnostics plugin (`PhoenixCampfire/mqperidotprobe`), 2026-09-25, on EQEmu 23.8.1:

| Requirement | State |
|---|---|
| House of Thule and Veil of Alaris expansion bits (critical finding 6) | sent: `0x0007ffff` |
| `profileType` 0 (trophy windows) | sent |
| Membership feature 10 (guild plots) | Gold, feature 10 = 1 |
| Guild rank permissions, `OP_GuildUpdate` action 5 (critical finding 17) | EQEmu sends them from `guild_permissions`; a Leader has 27 / 28 |
| The nine housing windows (critical finding 7: no show gate) | all present |

## Foundations

| # | Work | Readiness | Source |
|---|---|---|---|
| **F1** | **Item serialization: the housing tail.** Write every field of the RoF2 item definition's housing tail in the client's order: the "placeable in" bitmask, `RealEstateDefID`, scale range, **upkeep cost id**, `MaxPerRealEstate`, housepet file name, **trophy benefit id** (-1 for ordinary items), placement flags, defaults, interactive flag. EQEmu's RoF2 struct currently misaligns and zeroes it. Needs item data to carry the values | Specified | critical findings 11, 12, 19; `windows/RealEstateItemsWnd.md` (the field table) |
| **F2** | **Opcode map.** Every housing message (below) mapped in the RoF2 opcode configuration and the server's opcode list; none are named in EQEmu today | Specified | `COVERAGE.md` sections 2 and 3 |
| **F3** | **Housing zones.** The neighborhood (712), house interiors (714–720, 723, 739–750, 766) and guild halls (737, 738, 751, and the classic 345 for the Standard hall, in a version of its own holding only its front door) registered and loadable; 721 and 722 aren't client zones; the interior-zone crash (Q11) | Specified (ids); **Verify** (Q11) | `robustness.md` |
| **F4** | **Persistence.** Tables for definitions, cost elements, properties (records, balances, children), placed items, access lists, neighborhoods, regions, polls, trophies | Design (schema); the fields are Specified | this page's subsystems |
| **F5** | **Zone-in sequence.** Send in dependency order on every zone-in: definitions and costs, property records, the current property, regions and permissions, item lists, world objects (placed items don't survive zoning), NPC-item bindings | Specified | critical findings 3, 14; `flows/placeItem.md` |
| **F6** | **Data constraints.** Terminate every string inside its packet; guild ranks 0–8 and permission ids 0–31 only; access-list pairs `{group number, mask}`; the two transform orders; ids the client uses -1 for | Specified | `robustness.md`; critical findings 15, 18, 20 |

## Subsystems

### S1. Catalog: definitions and costs

- **Server → client:** `0x2698` (definitions, then cost elements with options and items).
- **State:** definitions (type, region id, cost ids, caps, layout origin and heading, child definitions); cost elements (options → items {currency, sub-key, amount}).
- **Server rules:** every definition a property references, and each definition's purchase and upkeep cost elements, must be sent before the property (the Purchase window stays empty or dark otherwise).
- **Depends on:** F2, F4. **Readiness:** Specified; the catalog itself is **Content**.
- **Source:** `opcode2698DefinitionsAndCosts.md`; critical finding 5.

### S2. Properties and context

- **Server → client:** property records `0x6fd0` (current property), `0x4f09`, `0x63fd`; the character's lists `0x2aa7` (owned), `0x3e00` (owned via items), `0x66df` (archived) and single-id updates `0x0c13`, `0x76c6`; result codes `0x1228`, `0x66bf`, `0x68ba`.
- **State:** properties (id, parent, definition, guild, owner and the owner's fellowship, names, upkeep option, upkeep current, voting opt-out, balances per currency, children).
- **Server rules:** send parent chains with their children; the client never marks a property as owned by itself (critical finding 8), so ownership comes only from these messages.
- **Depends on:** S1. **Readiness:** Specified.
- **Source:** `placedItemsAndProperties.md`; `resultCodes.md`.

### S3. Regions and placement permission

- **Server → client:** `0x49df` (region volumes: id and a 4x4 matrix each; clears the permission map); the permission pairs appended to the begin-placement `0x626e`.
- **State:** a region volume per definition (the definition's region id); a placement permission code per region.
- **Server rules:** the region containing the player decides where the client thinks it stands (critical finding 1); only plot-category regions count.
- **Depends on:** S1, S2. **Readiness:** Specified; the volumes are **Content** (authored per plot and yard).
- **Source:** `placementMode.md`, *Regions*.

### S4. Item lists

- **Server → client:** `0x1cef` (a property's whole list), `0x087f` (add or update several), `0x2cf7` (add or update one), `0x1097` (replace one), `0x1baf` (remove one; cancels its placement), `0x1967` (remove every entry of an owner), `0x7d28` (drop a property's list).
- **State:** placed items per property: record id, placed flag, transform (heading, pitch, roll, scale, x, y, z), owner name, the serialized item (F1).
- **Server rules:** an empty list fails the Sell gate's "loaded" test (the probe spec, chunk 2).
- **Depends on:** S2, F1. **Readiness:** Specified.
- **Source:** `placedItemsAndProperties.md`.

### S5. Placement and editing

- **Client → server:** `0x626e` (place request), `0x711a` (commit), `0x2036` / `0x3424` (move to inventory / to storage), `0x21ec` (menu request), `0x218c` / `0x050c` (interact, enter), `0x51e3` / `0x64a3` / `0x7ad1` (menu entries), `0x3c8b` (text answer), `0x58c7` (inspect), `OP_FindPersonRequest` type 3 (Locate), `OP_ClickObjectAction` / `OP_TradeSkillCombine` with property and record ids (housing containers).
- **Server → client:** the begin-placement `0x626e` (10 arguments, the explicit final pair; region permission list), `0x1228` (result), world objects `0x1045` (the placement-object layout; add or update), `0x2a9e` (remove) and `0x2523` (the item being placed moved), `0x21ea` (NPC-item spawn binding), `0x21ec` (menu, up to 40 entries; NPC items need the spawn id at `+0x2fc`), inspect reply `0x58c7`, `OP_FindPersonReply`.
- **Server rules:** **validate every commit with the client's 13 placement rules** (the client doesn't re-check a commit); create the entry, the world object or NPC spawn, and the inventory removal; for edits, `0x626e` with A = 1 and the spawn id; end an edit cleanly with `0x1baf` when storing for unpaid upkeep.
- **Depends on:** S3, S4, F1. **Readiness:** Specified, except the menu entries' `type` / `b` meanings (the server defines them; **Design**).
- **Source:** `flows/placeItem.md`, `flows/editPlacedItem.md`, `placementMode.md`, `worldLayer.md`.

**The Augmentater** (the plan's chunk 5): Live's nine modes, their lines and each race's and gender's options as captured, and Peridot's HELMET, PRIMARY and SECONDARY modes. Where each copy's look is kept is **Design**; the list of held models is content.

**Deferred to the last of the polish pass: Help.** Live gives every item Help (type `0xe`, eqstr 13300), which opens the client's own player-housing help window; RoF2's `0xe` asks for text instead (`0x3c8b`), so Peridot's menus leave Help out until then, when whether RoF2 can show housing help gets looked at. No research before that.

**Deferred to polish: Walking Death's shifting.** On Live the familiar (item 52198, `a_familiar_visage_of_death`) changes form in place among 16 looks (skeletons in five textures, vampires and zombies of both sexes, two banshees, a rat and a froglok ghost) at random intervals, 6 seconds to 19 minutes apart with a median near 3.4 minutes; the server swaps its look directly, with no buff (livedumps' `visageEvents`). Peridot first spawns it as the Banshee (`content/npcItemsToAuthor.md`); the polish pass adds the shifting on a random timer.

### S6. Neighborhoods and search

- **Client → server:** `0x2691` requests (types 0, 1, 2, 4, 5), `0x218c` with record id -1 (enter a neighborhood), `OP_FindPersonRequest` type 4 (plots), guild-name requests.
- **Server → client:** `0x2691` replies by mode (neighborhood list, details, single property, plot search results), guild names, `0x1d3c` (a new guild neighborhood announcement).
- **Server rules:** the client dispatches on the reply's mode, not the request; the Neighborhood window's filters start unchecked with the stock XML (critical finding 13: an empty-looking list isn't a server bug).
- **Depends on:** S2. **Readiness:** Specified; what an empty name search returns is **Design**; the neighborhood template is **Content**, and world generates the neighborhoods from it (`serverPlan.md` D11).
- **Deferred to polish: the teleflinger's fling (researched).** On Live the server arms Plot Search with `0x8a99 {u8}` (it opens the window and sets its fling flag), and Go then sends `0x1708 {rowValue}` once and is answered with a fling to the plot. RoF2 has only the sending half (`0x796c`, flag `+0x24c`) and no handler for the arming message, so its Go always sends `OP_FindPersonRequest` type 4; a fling there (EQEmu's `Client::Fling`, `OP_Fling`) would be a server-side choice on that request. **Live's behavior** (MQ2PeridotLive 1.4's flag watch): bumping or clicking the spring arms the flag, idempotently, and toggles the window; nothing else arms it (entering a neighborhood, the same or another, even through the Guild Lobby, doesn't); Go flings and disarms. **Disarming otherwise is unreliable on Live:** once, moving about 50 units from the spring disarmed it; other times no distance did, and an armed flag persisted across zoning. Distance may be meant to disarm it; replicating that exactly may need more testing. So a Peridot polish would keep an armed flag per character, set by the spring, consumed by a plot Go, and perhaps cleared beyond a distance from the spring. Revisit if testing shows a way to arm RoF2.
- **Source:** `opcode2691PropertySearch.md`, `flows/neighborhoods.md`, `flows/plotSearch.md`.

### S7. Purchase, sell and transfer

- **Client → server:** `0x45be` (purchase), `0x792c` (sell), `0x507f` (transfer), `0x5a63` (transfer-offer reply; the offer arrives as game dialog `0x89`).
- **Server → client:** `0x45be` `{u8 success}` (non-zero opens the Manage window), then the new owner (`0x63fd`) and the character's lists; result codes on refusal.
- **Server rules:** validate the option keys against the definition's cost elements, vacancy, funds, guild rules (permissions 27 / 28, membership) and the client's gate; a plot sells into a placed crate; a server-only minimum guild size for guild plots (string 5426).
- **Depends on:** S1, S2, S4. **Readiness:** Specified; the guild-size rule and the transfer-offer policy are **Design**.
- **Source:** `flows/purchase.md`, `windows/RealEstatePurchaseWnd.md`, `resultCodes.md`.

### S8. Escrow and upkeep

- **Client → server:** `0x2aa6` (type 0 deposit, 1 withdraw, 2 set the upkeep option).
- **Server → client:** `0x2aa6` replies (type 0 flag 0 refunds a failed deposit, type 1 flag 1 pays a withdrawal, type 2 flag 0 confirms the option), updated records.
- **Server rules:** **the client checks nothing**: validate funds and permission 2 (Withdraw Upkeep); keep balances per currency; charge upkeep on a schedule; when escrow runs out, expire upkeep and store items.
- **Depends on:** S2, S4, F1 (item upkeep ids). **Readiness:** Specified (wire); the schedule and the charging rules are **Design**.
- **Source:** `opcode2aa6PropertyAccount.md`, `flows/escrowAndUpkeep.md`.

### S9. Access and permissions

- **Client → server:** `0x3146` (set a property's access list).
- **Server → client:** `0x485d` (a list), `0x3968` (remove a list).
- **Server rules:** apply `0x3146` with the client's rank rules and validate names (the client doesn't); send pairs as `{group number, mask}` (critical finding 20); **enforce each permission where it matters** (entering, buffs, mail, withdrawing, adding, moving and removing items).
- **Depends on:** S2. **Readiness:** Specified (wire and groups); where each permission is enforced is **Design**.
- **Source:** `permissions.md`, `flows/accessAndPermissions.md`.

**Deferred to polish: the permissions pass (after implementation).** The rough shape comes from the menus captured so far: a visitor gets pet actions only where their group has Interact Item, the guild services work for anyone through the ordinary NPC interaction, and only guild members get their menu's Transact entry (`serverPlan.md`, the interaction menu). Which permission gates which entry is settled by an automated Live test in The Templar's hall, which holds every kind of item with its own menu: two viewers, a guild alt (its group set through `_Guild<rank>`) and a non-member named into Visitor, each through 15 iterations (every menu bit off, every bit on, then each of Enter, Interact Item, Move Item, Add Item, Remove Item, Mail Item and Manage flipped from both), coordinated over EQBC: the main sets the group's bits through the manage window's grid (its Live code still to trace), and the alt waits until its client holds the new list, then menu-scans. It needs MQ2PeridotLive to write compact per-iteration records (the access masks and members, and the menus) and access-list members in zone dumps (never captured so far: dumps hold neighborhoods' lists and the few plots stood on, without members), and a `permissions` kind with an analysis in mqperidotlive's `liveResearch.py`. Also for then: Beavus's plot's empty Items window menu (Fu World Order Village III, 101 Market Street), what a neighborhood's own list governs, and a world-click scan mode if a setting empties the Items window.

### S10. Houses, the moving crate and layouts

- **Client → server:** `0x21ba` (house swap), `0x6917` (store all), `0x613d` (pack / unpack), `0x5b41` (load a layout), `0x1da9` (leave real estate), `0x7bb1` (accept an anchor teleport).
- **Server → client:** entry updates and result codes; `0x7bb1` (anchor teleport offer); inside a house, the whole S1–S4 sequence again (an interior is an ordinary zone to the client).
- **Server rules:** bulk actions follow the individual-move rules per item; the crate is a child of its plot; layouts are validated record by record.
- **Depends on:** S4, S5, F3. **Readiness:** Specified (wire); how interiors are instanced per house is **Design**.
- **Source:** `flows/movingCrate.md`, `flows/layouts.md`, `commandsAndKeybindings.md`, `worldLayer.md`.

### S11. Polls

- **Client → server:** `0x15a8` (vote), `0x499a` (teleport to a ranked property), `0x6573` (the Purchase window's Vote button).
- **Server → client:** `0x15e2` / `0x39bb` (polls), `0x3311` (results), `0x6860` (option labels), `0x7497` (may vote / may see), `0x52b5` (your votes), `0x593a` (available), `0x7a4d` (open on a poll).
- **Server rules:** validate votes (the poll is Open, the player is in the may-vote set, the key belongs to the poll, at most its tick limit); property polls use property ids as keys.
- **Depends on:** S2 (property polls). **Readiness:** Specified; running polls (who creates them, when they open) is **Design**.
- **Source:** `polls.md`, `flows/polls.md`.

### S12. Trophy tribute

- **Client → server:** `0x51a9` (window opened), `0x4b65` (personal upgrade / downgrade), `0x7a41` (guild edit lock), `0x7c1d` (guild save), `0x2bc` (guild opt-in).
- **Server → client:** tribute definitions with `unknown128` = 1 for trophy benefits; `0x7697` / `0x4b65` / `0x08bf` (personal slots), `0x51ae` (personal timer), `0x7dd2` / `0x7db5` / `0x7a41` (guild set, available list, lock).
- **Server rules:** personal available benefits come from the trophies placed in the character's plots; validate tiers and costs; one guild editor at a time; permission 25 for guild changes.
- **Depends on:** S4, F1 (trophy benefit ids). **Readiness:** Specified; which items are trophies and their benefits is **Content**.
- **Source:** `trophyTribute.md`, `flows/trophies.md`.

### S13. The unshipped and edition-only houses (last)

- **What:** every interior Live never sold becomes a house like the standard ones, with its item and definition: Evantil's Abode (`phinteriortree`, sold only in Rain of Fear's Collector's Edition), the two dragon houses (`phinterior1b1`, `phinterior1d1`), the six ogre interiors (`plhogrinteriors1a1`, `1a2`, `3a1`, `3a2`, `3b1`, `3b2`) and the six dark elf interiors (`plhdkeinteriors1a1`-`1a3`, `3a1`-`3a3`). All 15 are committed; they come last because nothing on Live depends on them.
- **Per house:** the item (placeable in a yard), its definition (type, category, caps, pet cap, upkeep id and cost), its interior zone's entry and safe points (Evantil's Abode's are set; the rest derived, or by `/loc`), and its exterior model.
- **Server rules:** they are ordinary houses (S1, S5, S10), except **special houses that waive the plot's upkeep**, which do it with a reduction (a negative upkeep amount). The client nets it into the Manage window's daily cost, which must never show below 0c (owner's rule), so the catalog refuses a reduction larger than the smallest plot upkeep, a plot takes one reduction item at a time, and while one is placed the plot's upkeep stays in coin (a coin reduction can't offset another currency). Live's own display of a reduction item is still to observe (`flows/escrowAndUpkeep.md`, test cases).
- **Depends on:** S10. **Readiness:** **Content** (values and names are Peridot's to choose); the exteriors are **Verify**: Evantil's Abode's is very likely `IT23651`, and the dragon, ogre and dark elf houses' are still to be found in the client.
- **Source:** `serverPlan.md` (the interior zones), `robustness.md` (the client's zone table), `tools/makeHousingZonesSql.py` (zone rows, safe and entry points).

## Build order

```
F1 item tail --+
F2 opcodes ----+--> S1 catalog --> S2 properties --> S3 regions --+--> S5 placement --> S10 houses, crate, layouts --> S13 unshipped houses (last)
F4 schema -----+                        |                         |
F6 constraints                          +--> S4 item lists -------+
F3 zones (verify Q11 early)             +--> S6 neighborhoods, search
F5 zone-in sequence (grows with S1-S5)  +--> S7 purchase --> S8 escrow, upkeep
                                        +--> S9 access
                                        +--> S11 polls
                                        S4 + F1 --> S12 trophies
```

**Last, once the system is shaken out: the guild hall's teleport services.** The Teleportation Assistant sells 37 zone keys (items with no click spell) and the Teleportation Device, whose menu offers members Transact, takes them; Live keeps which key goes where and each landing spot on the server (door records carry no destinations either), so a way to capture them is worked out then.

## Open design decisions

Choices the client leaves to us, to settle in phase 2 before code: the catalog and neighborhoods to ship; region volume authoring; upkeep costs, schedule and expiry handling; where each access permission is enforced; house interior instancing; the `0x21ec` menu entries (types and submenus); the transfer-offer policy; the minimum guild size for guild plots; poll administration; what an empty neighborhood name search returns.

## To verify by test or Live comparison

Q11 (the interior-zone crash); Q25 (window clicks during placement); a plot sign click sending `0x218c`; the guild tribute names arriving one character short (an EQEmu packet quirk seen by the plugin).
