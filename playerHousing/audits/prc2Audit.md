# PRC-2 audit: serverPlan chunks 7 and 8, and the housing code

Run on 2026-11-18 at the `housing` branch's 9ced2648a, as the chunks 1-5 audit ran (chunks1to5Audit.md): ten agents in one multi-agent workflow, every finding checked by a second agent that tried to refute it. Two reviewers checked chunk 7 (S10-S12) and chunk 8 (validation) claim by claim against Live data, the RoF2 client and the code; three auditors read the housing code in three parts (the zone core with purchase, sale, transfer and escrow; placement, items and menus; the world side with upkeep, interiors, guild services, polls and the GM command) against the plan, the client's rules and Live; five verifiers re-checked each reviewer's or auditor's items and looked for what they missed. Everything was read-only. Live research was held (MacroQuest for the patched Live client was not ready), so only the Live data already on disk was used; what needs new Live data is routed as Live checks. Ghidra outputs are in `C:\Onyx\scratch\ghidra\prc2`. Every item, with its evidence and the verifier's verdict, is in `prc2AuditDetail.json`.

## Verdict per part

| Part | Items | Verifier |
|---|---|---|
| Chunk 7 claims | 53: 39 confirmed, 10 contradicted, 4 stale | 47 upheld, 6 partly |
| Chunk 7 gaps | 13 | 8 upheld, 4 partly, 1 overturned; 1 more found |
| Chunk 8 claims | 55: 39 confirmed, 12 contradicted, 3 stale, 1 unverifiable | 52 upheld, 3 partly |
| Chunk 8 gaps | 15 | 12 upheld, 3 partly; 4 more found |
| Code, zone core | 11 findings (3 medium, 8 low) | 8 upheld, 3 partly; 2 more found |
| Code, placement | 11 findings (3 high, 3 medium, 5 low) | 11 upheld; 2 more found |
| Code, world side | 8 findings (3 high, 2 medium, 3 low) | 6 upheld, 2 partly; 2 more found |

The contradicted and stale claims are plan text to correct: chunk 7's C7-05, C7-07, C7-14, C7-19, C7-23, C7-25, C7-27, C7-31, C7-34, C7-36, C7-40, C7-48, C7-50 and C7-53 (AUD-47), chunk 8's C8-01, C8-07, C8-09, C8-21, C8-22, C8-25, C8-29, C8-30, C8-34, C8-35, C8-38, C8-39, C8-51, C8-52 and C8-53 (AUD-54); C8-47 waits on INT-41.

