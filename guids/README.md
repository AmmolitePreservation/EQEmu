# Item GUIDs

Live-style item GUIDs for Peridot's EQEmu: every item instance carries the 12-byte identity Live gives it, from the moment it enters a character's possession until it leaves play, and keeps it through every move. This is the first half of the current work, **GUIDs and PlayerHousing**: GUIDs come first, on this `guids` branch with their own migration, and housing (the `housing` branch's `playerHousing/`) is reworked on top of them once this branch is merged and migrated.

The rule is the same as housing's: Live's data and client dictate the implementation. Everything under "What Live does" was observed on Live or read from a client; "The design" records the owner's decisions; [`worklist.md`](worklist.md) holds the work, the open questions and the polish items.

## What Live does

### The format

- **Text.** 16 characters over the alphabet `0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-`, six bits each, encoding 12 bytes. The same table is in the RoF2 client (2013, at 0xb61d20) and in today's Live client (file offset 0xd99f40). RoF2 decodes with 0x80c6a0 and encodes with 0x80c520; its setter 0x80c920 accepts only a string that survives decode and encode, and otherwise leaves the default `0000000000000000`. RoF2 also has 0x80c850 (a GUID built from three words) and 0x7b0ab0 (an item's GUID setter, the item's +0xd8).
- **Words.** The 12 bytes are three little-endian 32-bit words. Hex is wrong: housing's REC-3 and placedItemsAndProperties.md called it hex (DOC-39 on `housing`).
- **A character's GUID** for an item: word 1 is the stamping character's UniqueEntityID (EQEmu's character id), word 2 is the character's world id (16 bits) with a reserved half (16 bits, always 0), word 3 is that character's item counter. The player's own GUID is `{entity, world, reserved}`: Anastrel's, read from Live's PcClient (pLocalPC + 0x21e8), is `c32f220068000000`, entity 2240451, world 104. EQEmu sends RoF2's player record GUID (PcClient +0x2950) as all zeros today.
- **The world is the character's origin.** Characters from merged servers keep their original world: Pariator, on Bristlebane, stamps world 139, even on items bought in September 2026.
- **The server stamp.** Where no character applies, Live stamped GUIDs with a server-side generator, compiled into eqgame as 0x80c890 alongside the World guild bank loader's "WORLD: found an unset GUID on an item, correcting...": word 1 is the next value of `x = (16807 * x mod (2^31 - 1)) + 123`, word 2 a counter, word 3 the time of stamping. At process start (0x9b9530, 0x9b9550) the counter (0x15d3640) is set to `time() + GetCurrentProcessId()` and the sequence (0x15d363c) to `GetCurrentProcessId() XOR` that counter; each GUID takes the next sequence value and the counter's current value, then advances the counter. Checked against Live's eight server-stamped GUIDs: each same-server pair links exactly through the sequence (8, 212 and 11 steps, matching their counters' gaps), and solving back gives process ids 1844 to 3124 and every server starting in the same minute, 14:27-14:28 UTC on 2006-02-23 and 12:47-12:48 UTC on 2007-07-31, stamping within 0-37 seconds of starting: world restarts on patch days stamping ownerless items as they loaded. All eight survive as placed housing items. None of 5,077 captured held items (1,771 inventory, 2,939 bank, 367 shared bank) carries one.
- **Placeholders.** The client gives an item it split locally the stand-ins `{0, 0, 0}`, `{0, 0, 1}`, `{0, 0, 2}`... until the server sends the item again; it never makes a real GUID during play.
- **Uniqueness.** Of 945,606 placed items in the Live captures, GUIDs are unique per server except 39: 37 are the same item seen before and after a move, 2 are guild hall items shown in two places at once (Firiona Palatial Guild Hall, Bristle's Irregular Banner of the Guard). 6 appear on two servers, the same item type each time (transfers).

### Who stamps a new item

A new item instance takes the next number from the counter of the character who receives it, at that moment:

| Case | Observed |
|---|---|
| Loot | Items on a corpse carry no GUID (`0000000000000000`); the looter's counter stamps them when looted (Anastrel's Lightstone, 2250) |
| Merchant purchase | The buyer's next numbers, in order (2251, 2252, 2253-2257); a bought stack of 5 has one GUID |
| Player-sold stock resold | Live resells what players sell, pooled by item type (Diamond 224); a buyer gets their own new numbers (Gaulstaff 2326, 2327). The list shows the pooled row with a seller's GUID (210 of 210 limited rows; 5,894 of 5,894 unlimited rows are all zeros) |
| Tradeskill result | The crafter's counter (arrows, 2266) |
| Summon cast on another player | The target's counter (Gaulstaff's wand onto Pariator: entity 199176, world 139, 25223) |
| Split | The splitter's next number at the moment of the split, even for a piece consumed seconds later (eight one-each splits loaded into a kit took 2258-2265) |
| Server grant | The recipient's counter (ten Paragon's pieces at login, 2240-2249) |

