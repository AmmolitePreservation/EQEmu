# RoF2-only worklist

Status tags: `done`, `fix` (to build), `owner` (the owner's call), `peridotTest`.

## Owner decisions

- **One client (2026-11-01):** every client but Rain of Fear 2 goes; `guids` is rebased onto this branch and drops `trader.item_sn` there; player housing follows `guids`; bots are rewritten after housing.
- **Database columns (2026-11-01):** look for columns the removed clients made necessary and bring them with evidence, no guesses.

## Items

- **RC-1** `done` **The removal.** README.md's "What goes" and "What stays, and why". Built 2026-11-01 from `main`'s tip 6fedcae84; tested on peridotguids as README.md's "Tests" says.
- **RC-2** `owner` **Database columns and rows the removed clients gave meaning to.** None is required by code any more; each is content or history, so none was changed. With Peridot's data:
  - `doors.client_version_mask` and `zone_points.client_version_mask`: ProjectEQ content, read by the code to hide a door or zone point from RoF2 when its bit is clear (6 doors, 17 zone points on Peridot). Dropping them would show those rows and break ProjectEQ content imports, which carry the columns. Recommendation: keep.
  - `start_zones.player_choice`: Titanium's starting city choice, read by no code now; 389 of Peridot's 420 rows set it. ProjectEQ content. Recommendation: keep, unused.
  - `bug_reports.client_version_id` and `client_version_name`: the reporting client, always RoF2 now; Peridot has no bug reports. A server table. Options: drop both with a migration, or keep them as a record.
  - `merc_templates.clientversion` and `merc_types.clientversion`: ProjectEQ mercenary content gating types by client; the code no longer reads them. Peridot has no mercenary tables. Recommendation: keep, unused.
  - `rule_values` rows for removed or renamed rules: `World:SupportedClients` (Peridot: `Titanium,SoF,SoD,UF,RoF,RoF2`), `World:SoFStartZoneID` (-1, two rule sets), `World:TitaniumStartZoneID` (-1), `Spells:FRProjectileItem_Titanium` (1113), `Spells:FRProjectileItem_SOF` (80684). EQEmu ignores a rule row it does not know and the renamed rules keep their default values, so behaviour is unchanged on Peridot; the rows are stale. Options: a migration that deletes the removed rows and renames the two renamed ones, or leave them.
- **RC-3** `peridotTest` **Character creation through the client.** World's check and start zone lookup are the SoF-and-later paths RoF2 already took, renamed; not yet exercised from the client's creation window.
- **RC-4** `peridotTest` **Mercenaries, tasks and the guild management window.** Code paths the collapse touched (mercenary type filtering, task serialization, guild rank handling) that this server's content did not exercise; Peridot has no mercenary tables.