**One correction to this branch's own work:** Live's captures (15 unnamed strangers, every one in a stranger's yard or inside a house on it) show a client holding the visited plot's records, access lists and item lists, its houses and crate included. ZIN-1's note that a stranger on a plot holds nothing of it 'as on Live' was wrong for the plot being visited (AUD-8).

## The reviewers' and auditors' summaries

**Chunk 7.** Most of chunk 7 checks out against the code, Live's data and RoF2: interiors as copies of their dz templates, entering, the exit point, /leaverealestate, house pick-up, the swap's codes, Store All, the latch, layout validation, anchors (437/438, 5329, landing height) and polls.

Statements that are wrong or out of date:
- the crate model: "a child property of its plot". On Live, 16 of 34 placed crates stand in a house or hall, and RoF2 searches the whole plot.
- the zone registration count: about 524, not 272.
- "PEQ has none of them".
- the layout file's fields.
- the poll prompt at zone-in. RoF2 shows it itself, once per vote window, not by 0x7a4d.
- 0x154a is never sent.
- T23's timer.
- the upstream note on 437/438.
- opting another member out of trophies (TRO-1).
- the "verified in game" line.

Main gaps:
1. **High.** A crate placed in a house is refused by Pack, Unpack, Move to Storage and the sale. The crate limit only looks at the destination.
2. **Medium.** A forged swap naming a house stored in the outgoing house's own closet leaves that house's item inside its own property.
3. **Medium.** DOR-17's renumbering left the Guild Lobby's quest script on PEQ's old door ids, and Live's door 1 route into the guild's own guild hall exists nowhere.

There are also low items: no source for anchors, Pack/Unpack skipping houses, anchor messages, the layout load's ids, poll robustness, exit corners, evidence for INT-41, untested interiors, trophy rule 70 and plan text.

The Ghidra outputs are in C:/Onyx/scratch/ghidra/prc2/chunk7*.txt.

**Chunk 8.** Most of the tool table checks out. I re-ran three tools read-only (itemTailDecode, regionMatrix, sunriseHillsPlots) and they pass. The counts match the code and data (24 tables, 570 NPC items and 42 gaps, 27 dbstr labels, 56/255/55 client lines, custom migrations 1-12). The 570 is still right after the 2026-11-16 adoption regeneration: 568 Live-named items plus 3 Peridot-only. Stage 1's seven items match Live's itemDefinitions.tsv.

What no longer holds:
- **Stage 2 (the gate):** since ZIN-1 (2026-11-18), zone-in sends only related records. Stage 2's '75 plot records' and standing-in expectations, its neighborhoodZoneIn suite, stage 0's '0 findings' and DOC-22's recorded result all fail. Under 'stop at the first stage that fails', a rerun now halts at stage 2.
- **Deploy step 3:** 'world:version' reads the binary's constant, not the database.
- **Logging:** the Housing log category ships off, not informational.
- **Deploy step 4:** import_client_files re-imports every client file in import/ and wipes housing's strings on any later run.
- **Deploy omissions:** the steps leave out the quest scripts, the client files and the guids side of Peridot's day.
- **Stage 3 setup:** lowering the interval alone stops world from starting unless the jitter is lowered too.
- **Wording:** 'cursorItem is the only plugin change', 'each guard', 'each refusing a second run' and three of the six verification items are stale.

Missing from the stages:
- forged-request checks;
- guild housing, the copy lifecycle, GUID stability, the expiry lifecycle and reclaims, all already run ad hoc;
- a property poll;
- a two-process concurrency case;
- load tests for the two content files changed after PRC-3.

Two items need Live data and wait for the MacroQuest patch: what a client holds on a stranger's plot, and an empty neighborhood's discard delay. Scratch scripts are in C:\Users\claude\AppData\Local\Temp\claude\c6b88703b813b4ef\129dfa97-1ed9-57d8-b32c-7209d4647dee\scratchpad\c8.

**Zone core.** I read the core housing code at 9ced2648a against serverPlan.md, the client documents, worklist.md and the Live dumps. I also ran one Ghidra query on the RoF2 deposit path (0x741ee0); the files are in C:\Onyx\scratch\ghidra\prc2\, named auditCoreDeposit.cmd, .txt and .log.

Checked, nothing to change:
- **ZIN-1 session logic** (relatedIds, knownIds, setClientView, refreshClients, revealProperties) is consistent. Records go only to clients that know the property. Item and access lists go only to related clients. Removals cover deleted properties and dropped lists. The server's own permission checks use every access list the session loaded, so permissions are not limited to what the client was sent.
- **Wire formats** match the documents: catalog, property record, ID lists, regions, the 0x2691 replies, the 0x2aa6 account message, the access list and its removal, and the transfer offer.
- **Transactions**: nesting and row locks are used correctly. lockedProperty takes its lock before the first ordinary read. No MySQLRequestResult is iterated twice.

Problems found:
- **A-01:** On a player plot, `_Guild<rank>` access entries never match on the server, although the client and Live (ACC-3) match them.
- **A-02:** A deleted guild's guild plot reads as vacant and is sold with its hall, items and escrow still on it.
- **A-03 (needs Live):** No Trade alternate currency can move between accounts through escrow.
- **A-04 (needs Live):** Since ZIN-1, plots whose Everyone group grants permissions the client checks itself no longer reach strangers.
- **A-05:** A deleted or renamed character left on an access list blocks every later edit.
- **A-06:** Since UPK-27, coin moved in memory survives a rolled-back sale, withdrawal, purchase or deposit.
- **A-07:** Two officers buying guild plots at the same moment can both pass the guild limit.
- **A-08:** A GM's Sell on someone else's plot gives the GM the owner's crate and escrow.
- **A-09:** After a catalog reload, purchases and upkeep choices use a catalog the client has not received.
- **A-10:** The balance read is unchecked, and the wire reader accepts negative string lengths.
- **A-11:** Several passages of the plan and one flow document no longer match the code.

**Placement.** Audited housingPlacement.cpp, housingPlacementRules.h, the menus, containers, world objects, NPC items, the item tail and the RoF2 housing serialization. I checked them against serverPlan.md, the RoF2 rule sets and Live's item data. The rule sets are 0x5b5ff0, 0x5b69b0, 0x5b6eb0, 0x5b48b0/0x5b45b0 and 0x5b72c0; I disassembled them where the decompile hid the arguments (C:\Onyx\scratch\ghidra\prc2\placementDecomp1.txt, placementDecomp2.txt, placementDisasm2.txt).

Three findings are high:
- B-01: moves between plots skip the client's remove rule, so a Friend can move a neighbour's item to their own plot and take it.
- B-02: storing someone else's stack merges it into the mover's own stack, so the mover becomes its placer.
- B-03: a forged house swap makes a house its own parent and breaks its owner's housing data.

Three are medium:
- B-04: a crate placed in a house is ignored by Pack, Unpack and Move to Crate, and the one-crate limit is checked per destination rather than per plot.
- B-05: Move to Closet picks the highest-id house child, which can be a stored house.
- B-06: Move to Inventory chooses its free slot before the open container returns its contents into that slot.

Five are low (B-07 to B-11).

No defects found in:
- the housing tail and spawn binding serialization;
- world objects (0x1045 updates by GUID, so a move between plots leaves no stale copy);
- NPC spawn and respawn bookkeeping;
- the menus' gates;
- the guards in ContainerTakesItems.

**World side.** Eight findings. Three are high. C-01: a character saved inside a house that was later stored, sold or reclaimed logs in to a new copy and can't leave it. C-02: the guild bank never works in the Palatial, Grand or Modest hall (737/738/751), because EQEmu only builds its guild bank manager in zone 345. C-03: placing an item whose upkeep id has no cost (Peridot's two carved tables, id 2, and twelve trophies, id 4) makes every charge, deposit and Add Day on that plot fail; Live charges and stamps none of them. Two are medium. C-04 needs Live: a deposit that leaves the escrow short still clears the stamp and restarts the 90-day reclaim clock. C-05: with Housing:EnableUpkeep off, the time is still billed at the next deposit, and in one lump when the rule is turned back on. Three are low: C-06, house records not re-sent when stamped or cleared; C-07, anchor failure messages, needs Live; C-08, a poll label can be silently overwritten. These read clean against the plan and Live: neighborhood creation and the D11 balance, village names, copy numbering and the sweep, entering and front-door exits for a placed house, the anchor offer/answer checks, poll vote validation and ranking, the Augmentater, the writ path and the quest API. One Ghidra query ran with the prc2Lock taken and released; its output is C:\Onyx\scratch\ghidra\prc2\auditWorldAnchorStrings.txt and its command file is C:\Onyx\scratch\ghidra\prc2\auditWorldAnchorStrings.cmd. Nothing in any repository was changed.

