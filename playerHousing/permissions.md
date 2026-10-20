# Real Estate Permissions (client-side evaluation)

The client evaluates real-estate permissions itself to decide control states (for example the Purchase window's voting opt-out uses permission `0xc`). This file records the evaluator as it is decoded. Permission **names** are not mapped yet; the Manage window's access page (`REMW_Access_Combobox`) is expected to give them. **Permission `0xc` = manage access**: the Purchase window's upkeep-combobox check prints `0x1523` "You cannot do that because you do not have manage access" when it fails (with `0x14c7` House of Thule and `0x1535` **Veil of Alaris** as the other failure messages, which also confirms that `0x20000` is Veil of Alaris).

## Permission and group names (from the Manage window)

**Permissions** (the Access page's grid rows; the row index **is** the permission id; names from the table at `0xb41c7c`):

| Id | Name | Used by (client) |
|---|---|---|
| 0 | Receive Buffs | — |
| 1 | Enter | *(entering houses; server-side)* |
| 2 | **Withdraw Upkeep** | **nothing on the client**: escrow withdrawals (`0x2aa6` types 1/7) have no client check, so **the server must enforce this** |
| 3 | Remove Crate | placement rules |
| 4 | Add Crate | placement rules (`0x4d`); layout Use Inventory |
| 5 | Remove House | stored-item rules (`0x4c`) |
| 6 | Add House | placement rules (`0x4b`); layout Use Inventory |
| 7 | Mail Item | — |
| 8 | Remove Item | — |
| 9 | Add Item | placement rules (`0x47`); layout Use Inventory |
| 10 | Move Item | **the Items window's may-manage gate** (`0x5b71a0`, manager vtable `+0x14`): the window's enable rules and **Stow Placed**; codes House of Thule `0x2f`, Veil of Alaris `0x52`, membership `0x53` |
| 11 | Interact Item | **using a placed item, not placing it**: the placed-item interaction check (`0x5b75f0`, through the manager's vtable), codes too far `0x14`, not placed in the zone `0x15`, house or crate upkeep expired `0x19`, no interact access `0x4a` |
| 12 | **Manage** | the Access tab; may-manage gate; editing the access list |

**Every client call of the permission check** (`0x4ff6c0`, 32 sites, the permission pushed as a constant): 3, 5, 8 in the stored-item rules (`0x5b72c0`); 4, 6, 9 in the placement rules (`0x5b6b90`) and the Items window (`0x73ab10`, `0x73bc50`, `0x73bec0`); 5 and 6 together (`0x5b6eb0`); 2 and 3 (`0x5b7fa0`); 10 alone (`0x5b71a0`); 11 alone (`0x5b75f0`); 12 in the Manage and Purchase windows; and 3-10, any of them (`0x4ff730`). Permissions 0, 1 and 7 are never checked by the client.

**The server checks every permission itself** (Live's menus, ACC-3 and ACC-4): Enter (1) for the Enter entry and entering an interior, and an edit that takes it away moves anyone inside to the plot's exit point; Receive Buffs (0) on a guild hall's regeneration pool aura; Withdraw Upkeep (2) on `0x2aa6` withdrawals; Remove Crate (3) on a crate's Move to Inventory; Mail Item (7) on Mail to Owner; Move Item (10) on Adjust Placement, Place and Move to Storage; Interact Item (11) on a guild service's Transact for anyone outside the owning guild (its members always get it); Manage (12) on `0x3146`; and the placement rules' add and remove bits.

**Groups** (`REMW_Groups_Combobox` choices; the 7 masks): **0 Everyone, 1 Visitor, 2 Acquaintance, 3 Friend, 4 Trustee, 5 Co-Owner, 6 Owner**. A person or club can be assigned groups 1–5 (`REMW_Access_Combobox`: Visitor … Co-Owner); Everyone is implicit and Owner is the owner.

**Clubs:** the owner's **fellowship** and **guild ranks** are stored in the same name → group list under reserved keys: **`_Fellowship`** and **`_Guild<rank>`** (one entry per guild rank; `0x4fef40`, `0x4feff0`). A character's group on a property (`0x4ff250`) uses their name and character **`+0x204c`** (guild id), **`+0x2054`** (**fellowship id**) and **`+0x205c`** (**guild rank**); see *Group computation*.

**Who may edit what** (Manage window, `0x744200`): **may edit** = GM, or the owner, or the viewer's group holds **Manage (12)**. A grid cell (permission `p`, group `g`) is editable only if **`g` is below the viewer's own group and the viewer's group holds `p`**, or the viewer is the Owner or a GM. The Owner column is always checked and disabled. The server should enforce the same rules on `0x3146`, plus the Add/Remove rules (no adding above your own group, only removing lower groups unless owner/GM) and **name validity** (the client never checks that an added name is a real character).

## Entry point `0x4ff6c0(permission, record, character)`

Gathers from the character:
- name (profile `+0x44`)
- guild id (`+0x204c`)
- `+0x2054` (fellowship id) and `+0x205c` (guild rank)
- **expansion flags** (character virtual `+0x5c`, the same source as Q1)

Then calls `0x4ff570`. Returns false if the record or character is null.

## Evaluator `0x4ff570(permission, record, name, guildID, +0x205c, +0x2054, expansionFlags, …)`

Checks in order:
1. Null record → **deny**.
2. `ownerFlag` = `0x4d6b10(name, record)` (the character is the owner of the containing player Plot; see `windows/RealEstatePurchaseWnd.md`).
3. **Expansion requirement** (skipped if `ownerFlag` and the permission is 2, 3, 5, or 8): `required = 0x4ff440(permission, record)`. If `required & ~expansionFlags` ≠ 0 → **deny**.
   - `0x4ff440`: if the record has a **Guild Plot** ancestor (`0x4d6790(record, 0x80, 0)`): permissions 2-10 → requires **`0x20000`**; **any other permission (0, 1, 11, 12) → no requirement** (`0x4ff486` jumps to `0x4ff4d5`, which returns 0).
   - With no Guild Plot ancestor: every permission but 1 (Enter) → requires **`0x10000`** (House of Thule); Enter → no requirement.
   - Reading: every real-estate permission needs the House of Thule bit, and guild-plot permissions need `0x20000`, which fits the client strings requiring **Veil of Alaris** for guild plots. That bit is inferred, not verified.
4. `0x4ff4e0(permission, record, character, ownerFlag)` ≠ 0 → **deny** (read in full): an owner with permission 2, 3, 5 or 8 passes; otherwise, under a Guild Plot ancestor, permissions 2-10 need **membership feature 10** (without it the client shows the membership message and denies); anything else passes. **The server doesn't check feature 10**: EQEmu has no membership tiers, and Peridot grants Gold (feature 10 = 1).
5. If the record is a house (category 2) or crate (category 6), move to its containing plot (`0x4d6790(record, 0x88, 1)`); none → **deny**. **Category 2 is both Guild Hall (type 2) and Player House (type 3)**, so a guild hall uses its guild plot's list (`0x7f60f0` / `0x7f61b0`, types to categories verified entry by entry: 1 Regular Zone 1, 2 Guild Hall 2, 3 Player House 2, 4 Plot 3, 5 Neighborhood 4, 6 Town 5, 7 Moving Crate 6, 8 Guild Plot 3).
6. Plot (type 4) and `0x4d6b10` owner → **allow**.
7. Guild Plot (type 8), record guild id == character guild id, and the evaluator's guild-rank argument == 1 → **allow**.
8. Otherwise, the access list: `list = 0x4ff100(record +0x14 property id)`; none → **deny**; otherwise `0x4ff210(list, 0x4ff250(record, name, guildID, …))`.

### Group computation `0x4ff250(record, name, guildID, rank, …)`

Returns the character's **group index** (0–6) for the property:
1. For a house or crate, move to its containing plot (none → group 0).
2. Owner of a player Plot (`0x4d6b10`) → **6**.
3. Guild Plot whose guild == the character's guild with rank argument == 1 → **6**.
4. Otherwise, look up the property's access list (manager map `+0x54`; none → 0) and, in order:
   - the entry for the **character's name** (`0x4ff130(list, name)`)
   - if a character value (`+0x2054` or `+0x205c`) equals record **`+0x28`**: the entry for the key **`_Fellowship`** (`0x4fef40`: `sprintf("%c%s", '_', "Fellowship")`)
   - if the character's guild == record `+0x24`: the entry for the key **`_Guild<rank>`** (`0x4feff0`: `sprintf("%c%s%d", '_', "Guild", rank)`, called with the evaluator's **rank** argument, `[esp+0x2c]` at `0x4ff3d4`, after the guild id check at `0x4ff3c6`-`0x4ff3d2`)

### Access check `0x4ff210(list, group, permission)`

Tests bit `permission` of the u32 at **list `+0x04 + 8 × group`**. The list holds **seven pairs `{u32 group number, u32 permission mask}`** at `+0x00`-`+0x37` (pair `g` at `+8g`); the test reads the **second** u32 of pair `g`. So each property has **one 32-bit permission mask per group** (permission ids 0-12 use the low 13 bits), and groups are assigned by name, club key, or guild/rank key. (Verified: constructor `0x5b5250` numbers the first u32 of each pair 0-6; reader `0x5b7a90` stores the wire pairs at `+8g` / `+8g+4`; test `0x4ff210`.)

Also in `0x4ff4e0`: for guild-plot permissions 2–10 the character-side virtual slot 0 is called with **feature id 10**. If it returns 0, the client shows a "requires purchase" style message (`0x7dbc50(10, 1, 1, 1)` → `0x7db9c0`, the same mechanism as `0x66bf` code 13) and **denies**. **Feature 10 is an account membership feature**: the client's feature table (`0xa0aa28`) names it "access to a number of guild hall functions". It's evaluated by the membership manager (`0x7dbc50` → `0x7db9c0`), and the message comes from `0x7dbd10`: "Your %s %s been %s because you do not have at least a %s membership."

## Access-list messages (server → client)

Both use the list reader **`0x5b7a90`** (list object 0x48 bytes: the seven pairs at `+0x00`-`+0x37`, then the name -> group table at `+0x38`; constructor `0x5b5250`, then `0x5b5830`):

```
7 × { u32 groupNumber, u32 permissionMask }   groups 0..6 in order; the client tests the SECOND u32 (the mask)
u32   count
count × { string name, u32 group }            name -> group; names include individual names and synthetic club / guild-rank keys
```

**Send the group number (0-6) in the first u32 of each pair and the mask in the second.** A server that treats the pair as one 64-bit mask with the permission bits in the first u32 makes every permission read as denied.

**Client defaults** (`0x5b5830` applies them to every new list before the server's list is read over it; `0x5b3d50(group, permission)`). They are also **Live's own defaults for a new player plot** (7,962 of 7,993 captured lists), which the server writes at purchase (`Housing:DefaultAccess*`):

| Group | Mask | Permissions |
|---|---|---|
| 0 Everyone | `0x0003` | Receive Buffs, Enter |
| 1 Visitor | `0x0003` | Receive Buffs, Enter |
| 2 Acquaintance | `0x0803` | Receive Buffs, Enter, Interact Item |
| 3 Friend | `0x0e03` | Receive Buffs, Enter, Add Item, Move Item, Interact Item |
| 4 Trustee | `0x1f87` | Receive Buffs, Enter, Withdraw Upkeep, Mail Item, Remove Item, Add Item, Move Item, Interact Item, Manage |
| 5 Co-Owner | `0x1fff` | all thirteen |
| 6 Owner | `0x0000` | none (the owner is recognized before the list is consulted) |

**A new guild plot's list differs** (Live, ACC-1): masks `0x0000`, `0x0003`, `0x0803`, `0x0e03`, `0x1f87`, `0x1f87`, `0x0000` from Everyone up (Everyone may not enter, and Co-Owner matches Trustee), plus an entry for each rank but Leader: `_Guild2` group 5, `_Guild3` 4, `_Guild4` to `_Guild6` 3, `_Guild7` and `_Guild8` 2.

| Opcode | Payload | Effect | Evidence |
|---|---|---|---|
| **`0x485d`** | `u32 propertyID`, then the access list | **Set or replace** the property's access list (`0x5b7de0` → manager map `+0x54`); the Manage window refreshes before (`0x747ed0`) and after (`0x7465c0`) | case `0x5b2abd` |
| **`0x3146`** (client → server) | `u32 propertyID`, then that property's full access list (serializer `0x749420`) | **Set the access list**: the Manage window queues the plot id on every local edit to map `+0x54` (`0x5b0b10` set a name's/club's group, `0x5b0bb0` remove an entry, `0x5b0ad0` set a group's permission bit) and flushes the queue every 3 s (`0x747ed0`); the `0x485d` handler flushes it first | Manage `0x747ed0` |
| **`0x3968`** | `u32 propertyID`, an access list (parsed into a temporary and discarded), then `u32` | **Remove** the property's access list (`0x5b78d0`); Manage window refresh `0x7465f0`. The trailing u32 goes to `0x5b59a0` when global `[0xdcee30]` is 0 (not understood). | case `0x5b28eb` |

**Test preconditions this implies:** the character's expansion flags must include `0x10000` for any real-estate permission, and `0x20000` for guild plots. Non-owners need the property's access list (with no list the evaluator denies): the server sends it with `0x485d` (and receives edits as `0x3146`) for plots, guild plots, neighborhoods and guild halls (none for houses); the client evaluates a house, hall or crate by its plot's list.

**Group computation** (`0x4ff250(record, name, guild id, guild rank, fellowship id)`; callers pass character `+0x204c`, `+0x205c`, `+0x2054`): a house or crate uses its plot's record (`0x4d6790`); on a **player plot**, the owner's name -> **6 (Owner)**; on a **guild plot**, a character in the plot's guild (record `+0x24`) **with guild rank 1 -> 6 (Owner)**. Otherwise the plot's access list (map `+0x54`, by record `+0x14`) gives the **highest** of: the **name** entry (`0x4ff130`, map lookup `0x5b9eb0`); **`"_Fellowship"`** if the character's fellowship id (`+0x2054`) equals **record `+0x28` (the owner's fellowship id)**; **`"_Guild<rank>"`** if the character's guild (`+0x204c`) equals **record `+0x24`**. The reserved keys are built by `0x4fef40` (`"%c%s"`: `_Fellowship`) and `0x4feff0` (`"%c%s%d"`: `_Guild` + the guild rank).

**The server picks the group the same way: the most permissive matching entry wins**, and a matching entry replaces Everyone rather than adding to it (Live). An explicit deny would beat any match (owner's decision); Live's lists hold none, since a name or club can only be assigned Visitor to Co-Owner. EQEmu has no fellowships, so the owner's fellowship id goes out as 0 and `_Fellowship` entries never match.

## Guild rank permissions and account membership (Q9, Q21, Q22)

**Guild rank permissions** (`0x4241f0(permission)`, used for guild plots: **27 = buy, 28 = sell**): a GM bypass (character vtable `+0x90` = the spawn's GM flag `+0x25c`, Q7), otherwise `0x7d96c0`(character `+0x205c` = the player's **rank**, permission) on the **static guild object at `0xdd5cf8`**. Its rank table (`+0x9318`, 0x58 bytes, `0x7d9650`) holds **9 entries {u32 rank number, u32 permission mask} for ranks 0–8, all masks 0 by default**; a permission below 32 is bit (id) of the rank's mask. **So by default no rank may buy or sell guild plots.** The table is rebuilt from defaults when you leave the guild (`OP_GuildMemberDelete`) or the guild is deleted (`OP_GuildDeleteGuild`).

**Setting a rank permission: `OP_GuildUpdate` (`0x2958`) action 5**: payload `+0x00` u32 **action = 5**, `+0x50` u32 **rank**, `+0x54` u32 **permission id**, `+0x58` u8 **value** (1 grant, 0 revoke) -> `0x7d95e0`. **EQEmu 23.8.1 already sends it**: `Client::SendGuildRanks()` in `zone/guild.cpp`, through `GuildUpdateRanks_Struct`, for every rank 1–8 and permission 1–30 on zone-in, with values from the `guild_permissions` table. So guild plot buy (27) and sell (28) are rows in that table (confirmed live: a Leader's mask of `0x7ffffffe`). (Other actions share the frame: 3 guildmate achievement, 4 guild rename (`+0x50` guild id, `+0x54` name).) The guild object also has a key=value settings blob (`VERSION`, `GUID`, `URL`, `CHAN`, `PDS_FLAGS_VALID`, and "rank^mask" pairs; `0x7da560`, `0x7d9c40`), reached through its vtable; its trigger wasn't found and action 5 is sufficient.

**Account membership** (feature **10 = "Create Guilds?"**, required for guild plots): the housing checks (`0x5b5b90`, `0x5b5ff0`, `0x5b69b0`, `0x4ff4e0`) first ask the player's own membership values (vtable slot 0 of the membership object; **`OP_SendMembership` `0x7acc`**, entry 10) and otherwise consult the tier table (**`OP_SendMembershipDetails` `0x057b`**, `0x7dbe90`: {tier, feature, value} settings; getter `0x7dbc50`, lookup `0x7db9c0`), refusing with **code `0x53`** when a higher tier would allow it. **EQEmu's world server sends Gold (membership 2), entry 10 = 1, and value 1 for feature 10 in every tier, so the membership gate already passes with stock EQEmu.** Feature 13 ("Neighborhood-House Allowed?") is not checked by any housing code in this client.

## Guild rank permission ids and rank names (client table `0xb617d0`)

The client's guild name table gives the rank names and the permission names; **permission id = table index − 10**, which matches the ids the housing code checks (27, 28, 29) and the trophy window's (24, 25).

**Ranks:** 1 Leader, 2 Senior Officer, 3 Officer, 4 Senior Member, 5 Member, 6 Junior Member, 7 Initiate, 8 Recruit (0 = None). Rank 1 is why the leader counts as Owner on a guild plot.

| Id | Permission | Id | Permission |
|---|---|---|---|
| 0 | None | 16 | Bank: Promote Items |
| 1 | Banner: Change | 17 | Bank: Change Item Permissions |
| 2 | Banner: Plant | 18 | Change the MOTD |
| 3 | Banner: Remove | 19 | Guild Chat: See |
| 4 | Display Guild Name | 20 | Guild Chat: Speak In |
| 5 | Ranks: Change Permissions | 21 | Send the Whole Guild E-Mail |
| 6 | Ranks: Change Rank Names | 22 | Tribute: Change for Others |
| 7 | Members: Invite | 23 | Tribute: Change Active Benefit |
| 8 | Members: Promote | **24** | **Trophy Tribute: Change for Others** |
| 9 | Members: Demote | **25** | **Trophy Tribute: Change Active Benefit** (trophy window Modify, Upgrade, Downgrade) |
| 10 | Members: Remove | 26 | Members: Change Alt Flag for Others |
| 11 | Edit Recruiting Settings | **27** | **Real Estate: Guild Plot - Buy** |
| 12 | Edit Public Notes | **28** | **Real Estate: Guild Plot - Sell** |
| 13 | Bank: Deposit Items | **29** | **Real Estate: Modify Trophies** (the Items window's trophy rule) |
| 14 | Bank: Withdraw Items | 30 | Members: Demote Self |
| 15 | Bank: View Items | | |

**A server-only rule:** string 5426 ("…your guild only has %1 members. In order to purchase a guild plot, your guild must have at least %2 members.") is never shown by client code, so Live enforced a **minimum guild size for guild plots** on the server. Peridot enforces it with `Housing:GuildPlotMinimumMembers` (10, Veil of Alaris' launch value).

**The ancestor walk** (`0x4d6790(record, mask, skip)`) tests each property's definition type against `mask` through **`0x7f5e30`, verified entry by entry: type `n` → bit `1 << (n - 1)`** (1 Regular Zone 0x1 ... 4 Plot 0x8, 5 Neighborhood 0x10, 7 Moving Crate 0x40, 8 Guild Plot 0x80); `skip` levels are passed before testing (1 = start at the parent), and it gives up after 10 levels.
