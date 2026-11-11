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

The stack's `code/` is a clone of AmmolitePreservation/EQEmu; it is on `housing` and is built inside the `eqemu-server` container (`cd ~/code/build && make -j8`, clang, RelWithDebInfo; `server/bin` links to `code/build/bin`), after the launcher is stopped: `./bin/spire eqemu-server:launcher stop` in a shell of its own (it kills any shell whose command line names the launcher), then `pkill` of world, zone, ucs, loginserver and queryserv; `./bin/spire eqemu-server:launcher start` brings them back. The database was dumped first (`backup/peq-20261024-beforeHousing.sql.gz`, 35 MB). World ran the custom migrations 1 to 12 at boot once `common/version.h` carried version 12 (PRC-9). Then, in the container from `~/server`: `./bin/import_client_files eqstr` (7,137 strings), `python3 housing/tools/deployHousingContent.py --config eqemu_config.json --sql housing/sql` (the content under `server/housing/`, a copy of `playerHousing/sql` and the deploy script; it stopped at housingZones.sql until that file left the rows Peridot already has to housingZoneSettings.sql, and resumed with `--from`), the lobby SQL through the mariadb container as root, and `./bin/export_client_files` (`export/dbstr_us.txt`, `export/eqstr_us.txt`, copied into the client). At its next boot world made the ten random neighborhoods: 760 properties, 710 plots and 40 guild plots. The 13 housing quest scripts of AmmolitePreservation/projecteqquests `housing` were copied into `server/quests` (the stack cannot read the private fork). The `Housing` log category (104) logs to file and console here. The housing character is `Claudehousing` (human wizard, level 1, account 86, in the Plane of Knowledge), written as claudeperidot's Claudewright was.

**State on 2026-10-24:** Claudehousing enters the world and `#zone guildlobby` lands in version 1 (instance 104, the gate door 38 in place, Live's archive rendering), but the housing catalog refuses to load in every zone (`Housing catalog: NPC item [...] has no real_estate_npc_items row`, 296 times, then `Catalog [INVALID, housing disabled in this zone]`): Peridot's items table holds 571 Live NPC items after the adoption migration and npcItems.sql covers 274 (NPC-37). With the catalog made graceful the same day (placing an item without a row answers the client's 0x24) and the NPC content regenerated from the post-adoption set, the catalog loads valid with 528 NPC items placeable and the 42 of content/npcItemGaps.tsv named in one Warning line (the `Warning` log category is on here; it was off in Peridot's log settings).

### Chunk 8 in game

Stage 0 (baseline), stage 1 (the item tail: seven items, `itemTail`) and stage 2 (zone-in: the 75 plot records and regions, twelve plots stood on, `neighborhoodZoneIn`) pass with the mqperidotemu suites. Through the Guild Lobby gate the Go lands in a Sunrise Hills copy. A plot bought from its border stone (104 Market Street, 21 platinum), then a chair and a house placed from inventory once the commit read Live's field order (PLC-23). Stage 3 passes: escrow deposits and withdrawals (the coin to the cursor), daily upkeep prorated by the second and charged on schedule by world, a charge that takes what is left, the stamp on the next, 0x18 on a place request there, and a deposit clearing it. Stage 4 placed, edited, stored, picked up and placed an NPC item; stage 7 searched, used Go and sold, empty and into a crate. Stage 8's polls run: a question poll created, published and prompted at zone-in, a vote stored and its results shown. The captured neighborhood pets are gone (NBH-10), the coin lines read with one space (STT-15), and an empty plot sells while a full crate is carried (STT-18). The spare logs in with claudelink typing the login (no password on its command line, MQ2AutoLogin unloaded) and clicks in the world through claudelink's screenPoint and click (PRC-11), so menus, forges and house entry come next; access and transfer need a second client. What the shakeout finds goes to worklist.md.

Stage 4's forge works from its right-click menu (Use, Adjust Placement, Place, Move to Storage, Move to Inventory, Inspect, Show Items): Use opens the tradeskill window and a combine in it made A Ruby. Stage 6 placed a house (Behroe Style Brick House, record 2) on 101 Market Street (property 100525) in a Cedar Colonial Lake copy, entered it (phinterior1a3, a clean zone-in, so Q11 holds), left it by /leaverealestate and by the front door (both land on the exit point; INT-37 is the front door's zone name) and moved it with Adjust Placement from its menu (REC-15) and claudelink's nudge. Still to run in stage 6: picking the house up (the item display names its property, 5289), swapping houses, packing and unpacking a crate, layouts and a two-house yard (MaxHousesAdditive 1). The state left on peridothousing: Claudehousing owns 100525 with the house, a forge (record 6) and a crate (record 7); plot 100527 was sold.

