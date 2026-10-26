# GUID worklist

The work for Live-style item GUIDs ([README.md](README.md) has what Live does and the design). Statuses: `fix` (write code, SQL or docs; the content is given), `liveCheck` (observe Live), `peridotTest` (run on peridotguids), `question` (the owner decides), `polish` (after the first pass works), `done` (kept for the record).

**Counts.** fix 24, liveCheck 0, peridotTest 6, question 0, polish 4, done 7; 41 in all.

## Owner decisions

- **GUIDs first (2026-10-25):** the work is GUIDs and PlayerHousing, equal peers, GUIDs first: this branch (`guids`, from `main`'s tip 6fedcae84) with its own migration, tested on peridotguids (a new clone of Peridot); housing on `housing` is paused until `guids` is merged into `housing` and migrated, then reworked on top (its REC-16 to REC-18, DOC-39).
- **Live dictates.** Everything Live shows is copied exactly, the server stamp included; EQEmu's own structures are kept where they serve (side tables keyed by GUID, snapshots, the session serial).
- **Every decision in README.md's "The design"** was made with the owner on 2026-10-24/25: the format and world rule, the item table and its derived counter, side tables with foreign keys, the receiver rule and the single issuing path, the quest API, stacks and use, the nine retirement reasons and `World:RecoverableItemStates`, the session serial, the encoders, snapshots with skip-and-log plus `reissue`, and the migration's holder counter plus Live's server stamp.

## Process (PRC)

- **PRC-1** `done` **The branch and its docs.** `guids` from `main`'s tip 6fedcae84 in its own worktree (first cut from 954266eaa, a rewritten copy of the same tree, and rebased so a merge into `main` carries only this branch's commits); this folder.
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
- **ISS-3** `fix` **Retirement reasons.** destroyed, sold, stacked, consumed, expired, decayed, characterDeleted, removedByGM, rolledBack, petDonated, botDonated, set at every point an item leaves play (destroy, sell, merge, combine, hand-in, task delivery, charges, no-rent and temporary expiry, corpse rot, ground despawn, hard character delete, GM removal, rollback). A dropped item lives only in the zone's memory (`Client::DropItem` makes a `Temporary` object with a five-minute decay and never writes `object`), so its row says holder `ground` with the zone and position while it lies there, retires `decayed` when the decay fires, and, since a zone that dies loses it without a word, each zone retires any row still marked `ground` in that zone as `decayed` when it boots.
- **ISS-4** `fix` **No all-zero item.** An item instance without a GUID is a server error; things that aren't items yet are sent as zeros.
- **ISS-5** `fix` **Moves are written through, exchanges are transactions, counter collisions retry.** Every move writes the item's row at once, as EQEmu writes `inventory` today (one `SaveInventory` per slot change); a move between holders is one `UPDATE` of the row's location, never a delete and insert, which is what makes one item, one place hold. An exchange that moves several rows together (a trade, a Bazaar buy with its coin, a parcel retrieve with a bag's contents, a merge) runs in one database transaction, so a crash mid-way leaves either the old state or the new. The issuing path's `INSERT ... SELECT MAX(third) + 1` retries a key collision up to three times, then fails loudly; the collision is logged with the character and the two counters.
- **ISS-6** `fix` **Items a pet or bot takes out of play retire with their own reasons.** A pet's items (holder type pet, written when the pet is given them and kept while it is suspended, as `character_pet_inventory` does today) retire as `petDonated` when the pet dies, is dismissed or depops: EQEmu makes no corpse for an owned NPC (`!HasOwner()` in `NPC::Death`), as Live makes none. A bot's items stay held through its death; deleting the bot retires them as `botDonated`. Both reasons may be listed in `World:RecoverableItemStates` (Live has no bots; the default list stays `destroyed,decayed`). Mercenaries hold no item instances in RoF2 (no mercenary inventory traffic; EQEmu's `merc_inventory` is a content template), so there is nothing to stamp or retire for them.

## Trust (SEC)

- **SEC-1** `fix` **A client-sent GUID is a selector, never an authority.** The server decides whose items a packet may name from the session, and the GUID only picks one item within that scope. One scoped lookup in the item layer, `lookup(scope, guid)`, serves every handler; no handler runs a bare lookup by GUID on the item table. The scopes: the character's own held items (inventory, bank, cursor, their trader listings, their parcels); the current listings of the trader a Bazaar request names (buy and inspect, as Live allows); the character's own retired items (the Recovery tab). A GUID outside the scope, an all-zero or placeholder GUID (`{0, 0, n}`, what the client gives a locally split stack), or text that fails the round-trip decode is refused and logged at Warning with the character, the GUID and the scope tried; what follows is the server owner's call. Every update, move, listing, sale, destruction or recovery follows only from a scoped lookup, so one expressed against an item the character does not hold cannot be written. The paths on this branch: Bazaar buy and inspect, trader price update, evolving XP transfer, parcels, the Recovery tab.

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
- **MIG-2** `fix` **Pre-GUID inventory snapshots are cleared.** They name items by slot and item number and map to no row, so the migration recreates `inventory_snapshots` in the GUID shape, empty, and says so in its notes; Peridot has none (the feature is off there), so nothing is lost. Tested with peridotguids' `seeded` state (peridotguids.md): three pre-GUID snapshots on Claudeguids, gone after the migration, and `#invsnapshot capture` and `restore` working on GUID rows after it; the owner's decision 2026-10-26, so a PR to upstream carries a tested path.

## Snapshots and recovery (SNP)

- **SNP-1** `fix` **Restore by GUID.** Held items move back and take their snapshot values; retired items come back; `stacked` pieces only with their stack; items acquired since retire `rolledBack`; items held by others are skipped and logged (GUID, item, holder).
- **SNP-2** `fix` **`#invsnapshot restore <timestamp> reissue`.** Skipped items are recreated with new GUIDs from the restored character's counter; the log pairs old and new.
- **SNP-3** `fix` **`World:RecoverableItemStates`.** The rule (default `destroyed,decayed`), loud on an unknown name, refusing `stacked`. The Recovery tab itself (merchant tab, recovered rows keeping their GUID) is built with it.

## Tests on peridotguids (TST)

- **TST-1** `peridotTest` **Issuing.** Loot, purchase, combine, summon on another player, split, merge, trade, drop and pickup, NPC return: the GUIDs follow README.md's tables.
- **TST-2** `peridotTest` **The counter.** Consecutive, never reused after a destroy, survives zoning, crashes and restarts.
- **TST-3** `peridotTest` **The migration.** On peridotguids' `seedComplete` state (Peridot's data plus the seed set, peridotguids.md): every item has a row, a rerun gives the same GUIDs, server stamps decode as Live's do.
- **TST-4** `peridotTest` **Snapshots.** A restore with an item traded away (skipped and logged), with `reissue`, and undone by the "before restore" snapshot.
- **TST-5** `peridotTest` **Spoofed GUIDs are refused (SEC-1).** With two characters on peridotguids (Claudeguids and a second on a new account made at its first login, so nothing of Peridot's data is touched), each attempt is refused, nothing changes, and the log carries the character, the GUID and the scope tried: a Bazaar buy naming trader A's id with a GUID listed by trader B, and with a GUID B holds but has not listed; a Bazaar inspect naming a GUID in B's inventory but not listed; a price update on a GUID the character does not list; an evolving XP transfer naming the other character's evolving item as source or destination; a parcel retrieve naming another character's parcel; a Recovery request for a GUID another character destroyed; an all-zero GUID, a placeholder `{0, 0, 1}` and text that fails the round-trip decode, on the buy path. The real client cannot be made to send these, so each case is a crafted packet in the RoF2 shapes sent through the link; that sender is part of this item.
- **TST-6** `peridotTest` **Every item holder, before and after the migration.** On the `seedComplete` state, each holder has pre-migration rows and the same items are found after, each with a GUID, its side-table links intact and no row lost or duplicated: inventory (worn, general, cursor, cursor queue, bank, general bag contents, bank bag contents), shared bank with a bag's contents, guild bank, augments in their parents, attuned, tinted, custom-data and ornamented rows, charged items, evolving items across the XP range (held, banked, on a corpse), corpse items (a full corpse, one with an item from the cursor slot, one seeded by hand), parcels (plain, a bag with an augmented item inside, one for a soft-deleted character), a suspended pet's inventory, a soft-deleted character's items, ground objects with an item, forge contents, pooled merchant rows (one from a real sale), and inventory snapshots. Not seeded, with the reason recorded: `trader` (world empties it at boot), player drops (never written to `object`), `bot_inventories` (Peridot has no bot tables; the migration skips absent tables and says so). Post-migration function through the client: the second character becomes a trader in the Bazaar with a satchel of priced items (a stack, an augmented item, an evolving item); Claudeguids searches, inspects the listing and sees the real instance with its augment, buys one, and the bought item carries the seller's GUID as Live's do; the trader reprices and ends trading; `#invsnapshot capture` and `restore` work on GUID rows. The checks are a script over the database (row counts per holder before and after, every pre-migration item matched to exactly one GUID row by holder, slot and item number), plus the client steps; the script is part of this item.

## Research (RES)

- **RES-1** `done` **Live's format, issuing rules and server stamp.** README.md's "What Live does", from the Live tests of 2026-10-24/25 and the RoF2 and Live clients.
- **RES-2** `polish` **The Token of Reclamation test.** What Live's Recovery covers beyond player destroys, and whether a recovered item keeps its GUID (the owner may supply a token).
- **RES-3** `polish` **Does `decayed` ever happen on Live,** given its corpse recovery options? A discussion for the polish pass.
- **RES-4** `polish` **Which sold item a pooled merchant row shows:** the first sale or the latest (two characters sell the same item to one merchant).
- **RES-5** `polish` **The scope of Live's session instance counter** (per zone process or wider). Not needed by the design; a dump, zone out and back, dump.
