## Parallel or sequential

- **Audits: parallel.** Chunks 1-5 were audited and verified in parallel, and that held up. No chunk's audit needed another chunk's conclusions. The overlapping findings came out consistent: the pet cap turned up in chunks 1, 3 and 4, and the GUID in chunks 4 and 5.
- **Fixes: sequential by artefact.** Several defects run through the schema, the catalog generator, the wire writers and the plan text at once:
  - the pet cap and the upkeep flag: schema, catalog and writers;
  - the GUID: schema (chunk 3), item lists (chunk 4) and world objects (chunk 5);
  - axis order: catalog, entries and placement;
  - the D1 adoption: migration, item tail and NPC items.
- **Recommended order:**
  1. The owner's decisions (below).
  2. One pass over the schema, catalog and migrations, which are edited in place, with the repositories regenerated.
  3. The wire writers. These can run in parallel by file.
  4. The plan and doc text. These can run in parallel by chunk, with one merge.
- **Live research is independent and can start now.** The upkeep experiment has the longest lead time, and the 90-day reclaim checks are tied to calendar dates.
- **The chunk 6 audit can run alongside the fixes** if it takes the findings routed to it here: the expiry stamping, the sell crate, reclaim, NO TRADE and cadence.

## Verdict per chunk

**Chunk 1 (architecture, D1-D4, rules, X1-X3).**
- **What held:** almost every EQEmu touch point (lines, encoders, manifest, timers, rule injection, versioned tables), the 34 client-to-server opcodes, and the structural Live claims:
  - the placeable bit rule `1<<(type-1)`, with crates accepting anything;
  - definition 2 on all 7,808 neighborhood records, each with 75 children;
  - `max_houses` 1 on every yard;
  - one interactive object;
  - the Regular Zone bit only on the Candy Dish and the String of Bones;
  - 102 items with a limit of one;
  - selling returns the escrow (eqstr 7634).
- **What was overturned:**
  - The pet cap of 0 (blocking).
  - Every `DefaultAccess` "Live" value.
  - "Live charges hourly".
  - D7's "Live never evicted".
  - D1's Live item adoptions, which exist only in the plan text.
  - SPA 437/438 being "defined" upstream.
  - The status line.
  - Two "definition 1" mentions.
  - The respawn list missing 38.
- **What the verifier changed:** it upheld every verdict and added three things:
  - eqstr 728/750 carry the plot limits as server parameters;
  - strangers' balances never refresh in captures;
  - `chargeDueUpkeep()` is declared with no body.
- **Not checkable:** Peridot's own D1 counts. No item export is on disk.

**Chunk 2 (item tail, opcode map).**
- **What held:**
  - The 18-field wire order, confirmed in both clients (RoF2 `0x7f6bf0`, Live `0x1406c3880`).
  - The serializer. Its test, compiled with MSVC, passes: 171, 168 and 175 bytes. The caller's 5 trailing bytes were traced, which turned C2-11 into confirmed.
  - All 74 table values, resolved through both dispatch switches and their send sites.
- **What was overturned:**
  - The "6 extra bytes" are 3.
  - Today's MaxPerRealEstate reads `0xFF000000`, not 0.
  - The adopted Live values and housepet names are not in the migration (blocking).
  - `0x06c8` is the generic `OP_OpenContainer`.
  - `0x15e2` also travels client to server, after a UI reload.
  - `0x02bc` has two more senders, with no client-side gate.
  - The provider's default MaxPerRealEstate is 0, against -1 on Live and in the DB.
  - A failed tail load is silent (important).
- **New from the verifier:** zone does not link (`housing::handlePacket` is undefined), and `itemsTxtExtract.tsv` holds a second Live source covering 12,674 uncaptured placeable items.

**Chunk 3 (schema).**
- **What held:**
  - The 23 `CREATE TABLE`s are byte-identical to the manifest.
  - The 22 repositories show no drift.
  - The catalog equals Live on all 119 definitions except the pet caps and the neighborhood's `max_houses`.
  - 7 access groups.
  - 10 and 4 trophy slots.
  - Single-item cost options.
  - The DECIMAL mapping.
- **What was overturned:**
  - The pet cap (blocking).
  - The counts (23 tables, 8 catalog tables, 22 repositories).
  - The test description.
  - The layout origin is sent in the wrong axis order (now a settled defect).
  - The Standard hall uses PEQ's version 0.
  - Names over 64 characters unless the path is composed when sent.
  - No column for the item's upkeep stamp.
  - The stale content lines 505 and 509.
- **What the verifier changed:** C3-14 became checkable against Live, and the house-finish mapping is confirmed. G3-17 (sizing the access names) is already answered by the dumps.

**Chunk 4 (catalog, records, regions, item lists, zone-in, template).**
- **What held:**
  - The definition and record wire orders.
  - The per-account handle (72 record pairs and 1,000 item pairs, no character with two).
  - The template's stones and headings (75 of 75 within 0.1).
  - Region containment at all 9 Live yard positions.
  - The house and hall interior mappings.
  - Live's costs.
  - The 127/129/131 label fixes.
