# peridotguids: the GUID development server

Claude's server for building and testing item GUIDs, apart from Peridot, peridothousing and claudeperidot. Nothing on it is production; it can be rolled back to a snapshot or rebuilt from Peridot at any time.

## The machine

| | |
|---|---|
| Proxmox | VM 145 on labradorite, a full clone of Peridot (VM 104) made 2026-10-26 by a live drive mirror, Peridot's guest filesystem frozen only for the final switch |
| Size | 8 cores, 16 GB; no autostart; tags `eqemu;peridotguids;ubuntu` |
| Disk | `alpha:145/vm-145-disk-0.qcow2`, 350 GB virtual |
| Network | DHCP on `ens18`, MAC `BC:24:11:54:66:96`, reserved `10.111.111.239` on the SonicWall (PeridotGuids); A record `peridotguids.strata.unseencolor.com` on olivine |
| Access | facet host `peridotguids`; Claude's login is Peridot's account, copied to OpenBao `hosts/peridotguids/claude`; SSH host keys and machine-id regenerated on the clone |

Changed from Peridot's copy while its network link was still down (`link_down=1` on net0): the hostname and `/etc/hosts`, the Zabbix agent's `Hostname`, the Zabbix MySQL plugin's address, the Alloy `host` labels, the akk-stack `.env` `IP_ADDRESS`, and the world's `address` and `localaddress` in `server/eqemu_config.json` (`peridotguids.strata.unseencolor.com`; Peridot's say `peridot.unseencolor.com`). The world registers only with the stack's own login server (`127.0.0.1:5998`). The test servers carry no monitoring: the Zabbix agent and Alloy copied from Peridot are stopped and disabled (PRC-5), so the Zabbix and Alloy edits above only keep the copies from naming themselves Peridot. The containers were recreated for the new address; pok and EQBCS, which Peridot runs, are stopped and set not to restart.

## Snapshots

| Snapshot | State |
|---|---|
| `pristine` | The clone set up, the `guids` build running, the database at 9325 / bots 0 / custom 0 with no migration applied; RAM included |
| `ready` | `pristine` plus the claudelink login and character below, camped; RAM included |
| `seeded` | `ready` plus the pre-migration seed below; RAM included |

`qm rollback 145 seeded` (or `ready`, or `pristine`) returns to that running state. A plain reboot also brings the server back by itself: Docker restarts the stack's containers and Spire starts the launcher (`spire.launcher_start` in eqemu_config.json). The containers carried over from Peridot first had to be recreated, since their ports were pinned to Peridot's address (PRC-4).

## The server

The akk-stack at `/opt/eqemu/akk-stack`, Peridot's database as it stood at the clone (dumped first: `backup/peq-2026-10-26-beforeGuids.sql.gz`, 35 MB) and Peridot's quests. The stack's `code/` is a clone of AmmolitePreservation/EQEmu on `guids`, built inside the `eqemu-server` container as the `eqemu` user (`cd ~/code/build && make -j8`, clang, RelWithDebInfo, 161 s; `server/bin` links to `code/build/bin`). The server processes are stopped and started as on peridothousing (`./bin/spire eqemu-server:launcher stop` from a script file, since it kills any shell whose command line names the launcher, then `pkill` of world, zone, ucs, loginserver and queryserv; `launcher start` brings them back).

## The client

`C:\Games\EverQuest - peridotguids` on onyx: the pristine RoF2 install (`C:\Onyx\research\rof2Pristine\everquest_rof2`) with `eqgame.exe` made large-address-aware, `eqhost.txt` at `peridotguids.strata.unseencolor.com:5999` and onyx's window settings in `eqclient.ini`. None of housing's client files: `guids` predates housing. claudelink drives it with `launchClient(clientFolder = C:\Games\EverQuest - peridotguids, loginServer = Peridot, login = services/peridotguids/eqLogin, character = Claudeguids)`.

Claude's account is `claudeguids` (login account 85, game account 86 at status 250), its password in OpenBao at `services/peridotguids/eqLogin`; the login server made it at the first login. `Claudeguids` (character 117) is a level 1 human male wizard bound at the Plane of Knowledge's safe point, written with the rows EQEmu's character creation writes, as Claudehousing and Claudewright were.

## The pre-migration seed

What Peridot's own data holds, as copied (2026-10-26): 2,994 `inventory` rows (1,844 in banks, 28 with augments), 72 `sharedbank`, 81 `guild_bank`, 106 `object` rows with an item, 193 `merchantlist_temp`; nothing in `character_corpse_items`, `character_parcels`, `character_pet_inventory`, `object_contents` or `inventory_snapshots`; no bot tables; `items_evolving_details` has 443 rows, 6 with a required amount; `trader` is always empty at boot (world clears it, `ClearTraderDetails`), so no migration meets trader data.

Added on Claudeguids so the migration meets what Peridot lacks (snapshot `seeded`): `Character:ActiveInvSnapshots` set true in `rule_values`; three inventory snapshots taken with `#invsnapshot capture` (self-targeted), the third after a move; a Backpack (17005) in general slot 1 holding Bone Chips x20, Spider Silk x5 and Water Flask x10 (slots 4010 to 4012); a Ruined Heretic Longsword (1074) with a Green Shard of Might (41001) in its first augment slot, worn in the primary slot; and seven evolving items, one per line because each line is one lore group, with required amounts written into `items_evolving_details` for those seven item numbers on this server only (Peridot's rows carry 0), and their `character_evolving_items` rows set to a range: Broken Club of the Warcaller 86900 (stage 1 of 5) 0 of 1000, not activated; Wand of Infused Chitin 86982 (3 of 5) 1000 of 4000, activated; Twisted Shank 86940 (1 of 6) 500 of 1000, activated; Blade of the Once Bitten 89310 (1 of 6) 1500 of 2000, not activated; Blade of the New Moon 86951 (2 of 7) 1000 of 1000, activated; Roarm, Light in the Darkness 89532 (final, 3 of 3) 500 of 500, activated; The Clawed Hammer of Grokui 90010 (final, 10 of 10) 1000 of 1000, not activated. The zones cache `items_evolving_details` at boot, so a change there needs the launcher restarted; the client then shows the percentages (`${FindItem[...].Evolving.ExpPct}`).
