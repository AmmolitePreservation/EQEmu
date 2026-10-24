# peridothousing: the housing development server

Claude's server for building and validating player housing, apart from Peridot and from claudeperidot (zonewright's). Nothing on it is production; it can be rebuilt from Peridot at any time.

## The machine

| | |
|---|---|
| Proxmox | VM 144 on labradorite, a full clone of Peridot (VM 104) made 2026-10-24 with the guest filesystem frozen for the copy |
| Size | 8 cores, 16 GB (Peridot has 16 and 32); no autostart; tags `peridothousing;eqemu;ubuntu` |
| Disk | `alpha:144/vm-144-disk-0.qcow2`, 350 GB virtual |
| Network | DHCP on `ens18`, MAC `BC:24:11:4D:E5:B4`, leased `10.111.111.47`; A record `peridothousing.strata.unseencolor.com` on olivine; no DHCP reservation yet |
| Access | facet host `peridothousing`; Claude's login is Peridot's account, copied to OpenBao `hosts/peridothousing/claude`; SSH host keys regenerated on the clone |

What was changed from Peridot's copy, before its network link came up: the hostname and `/etc/hosts`, the Zabbix agent's `Hostname`, the Alloy `host` labels, the Zabbix MySQL plugin's address, the akk-stack `.env` `IP_ADDRESS` and the world's `address` and `localaddress` in `server/eqemu_config.json` (`peridothousing.strata.unseencolor.com`). The containers were recreated for the new address, and the services Peridot keeps stopped (backup-cron, phpmyadmin, ftp-quests, peq-editor) were stopped again. The login server is the stack's own on `5998`/`5999`; nothing registers with a public login server.

## The server

The akk-stack at `/opt/eqemu/akk-stack`, Peridot's database as it stood at the clone (Expansion:CurrentExpansion 4, Planes of Power) and Peridot's quests. The housing build, its migrations and content go here first; the Peridot-only lobby SQL (`sql/peridot/guildLobbyExpansions.sql`) applies here as it would on Peridot.

## The client

`S:\Research\EverQuest\peridothousing`: a copy of the stock RoF2 install (`S:\Research\EverQuest\housingclient`, which stays stock as the clean install makeClientFiles.py reads) with the eqemu-client-files `rof2/` tree laid over it and `eqhost.txt` at `peridothousing.strata.unseencolor.com:5999`. claudelink's spare path still names housingclient, so driving this client needs claudelink to accept a second spare.

## Open

- A DHCP reservation for `BC:24:11:4D:E5:B4` (the sonicwall MCP, once its session reads OpenBao).
- A Zabbix host for the agent, which now reports as peridothousing.
- The claudelink spare path.
