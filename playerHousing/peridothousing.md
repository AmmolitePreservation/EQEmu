# peridothousing: the housing development server

Claude's server for building and validating player housing, apart from Peridot and from claudeperidot (zonewright's). Nothing on it is production; it can be rebuilt from Peridot at any time.

## The machine

| | |
|---|---|
| Proxmox | VM 144 on labradorite, a full clone of Peridot (VM 104) made 2026-10-24 with the guest filesystem frozen for the copy |
| Size | 8 cores, 16 GB (Peridot has 16 and 32); no autostart; tags `peridothousing;eqemu;ubuntu` |
| Disk | `alpha:144/vm-144-disk-0.qcow2`, 350 GB virtual |
| Network | DHCP on `ens18`, MAC `BC:24:11:4D:E5:B4`, reserved `10.111.111.238` on the SonicWall (PeridotHousing); A record `peridothousing.strata.unseencolor.com` on olivine |
| Access | facet host `peridothousing`; Claude's login is Peridot's account, copied to OpenBao `hosts/peridothousing/claude`; SSH host keys regenerated on the clone |

What was changed from Peridot's copy, before its network link came up: the hostname and `/etc/hosts`, the Zabbix agent's `Hostname`, the Alloy `host` labels, the Zabbix MySQL plugin's address, the akk-stack `.env` `IP_ADDRESS` and the world's `address` and `localaddress` in `server/eqemu_config.json` (`peridothousing.strata.unseencolor.com`). The containers were recreated for the new address, and the services Peridot keeps stopped (backup-cron, phpmyadmin, ftp-quests, peq-editor) were stopped again. The login server is the stack's own on `5998`/`5999`; nothing registers with a public login server.

## The server

The akk-stack at `/opt/eqemu/akk-stack`, Peridot's database as it stood at the clone (Expansion:CurrentExpansion 4, Planes of Power) and Peridot's quests. The housing build, its migrations and content go here first; the Peridot-only lobby SQL (`sql/peridot/guildLobbyExpansions.sql`) applies here as it would on Peridot.

### The housing build and content, deployed 2026-10-24

The stack's `code/` is a clone of AmmolitePreservation/EQEmu; it is on `claude` and is built inside the `eqemu-server` container (`cd ~/code/build && make -j8`, clang, RelWithDebInfo; `server/bin` links to `code/build/bin`), after the launcher is stopped: `./bin/spire eqemu-server:launcher stop` in a shell of its own (it kills any shell whose command line names the launcher), then `pkill` of world, zone, ucs, loginserver and queryserv; `./bin/spire eqemu-server:launcher start` brings them back. The database was dumped first (`backup/peq-2026-10-24-beforeHousing.sql.gz`, 35 MB). World ran the custom migrations 1 to 12 at boot once `common/version.h` carried version 12 (PRC-9). Then, in the container from `~/server`: `./bin/import_client_files eqstr` (7,137 strings), `python3 housing/tools/deployHousingContent.py --config eqemu_config.json --sql housing/sql` (the content under `server/housing/`, a copy of `playerHousing/sql` and the deploy script; it stopped at housingZones.sql until that file left the rows Peridot already has to housingZoneSettings.sql, and resumed with `--from`), the lobby SQL through the mariadb container as root, and `./bin/export_client_files` (`export/dbstr_us.txt`, `export/eqstr_us.txt`, copied into the client). At its next boot world made the ten random neighborhoods: 760 properties, 710 plots and 40 guild plots. The 13 housing quest scripts of AmmolitePreservation/projecteqquests `claude` were copied into `server/quests` (the stack cannot read the private fork). The `Housing` log category (104) logs to file and console here. The housing character is `Claudehousing` (human wizard, level 1, account 86, in the Plane of Knowledge), written as claudeperidot's Claudewright was.

**State on 2026-10-24:** Claudehousing enters the world and `#zone guildlobby` lands in version 1 (instance 104, the gate door 38 in place, Live's archive rendering), but the housing catalog refuses to load in every zone (`Housing catalog: NPC item [...] has no real_estate_npc_items row`, 296 times, then `Catalog [INVALID, housing disabled in this zone]`): Peridot's items table holds 571 Live NPC items after the adoption migration and npcItems.sql covers 274 (NPC-37). With the catalog made graceful the same day (placing an item without a row answers the client's 0x24) and the NPC content regenerated from the post-adoption set, the catalog loads valid with 528 NPC items placeable and the 42 of content/npcItemGaps.tsv named in one Warning line (the `Warning` log category is on here; it was off in Peridot's log settings).

### Chunk 8 in game

Stage 0 (baseline), stage 1 (the item tail: seven items, `itemTail`) and stage 2 (zone-in: the 75 plot records and regions, twelve plots stood on, `neighborhoodZoneIn`) pass with the mqperidotemu suites. Through the Guild Lobby gate the Go lands in a Sunrise Hills copy. A plot bought from its border stone (104 Market Street, 21 platinum), then a chair and a house placed from inventory once the commit read Live's field order (PLC-23). Stage 3 passes: escrow deposits and withdrawals (the coin to the cursor), daily upkeep prorated by the second and charged on schedule by world, a charge that takes what is left, the stamp on the next, 0x18 on a place request there, and a deposit clearing it. Stage 4 placed, edited, stored, picked up and placed an NPC item; stage 7 searched, used Go and sold, empty and into a crate. Stage 8's polls run: a question poll created, published and prompted at zone-in, a vote stored and its results shown. Menus, forges and house entry wait on an in-game cursor (PRC-11); access and transfer need a second client. What the shakeout finds goes to worklist.md.

## The client

`S:\Research\EverQuest\peridothousing`: a copy of the stock RoF2 install (`S:\Research\EverQuest\housingclient`, which stays stock as the clean install makeClientFiles.py reads) with the eqemu-client-files `rof2/` tree laid over it and `eqhost.txt` at `peridothousing.strata.unseencolor.com:5999`. claudelink drives it as it drives any client copy: `launchClient(clientFolder = S:\Research\EverQuest\peridothousing, loginServer = Peridot, login = services/peridothousing/eqLogin, character)`.

Claude's account there is `claudehousing`, its password in OpenBao at `services/peridothousing/eqLogin`; the clone's login server creates accounts at their first login (`login.json` `auto_create_accounts`), and the first launch on 2026-10-24 made login account 85 and game account 86 at status 0. Before a character enters the world: the game account's status raised for the GM commands the validation uses, and the per-character MacroQuest config `Peridot_<character>.cfg` in mqPeridot's config folder (MQ2EQBC and mq2netbots unloaded, MQClaudeLink loaded, each with `noauto`), as claudeperidot's character has, so the client never joins the owner's EQBCS.

## Open

- A Zabbix host for the agent, which now reports as peridothousing.