The counter is per character, starts at 0, is consecutive and is never reused: a destroyed item's number is gone (the Torch, 2251, destroyed; the next purchase, 2252).

### What keeps its GUID

Trades in both directions (a stack of 26 Tiny Daggers, `moyY06W0003d0000`), Bazaar purchases delivered by parcel (the seller's stamp: Polished Apatite 2301041/261155, Elaborate Defiant Fire Bauble 353205/312), parcels, a ground drop and pickup (the Lightstone), an item an NPC hands back ("I have no need for this": the Small Lantern, 2252), bag and bank moves, placing and moving in a house, transfers between servers. An augment is an item of its own and keeps its own GUID inside its parent (a Coldstone, 1551, in an earring, 1078; the Fire Bauble inserted into a ring). A merge keeps the destination stack's GUID; the merged piece's GUID is gone.

### Things that aren't items yet

Live sends all zeros for unlimited merchant stock and for a corpse's loot before it is looted.

### World objects and dropped items

The client holds no item for a ground object, static or dropped. In the Plane of Knowledge on 2026-10-26 (Pariator, Bristlebane) the client's ground-object list held 37 entries: forges, ovens, looms, brew barrels, kilns, pottery wheels and the pickable ground spawns, every one with its `Item` pointer null, and 174 zone dumps from the Live tests of 2026-10-24/25 hold no ground object with an attached item either. A world container is not an item, and a ground spawn becomes one only when picked up.

A player's own drop is the same. On Firiona Vie on 2026-10-26, Xekirley (a new Iksar monk) looted a Snake Egg stamped `5fGS06y000070000` (entity 1872918, world 111, counter 7), dropped it in Crescent Reach, and the client's ground list gained object 319 (`IT63_ACTORDEF`, zonefile id none, weight 2, expiry 4500) with no item attached, while every other ground object kept the far-future expiry 2141454933; picked up again, the egg carried `5fGS06y000070000` unchanged. So the GUID lives on the server while an item lies on the ground and nobody, the owner included, can read it from the ground object; only the pickup brings it back (as the Lightstone showed on Bristlebane). Firiona Vie stamps world 111, and a new character's creation grants take its first counters: the egg, the monk's first loot, was its seventh stamp.

### Inspecting a Bazaar listing

Any listed item can be inspected from the Bazaar search window without opening its trader: with Pariator in the Plane of Knowledge on 2026-10-26, a search for "Spider Silk" returned 14 rows, and Inspect (`BZR_RequestItemButton`) on the first opened the item display for that listing (Bloodied Frozen Crystal Spider Silk) from a trader never visited. The request (RoF2 `BazaarInspect_Struct`) names the trader, the item's GUID and its item number. EQEmu answers it today with a fresh copy of the base item, never the listed instance.

### The per-session instance number

Besides the GUID, Live gives every item a per-session number (eqlib's `MerchantSlot` at the item's +0xf8): consecutive across one character's inventory as it is sent, new numbers after a relog, new items later in the session getting later numbers (Anastrel 81832095..., then 82795264... after a relog). It is a server counter advanced per item sent, not per login. Evolving items get it like every other item (seven tradeskill trophies, 84007286... inside the run 84006972-84007527).

### Recovery

Live's merchant window has a Recovery page listing the character's destroyed items (the Torch appeared; a sold Lightstone did not); recovering costs a Token of Reclamation.

## EQEmu today

- **The serial.** `GetNextItemInstSerialNumber` (common/item_instance.cpp) is a per-process counter; RoF and RoF2 write it as `{:016}` decimal into the item header's 17-byte string (rof2.cpp:6497) and as `instance_id` (or the merchant slot; RoF2 puts the evolving item's id there, rof2.cpp:6533). Titanium, SoF, SoD and UF carry only `instance_id`. RoF2's Bazaar structs (BazaarInspect, TraderItemSerial, TraderPriceUpdate, TraderBuy) carry `serial_number[17]`.
- **#4460** saves serials in `inventory`, `sharedbank` and `inventory_snapshots` `guid` columns and reissues fresh serials around them at load (AddGUIDToMap, ClearGUIDMap, shareddb.cpp:686, :893).
- **Item storage.** Items live in `inventory`, `sharedbank`, `inventory_snapshots`, `character_corpse_items`, `character_parcels` (+ `character_parcels_containers`), `guild_bank`, `trader`, `bot_inventories`, `character_pet_inventory`, `object` and `object_contents`; augments are item numbers in the parent's row (`augment_one`...). `character_evolving_items` keeps evolving state under its own id, matched to inventory at load by character and item number (shareddb.cpp ~800).
- **Whole-row saves.** `SaveCharacterData` reads the character row and writes it back with `REPLACE INTO` (zonedb.cpp:1062, :1165). Tested on MariaDB 10.5.4: a `REPLACE` on a parent row deletes its `ON DELETE CASCADE` children, and with the default restrict the `REPLACE` fails (error 1451); `INSERT ... ON DUPLICATE KEY UPDATE` keeps them.
- **No player-facing recovery.** The Recover tab values and `ItemPacketRecovery` are defined but unused (client_packet.cpp:14574 opens only Sell/Buy or Sell/Buy/Parcel). `#invsnapshot` (rule `Character:ActiveInvSnapshots`) is the GM rollback.

