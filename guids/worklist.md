# GUID worklist

The work for Live-style item GUIDs ([README.md](README.md) has what Live does and the design). Statuses: `fix` (write code, SQL or docs; the content is given), `liveCheck` (observe Live), `peridotTest` (run on peridotguids), `question` (the owner decides), `polish` (after the first pass works), `done` (kept for the record).

**Counts.** fix 20, liveCheck 0, peridotTest 4, question 1, polish 4, done 7; 36 in all.

## Owner decisions

- **GUIDs first (2026-10-25):** the work is GUIDs and PlayerHousing, equal peers, GUIDs first: this branch (`guids`, from 954266eaa) with its own migration, tested on peridotguids (a new clone of Peridot); housing on `claude` is paused until `guids` is merged into `claude` and migrated, then reworked on top (its REC-16 to REC-18, DOC-39).
- **Live dictates.** Everything Live shows is copied exactly, the server stamp included; EQEmu's own structures are kept where they serve (side tables keyed by GUID, snapshots, the session serial).
- **Every decision in README.md's "The design"** was made with the owner on 2026-10-24/25: the format and world rule, the item table and its derived counter, side tables with foreign keys, the receiver rule and the single issuing path, the quest API, stacks and use, the nine retirement reasons and `World:RecoverableItemStates`, the session serial, the encoders, snapshots with skip-and-log plus `reissue`, and the migration's holder counter plus Live's server stamp.

## Process (PRC)

- **PRC-1** `done` **The branch and its docs.** `guids` from 954266eaa in its own worktree; this folder.
- **PRC-2** `done` **peridotguids.** VM 145, a clone of Peridot set up as peridothousing was, building this branch; in facet's inventory, with a claudelink login, a RoF2 client copy and the snapshots `pristine` and `ready` ([peridotguids.md](peridotguids.md)).
- **PRC-3** `done` **README.md's current work.** The repository README names GUIDs as this branch's work, points to this folder and its worklist, and names peridotguids as the test server.
- **PRC-4** `done` **The stack comes back after a reboot.** The containers carried over from Peridot had their ports pinned to Peridot's address (`${IP_ADDRESS}` in docker-compose.yml, 10.111.111.211), which the clone does not have, so at its first reboot Docker could not bind them and `akk-stack-eqemu-server-1` and `akk-stack-mariadb-1` exited with 255. `docker compose up -d --force-recreate` re-pinned them to 10.111.111.239; Spire then starts the launcher at boot (`spire.launcher_start` true in eqemu_config.json). A reboot on 2026-10-26 brought back the four containers, world, the login server, UCS and 11 zones with nothing done by hand.
- **PRC-5** `done` **No monitoring on the test servers.** The owner decided peridotguids and peridothousing get no Zabbix and no Grafana: their `zabbix-agent2` and `alloy` (copied from Peridot) are stopped and disabled, and facet's inventory no longer checks them.

## Schema (SCH)

- **SCH-1** `fix` **The item table.** One row per item instance, keyed by the three raw words; item columns, location (holder type, holder id, slot, parent GUID, index), retirement state and time; restricted deletes; never written with `REPLACE`. The repository is hand-shaped or generated without `ReplaceOne` for it.
- **SCH-2** `fix` **The world column and rule.** `character_data` gains the 16-bit world id; a World rule (default 0) gives it; character creation stamps it.
- **SCH-3** `fix` **Side tables keyed by GUID.** `trader`, `character_parcels`, `guild_bank`, `character_evolving_items` and a ground-drop table move to GUID keys with foreign keys and drop their copied item columns; `object` keeps static content only.
- **SCH-4** `fix` **Absorbed tables dropped.** `inventory`, `sharedbank`, `character_corpse_items`, `bot_inventories`, `character_pet_inventory`, `character_parcels_containers`, `object_contents`, after their rows move to the item table; every load and save path that read them reads the item table.
- **SCH-5** `fix` **Snapshots by GUID.** `inventory_snapshots` rows carry the GUID (foreign key) and the copy of location, count or charges and data.
- **SCH-6** `fix` **Pooled merchant rows.** `merchantlist_temp` rows point at a sold item's GUID.

## Issuing and lifecycle (ISS)