## Routed

Every upheld or partly upheld item went to worklist.md's audit section (AUD-1 to AUD-59), merged where two agents found the same thing. G7-12 (a forged commit placing a guild trophy without rank permission 29) was overturned: PLC-21 already holds it for the trophy step.

| Entry | Audit items | Severity | Routed as | Title |
|---|---|---|---|---|
| AUD-1 | B-01 | high | fix | Moves between plots skip the client's remove rule |
| AUD-2 | B-02 | high | fix | Storing another player's stack merges it into the mover's stack |
| AUD-3 | C-01 | high | fix | A login into a new copy of a house that stands in no yard leaves the character stuck inside |
| AUD-4 | C-02 | high | fix | The guild bank never works in the Palatial, Grand and Modest halls |
| AUD-5 | C-03 | high | fix | An item upkeep id with no cost (2, 4) breaks its plot's upkeep |
| AUD-6 | world-missed-1 | high | fix | Houses reclaimed into a Crate of Eviction stay stamped expired |
| AUD-7 | B-04, G7-01 | high | fix | A crate placed inside a house is never the active crate, and one crate per plot is checked per destination |
| AUD-8 | core-missed-1, A-04, G8-14 | medium | fix | The visited plot reaches whoever stands in its yard or is inside one of its houses |
| AUD-9 | A-01, core-missed-2 | medium | fix | `_Guild<rank>` entries never match on a player plot, and the property window sends the plot's guild as none |
| AUD-10 | A-02 | medium | fix | A deleted guild's guild plot sells with its hall, items and escrow |
| AUD-11 | B-03, G7-02 | medium | fix | A forged house swap can make a house its own parent |
| AUD-12 | B-05, chunk7-missed-1 | medium | fix | Move to Closet stores into the plot's highest-id house, which may be a stored one |
| AUD-13 | B-06 | medium | fix | Move to Inventory picks its free slot before the open container returns its contents |
| AUD-14 | placement-missed-1 | medium | fix | A placed item moved to another property is never recorded as withdrawn |
| AUD-15 | C-04 | medium | liveCheck | A deposit that leaves the escrow short still clears the expiry |
| AUD-16 | G7-03 | medium | fix | The Guild Lobby's quest script after DOR-17, and Live's door 1 into the guild's own hall |
| AUD-17 | G8-01 | high | fix | Stage 2's expectations and suite predate ZIN-1 |
| AUD-18 | G8-02 | medium | fix | import_client_files empties every table it finds files for |
| AUD-19 | G8-03 | medium | fix | The deploy steps omit the quest scripts, the client files and the guids side |
| AUD-20 | G8-04 | medium | fix | No stage sends a request the stock client would not |
| AUD-21 | G8-05 | medium | fix | The stage table lags behind what the shakeout has run |
| AUD-22 | G8-06 | medium | fix | The Housing log category ships off |
| AUD-23 | A-03 | low | liveCheck | NO TRADE alternate currency crosses accounts through escrow |
| AUD-24 | A-05 | low | fix | A deleted or renamed character on an access list blocks every later edit |
| AUD-25 | A-06 | low | fix | Coin moved in memory survives a rolled-back act |
| AUD-26 | A-07 | low | fix | Two simultaneous guild plot purchases both pass the guild limit |
| AUD-27 | A-08 | low | fix | A GM's Sell on someone else's plot gives the GM the owner's crate and escrow |
| AUD-28 | A-09 | low | fix | After a catalog reload, prices resolve against a catalog the client has not received |
| AUD-29 | A-10 | low | fix | An unchecked balance read, and negative string lengths accepted |
| AUD-30 | A-11 | low | fix | Plan text the core code no longer follows |
| AUD-31 | B-07 | low | fix | A request naming an equipment slot places or stores a worn item |
| AUD-32 | B-08 | low | fix | Store, commit, swap and Mail to Owner act on a source row read before the transaction |
| AUD-33 | B-09 | low | fix | The pick-up limit treats guild halls and houses as different kinds and reads a lagging carried list |
| AUD-34 | B-10 | low | fix | Two corrections to PLC-19's reading of the within-plot path |
| AUD-35 | B-11 | low | fix | A layout load accepts up to 65,535 records |
| AUD-36 | placement-missed-2 | low | fix | Every placement sends a property-changed event for the destination |
| AUD-37 | C-05 | low | fix | Housing:EnableUpkeep off does not pause upkeep |
| AUD-38 | C-06 | low | fix | Stamping and clearing change house records, but only the plot's record is re-sent |
| AUD-39 | C-07, G7-06 | low | fix | Anchor failures: the wrong line on a self gate, and nothing to the caster of a failed cast |
| AUD-40 | C-08, world-missed-2, G7-09 | low | fix | The poll commands: option relabels another poll's option, and unoption with votes breaks every poll |
| AUD-41 | G7-04 | low | fix | Anchors have no in-game source on Peridot |
| AUD-42 | G7-05 | low | liveCheck | What Pack and Unpack do with houses |
| AUD-43 | G7-07 | low | fix | A layout load trusts its yard and house ids |
| AUD-44 | G7-08 | low | liveCheck | Exit points of the plots Live was never measured on |
| AUD-45 | G7-10 | low | fix | A carried property keeps the name of where it last stood (INT-41) |
| AUD-46 | G7-11 | low | peridotTest | Six interior templates never entered with housing data |
| AUD-47 | G7-13 | low | fix | Chunk 7 plan text corrections |
| AUD-48 | G8-07 | low | fix | Content files changed since their last whole-file load, and the proofs table |
| AUD-49 | G8-08 | low | peridotTest | Stage 9's concurrent edits were never contended |
| AUD-50 | G8-09 | low | fix | The custom database version check reads the binary's constant |
| AUD-51 | G8-10 | low | fix | Stage 3 lowers the upkeep interval without the jitter |
| AUD-52 | G8-11 | low | fix | No guard that each option key equals its position |
| AUD-53 | G8-12 | low | peridotTest | Property polls and the account-wide ballot in game |
| AUD-54 | G8-13 | low | fix | Chunk 8 statements that no longer hold |
| AUD-55 | G8-15 | low | liveCheck | How long Live keeps an empty neighborhood copy |
| AUD-56 | chunk8-missed-1 | low | fix | Stage 9 expects revisions the client never carries |
| AUD-57 | chunk8-missed-2 | low | fix | PAT-8's result: eqStrings.sql has no second-run guard |
| AUD-58 | chunk8-missed-3 | low | fix | Stage 6 changes MaxHousesAdditive without a catalog reload |
| AUD-59 | chunk8-missed-4 | low | fix | A failed access-list save is logged as a player refusal |