- **What was overturned:**
  - **Record `+0x40` polarity (blocking):** non-zero means expired.
  - The pet cap (blocking).
  - The handle is the string at `+0x30`; `+0x34` is `defaultPermissionLevel`, always 0. The verifier downgraded this to minor.
  - `+0x18` is read, but it is inert at -1 or 0.
  - Stale text: definition 1, regions 42-116, costs derived from price, every plot 120/6.
  - Standard hall version.
  - The Modest's door.
  - The derived safe points (19-121 units off Live's).
  - Axis order.
  - The plot sign is a border door, not a world object.
  - Live's zone-in is much narrower than the plan's.
- **Counts:** the verifier replaced the auditor's zone-in counts with reproducible ones (118 lists and 137 records out of 53,261 object pairs).

**Chunk 5 (placement, world layer).**
- **What held, on a far larger sample:**
  - Defaults equal the item's on all 891,440 known-item objects.
  - The held-item tilt rule (2 exceptions, both explained by augments).
  - The wall-mount code is identical in both clients.
  - Menus, the pool's aura spell, the Augmentater's counts, and the respawn values.
- **What was overturned:**
  - **The GUID has no stable source (blocking):** item-list GUIDs already change with every send.
  - `+0x98` is `bIgnoreCollisions`, which comes from the item flag.
  - Live keeps no world object for a bound NPC item.
  - RoF2's spawn reader carries the NPC binding.
  - C5-29 is overturned: both clients do compute tilts from the item for a UI actor, though world objects are still copied verbatim.
  - Enter is menu type 1, not 8.
  - The NO DROP/LORE rule.
  - The expired-plot lock.
  - Every quoted count is stale.
  - The training dummy's labels are missing from RoF2's dbstr.

## Contradictions found (plan says X, data or binary shows Y)

1. **Upkeep flag polarity (blocking).**
   - *Plan:* record `+0x40` is "upkeep-current", non-zero = paid (serverPlan.md:349, :447 "`upkeep_current` 0"; placedItemsAndProperties.md:100; placementMode.md:308; flows/escrowAndUpkeep.md:14; flows/placeItem.md:18; RealEstateItemsWnd.md:110,120; housingSchema.sql:115 default 1; housingMessages.cpp:27; serverWork.md:47).
   - *Binary:* RoF2 `0x5b4780` returns `[plot+0x40] != 0` (`0x5b47c2-0x5b47c8`, re-disassembled here). Its caller `0x5b6aa9` returns `0x18` when that is true.
   - *Live:* the same wire slot is `upkeepExpiredTime`, 0 while paid (44730591: 0 -> 1790577555 -> 0 -> 1790666988).
   - *Effect:* every paid plot would refuse placement with `0x18`.
2. **Pet cap 0 means full in RoF2 (blocking).**
   - *Plan:* "a pet cap of 0 is no separate pet cap" (serverPlan.md:509, :507; rules :161-162; the summary's "MaxPets... 0, no cap").
   - *Binary:* `0x5abc60` treats only a cap below 0 as no cap; a count at or over the cap gives code `0x26` in placement (`0x5b6d3d`) and moves (`0x5b63d2`).
   - *Live and catalog:* Live has -1 on all 60 non-yard definitions. The catalog writes 0 on 44 rows into an `INT UNSIGNED` column, and testHousingCatalog.py:80-86 asserts 0.
   - It is masked today only because every housepet name is `''` (see 3).
3. **D1's adopted Live values aren't in the migration (blocking).**
   - *Plan:* "The migration applies them after decoding" (serverPlan.md:102).
   - *Code:* itemHousingTail.sql:14 sets `hd_housepet_file_name` to `''` in every branch, with no value of 142 and no adoption rows. The manifest's version 1 is the same SQL. Commit 3d513b17c (the decision) touched only serverPlan.md.
   - *Effect:* RoF2's `0x7b1640` needs a non-empty housepet name, so no Peridot item is an NPC item.
4. **No stable GUID (blocking).**
   - *Plan:* the world object's GUID is "the instance's `guid`" and moves into `real_estate_items` (serverPlan.md:385, :388).
   - *Code:* no guid column exists (housingSchema.sql:138-166). housingItems.cpp:61-82 builds a new `ItemInstance` for every list, and every instance gets a new serial (item_instance.cpp:84,107; rof2.cpp:6471 writes it with `{:016}`).
   - *Effect:* RoF2 matches world objects, the Items window and spawn bindings by GUID (`0x5060c0`, `0x730ef9`, `0x54ed00`).
5. **Selling packs the placed crate, not a new Crate of Eviction.** *(New: my own check.)*
   - *Plan:* serverPlan.md:437 "Everything on the plot goes into a new moving crate (definition 31, the Crate of Eviction, item 52147)".
   - *Both clients:* eqstr 7634 reads "the placed moving crate will be packed with all of the plot's items and added to your inventory".
   - *Client sell gate:* it refuses without a placed crate with room (code 4 at `0x5b47d0`, code 5 at `0x5b65b0`; windows/RealEstatePurchaseWnd.md:116-117).
   - *Other docs:* serverWork.md:97 and flows/movingCrate.md:41 say "a plot sells into a placed crate".
   - Definition 31's caps are 0/0, which RoF2 reads as full.
6. **Upkeep cadence.**
   - *Plan:* "hourly, 1/24 of a day (as Live did)" at serverPlan.md:139, :151, :321, :447 and :627; ruletypes.h:1184; housingSchema.sql:129; testHousingSchema.py:92.
   - *Live:* plot 65484207 held exactly still for 11.47 h over 450 samples. Its 6 charges of -5,929 to -6,710 copper are each about half a day. No property shows two decreases within 3 h. Two expiries of 44730591 fell 24h50m33s apart (`audit/c1/liveEscrow.py`, `audit/v1/escrow.py`).
7. **D7: "Live never evicted for unpaid upkeep".**
   - *Plan:* serverPlan.md:155 and :468; ruletypes.h:1188.
   - *Live:* eqstr 13333 reclaims a plot "if escrow has been expired for 90 days or 30 days after the owner's guild membership has changed…". 13334 reclaims a guild plot "if you have less than 10 guild members or 90 days after escrow expires". The same text is on Live's `REMW_`/`REPW_Eviction_Label`.
   - *RoF2:* its text is login-based (90 and 30 days).
   - *Also:* serverPlan.md:449 "(90, as on Live; `character_data.last_login`)" is RoF2's rule, not Live's.
8. **DefaultAccess "Live" values.**
   - *Plan:* serverPlan.md:177-183 and :462; ruletypes.h:1208-1214 have 2, 2, 2, 2050, 4095, 8191, 8191, with "Trustee lacks Manage".
   - *Live:* 7,962 of 7,993 access lists are 3, 3, 0x803, 0xe03, 0x1f87, 0x1fff, 0. Trustee includes Manage.
   - *Binary:* RoF2's `0x5b3d50`/`0x5b5830` produce the same values, and the plan's own permissions.md:87-97 agrees.
9. **Layout origin axis order.**
   - *Plan and code:* the catalog stores and sends EQEmu (x, y) (housingCatalog.cpp:208-210; `#realestate definition origin`). Item entries send x first (housingItems.cpp:76-77).
   - *Live:* switchCoords, item-entry positions and `EQPlacedItem` are all y first. Definition 173 goes out as (2222.94, -2646.85) where Live sends (-2646.85, 2222.93). Over 2,960 plot items the median distance is 57 in the same order and 3,615 swapped.
   - *Peridot's own region matrix* already sends y first (housingGeometry.h:31).
10. **`0x06c8` is `OP_OpenContainer`.**
    - *Plan:* "a housing tradeskill container was opened" (serverPlan.md:272; worldLayer.md:130; COVERAGE.md:163).
    - *Binary:* its only sender is `CContainerMgr::OpenContainer` (`0x65cae0`), on every bag open. RoF's own opcode is `0x654f`.
    - *Effect:* client_packet.cpp:449 routes every bag open into `Handle_OP_Housing`.
11. **Placement object `+0x98`.**
    - *Plan:* "the NPC flag again" (serverPlan.md:388; worldLayer.md:51).
    - *Live:* the field is `bIgnoreCollisions`. It equals the item's `bPlaceableIgnoreCollisions` on 891,440 of 891,440 objects and is set on 30,717 non-NPC objects (anchors, translocators, the Boxes of …). RoF2 never reads item `+0x5b0`.
12. **NPC items get a world object as well as a spawn.**
    - *Plan:* serverPlan.md:385; worldLayer.md step 6.
    - *Live:* 54,509 bound-spawn sightings, none with a world object. README CF14 already says a `0x1045` NPC item creates no body.
13. **Enter is menu type 1.**
    - *Plan:* serverPlan.md:266 (`0x050c` "enter (menu type 8)"), :565 and :201 (X1 "`0x218c` / `0x050c`").
    - *Live:* Enter is `1:-1:723` on all 31 menus that offer it; type 8 is Use (5236) and the aura toggles. The plan's own table row :265 says `0x218c` is "interact (menu type 1)".
    - *Effect:* `0x050c` is also sent on any near-miss left click within 20 units (worldLayer step 4), so binding Enter to it would enter houses by accident.
14. **Record handle fields.**
    - *Plan:* `+0x30` "an empty string, never read" and the account id at `+0x34` (serverPlan.md:349, :353; placedItemsAndProperties.md:91-93; summary section 4).
    - *Binary:* Live's reader `0x1406e0740` and RoF2's `0x7fdf00` show the handle is the string at `+0x30`, and `+0x34` is `defaultPermissionLevel` (0 on 8,038 of 8,038).
    - RoF2 reads neither, so this is minor.
15. **X1's interior region would be ignored.** *(New: my own check.)*
    - *Plan:* serverPlan.md:201 says an X1 interior has "one region enclosing the zone".
    - *Binary:* `0x4b2f40` returns a region's property only when `0x7f6030(type)` reports the plot category (`0x4b2fca-0x4b2fd4`). Otherwise it falls back to the manager's `+0x8c`.
    - *Plan and work list elsewhere:* serverPlan.md:351 says "Interiors need no regions", and serverWork S3 says "only plot-category regions count".
16. **Everything that carries a property must be NO TRADE.** *(New: my own check.)*
    - *Plan:* serverPlan.md:449.
    - *Live:* every regular house deed (definitions 5-29, 126 items) and every storage crate (174-184, 11 items) has `IsDroppable` 1. Manors, halls, special houses, the Crate of Eviction and 191/192 have 0, as do NO DROP items such as the epics 9955 and 5532.
    - *Both clients:* eqstr 7637 "Your attempt to transfer this plot failed because there is a NO TRADE house or moving crate in the plot"; eqstr 5420 covers a NO TRADE active crate.
    - *Effect:* with every house NO TRADE, mirroring 7637 would block the transfer of any plot holding a house.
17. **"Live's client never derives the tilts from the item".**
    - *Plan:* serverPlan.md:388; worldLayer.md:55; summary.
    - *Binary:* Live's `0x140679af0`/`0x140679b80` and RoF2's `0x7bac00`/`0x7bac70` compute the item's pitch and roll with the held-item turn for a UI item actor. The design conclusion still holds, because both world-object readers copy the tilts verbatim. Only the citation is wrong.
18. **The pool's model.** *(New: my own check.)*
    - *Plan:* serverPlan.md:408 counts Peridot's 66645 among the 275 items "all showing the generic `IT63` model".
    - *Live:* the pool is `IT23628_ACTORDEF`, an interactive world object that `0x7b1640` does not count as an NPC item, so its model is `items.idfile`.
    - *RoF2:* it has `it23628.mod` in ghall_furniture01.eqg, loaded on demand through Resources/OnDemandResources.txt (`audit/final/findModel.py`).
19. **`0x15e2` direction.** *Plan:* S→C only (serverPlan.md:292); polls.md:5 "client never requests". *Binary:* `0x49d990` sends it bare after `CDisplay::ReloadUI` (`0x49d6b0`). No handler is registered.
20. **`0x02bc` senders.** *Plan:* "the guild opt-in checkbox", own name (serverPlan.md:309; trophyTribute.md:62). *Binary:* the Guild Management window's TrophyTributeOptOutButton sends the selected member's name (`0x68776a`), and TributeBenefitWnd sends the player's own (`0x784f5e`). Neither is gated client-side.
21. **"Only the owner adds or removes NO DROP and LORE".**
    - *Plan:* serverPlan.md:383 and :461.
    - *Live:* 1,331 NO DROP items on player properties belong to characters on other accounts.
    - *Client:* codes 64/65 mean removal by the item's own owner, and the LORE rule is a duplicate check on removal.
22. **The expired-plot lock.** *Plan:* "Live gives even the owner only Inspect, Show Items, Help and Manage" (serverPlan.md:394). *Live:* while 44730591 was expired, its bed (record 188) kept Take a Nap (menus.tsv, 20260928-211348).
23. **Item tail default.** *Plan:* serverPlan.md:226 has the provider send MaxPerRealEstate 0 for non-placeable items, while serverPlan.md:99 sets -1 in the database. *Live:* -1 on all 4,524 non-placeable items.
24. **Stale "today" bytes.** serverPlan.md:217 says the struct's 6 extra bytes are absorbed, and RealEstateItemsWnd.md:166 says "everything else reads as 0". In fact there are 3 extra bytes and MaxPerRealEstate reads -16777216. `rof2.cpp:6850-6892` is now `:6915-6950` (T6 has landed).
25. **Live Modest hall door.** *Plan:* "the Modest's none", with doors added only for the Palatial and Grand (serverPlan.md:513). *Live:* guildhall3_int door 2 is `GUILDHALL_DOOR_SWITCH`, type 161, at (265.698, 1.952, -0.683), heading 384. Its model is in guildhallsmldoor.eqg, which guildhall3_assets.txt loads.
26. **Safe and entry points.**
    - *Plan:* derived points (serverPlan.md:511; housingZoneSettings.sql): house interiors at (-7.0, 6.2) and (-0.4, 5.4); Grand and Modest at about (133-134.6, -0.2, -13.9).
    - *Live headers and arrivals:* houses (0, -12, 5) with arrival heading 0; guildhall3_int and guildhallsml_int (255, 2, -10, 386).
27. **Standard hall version.** *Plan:* "a version of its own" (serverPlan.md:513; serverWork F3). *Catalog:* row 38 is (345, 0), and makeHousingCatalogSql.py:235 always writes 0. No SQL creates the version.
28. **phinterior1d1.** *Plan and work list:* a dragon house Live "never sold" (serverPlan.md:564; serverWork S13). *Live:* zone 723 hosts definition 30, Hermit's Hideaway (deed 54999); serverPlan.md:517 says so too. Also, plhbixieint (774), plhpirateshipint (786) and dragonhouseinta (877) are outside RoF2's zone range (1-767), and RoF2 has none of their files.
29. **Spell effects 437/438 "defined, not implemented" upstream.** *Plan:* serverPlan.md:60, :556, :571. *Upstream:* spdat.h:1499-1500 has them only as commented-out `#define`s. The housing branch now defines them.
30. **Status vs code.**
    - serverPlan.md:3 says "nothing here is code yet", and :11 says "ready to implement", "verified by reading" (:609).
    - In fact zone/housing has 21 files, and neither process links: `housing::handlePacket` is declared (housing.h:18) and called (client_packet.cpp:17409) with no definition, and `chargeDueUpkeep()` is declared and called (world/housingUpkeep.cpp:12, :31, :38) with no body.
31. **Stale numbers.**
    - Definition 1 for Sunrise Hills: serverPlan.md:458 and :505 (it is 2).
    - "Every player plot holds 120 items and 6 pets" (serverPlan.md:509). Live has three tiers: 120/6 on 25 plots, 105/5 on 39, 90/4 on 7.
    - "Regions 42-116", "under 174", "costs derived" and "single coin option" (serverPlan.md:505).
    - 24 tables, 9 catalog tables and 23 repositories (serverPlan.md:313, :333, :600, :615). The true counts are 23, 8 and 22.
    - "Custom database version 2" (serverPlan.md:615); the code is at 3.
    - "73 plot records under the neighborhood" (serverPlan.md:626). The catalog has 75 children rows for definition 2, and Live has 75.
    - The respawn list is missing 38 (serverPlan.md:101; the plan's own :408 has it).
    - "15 copies of Tile - Shadow": Live has 2,053 copies at 768 distinct orientations.
    - 13,723 / 2,666 / 1,271 / 77% / "14 guild member menus": now 98,095 / 3,679 / 3,419 / 75.6% / 18 on Bristlebane (891,440 objects, 4,377 items and 9,055 of 9,060 items on all servers).
    - The definition id range "5-34 ... upkeep 17-24".

## Cross-chunk inconsistencies

1. **The pet cap appears in five places** with one wrong meaning: chunk 1's rules :161-162, chunk 3's schema (`INT UNSIGNED`, the CHECK), chunk 4's content :507/:509/:543 ("as with pets 0 is no cap" for the Crate of Eviction), the catalog generator line 197 (`pets=0`), testHousingCatalog.py, and the summary. The two MaxPets rules aren't in ruletypes.h at all.
2. **The GUID.**
   - Chunk 4's S2 (:349) says EQEmu reassigns `inventory.guid` at every load.
   - Chunk 5 (:385, :388) uses "the instance's guid".
   - Chunk 3's `real_estate_items` (:322) has no guid column.
3. **Axis order differs between parts of the plan:**
   - S3 regions (:351) send y first;
   - S1's layout origin (:347) and S4's positions (:353) name no order, and the code sends x first;
   - layouts (:570) say "carry between plots".
4. **Upkeep state.**
   - Chunk 4 S2 sends an "upkeep-current flag".
   - Chunk 6 S8 (:447) sets `upkeep_current` 0 on expiry.
   - Stage 3 (:627) checks the "upkeep flag".
   - All three carry the inverted meaning. None carries Live's stamp-and-store behaviour (G23): at the expiring pass Live stamps the property and every placed item with an upkeep id, stores the non-house ones that same second, leaves houses placed, and resets the placed items' stamps on refund. S8 instead stores items "at the next charge".
5. **Item upkeep stamp, conflicting verifier advice.**
   - Chunk 3's G3-03 recommends sending 0 and dropping the clause.
   - Chunk 4's G23, and the summary's recorded owner decision ("Peridot sends Live's values … the item's upkeep expiry in field 4"), call for storing it.
   - The schema has no column either way. Resolved below as a plan fix following the recorded decision.
6. **Option keys, conflicting verifier advice, resolved here.**
   - Chunk 4's G16 recommends Live's 1-based component ids.
   - Chunk 3's C3-17 says RoF2 needs positions.
   - My disassembly: the Manage path `0x743c45-0x743c4d` passes record `+0x3c` straight to `0x7fb970`, a bounds-checked index at stride 0x14. The Purchase combobox (`0x74d2ec-0x74d30e`) stores keys from `0x7fb930` and selects by matching the key. Only key == position satisfies both.
   - So keep 0-based keys (as serverPlan.md:443 and stage 2's "keys 0..n-1" say) and drop G16's recommendation.
7. **The world object or the NPC spawn.** serverWork.md S5 ("create the entry, the world object or NPC spawn") matches Live. serverPlan.md:385 and worldLayer step 6 send both.
8. **The sell crate.** serverWork S7, flows/movingCrate.md:41, RealEstatePurchaseWnd.md:116-117 and eqstr 7634 all say "the placed crate". serverPlan S7 (:437) says "a new Crate of Eviction". D6 (:467) inherits the plan's version.
9. **NO TRADE.** serverPlan.md:449 makes every house and crate NO TRADE. S7's transfer (:438) never mentions the client's eqstr 7637, which refuses a transfer when a NO TRADE house or crate is on the plot. Line 513 calls guild halls No Trade, and line 521 the manors, both correctly.
10. **DefaultAccess.**
    - serverPlan's rules (:177-183, :462) and ruletypes.h give one set of values.
    - The plan's own permissions.md:87-97, serverWork S9, and the summary give Live's.
    - The plan was never corrected after aed294915.
11. **Interiors.** D3 (:137) says "Decided: one instance per house". S10 (:563), serverWork S10 and the summary section 6 say "dynamic-zone templates lead".
12. **X1 region (:201) vs S3 (:351)**, as in contradiction 15.
13. **Sunrise Hills' definition.** Lines :167, :472 and :474, the rule and the catalog say 2; lines :458 and :505 say 1. Line :505 (stored costs, regions 42-116) also contradicts D13 (:474).
14. **The regeneration pool.**
    - :390 gives it a 38-second respawn.
    - :408 calls it "the one interactive NPC item" showing IT63.
    - :396 and npcItemsToAuthor.md:17 call it an interactive world object, `IT23628`.
    - RoF2's `0x7b1640` says it is not an NPC item.
15. **Enter.** The opcode table (:265 type 1 `0x218c`; :266 type 8 `0x050c`), S10 (:565) and X1 (:201) disagree with each other and with Live's menus (:394).
16. **Doors.** T21 (:59) and zone/doors.cpp:547-548 hand only open types 57-58 with a destination to housing. Chunk 4 shows the plot sign is the type-160 border door with a `RealEstateDoorID`, and Live's type-159 void portal opens the Neighborhood window. Two documents still assume `0x218c` from a world-object sign: serverWork "To verify: a plot sign click sending `0x218c`" and flows/purchase.md:30/:104, as does the summary's test list.
17. **T12 and D11.** T12 (:50) and the code (world/zoneserver.cpp:1676) only forward `ServerOP_Housing`. D11 (:458) needs world itself to see ownership changes. D2 (:108) names `ZSList::SendPacket`; the code uses `SendPacketToBootedZones`.
18. **Upkeep percent.** S8 (:444) has world charge "at its host zone's ruleset's percent", and X3 (:203) uses the same idea. No such rule exists, and world loads only the default ruleset (world_boot.cpp:318-332).
19. **The verification list (:640)** still lists items that are now settled: "the inferred world-object fields (heading offset, wall tilts)" (settled by Live data) and "the NPC respawn interval's unit" (:390 makes it the owner's decision).
20. **serverWork's "Open design decisions"** still lists upkeep schedule, access enforcement and interior instancing, which the plan decided (D4, D7, D3). It also lists only types 0-2 for `0x2aa6`, where the plan has 0-7.
21. **The owner's summary has not been folded back:**
    - section 4 still has the handle at `+0x34`, MaxPets "0, no cap", and the 13,723/2,666/1,271/77% counts;
    - section 1 says strangers' records carry their balance, but captures show those balances never change (bristle 105717718: 461 samples over 21.9 h at one value), so a balance held is not a balance refreshed;
    - section 1's "Open: whether expired items stay placed" is now settled;
    - section 4 has no chunk 2 entry at all.
22. **Housing-disabled zones.** Line :226 says only world sends the default tail. Zones with `Housing:EnableHousing` off never register the provider either (housing.cpp:276-283), so trophies and placeable flags vanish from item displays there. X3 consequence 4 (per-zone rulesets) makes this reachable.

## Questions for the owner

1. **Upkeep cadence.**
   - *Options:* (a) a world tick every hour charging 1/24, as planned; (b) Live's way, a per-property clock about every 12.5 h, prorated by elapsed time.
   - *Recommendation:* (b). `next_upkeep_at` already supports it. Set the interval once the section 2 experiment fixes it, and label any default as Peridot's choice until then. Totals are equal either way.
2. **Reclaim rules (D7, inactivity, guild plots).**
   - *Options:* (a) RoF2's text: 90 days without the owner's login, 30 days without any member's; (b) Live's: a plot 90 days after its escrow expires; a guild plot below 10 members or 90 days after expiry; a guild-village plot 30 days after its owner's guild membership changes, behind Live's guild-leader auto-eviction option (eqstr 11035, Live only); (c) never.
   - *Recommendation:* (b) for plots and guild plots, with `UnpaidUpkeepEvictionDays` at 90, a new guild-plot minimum-members reclaim, and eqstr 13333/13334 patched through the patcher so RoF2's labels match. Make the guild-village leave rule a separate rule defaulting to off, since RoF2 has no UI for the leader's option.
   - *Also:* Live never reclaims an expired house on a paid plot (Xuurk's 33217796, expired 172 days and still placed). Recommend the same.
   - Then fix D7, :153-155, :449 and :513's "loses the hall below that".
3. **Pet cap representation.**
   - *Options:* (a) a signed `max_pet_items` with -1 = none and 0 = no pets; (b) keep it unsigned and write `0xFFFFFFFF` on the wire when 0, which loses "no pets allowed".
   - *Recommendation:* (a). Make the MaxPets rules -1 = none, defaulting to -1, and add them to ruletypes.h.
4. **Default access masks.**
   - *Options:* adopt Live's and RoF2's 3, 3, 2051, 3587, 8071, 8191; for Owner, 0 (Live) or 8191.
   - *Recommendation:* all of Live's values, Owner included at 0. The evaluator (:416) passes the owner without the mask. Bit 0's meaning stays with the permissions pass.
5. **House interiors.**
   - *Options:* (a) plain `instance_list` rows per house (D3 as written); (b) `dynamic_zone_templates` keyed by `dz_id` (houses 23000-23005, halls 35633-35642), which supports `never_expires`.
   - *Recommendation:* (a). The zone-in points already live in `entryPoints` and T21, and the DZ machinery adds expedition semantics housing doesn't need. Delete "leading" from S10 and the work list. Separately, add the instance-id ceiling (31,900 usable, shared with expeditions) to D3, failing loudly at exhaustion.
6. **Property names.**
   - *Options:* (a) store only the neighborhood's generated name and compose each record's name when sent: `<parent name>, <segment>`, the segment being a plot's address or a per-definition display name ("Three Room House", "One Room House", "Bixie Hive House", a hall's or crate's item name); (b) store whole names, which needs a longer column than VARCHAR(64), since Live's reach 99 characters.
   - *Recommendation:* (a). An empty name makes RoF2 print 5288 instead of 5289.
7. **Zone-in scope.**
   - *Options:* (a) records and item lists for every hosted property, as planned; (b) Live's: only own, guild, related and access-granted properties. Live loaded item lists for 118 and records for 137 of 53,261 object pairs.
   - *Recommendation:* send records for every hosted plot (RoF2's standing-in lookup `0x4b30f0` and vacant Purchase windows need them), but item lists, with their owner names and handles, only for related or access-granted properties, as Live does.
8. **Houses and crates tradeable?**
   - *Options:* (a) every property-carrying item NO TRADE, as planned. This diverges from Live, and with eqstr 7637 mirrored no plot holding a house could be transferred. (b) Live's flags: deeds and storage crates tradeable. Only paths that drop custom data (parcels, bazaar) are blocked at runtime for an instance carrying a property, and 7637 applies only to the NO TRADE manors and halls.
   - *Recommendation:* (b), if a Peridot test shows EQEmu's trade keeps `custom_data` (below). Otherwise (a), and say explicitly that 7637 is not mirrored.
9. **Spawn binding route.**
   - *Options:* (a) fill RoF2's spawn GUID, property and record fields (rof2.cpp:4935-4937, now `'0000000000000000', -1, -1`) and keep `0x21ea` only for the item being placed; (b) `0x21ea` only, re-sent after every spawn send.
   - *Recommendation:* (a). Every respawn and look-change respawn otherwise clears the binding.
10. **An NPC item whose NPC can't spawn** (Live shows an invisible NPC-flagged IT63 object, 61009). *Recommendation:* refuse placement of an NPC item that has no `real_estate_npc_items` row, and fail the catalog or content load loudly, per the no-silent-fallback rule.
11. **Item tail load failure and housing-disabled zones.**
    - *Options:* turn housing off in the zone, or fail zone boot.
    - *Recommendation:* fail zone boot when the tail query fails. Register the provider whether or not `EnableHousing` is on, so item displays stay right everywhere.
12. **MaxPerRealEstate default.** *Recommendation:* -1 in `EQ::ItemHousingTail`, as Live and the database have it.
13. **Expired plot interactions.**
    - *Options:* strict (the plan); Live's, which kept the bed's Take a Nap.
    - *Recommendation:* keep strict until the Live nap test below shows the action actually works, then follow Live.
14. **Rotation and free-placement locks on commit.** *Recommendation:* reject pitch or roll changes on rotation-locked items, and free placement on free-placement-locked items. Live kept tilts off their defaults on only 7 of 59,800 locked objects. Heading stays free.
15. **Training dummy labels** (dbstr category 28, ids 100-504, absent from RoF2).
    - *Options:* ship the rows in a patched dbstr_us.txt; relabel with eqstr types; drop the actions.
    - *Recommendation:* patch dbstr through the patcher, which is the same channel as the corrected map files.
16. **Player-plot guild id on the record.**
    - *Options:* the owner's current guild (Live: 85 of 85 owned plots carry one, 9 differing from their neighborhood's guild); the guild at purchase; -1.
    - *Recommendation:* -1 unless the RoF2 Purchase-window trace (client analysis below) shows `+0x24` matters for player plots. Either way, send the stored 0 as -1.
17. **106 Brimming Way** (definition 102: buys at cost 12 but pays upkeep 9, 16.8 pp a day, twice the 10% rule).
    - *Recommendation:* keep Live's value if the Live check confirms Live charges it. Otherwise normalise to cost 7.
18. **`#realestate cost set` scope and sign.** *Recommendation:* any cost, any non-zero amount. The catalog validator enforces S13's reduction bound (cost 25 is -42,000 on Live). Align :372 and housingCommand.cpp:316.
19. **Live-only catalog entries:** definition 1 (Town, linked from the portal and gate doors' `RealEstateDoorID` 1), 6, 35-37, 185/186 and 191/192; costs 5 and 999997-999999; the neighborhood's `max_houses` 1711. *Recommendation:* omit them and document the omission, unless G10's door work keys on definition 1.
20. **Standard hall version of 345.** *Recommendation:* version 2, since PEQ uses 0 and 1.
21. **Neighborhood names.** Live also uses compound-prefix names ("Woodgate II", "Wyvernlake III") on 6 servers, 181 records. *Recommendation:* the tree/middle/place scheme only; the others look hand-made or merger-made. Confirm guild villages as `<guild> Village[ N]`.
22. **Character deletion** (properties, items, offers and votes aren't in the character map). *Recommendation:* on a hard delete, treat it as inactivity (pack into a crate, release the plot). On a soft delete, keep everything.
23. **Column naming rule.** *Recommendation:* narrow :313's wording to "fields that were unknown before Live's names were known" rather than renaming eight columns.
24. **`MaxPlotsPerCharacter` 3.** Keep it as the owner's value if the Live check below isn't run. Note that Live's limit may count per account.
25. **Trophy opt-out of other members** (only if Live can't settle it). *Recommendation:* self always, others only for officers with trophy rank permission 25.

## Live research needed (what the dumps truly lack)

1. **Upkeep cadence, anchor and proration.**
   - *Searched:* `[properties]` escrow series across 5,026 zone captures. master/upkeepRecordings.tsv has only its header.
   - *Step:* run the designed experiment (summary section 2) with `/peridotlive upkeep auto on`. Two additions:
     - the setup check in step 4 must show a test plot's balance *changing* on an observer-trustee, because strangers' records never refresh;
     - dump each plot right after purchase and before funding (`/peridotlive dump`), to read `upkeepExpiredTime`.
   - *Settles:* owner question 1, and D9's first-charge timing (C1-45).
2. **Plot limit, its scope, and definition 102's upkeep** (cheap: plots cost 21-168 pp).
   - *Searched:* 85 owned plots, at most 2 per character and 2 per handle. eqstr 728 and 750 carry `%1`.
   - *Steps:*
     1. On Bristlebane, Pariator (2 plots: 65484207, 44730591) buys a vacant 106 Brimming Way (84 pp). The Purchase window is auto-captured.
     2. He opens Manage and reads the Daily Cost label.
     3. He attempts a 4th plot and screenshots the chat (eqstr 728 prints the limit).
     4. Clerictrader buys a 2nd plot, bringing account 633393165 to 3. Anastrianaa (same account, owns 65484240) then attempts one.
   - *Settles:* `MaxPlotsPerCharacter`, character vs account scope, and G15.
   - *Optional:* a second guild plot attempt (21 pp) prints eqstr 750's guild limit.
3. **90-day reclaim.**
   - *Searched:* properties.tsv; the oldest expired plot is 46 days.
   - *Steps:*
     - After 2026-11-11 19:00 UTC on Bristlebane: Neighborhood window, search "Fu World Order Village III", Go, `/peridotlive dump`. Check `[placedObjects]` for realEstateId 29661343 (Ginsberg's plot). Absent means reclaimed. If present, right-click his Monel Style Country Domicile so the menus log records the property.
     - Repeat for Quizp's 133141067 (Forgotten of Gallifrey Village) after 2026-12-10 20:00 UTC.
     - Optionally leave Pariator's 44730591 unfunded and dump after 2026-12-28 07:30 UTC. His own record refreshes, but the plot may be lost.
   - *Settles:* owner question 2 on Live.
4. **Expiry notice bodies.**
   - *Searched:* no capture holds mail bodies.
   - *Step:* while chat is up, open each "Escrow expired" mail, find the reader window with `/windows open`, then `/peridotlive window <name>`. Keep 44730591 unfunded to collect the later notices.
   - *Settles:* D7's notice sequence.
5. **Expired plot interactions.**
   - *Searched:* menus.tsv, 20260928-211348.
   - *Step:* while 44730591 is expired (since 2026-09-29 07:29:48 UTC unless refunded), Pariator clicks Take a Nap on the bed (record 188) and notes the result.
   - *Settles:* owner question 13.
6. **Plot border and guild square click.**
   - *Searched:* signs.py, 400 neighborhood captures: no sign-like world object exists. fields/doors.tsv gives `RealEstateDoorID` 101-173 and 187-190.
   - *Step:* click a vacant plot's border plaque, then a guild square's. The Purchase window is auto-captured if it opens; otherwise note the chat.
   - *Settles:* what the server must answer for a type-160 `OP_ClickDoor`.
7. **Region extents.**
   - *Searched:* 9 in-yard samples, all near ground level.
   - *Step:* on 104 Market Street (44730591), `/peridotlive dump` just inside and just outside each side, then levitating about 100 and 500 above. Compare the manager line's `currentYardId` using `audit/yardCheck.py`.
   - *Settles:* the "100 below, 900 above" assumption.
8. **Zone 718 (phinterior1a2) safe and arrival point.**
   - *Searched:* zoneHeaders.tsv has no 718.
   - *Step:* Anastrianaa enters her One Room House 117754208 (definition 25, Wood finish, 101 Market Street) and runs `/peridotlive dump` before moving.
   - *Settles:* 718's points, and confirms the 1a2 = Wood mapping.
9. **Pool aura.**
   - *Searched:* buffs.tsv, 7 rows, none with 31874.
   - *Step:* in Bristlebane's Palatial 65484252 with the aura active, stand on the pool and run `/peridotlive buffs` and `/loc`. Step out 5 units at a time until 31874 drops. Record its duration over two dumps, then Deactivate and Activate (b 2 and b 1) and repeat.
   - *Settles:* radius and application (S5's pool).
10. **A visitor using another guild's services.**
    - *Searched:* every vendor capture comes from Pariator's own hall.
    - *Step:* Pariator enters the Palatial 63846974 (Fu World Order Village III), right-clicks Friendly Guild General Supply Merchant 66639 (`peridotLiveVendor_*` is written automatically), then the Personal-Banker.
    - *Settles:* :394's "open to everyone".
11. **Does the begin-placement object carry the tilts?**
    - *Searched:* MQ2PeridotLive has no placement-state dump; only final states are captured.
    - *Step:* place a fresh, unaugmented shield from a bag with one click and no rotation, then `/peridotlive dump`. Angle/Roll of 128/256 confirms it.
    - *Optional:* Inspect antonius 134051344 record 77 and drinal 64195792 record 29 for augments, to confirm the binary's explanation of the odd copies.
12. **Trophy opt-out of others.**
    - *Searched:* no GuildManagementWnd capture exists.
    - *Step:* as an officer, open Guild Management, select another member, `/peridotlive window GuildManagementWnd`, press Trophy Tribute Opt Out, then capture again together with `/peridotlive window TributeBenefitWnd`. Repeat as an ordinary member and note by eye whether the button is greyed (the plugin doesn't record the enabled state).
13. **Polls.**
    - *Searched:* the one VoteResultsWnd capture has 0 rows.
    - *Step:* open the Polls window (auto-captured), press Open Polls (VoteWnd auto-captured), on two or more servers. If both stay empty, record "Live runs no polls".
14. **Base personal trophy slots.**
    - *Searched:* TributeTrophyWnd shows 4 active and 7 available; dbstr 30195 is "Trophy Collector".
    - *Step:* check Pariator's AA window for Trophy Collector, then try to activate a 5th trophy. eqstr 5254 means the slots are full.
15. **Optional, if owner question 8 leans to Live's flags:** Pariator tries to trade, then parcel, a picked-up house holding items (Clerictrader's hut 117754115 holds 11 items) to Anastrianaa, a different account. Note any refusal text.

## Client analysis needed

1. **RoF2 door spawn and click path** (G10). Does the `OP_SpawnDoor` (`0x7291`) reader feed an `EQSwitch` real-estate field from `Door_Struct` `unknown0068-0088`? What does a click on open types 159, 160 and 161 send? `0x50f540` is the only type compare, and it's for sounds.
2. **Layout writer and loaders** `0x5ae900`, `0x5b0380` and `0x5b0680`. Confirm that the origin and positions combine y first. This is optional: the wire order is already settled from data.
3. **`0x57ef90`, the pick-up limit:** one non-empty house *and* one non-empty crate, or one in total (C5-15).
4. **Guild Management member list:** which field feeds the trophy opt-in column (GuildMemberEntry `unknown01`, after `tribute_enable`).
5. **Guild-name cache miss:** which message answers it (serverPlan.md:432, still open).
6. **The Purchase window's reads of record `+0x24`** on player plots, before settling owner question 16.
7. **World `0x2698` at character select.** `WorldAuthenticationHandler::OnRoutePacket` (`0x562cd3`) forwards it to the manager. Decide whether world should send the catalog.
8. **The item preview path** (`ItemPacketInvalid`, `unknowna29 = 0xff`), only if the owner wants housing lines in previews.
9. **Placement rules after `0x0f`** in `0x5b69b0` (the moving-crate, capacity, container, upkeep and permission codes). Minor.
10. **A full scan for reads of record `+0x30`/`+0x34`.** Low value; none found by the heuristic scan.

## Peridot-side tests

1. **Owner runs the read-only exports** tools/exportPlaceableItems.sql and tools/exportItemDefinitions.sql, and commits the output, for example as content/placeableItemsCapture.tsv. Recompute every D1 count against fields/itemDefinitions.tsv plus itemsTxt/itemsTxtExtract.tsv:
   - 5,577 placeable (5,205 shifted, 372 aligned);
   - 2,519 shared;
   - the 485/446/17/16/4/1/1 bitmask changes, 56 scale minimums, 46 orientations, 134 housepet names, 88 trophies, 170/455 flags, "10 of 17 identical";
   - the 7 off-grid Live orientations, Meleen Estarim, and the 4th roll-384 tile.
2. **Read-only queries:**
   - `SELECT custom_database_version FROM db_version; SHOW TABLES LIKE 'new_table';`
   - `SELECT zoneidnumber, version, short_name FROM zone WHERE zoneidnumber BETWEEN 700 AND 770 OR zoneidnumber=345;`
   - `SELECT guild_id, perm_id, permission FROM guild_permissions WHERE perm_id IN (24,25,27,28,29);`
   - the ids in `tributes`;
   - `SELECT idfile FROM items WHERE id=66645;` (expect IT63; Live's is IT23628).
3. **Schema tests:** run testHousingSchema.py and testHousingCatalog.py on a disposable MariaDB, after adding negatives for: vault greater than max items, poll kind 3, selection type 4, access-name group 7, and `max_pet_items` -1 accepted.
4. **Stage 1 (item tail):** add NPC items. Hold 66639 (housepet `NGH_Merchant_General`, interactive 0, respawn 750) and one captured familiar. The Items window's Type column must show "P".
5. **Upkeep flag:** on a funded plot, Place is not refused with `0x18`. After expiry it is, and a house on that plot gets `0x19`.
6. **Pet cap:** place a familiar in a house and Personal-Banker 66641 in a Palatial hall, with the cap at -1 (succeeds) and at 0 (`0x26`).
7. **`0x06c8`:** open an inventory bag and a bank bag. Neither may produce a Housing log line.
8. **`0x15e2`:** `/loadskin default 1` in a housing zone. The Vote and Polls windows repopulate, and the log has no unhandled `0x15e2`.
9. **World objects:**
   - an `IT<n>_ACTORDEF` name renders;
   - `+0x98` = 1 makes a Primary Anchor (52584) walk-through;
   - an ornamented weapon shows the ornament's model.
10. **NPC items:**
    - edit and Locate with no world object present (begin path 0; `0x730de0` resolves through the spawn GUID);
    - after a respawn, right-clicking the NPC still sends `0x21ec`.
11. **GUID:** zone out and back in. The same placed item keeps its GUID in `0x1cef`, `0x1045` and `0x21ec`.
12. **Layouts:** save on one plot and load on a plot with a different heading, and with Rotate Yard. Nothing may come out mirrored.
13. **Doors:** click a plot border (type 160) and the void portal (type 159). Each should reach housing once T21 is widened.
14. **Zoning:** the probe's `manager` and `records` after zoning out of a neighborhood (G14).
15. **Tail failure:** rename the column on a scratch DB copy. The zone must refuse loudly, per owner question 11.
16. **Trade and custom data** (for owner question 8): trade a crate item that carries a `real_estate_property_id` between two characters. Its custom data must survive.
17. **Already listed elsewhere:**
    - the position check against the client's box at plot edges;
    - Q11 (interior zone with no housing messages);
    - Q25;
    - guild tribute names one character short;
    - the mail-date test;
    - the type-0 Manage entry opening the right property;
    - the preview path (OP_ItemPreviewRequest) with 54997 and a house item.

## Plan fixes (text, SQL and docs, with the correct content)

**Schema and catalog** (migrations edited in place, repositories regenerated):
- `real_estate_properties`: replace `upkeep_current TINYINT DEFAULT 1` with `upkeep_expired_time INT UNSIGNED NOT NULL DEFAULT 0`. It holds the UTC second the charge pass found escrow empty, is 0 while paid, and is sent at record `+0x40` (housingMessages.cpp:27).
- `real_estate_items`:
  - add `upkeep_expired_at`, per the summary's recorded decision on field 4. Set it at the expiring pass on placed items with an upkeep id; clear it on refund for placed items; stored items keep it, as on Live;
  - add a stable GUID: either a column, or derive 16 hex digits from the row id (for example `{:016X}` of `id`). Use it in every entry (passed to the serializer in place of the serial), and in `0x1045`, `0x2a9e`, `0x626e`, `0x711a`, the spawn binding and `0x21ec`.
- `max_pet_items`: make it `INT NOT NULL DEFAULT -1` with `CHECK (max_pet_items = -1 OR max_pet_items BETWEEN 0 AND max_items)`.
- makeHousingCatalogSql.py: copy Live's `maxNpcItems` (lines 166 and 197). Change testHousingCatalog.py:80-86 to expect -1. housingCatalog.cpp:207 sends the value signed.
- Origin order: `Catalog::message()` writes `layout_origin_y` first. housingItems.cpp:76-77 writes y first. Add a schema comment that x/y are EQEmu's and the wire is y first. Test: definition 101's origin bytes are (-219.06, 332.21, 0.127).
- Drop `real_estate_poll_options.unknown_value`. Drop, or give a reader to, `real_estate_properties.created_at`, `real_estate_transfer_offers.created_at` and `real_estate_poll_votes.voted_at`. Remove "poll labels" from housingSchema.sql:3's content list.
- Standard hall: 345 at version 2. The generator writes it for definition 38. housingZones.sql adds the zone row; door SQL adds `OBJ_GUILD_DOOR` (type 161) at (-3.4, -162.899, 6.9), heading 128.
- Hall doors: generate them from master/doors.tsv for 751 (one `GUILDHALL_DOOR_SWITCH`), 738 (one), 737 (the front door, with its 26 `OBJ_INTDOOR`s as polish) and 345 v2. Optionally do the same for the captured house interiors' front doors.
- Safe and entry points (makeHousingZonesSql.py):
  - phinterior3a1/3a2/3a3/1a1/1a3/1d1: (0, -12, 5), heading 0;
  - guildhall3 and guildhallsml: (255, 2, -10), heading 386;
  - phinteriortree: (0, 0, 3);
  - plhbixieint and plhpirateshipint: (0, 1, -8);
  - dragonhouseinta: (0, -6, 7);
  - keep the Palatial's entry at (71, -1).
- Guild borders: take doors 226-229 (x, y, z, heading) from master/doors.tsv. z is 108.87/108.80/-0.72/-0.42, not 110/0.
- Add a door-to-definition mapping: a lot square is its PEQ door id + 100; guild squares need a column, because guildBorderDoors.sql assigns `MAX(doorid)+1`.

**Item tail (chunk 2, D1):**
- Add a **new** manifest entry after `2026_09_26_items_housing_tail`. Version 1's `UNK224` condition can't run again. Generate it from fields/itemDefinitions.tsv plus itemsTxtExtract.tsv, which covers about 3,058 placeable items outside the capture (255 means -1). It sets the Live values for:
  - the placeable bitmask;
  - scale min;
  - the default heading, pitch and roll;
  - the rotation lock;
  - max per real estate;
  - the housepet name, with all 113 listed NPC items available from npcItems.tsv.
- testItemHousingTail.py asserts the per-field change counts, and that every row with respawn > 0 and not interactive has a non-empty housepet name. Decide on 93435 (a housepet name with respawn 0).
- Classify NPC items by `0x7b1640`'s rule (housepet name non-empty and not interactive), not by respawn interval. Live has 12 respawn-4500 items that are ordinary objects.
- Set 66645's `idfile` to IT23628 (Live's).
- `EQ::ItemHousingTail` default `maxPerRealEstate` = -1.
- Check the result of `loadItemHousingTails()` (housing.cpp:282).
- patch_RoF2.conf: `OP_OpenContainer=0x06c8`. Delete `OP_RealEstateContainerOpen` from emu_oplist.h, the conf and client_packet.cpp:449, which makes 73 new opcodes and 3 reused. `Handle_OP_OpenContainer` hands only location type 4 to housing.
- Mark `0x15e2` as both directions and register it. S11 answers a bare request with `0x6860`, `0x15e2`, `0x7497`, `0x52b5` and `0x593a`. Fix polls.md:5.
- `0x02bc`: document both extra senders, and enforce on the server who may opt out whom.

**serverPlan.md:**
- :3 and :11: the status now includes code under audit. Note the two link gaps (`housing::handlePacket`, `chargeDueUpkeep`).
- :60, :556 and :571: "documented upstream only as comments; housing defines them".
- :78 and :103: houses 5 and 7-29; manors 32-34; Crate of Eviction 31; halls 38-41; storage crates 174-184 (upkeep 26-35). 30 and 42-47 are Live-only.
- :101: respawn values 4500, 750, 38 (the pool) or 0.
- :102: "Tile - Shadow's definition has roll 384; Live's 2,053 copies sit at 768 orientations". Note the Meleen Estarim painting and the 4 roll-384 tiles.
- :108: name `SendPacketToBootedZones`.
- :50 (T12) and :458 (D11): world observes property-changed events with owner changes, or D11 balances on the world tick.
- :151, :321, :447, :627; ruletypes.h:1184; housingSchema.sql:129; testHousingSchema.py:92: remove "Live: hourly". State the owner's cadence once it is chosen.
- :153-155, :449, :468 (D7) and :513: per owner question 2. Add `Housing:UpkeepPercent` (default 100), and state how world reads each host zone's ruleset (:444, :203). Drop "smaller plots" from X3.
- :161-162: MaxPets rules with -1 = none; declare them in ruletypes.h.
- :167 is correct. Fix :458 and :505 to definition 2.
- :177-183 and :462: Live's masks and names (Everyone and Visitor: Receive Buffs + Enter; Acquaintance: + Interact Item; Friend: + Add, Move and Interact Item; Trustee: everything but crate and house add/remove, Manage included; Co-Owner: everything; Owner: 0).
- :201: X1 interiors need no region (as :351 says).
- :217 and :224: the past tense; 3 bytes; MaxPerRealEstate -16777216; rof2.cpp:6915-6950. :215 and RealEstateItemsWnd.md:164: name `+0x609`/`+0x60c` as SocketSubClassCount/SocketSubClass[10] (send 0, never more than 10). RealEstateItemsWnd.md:155: "no wire limit; the client keeps 31 characters".
- :230: "mapped since f151f36ec".
- :266, :565 and :201: Enter is type 1 on `0x218c`. Rename `0x050c` to a generic use/proximity click (type 8: Use and the aura toggles).
- :313, :333, :600 and :615: 23 tables, 8 catalog tables, 22 repositories, custom database version 3. Drop "the real `inventory` definition".
- :317: `dz_id` is also set on the neighborhood (20504). :337: six exterior styles for three-room houses and manors, four for one-room houses.
- :341: nothing prints unless both the property and its item list are loaded; then 5289 for a non-empty name, 5288 for an empty one.
- :345-349: name the composed record name (owner question 6).
  - The handle is the string at `+0x30`; `+0x34` is 0.
  - `+0x18` is -1. RoF2 indexes it only when positive (`0x5b5a90`). Code fix: housingMessages.cpp:9 currently sends 0.
  - The guild goes out as -1 when none.
- :347 and :353: the origin and positions are y first.
- :349, :353 and :447: the expiry stamp semantics (contradiction 1 and cross-chunk item 4). The expiring pass stamps and stores in the same second, houses stay placed, and refunds clear the stamps. Route this to chunk 6.
- :372: the `cost set` scope, per owner question 18.
- :383 and :461: "removing a NO DROP item requires being its owner (0x40); removing a LORE item is refused if the taker already has one (0x41); adding needs the add permission". :383 also needs `MaxPlotsPerCharacter` checked on transfer acceptance with eqstr 761, and the purchase refusal named (eqstr 728/750).
- :384, :388, worldLayer.md:55 and the summary: current counts with their populations. Cite the clients' item-actor tilt functions, and use effective equip slots (item AND augments; 0 for non-common types) for the turn.
- :385, :388 and worldLayer.md steps 6 and :98:
  - no `0x1045` for an NPC item whose spawn exists;
  - `+0x98` = NPC flag OR `placeable_ignore_collisions`;
  - `+0x0c` = the item id;
  - the model is `IT` + (`ornament_idfile` or `idfile`) + `_ACTORDEF`;
  - `0x590dd0` reads the binding and `0x5a0000` resets it.
- :390: drop the pool from the respawn sentence.
- :394:
  - Manage (type 0, 722) comes after Inspect;
  - Move to Storage on a house or hall only while it is empty;
  - Enter only with permission, in the item's zone, and while the plot's upkeep is current, never on storage crates;
  - the aura entries are guild-only, with b 1 and b 2;
  - "18 guild member menus";
  - fix the expired-plot sentence.
- :408: fix the pool description and the "fallback to NPC names" (113 of 113 have Live housepet names).
- :437 and :467 (D6): selling packs the plot's **placed** crate into the seller's inventory, after the client's sell gate (codes 3, 4, 5, 6, 10, 11 and 12). The Crate of Eviction is only for evictions, and the server fills it regardless of its 0/0 caps (say so at :543). Cite eqstr 7634 for the escrow return.
- :449: per owner question 8.
- :505, :509 and :533: rewrite from makeHousingCatalogSql.py's docstring and the three Live plot tiers. State the carved tables and trophies plainly: Live's current items name upkeep 2 and 4, and Live's cost table has neither.
- :513: the Modest's door exists on Live. Adopt Live's safe points.
- :517 and :564; serverWork S13: definition 30 is Hermit's Hideaway in phinterior1d1 (sold on Live). Hive of the Bixie, The Wayward Lady and the Aureate Chalet have no RoF2 zone, so they need X1 substitute interiors or are dropped. Recount S13.
- :626: "75 property records (71 plots, 4 guild plots)".
- :640: drop the settled items (tilts, heading offset, the respawn unit).
- D3 (:137): the instance ceiling. D3 and S10 should agree per owner question 5.
- T21 (:59): it also covers housing door types 159 and 160 (the border plaque opens that plot's 0x2691 single-property reply, and the portal opens the Neighborhood window). zone/doors.cpp:547 currently handles only 57-58.

**serverWork.md, flows and the summary:**
- S2: "upkeep expired time" in place of "upkeep current".
- S5: the world object *or* the spawn.
- S7: correct already. The plan follows it.
- S10: per owner question 5.
- S13: as above.
- "To verify": replace the plot sign item with the border-door click.
- "Open design decisions": remove the decided items.
- flows/placeItem.md, editPlacedItem.md, movingCrate.md: `0x1045`/`0x2a9e` for ordinary items; spawn and binding for NPC items; F only for NPC items; "world object not found by GUID".
- flows/purchase.md:30, :104: the sign is the border door.
- The six polarity docs listed in contradiction 1.
- placedItemsAndProperties.md:86 and :91-93.
- The summary's sections 1 and 4, per cross-chunk item 21, plus a chunk 2 entry.

## Settled from the dumps (gaps the audit closed)

- **Whether expired items stay placed** (summary section 1): 22 stamped items, of which 19 are stored and 3 are placed houses. At expiry, Live stamps the property and every placed item with an upkeep id in the same second (44730591 at 1790666988). It stores the non-house items (193 and 194 went placed 1 -> 0) and leaves houses placed. A refund resets the placed items' stamps to 0. Stored items keep old stamps (bristle 133141063 has 4 items stamped 2022-11-22 while funded). Source: propertyItems.tsv, `verify4/expiry.py`.
- **An expired house on a paid plot** is never reclaimed. Cazic 33217796 was expired 172 days and still placed.
- **Access-name sizing** (G3-17): the special keys are `_Fellowship` and `_Guild1`-`_Guild8` (at most 11 characters). 53 of 4,931 lists hold names, at most 116 each. Character names run to 15 characters. VARCHAR(64) is enough.
- **Live's default access masks:** 3, 3, 0x803, 0xe03, 0x1f87, 0x1fff, 0, on 7,962 of 7,993 lists.
- **Live's upkeep shape:** about every 12.5 h per property, prorated, not hourly. Only the character's own properties are valid evidence.
- **Pet caps on Live:** -1 on every house, hall and crate. NPC items are common there: up to 42 in a house, 143 in a Palatial hall, 40 in a crate. Yards never exceed their caps (6 and 12).
- **Plot tiers:** 120/6 on 25 plots, 105/5 on 39, 90/4 on 7; guild plots 210/0/12.
- **75 children on every neighborhood record** (7,808 of 7,808).
- **The handle is per account,** and no character ever has two. staticId is -1, or 0 on 5 vacant plots.
- **Option numbering:** Live sends 1-based component ids, 0 = none. RoF2 needs key == position, so keep 0-based keys (my own disassembly).
- **Interior and zone mappings:**
  - house finishes 1a1/1a3/3a1/3a2/3a3 as the catalog has them;
  - 1d1 is Hermit's Hideaway;
  - Live runs hall interiors as guildhall_int (2937 on base 345), guildhall3_int (2943 on 751) and guildhallsml_int (2936 on 738);
  - safe and arrival points for 13 zones (values in Plan fixes).
- **Hall doors:** Palatial 27, Grand 1, Modest 1, Standard 1. **Guild square** door positions (226-229).
- **The plot sign** is not a world object. It is the type-160 border door carrying `RealEstateDoorID` 101-173 (187-190 for guild squares). The void portal and the Guild Lobby gate carry 1 and open the Neighborhood window.
- **World objects:**
  - Live keeps none for a bound NPC item;
  - every placed object's defaults equal its item's;
  - `bIgnoreCollisions` equals the item's flag;
  - model names are `IT<idfile>_ACTORDEF`, with ornaments overriding on 1,063 objects;
  - the 5 odd-copy tilts are explained by augments (both clients' effective-slot getters).
- **Menus:** Enter is type 1. Every NPC item listed for authoring has a Live housepet name (113 of 113). The Augmentater's counts are cycle steps.
- **Carved tables (2) and trophies (4):** Live's current items name these upkeep ids, and Live's cost table has no element for either.
- **Selling returns escrow and packs the placed crate** (eqstr 7634, both clients).
- **Currencies:** Live's alternate currency ids 10, 11, 12, 14 and 15 equal Peridot's `alternate_currency` (Orum, Faycetum).
- **A second Live item source** exists (itemsTxtExtract.tsv, 17,371 ids, with 255 meaning -1) for adopting values on Peridot's uncaptured placeable items.

**Scripts** are under `<scratchpad>\audit\` in `c1`, `chunk2`, `v1`, `verify2`, `v3`, `verify4`, `c5` and `v5`. My own checks are `final\findModel.py`, plus `fnDis.py` runs on RoF2 `0x5b4780`, `0x7fb930`, `0x7fb970`, `0x743c45`, `0x74d2ec` and `0x4b2f40`.