- **ISS-1** `fix` **One issuing path.** Issue (derived counter, receiver rule), retire (with reason) and update live in the item layer; every creation site calls it: loot, merchant purchase, combine, summon (the target's counter), split, grants, quest rewards, starting items, bots and pets (the owner's counter), copied characters (new GUIDs).
- **ISS-2** `fix` **Stacks and use.** Split = new GUID from the splitter; merge = destination keeps, the piece retires `stacked`; partial use decrements; last unit retires `consumed`; partial sale or destroy splits first.
- **ISS-3** `fix` **Retirement reasons.** destroyed, sold, stacked, consumed, expired, decayed, characterDeleted, removedByGM, rolledBack, set at every point an item leaves play (destroy, sell, merge, combine, hand-in, task delivery, charges, no-rent and temporary expiry, corpse rot, ground despawn, hard character delete, GM removal, rollback).
- **ISS-4** `fix` **No all-zero item.** An item instance without a GUID is a server error; things that aren't items yet are sent as zeros.

## Quest API (QST)

- **QST-1** `fix` **Every item call goes through the issuing path.** The create, remove, update, return and not-an-item calls in README.md's table, Lua and Perl.
- **QST-2** `fix` **GUID calls.** `GetGUID()`, a lookup by GUID; `GetEvolveUniqueID` replaced by the GUID; `GetSerialNumber` kept as the session handle; no setter.

## Session serial and #4460 (SER)

- **SER-1** `fix` **The serial as a session handle.** Never saved; `instance_id` for every client; in-zone lookups keep it; RoF2's evolving special case (rof2.cpp:6533) goes; trader rows and evolving state use the GUID.
- **SER-2** `fix` **#4460 removed.** AddGUIDToMap, ClearGUIDMap, the save-and-reissue logic and the `guid` columns.

## Encoders (ENC)

- **ENC-1** `fix` **RoF2.** GUID text in the item header, the Bazaar structs and the player record (the character's own `{id, world, 0}`); incoming text decoded with the round-trip check, invalid text refused.
- **ENC-2** `fix` **RoF.** The same as RoF2. **Untested:** no RoF client (December 10, 2012) survives anywhere we could find; it rests on the alphabet RoF2 and Live share.
- **ENC-3** `done` **Older clients.** Titanium, SoF, SoD and UF have no GUID field; unchanged.

## Migration (MIG)

- **MIG-1** `fix` **Its own migration.** Every existing item and augment gets a row in a fixed order (table, then row key): character-held items take their holder's counter (bots their owner, parcels their recipient); the shared bank, the guild bank, ground drops and world-container contents take Live's server stamp, generated exactly as 0x80c890, with the process id and start time recorded so a rerun reproduces every GUID. Every character is stamped with the world rule; #4460's serials are discarded.
- **MIG-2** `question` **Existing inventory snapshots.** Taken before GUIDs, they map to no item; clearing them in the migration is the proposal.

## Snapshots and recovery (SNP)

- **SNP-1** `fix` **Restore by GUID.** Held items move back and take their snapshot values; retired items come back; `stacked` pieces only with their stack; items acquired since retire `rolledBack`; items held by others are skipped and logged (GUID, item, holder).
- **SNP-2** `fix` **`#invsnapshot restore <timestamp> reissue`.** Skipped items are recreated with new GUIDs from the restored character's counter; the log pairs old and new.
- **SNP-3** `fix` **`World:RecoverableItemStates`.** The rule (default `destroyed,decayed`), loud on an unknown name, refusing `stacked`. The Recovery tab itself (merchant tab, recovered rows keeping their GUID) is built with it.

## Tests on peridotguids (TST)

- **TST-1** `peridotTest` **Issuing.** Loot, purchase, combine, summon on another player, split, merge, trade, drop and pickup, NPC return: the GUIDs follow README.md's tables.
- **TST-2** `peridotTest` **The counter.** Consecutive, never reused after a destroy, survives zoning, crashes and restarts.
- **TST-3** `peridotTest` **The migration.** On a copy of Peridot's data: every item has a row, a rerun gives the same GUIDs, server stamps decode as Live's do.
- **TST-4** `peridotTest` **Snapshots.** A restore with an item traded away (skipped and logged), with `reissue`, and undone by the "before restore" snapshot.

## Research (RES)

- **RES-1** `done` **Live's format, issuing rules and server stamp.** README.md's "What Live does", from the Live tests of 2026-10-24/25 and the RoF2 and Live clients.
- **RES-2** `polish` **The Token of Reclamation test.** What Live's Recovery covers beyond player destroys, and whether a recovered item keeps its GUID (the owner may supply a token).
- **RES-3** `polish` **Does `decayed` ever happen on Live,** given its corpse recovery options? A discussion for the polish pass.
- **RES-4** `polish` **Which sold item a pooled merchant row shows:** the first sale or the latest (two characters sell the same item to one merchant).
- **RES-5** `polish` **The scope of Live's session instance counter** (per zone process or wider). Not needed by the design; a dump, zone out and back, dump.