The shakeout stopped here on 2026-10-25 for the GUID work (worklist.md's owner decisions). It restarts on the reworked build and the fresh database below, from this section and worklist.md's open items; the state above went with the old database.

### The `guids` merge and the rework, 2026-11-10

`guids` is merged into `housing` (588795685) and housing is reworked on real item GUIDs (a4e627759; worklist.md REC-16 to REC-19, MNU-12). The VM was stopped and snapshotted first as `preGuidsMerge`: database version 9325 with custom 12 and the shakeout state above, in housing's old shape. The rework built here at 7e4b0430c (`make -k -j8`, 152 seconds, no errors, no warnings) and the unit tests pass, 148 of 148 (`cd ~/server && ./bin/tests`).

Git runs in the container, since the host's `sudo git` stops at "dubious ownership": `sudo docker exec -u eqemu akk-stack-eqemu-server-1 bash -c 'cd ~/code && git ...'`. Three scripts in the container's `/tmp` do the routine steps, each run as `sudo docker exec -u eqemu akk-stack-eqemu-server-1 bash /tmp/<script>`: `stopServer.sh` (the launcher stop and the `pkill`), `buildMerged.sh` (fast-forward to `origin/housing`, build, the log in `/tmp/buildMerged.log`) and `startServer.sh`.

On the host, three helpers reach `peq` as root, the password going from `.env` into the environment and never printed: `~/runSQLFile.sh <file>` (a SQL file, table output), `~/q.sh -e '<query>'` (bare rows; it reads its standard input, so a loop gives it `</dev/null`) and `~/dumpAsRoot.sh <mysqldump arguments>`.

### The fresh database, 2026-11-10

Housing's custom migrations are edited in place, so only a database that never ran them proves them (worklist.md PRC-13). The database was rebuilt from the dump taken before housing's first deploy, `backup/peq-20261024-beforeHousing.sql.gz`: version 9325, no custom migration, no housing table. The database it replaced (the old shape, migrated to 9330) is kept as `backup/peq-20261110-beforeFreshPass.sql.gz`. With the server stopped:

1. `peq` dropped, created and restored from the dump (137 seconds).
2. `./bin/world database:updates --skip-backup`: 9325 to 9330, then custom 1 to 12, every step ok. The custom manifest runs after the main one, so housing's holder statement finds 9328's `item_instances`.
3. The shape checks, `~/housingCheck/shapeChecks.sql` through `~/runSQLFile.sh` (the table below).
4. The content as on 2026-10-24: `./bin/import_client_files eqstr`, deployHousingContent.py (all 11 files in one pass), the lobby SQL, `./bin/export_client_files`. Both exported files match the client's copies, so the client needed none.
5. Claudehousing: the dump has the account and not the character. `~/housingCheck/claudehousingCharacter.sql` holds its rows of the seven base character tables (character_data, character_bind, character_currency, character_exp_modifiers, character_languages, character_skills, character_stats_record) as the old database had them, with no item; it stands at the Plane of Knowledge's safe point.
6. The `Housing`, `Warning` and `Error` log categories set to console and file.
7. The server started, and world made the neighborhoods.

| Check | Result |
|---|---|
| version, custom version | 9330, 12 |
| items, retired | 3162, 0 |
| old item tables left | 0 |
| `real_estate` tables | 24 |
| `item_instances.holder_type` and its location CHECK | carry `real_estate` |
| real_estate_properties | `container_guid` words, unique index, foreign key to `item_instances`, the whole-or-null CHECK |
| real_estate_items | GUID words, unique index, foreign key to `item_instances`, no item column |
| properties made by world | 760: 710 plots, 10 neighborhoods, 40 guild plots |

**State:** the server runs the rework. Claudehousing owns no property and carries nothing; its account has status 250, so the tests summon what they place.

### The pre-build tools

The tools of serverPlan.md's "Proven before the build" that need a MariaDB run on the host against a scratch server, a container of the stack's own image with an empty `peq`:

```
sudo docker rm -f housingTest
sudo docker run -d --name housingTest -e MARIADB_ALLOW_EMPTY_ROOT_PASSWORD=1 -e MARIADB_DATABASE=peq akk-stack-mariadb:latest
```

It answers after about 15 seconds (MariaDB 10.11.19). The tools call `mariadb`, so `~/bin/mariadb` is a wrapper (`exec sudo docker exec -i housingTest mariadb "$@"`) put first in `PATH`. The tools and the files they read go to `~/housingToolsTest` in the repo's own layout, Live's dump tables to `~/housingToolsTest/live/master` (`definitions.tsv`, `costs.tsv`, `properties.tsv` from everquest-live-dumps):

```
cd ~/housingToolsTest
PATH=~/bin:$PATH python3 playerHousing/tools/testHousingSchema.py
PATH=~/bin:$PATH python3 playerHousing/tools/testHousingCatalog.py --live ~/housingToolsTest/live
```

## The client

`S:\Research\EverQuest\peridothousing` (on onyx `C:\Games\EverQuest - peridothousing`): a copy of the stock RoF2 install (`S:\Research\EverQuest\housingclient`, which stays stock as the clean install makeClientFiles.py reads) with the eqemu-client-files `rof2/` tree laid over it and `eqhost.txt` at `peridothousing.strata.unseencolor.com:5999`. claudelink drives it as it drives any client copy: `launchClient(clientFolder = S:\Research\EverQuest\peridothousing, loginServer = Peridot, login = services/peridothousing/eqLogin, character)`.

Claude's account there is `claudehousing`, its password in OpenBao at `services/peridothousing/eqLogin`; the clone's login server creates accounts at their first login (`login.json` `auto_create_accounts`), and the first launch on 2026-10-24 made login account 85 and game account 86 at status 0. Before a character enters the world: the game account's status raised for the GM commands the validation uses, and the per-character MacroQuest config `Peridot_<character>.cfg` in mqPeridot's config folder (MQ2EQBC and mq2netbots unloaded, MQClaudeLink loaded, each with `noauto`), as claudeperidot's character has, so the client never joins the owner's EQBCS.

## Open

- The rework's tests in game (worklist.md PLC-24, REC-4, REC-14, STT-19, RCL-5, MNU-13) and the fresh shakeout pass from "Chunk 8 in game".
- The pre-build tools still to rerun (worklist.md PRC-3, DOR-20).
