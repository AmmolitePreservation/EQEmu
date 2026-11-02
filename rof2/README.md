# One client: Rain of Fear 2

This branch removes every client but Rain of Fear 2 (the May 10 2013 build) from EQEmu. It starts from `main`'s tip 6fedcae84 and is the base the `guids` branch is rebased onto, which is where item GUIDs replace the last per-client item handle (`trader.item_sn`); player housing follows `guids`, and bots are rewritten after housing.

## What goes

- **The translators** for Titanium, Secrets of Faydwer, Seeds of Destruction, Underfoot and Rain of Fear (`common/patches/`: their patch, structs, opcode and limits files), the patch template and `makepatch`, the deprecated item field headers, and the five opcode files in `utils/patches/`.
- **The version enums.** `ClientVersion` is `Unknown` or `RoF2`; `MobVersion` keeps `RoF2` beside the NPC, merchant, mercenary, bot and pet versions. `bitRoF2` stays `0x40`, because content carries it (below). The UCS connection type is RoF2's combined chat and mail.
- **The limits tables** (`common/eq_limits.cpp`) keep one client row each. The expansion-driven inventory shape stays, as named constants: the bank has 16 slots before Prophecy of Ro, the power source slot comes with The Buried Sea, the ninth and tenth general slots with House of Thule; a GM keeps the slots but not the bank.
- **Every client version branch** collapses to RoF2's outcome, about 290 sites in zone, world, common, the chat server and the login server. Where RoF2 was the newer path, that path stays; where only older clients took a branch (Titanium consider colours, pre-SoF group and spell packets, pre-RoF guild rank conversions, the Berserker piercing skill swap), it goes.
- **The login server** listens for SoD-and-later and Larion clients; the Titanium listener, its opcode file `login_opcodes.conf` and the `titanium_port` and `titanium_opcodes` config keys go.
- **World** has one character creation check and one start zone lookup (the client sends the zone id, matched by zone, class, deity and race in `start_zones`), and the client version summary counts RoF2 clients.
- **The chat server** sends every message as RoF2 reads it; the say-link converters for the older clients go.
- **Mercenaries** no longer filter their types and templates by the client's version.
- **Rules:** `World:SupportedClients`, `World:SoFStartZoneID`, `World:TitaniumStartZoneID`, `Spells:FRProjectileItem_Titanium` and `Spells:FRProjectileItem_SOF` go; `World:StartZoneID` and `Spells:FRProjectileItem` take the place of the last three. `Character:SoDClientUseSoDHPManaEnd` keeps its name and now chooses the formula set for everyone.
- **The Lua `ClientVersion` constants** are `Unknown` and `RoF2`. The Perl and Lua `GetClientVersion`, `GetClientVersionBit` and a door's `GetClientVersionMask` stay, since quest scripts call them.

## What stays, and why

- **Content masks.** `doors.client_version_mask` and `zone_points.client_version_mask` are ProjectEQ content. On Peridot's data 6 of 19,253 doors and 17 of 1,831 zone points exclude RoF2 (masks 0, Titanium-and-earlier or RoF-and-earlier), and the code keeps hiding them by testing RoF2's bit.
- **`ClientVersionBit()`** on a client, for those masks and for the quest API.
- **The `MobVersion` lookups**, since NPCs, mercenaries, bots and pets have inventory shapes of their own.

## Database

The branch makes no schema change. Columns and rows that only the removed clients gave meaning to, each with what Peridot's data holds, are listed for the owner's decision in the worklist (`rof2/worklist.md`).

## Tests on peridotguids (2026-11-01)

Built on peridotguids from its `preGuids` snapshot (Peridot's data at database version 9325), with the RoF2 client through claudelink and two characters, Claudeguids and Claudeguidstwo:

- Login through the SoD-and-later listener, character select, zone entry into the Plane of Knowledge and the Bazaar.
- Inventory, bags, augments, bank and shared bank load; `/who all`; `/consider`; a banker and a parcel merchant open; a merchant purchase with its quantity window.
- Grouping: an invite accepted, the group disbanded.
- A pet guards; doors are sent (`/doortarget` finds one).
- The Bazaar: a trader lists a stack and an item, a search finds the stack (the Find button is disabled until a trader exists while the window is open), a customer buys part of the stack, the trader's row and satchel count down, trading ends and the rows go.
- Chat server: both clients connect as RoF2 combined chat and mail, join a channel and exchange messages (peridotguids' config pointed clients at Peridot's chat server until this test; it now names peridotguids).

Not tested: character creation through the client, mercenaries (Peridot has no mercenary tables), tasks, the guild management window.