## The design

### Identity

- **The world id** is the rule `World:WorldID`, default 0 (0 is a valid single-realm id), which operators may set from 0 to 65535. `character_data.origin_world_id` (16 bits, migration 9327) holds each character's origin world, the `WorldUniqueID` of its own GUID (eqlib's `EqGuid`), not the world it is on now: character creation stamps the rule's value, refusing the character and logging an error when the rule is outside that range, and the migration stamps every existing character with it. Changing the rule never restamps a character; the world is its origin. Duplicate world ids are only detectable where two realms meet; a transfer tool refuses them. Each character is sent its own GUID in the player record.
- **No all-zero item.** An item instance always has a valid GUID; an item without one is a server error. Things that aren't items yet are sent as all zeros, as Live does.
- **The reserved half** is 0 in every character GUID; it is Live's and unused by EQEmu.

### The item table

`item_instances`, one row per item instance, never deleted; created by database migration 9326 (`CURRENT_BINARY_DATABASE_VERSION`), the way EQEmu's own migrations are done:

- **Key:** the GUID as its three raw 32-bit words, `guid_first`, `guid_second`, `guid_third`. A character GUID's second word is the world id (the reserved half is 0); a server-stamped GUID uses all three words freely. Server-stamped second words are over a billion and a world id fits in 16 bits, so the two never collide. The first word is never 0 (a CHECK): a character's id or a server stamp's sequence value, never a client placeholder.
- **The item:** item number (never 0), charges (signed; -1 is unlimited, stored as itself rather than as `inventory`'s 32767), color, attuned, custom data, ornament icon, IDFile and hero model. Augments are rows of their own, not columns of their parent. No foreign key to `items`: content may live in a separate database, and a row must outlive a content change.
- **Location:** a top-level item has a holder type, holder id and slot; a bag's contents and an augment have their parent's GUID and an index (the bag slot or the augment socket) instead, so moving a bag or an augmented item is one `UPDATE` of its own row. A CHECK makes every row exactly one of the two. Slots are numbered within their holder, as RoF2's and Live's slot addresses are (`{Type, Slot, SubIndex, AugIndex}`); `InventoryProfile` converts between them and EQEmu's global slot ids, which stay the server's in-memory address:

  | Holder type | Holder id | Slot |
  |---|---|---|
  | `inventory` | character | worn and general, 0-32 |
  | `cursor` | character | queue position, 0 on the cursor |
  | `bank` | character | 0-23 |
  | `shared_bank` | account | 0-1 |
  | `corpse` | corpse | corpse slot |
  | `parcel` | character | parcel slot |
  | `pet`, `suspended_pet` | owning character | pet equipment slot; a pet's base equipment is not an item and has no row |
  | `bot` | bot | bot equipment slot; a bot's starting items are issued from its owner's counter |
  | `guild_bank_deposit`, `guild_bank_main` | guild | slot |
  | `ground` | zone | none; the ground-drop table holds the position |

  Housing adds its holder type when it is reworked on this branch's tables.
- **One item, one place:** unique indexes on the holder and slot, and on the parent and index, over generated columns that are NULL once a row is retired, so a retired item keeps its last location (the Recovery tab lists a character's retired items by it) without holding the slot.
- **Retirement:** a state (ENUM of the reasons below, NULL while the item is in play) and its time; a CHECK keeps the two together.
- **The next counter** is derived, not stored: `MAX(third) + 1` for the character and world, answered from the key alone, and the new row is inserted with it. Two racing issues for one character collide on the key and fail loudly, a retryable fault. Correct because no row is ever deleted.
- **Writes:** `INSERT` and `UPDATE` only. A `BEFORE DELETE` trigger refuses every delete, `REPLACE` included (a delete and an insert); once a side table has a foreign key to it, `TRUNCATE` is refused too. It stops code and mistaken SQL, not an administrator who drops the trigger or the table. Creating a trigger needs the `TRIGGER` privilege (EQEmu's usual all-privileges grant on its database has it) and, on a server with binary logging, `SUPER` or `log_bin_trust_function_creators`.
- **The repository** (`ItemInstancesRepository`) is written by hand and on the generator's ignore list: the generator keys a table by one column, cannot write NULL, and emits `ReplaceOne` and `DeleteOne`.

### Side tables

EQEmu's tables that carry their own details stay, keyed by the item's GUID with a foreign key, so they cannot hold orphans:

| Table | Keeps | Loses |
|---|---|---|
| `trader` | the seller, `item_sn` (the handle the RoF2 client echoes, which selects within the seller's listings), price, listing slot and date, transaction | item number, augments, charges |
| `character_parcels` | recipient and slot (a money parcel has no row to carry them), `money`, sender, note, sent date | item number, augments, quantity |
| `guild_bank` | permissions, `who_for`, donator | item number, augments, quantity, guild, area and slot (the row's holder and slot) |
| `character_evolving_items` | its id (the client's unique id), activated, equipped, amount, progression, final item | item number, character, `deleted_at` (the row's retirement) |
| `ground_items` (new; `object` keeps only static content) | instance, position, heading (the zone is the item row's holder id) | |
| `real_estate_items` (housing, after the merge) | property, placed, transform, upkeep | item columns |

**As built (SCH-3).** The four side tables' GUID columns (and `character_parcels.money`) are added by `DatabaseUpdate::InjectItemGUIDColumns` before the manifest runs, because the 9328 data move (MIG-1) fills them from the old columns and must run before 9328's SQL drops those columns; 9328 today truncates and rebuilds `trader` (its rows live only while a Bazaar zone does), puts the unique index and foreign key on the other three, whose copied columns stay until the move, and creates `ground_items`. The code reads and writes only the GUID columns. A Bazaar listing is loaded from its row for the customer's window, inspection and a purchase; a purchase from another zone moves the whole item's row to the buyer's parcel or splits the bought part off, issued from the seller's counter, and the seller's zone only forgets the sold item from memory (`Client::RemoveSoldListing`); a price change for all identical listings joins `trader` to the item table. A guild bank entry is its side row plus the item row's place; splitting a stack issues the new piece from the splitter's counter, merging retires the absorbed stacks `stacked`, and part of a withdrawn stack leaves as a new item issued from the withdrawer. An evolving item's record is created at its first landing, after the save that issued its GUID (`DoLootChecks` runs after the save), and found by GUID at load; a toggle names a unique id the character must hold. A dropped item gets a `ground_items` row with the drop's position, removed when it is picked up or decays, and `Zone::Init` retires as `decayed` any `ground` row of its zone and instance that a dead zone left behind.

Tables that only record location are absorbed into the item table and dropped: `inventory`, `sharedbank`, `character_corpse_items`, `bot_inventories`, `character_pet_inventory`, `character_parcels_containers`, `object_contents`. A side table is a child, so `REPLACE` on it is safe.

The Bazaar shows a listed item by loading it and its children (augments, bag contents) from the item table; search joins `trader` to the item table to `items`. Pooled player-sold merchant rows (`merchantlist_temp`) point at a sold item's GUID, as Live's list shows one; a buyer always gets a new item.

### Issuing

One path in the item layer issues, retires and updates; everything else, quest scripts included, calls it. A new item takes the receiving character's counter when it lands with them: loot, merchant purchases (pooled stock included), combine results, summons (the target's counter), splits (the splitter's), server grants, quest rewards. Items for a bot or a pet are stamped by the owning character. Items an NPC holds (loot tables, merchant stock) get no GUID until a player loots or buys them; an item a player hands an NPC is already an item and keeps its GUID if the NPC keeps it. A copied character's items get new GUIDs.

**How the layer is reached** (`common/item_layer`, `ItemLayer`). EQEmu creates item instances unowned (`CreateItem`), and an instance becomes a character's only when one of the landing functions stores it in their `InventoryProfile` and saves it: `PutItemInInventory`, `PushItemOnCursor`, `PutLootInInventory`, `MoveItemCharges` and the world's `StoreCharacter`. Every save goes through `SharedDatabase::SaveInventory(client, slot)` to `ItemLayer::Save`, which issues an instance without a GUID from the receiver's counter and otherwise updates its row's location and fields; augments and bag contents are saved under their parent. So creation sites never issue: a summon onto another player lands through the target's funnel and takes the target's counter, as Live does. The row's location is authoritative and changes only through `Save`, `MoveRow` and `Retire`; nothing writes "the slot emptied". An item that leaves play goes through `Client::DeleteItemInInventory(slot, quantity, update, reason)`, which retires it (or saves the smaller stack); an item that only left this inventory, because another holder's landing moved its row or because it never had one (a tribute display item), goes through `RemoveItemFromInventory`. A forgotten retirement collides on the held-slot index at the next landing in that slot, loudly. Two slots that traded places are saved by `SaveSwap` in one transaction with one row parked at the in-transit location (the cursor's slot 65535, never seen outside that step); a Bazaar sale of part of a stack issues the piece from the seller's counter at the seller's in-transit location, and the buyer's landing moves it, all inside the purchase's transaction, so an interruption rolls the coin, the piece and the landing back together (measured on peridotguids: a connection killed or a client process killed before `COMMIT` leaves the rows where they were). `DBcore`'s transaction calls nest, since MySQL commits an open transaction when it sees a second `START TRANSACTION`: only the outermost begin and commit reach the server, and a rollback at any depth turns the outermost commit into a rollback. `ItemInstance` carries its GUID and copies it with the instance, as the serial is copied; a split piece made by cloning gets `ClearGUID` so it is issued afresh, and an item that becomes a new instance of the same thing (an evolving item's next level, a bag replaced by its disenchanted kind, a weapon through a transformation mold) gets `CarryGUID` so its row is updated in place.

**Holders beside the character.** A pet's loot entry for a player's item carries the item's GUID (`NPC::AdoptPetItem` when the item is handed over, `AdoptPetItemRows` when a pet is restored from its rows at zone-in or unsuspend); suspending moves the pet's rows to `suspended_pet` and back (`ItemLayer::MoveHolder`); a pet's death, dismissal, a suspension or recall without equipment, and a save with no pet retire its rows `petDonated`. A bot's items are rows at the `bot` holder, saved through the layer with the bot's owner as issuer; deleting a bot retires them `botDonated`, a cloned bot gets copies issued from the owner's counter. A parcel's item is its row, contents and augments under it, and retrieval loads the instance from the row; a pruned parcel retires `expired`. A hard-deleted character's possessions, parcels, pet items, corpse items and bot items retire `characterDeleted`; a copied character gets every held row copied as a new item from its own counter, parents before children (`ItemLayer::CopyRows`). The shared bank stays with the account in both.

**The quest API** (Lua and Perl) goes through the same path:

| Kind | Calls | GUID handling |
|---|---|---|
| Create | SummonItem, SummonItemIntoInventory, SummonBaggedItems, PushItemOnCursor/PushCursor, PutItem/PutItemInInventory, createitem, send_parcel, AddBotItem, ForageItem | Issued from the receiving character's counter when the item lands with them; a createitem not yet given has none |
| Remove, consume | DeleteItemInInventory, NukeItem, RemoveItem, RemoveItemByID, DeleteItem, PopItem, CheckHandin/handin, RemoveTaskDeliveredItems, collectitems, RemoveBotItem | The row retires with its reason |
| Update | SetCharges, SetCustomItemData/DeleteCustomData, UseAugmentContainer, AddEvolveAmount, SetEvolveAmount, SetEvolveProgression | Updates the row by GUID; evolving state is keyed by GUID |
| Return, move | ReturnHandinItems, SwapItem, corpse summons | Keeps the GUID |
| Not items | addloot, AddLootTable, MerchantSetItem | No GUID until looted or bought |

Scripts get `GetGUID()` and a lookup by GUID; `GetEvolveUniqueID` is replaced by the GUID; `GetSerialNumber` stays as the session handle; no script can set a GUID.

### Trust

A GUID the client sends is a selector, never an authority: the server decides from the session whose items a packet may name (the character's own, the current listings of the trader a Bazaar request names, the character's own retired items for recovery), and the GUID only picks one item within that scope, through one scoped lookup in the item layer. GUIDs are predictable (character, world, counter), so knowing one proves nothing; a GUID outside the scope, a placeholder or an all-zero one, or text that fails the round-trip decode is refused and logged, and no update can be expressed against an item the character does not hold.

### Stacks and use

A split piece is a new item with the splitter's next number. A merge keeps the destination's GUID and retires the merged piece as `stacked`. Using part of a stack (reagents, arrows, food, charges) decrements the row; the last unit retires it as `consumed`. Selling or destroying part of a stack splits it first, so the sold or destroyed units have a row of their own.

### Retirement and recovery

| Reason | Covers |
|---|---|
| `destroyed` | the player destroys it |
| `sold` | sold to a merchant |
| `stacked` | merged into another stack |
| `consumed` | used in a combine, handed in, delivered for a task, removed by a quest script, its last unit or charge used |
| `expired` | no-rent or temporary items removed on logout, summoned items that vanish |
| `decayed` | lost with a rotted player corpse, a dropped item that despawned |
| `characterDeleted` | held by a character who was hard-deleted (a soft delete keeps everything) |
| `removedByGM` | taken by a GM command |
| `rolledBack` | removed by an inventory snapshot restore |
| `petDonated` | held by a pet that died, was dismissed or depopped (no owned NPC leaves a corpse) |
| `botDonated` | held by a bot that was deleted |

Which states a merchant's Recovery tab may bring back is a rule:

```cpp
RULE_STRING(World, RecoverableItemStates, "destroyed,decayed", "Comma-delimited list of retirement states whose items can be recovered from a merchant's Recovery tab. Values: destroyed | sold | consumed | expired | decayed | characterDeleted | removedByGM | rolledBack | petDonated | botDonated. Live allows recovery of destroyed items; it does not allow recovery of sold items.")
```

An unknown name fails loudly when the rule loads; `stacked` is refused, since the merged units already live on in the stack they joined. A recovered item is the same row, so it keeps its GUID.

### The session serial

The 32-bit serial stays a per-session handle for every client, never saved: the header's `instance_id` and the only item id Titanium through UF have, as Live's per-session number is. In-zone lookups keep it. Evolving items use it like any other item (RoF2's evolving special case goes). Trader rows and evolving state move to the GUID. #4460's persistence is removed entirely.

### Encoders

RoF and RoF2 send the GUID as text in Live's alphabet in the item header, the Bazaar structs and (RoF2) the player record; housing's fields follow after the merge. Incoming GUID text is decoded with the client's round-trip check and anything invalid is refused. Older clients are unchanged. The RoF encoder is untested: no RoF client (December 10, 2012) survives anywhere we could find, so it uses the alphabet and layout RoF2 and Live share.

### Inventory snapshots

`#invsnapshot` keeps what it does in EQEmu; only its rows change. A snapshot row is an item's GUID (with a foreign key) and a copy of its row's values and location, `item_id` included, since an evolve or a mold changes the item number under the same GUID; bag contents and augments are rows pointing at their parent. `capture` takes everything the character holds in the holders EQEmu's `inventory` rows covered (`inventory`, `cursor`, `bank`, `trade`; the shared bank is the account's) in one statement. `parse` and `compare` list EQEmu slot ids and item numbers as before, worked out from the copied locations; augments are not listed, as they were columns of their item before. `restore` does what deleting and reinserting the character's `inventory` rows did, in one transaction after the "before restore" snapshot: items held now that the snapshot lacks retire `rolledBack`; an item the character still holds, or one retired since, goes back to its snapshot place with its snapshot values; an item someone else holds now (another character, a parcel, a corpse, the guild bank) is made again with a new GUID from the restored character's counter, under its restored parent, and the log pairs the original's GUID and place with the new GUID, which is the duplicate EQEmu's restore already made, now traceable. Rows that swap places wait on each other and step aside through the in-transit slot. Evolving progress was never in a snapshot and still is not. The pre-GUID snapshots are cleared: 9328 recreates the table in this shape (`ItemLayer::RestoreSnapshot`, `InventorySnapshotsRepository::Capture`).

### The migration

Its own migration, separate from housing's. Every existing item, each augment pulled out of its parent's columns, gets a row in a fixed order (table, then row key), so a rerun gives the same GUIDs:

- **Character-held items** (inventory, corpses, trader, bots by their owner, pets, parcels by their recipient) get their holder's counter.
- **Items no character holds** (the shared bank, the guild bank, ground drops and world-container contents) get Live's server stamp, generated exactly as Live's 0x80c890 does; the migration records the process id and start time it used, so a rerun reproduces every GUID.
- Every character is stamped with the world rule's value; #4460's saved serials are discarded.
- After the move, 9328's SQL drops the copied columns from `guild_bank`, `character_parcels` and `character_evolving_items`, makes the guild bank's and evolving items' GUID columns `NOT NULL`, adds the parcel check (a GUID or `money`, never both or neither) and drops the absorbed tables; its additive half (the trader rebuild, the unique indexes and foreign keys, `ground_items`) is in place from SCH-3.

## Where the work lives

Branch `guids` of AmmolitePreservation/EQEmu, from `main`'s tip 6fedcae84 ("Peridot local changes", the code Peridot runs), in its own worktree; this folder for the design and worklist; tested on **peridotguids**, a new clone of Peridot. Housing is paused on `housing` (its worklist's "GUIDs first" decision, REC-16 to REC-18, DOC-39) until this branch is merged into `housing` and migrated.
