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

Changed from Peridot's copy while its network link was still down (`link_down=1` on net0): the hostname and `/etc/hosts`, the Zabbix agent's `Hostname`, the Zabbix MySQL plugin's address, the Alloy `host` labels, the akk-stack `.env` `IP_ADDRESS`, and the world's `address` and `localaddress` in `server/eqemu_config.json` (`peridotguids.strata.unseencolor.com`; Peridot's say `peridot.unseencolor.com`). The world registers only with the stack's own login server (`127.0.0.1:5998`). The containers were recreated for the new address; pok and EQBCS, which Peridot runs, are stopped and set not to restart.

## Snapshots

| Snapshot | State |
|---|---|
| `pristine` | The clone set up, the `guids` build running, the database at 9325 / bots 0 / custom 0 with no migration applied; RAM included |
| `ready` | `pristine` plus the claudelink login and character below, camped; RAM included |

`qm rollback 145 ready` (or `pristine`) returns to that running state. Both include RAM because the stack's server and database containers did not come back by themselves after a reboot of the clone (PRC-4).

## The server

The akk-stack at `/opt/eqemu/akk-stack`, Peridot's database as it stood at the clone (dumped first: `backup/peq-2026-10-26-beforeGuids.sql.gz`, 35 MB) and Peridot's quests. The stack's `code/` is a clone of AmmolitePreservation/EQEmu on `guids`, built inside the `eqemu-server` container as the `eqemu` user (`cd ~/code/build && make -j8`, clang, RelWithDebInfo, 161 s; `server/bin` links to `code/build/bin`). The server processes are stopped and started as on peridothousing (`./bin/spire eqemu-server:launcher stop` from a script file, since it kills any shell whose command line names the launcher, then `pkill` of world, zone, ucs, loginserver and queryserv; `launcher start` brings them back).

## The client

`C:\Games\EverQuest - peridotguids` on onyx: the pristine RoF2 install (`C:\Onyx\research\rof2Pristine\everquest_rof2`) with `eqgame.exe` made large-address-aware, `eqhost.txt` at `peridotguids.strata.unseencolor.com:5999` and onyx's window settings in `eqclient.ini`. None of housing's client files: `guids` predates housing. claudelink drives it with `launchClient(clientFolder = C:\Games\EverQuest - peridotguids, loginServer = Peridot, login = services/peridotguids/eqLogin, character = Claudeguids)`.

Claude's account is `claudeguids` (login account 85, game account 86 at status 250), its password in OpenBao at `services/peridotguids/eqLogin`; the login server made it at the first login. `Claudeguids` (character 117) is a level 1 human male wizard bound at the Plane of Knowledge's safe point, written with the rows EQEmu's character creation writes, as Claudehousing and Claudewright were.
