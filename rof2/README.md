# One client: Rain of Fear 2

This branch removes every client but Rain of Fear 2 (the May 10 2013 build) from EQEmu. It starts from `main`'s tip 6fedcae84 and is the base the `guids` branch is rebased onto, which is where item GUIDs replace the last per-client item handle (`trader.item_sn`); player housing follows `guids`, and bots are rewritten after housing.

## What goes

- **The translators** for Titanium, Secrets of Faydwer, Seeds of Destruction, Underfoot and Rain of Fear (`common/patches/`: their patch, structs, opcode and limits files), the patch template and `makepatch`, the deprecated item field headers, and the five opcode files in `utils/patches/`.
- **The version enums.** `ClientVersion` is `Unknown` or `RoF2`; `MobVersion` keeps `RoF2` beside the NPC, merchant, mercenary, bot and pet versions. The UCS connection type is RoF2's combined chat and mail.
- **The client axis on content.** The client version is neither the era the server emulates (`Expansion:CurrentExpansion`, min and max expansion, content flags, a zone's `bypass_expansion_check`) nor the zone version (`version` rows per layout); with one client it carries no information. So `doors.client_version_mask` and `zone_points.client_version_mask` go with the check that tested the connected client's bit against them, the client bit itself (`ClientVersionBit()`, the bit enum, the bit conversions), the per-client-bit filters on `QueueClientsByTarget` and `QueueClientsByXTarget`, and the quest API's `GetClientVersionBit` and a door's `GetClientVersionMask`. Migration 9326 settles the rows the mask excluded (Database, below).
- **The limits tables** (`common/eq_limits.cpp`) keep one client row each. The expansion-driven inventory shape stays, as named constants: the bank has 16 slots before Prophecy of Ro, the power source slot comes with The Buried Sea, the ninth and tenth general slots with House of Thule; a GM keeps the slots but not the bank.
- **Every client version branch** collapses to RoF2's outcome, about 290 sites in zone, world, common, the chat server and the login server. Where RoF2 was the newer path, that path stays; where only older clients took a branch (Titanium consider colours, pre-SoF group and spell packets, pre-RoF guild rank conversions, the Berserker piercing skill swap), it goes.
- **The login server** listens for SoD-and-later and Larion clients; the Titanium listener, its opcode file `login_opcodes.conf` and the `titanium_port` and `titanium_opcodes` config keys go.
- **World** has one character creation check and one start zone lookup (the client sends the zone id, matched by zone, class, deity and race in `start_zones`), and the client version summary counts RoF2 clients.
- **The chat server** sends every message as RoF2 reads it; the say-link converters for the older clients go.
- **Mercenaries** no longer filter their types and templates by the client's version.
- **Rules:** `World:SupportedClients`, `World:SoFStartZoneID`, `World:TitaniumStartZoneID`, `Spells:FRProjectileItem_Titanium` and `Spells:FRProjectileItem_SOF` go; `World:StartZoneID` and `Spells:FRProjectileItem` take the place of the last three. `Character:SoDClientUseSoDHPManaEnd` keeps its name and now chooses the formula set for everyone.
- **The Lua `ClientVersion` constants** are `Unknown` and `RoF2`. The Perl and Lua `GetClientVersion`, `GetClientVersionBit` and a door's `GetClientVersionMask` stay, since quest scripts call them.

## What stays, and why

- **The era and zone version axes.** Content is still filtered by the server's current expansion and content flags when a zone loads, zoning still checks a zone's expansion, and a zone's layouts still live in `version` rows.
- **The `MobVersion` lookups**, since NPCs, mercenaries, bots and pets have inventory shapes of their own.

## Database

Migration 9326 (`2026_11_01_one_client_content.sql`, a content update) deletes every door and zone point whose mask excluded RoF2 and drops `client_version_mask` from `doors` and `zone_points`. On Peridot's data that is 6 doors and 17 zone points, all of them rows this client cannot use:

- **Client pairs**, two rows at the same place, one for each client generation's files: the Highpass lines from East Karana, High Keep (two) and Kithicor (classic Highpass, zone 5, for Titanium; Highpass Hold, zone 407, for SoF and later), Plane of Knowledge zone point 17 into Nektulos, the Corathus door `POKTELE500` and the Plane of Knowledge door `POKRVPORT500`. The RoF2 client has no zone 5 at all: its zone table answers nothing for the id or the name (the client's log says "Unknown Zone" and fails to load a world file), so classic Highpass is unreachable from this client whatever the server's era, and the stock client also lacks `highpass_obj.s3d`. For the other pairs the client ships the newer zone files. In every pair the row RoF2's bit already selected stays and the other goes.
- **Mask 0**, off for every client: 12 zone points into the 2006 revamp zones and 4 Guild Hall doors.

The rows RoF2's bit passed stay as ProjectEQ shipped them, era columns included. Where a pair's two targets were zones of different eras, the era is not a choice this client offers: a Planes of Power server sends RoF2 players from East Karana into the Serpent's Spine Highpass Hold, which ProjectEQ's `bypass_expansion_check` on the revamped zones allows. Peridot's rule rows for the removed and renamed rules needed nothing: world deletes rule rows it no longer knows and inserts missing defaults at boot. The remaining client-axis leftovers are worklist items (`rof2/worklist.md`).

## Tests on peridotguids (2026-11-01)

Built on peridotguids from its `preGuids` snapshot (Peridot's data at database version 9325), with the RoF2 client through claudelink and two characters, Claudeguids and Claudeguidstwo:

- Login through the SoD-and-later listener, character select, zone entry into the Plane of Knowledge and the Bazaar.
- Inventory, bags, augments, bank and shared bank load; `/who all`; `/consider`; a banker and a parcel merchant open; a merchant purchase with its quantity window.
- Grouping: an invite accepted, the group disbanded.
- A pet guards; doors are sent (`/doortarget` finds one).
- The Bazaar: a trader lists a stack and an item, a search finds the stack (the Find button is disabled until a trader exists while the window is open), a customer buys part of the stack, the trader's row and satchel count down, trading ends and the rows go.
- Chat server: both clients connect as RoF2 combined chat and mail, join a channel and exchange messages (peridotguids' config pointed clients at Peridot's chat server until this test; it now names peridotguids).

- Character creation from the client's creation window: a human warrior with a generated name; world matched the start zone row by the zone the client sent (Freeport West) and the character entered the tutorial with 9 starting items and 18 skills.

- The client axis (RC-2): migration 9326 ran through world's updater from 9325; East Karana sends its three zone points and the walk into the Highpass line zones the client by the one row the server holds. With Highpass Hold as the target the client arrives; with classic Highpass it stops at "Unable to load World", since its zone table has no zone 5 (`${Zone[5]}` and `${Zone[highpass]}` are empty; the stock client also lacks `highpass_obj.s3d`, which Peridot's client package adds). Plane of Knowledge shows `POKRVPORT500` and the pre-Darkhollow `POKNRKPORT500` and not the Serpent's Spine `OBJ_PORT_CRESCENT`. Character select and login unchanged.

Not tested: mercenaries (Peridot has no mercenary tables), tasks, the guild management window.
