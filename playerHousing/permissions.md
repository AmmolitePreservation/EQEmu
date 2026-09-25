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
| 10 | Move Item | — |
| 11 | Interact Item | the placed-item interaction check (`0x4a`) |
| 12 | **Manage** | the Access tab; may-manage gate; editing the access list |

**Groups** (`REMW_Groups_Combobox` choices; the 7 masks): **0 Everyone, 1 Visitor, 2 Acquaintance, 3 Friend, 4 Trustee, 5 Co-Owner, 6 Owner**. A person or club can be assigned groups 1–5 (`REMW_Access_Combobox`: Visitor … Co-Owner); Everyone is implicit and Owner is the owner.

**Clubs:** the owner's **fellowship** and **guild ranks** are stored in the same name → group list under reserved keys: **`_Fellowship`** and **`_Guild<rank>`** (one entry per guild rank; `0x4fef40`, `0x4feff0`). A character's group on a property (`0x4ff250`) uses their name and character **`+0x204c`** (guild id), **`+0x2054`** (**fellowship id**) and **`+0x205c`** (**guild rank**); see *Group computation*.

**Who may edit what** (Manage window, `0x744200`): **may edit** = GM, or the owner, or the viewer's group holds **Manage (12)**. A grid cell (permission `p`, group `g`) is editable only if **`g` is below the viewer's own group and the viewer's group holds `p`**, or the viewer is the Owner or a GM. The Owner column is always checked and disabled. The server should enforce the same rules on `0x3146`, plus the Add/Remove rules (no adding above your own group, only removing lower groups unless owner/GM) and **name validity** (the client never checks that an added name is a real character).

## Entry point `0x4ff6c0(permission, record, character)`

Gathers from the character:
- name (profile `+0x44`)
- guild id (`+0x204c`)
- `+0x2054` and `+0x205c` (unknown)
- **expansion flags** (character virtual `+0x5c`, the same source as Q1)

Then calls `0x4ff570`. Returns false if the record or character is null.

## Evaluator `0x4ff570(permission, record, name, guildID, +0x205c, +0x2054, expansionFlags, …)`

Checks in order:
1. Null record → **deny**.
2. `ownerFlag` = `0x4d6b10(name, record)` (the character is the owner of the containing player Plot; see `windows/RealEstatePurchaseWnd.md`).
3. **Expansion requirement** (skipped if `ownerFlag` and the permission is 2, 3, 5, or 8): `required = 0x4ff440(permission, record)`. If `required & ~expansionFlags` ≠ 0 → **deny**.
   - `0x4ff440`: if the record has a **Guild Plot** ancestor (`0x4d6790(record, 0x80, 0)`) and the permission is one of 2, 3, 4, 5, 6, 7, 8, 9, 10 → requires **`0x20000`**.
   - Otherwise, for permissions 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 → requires **`0x10000`** (House of Thule).
   - Otherwise → no requirement.
   - Reading: every real-estate permission needs the House of Thule bit, and guild-plot permissions need `0x20000`, which fits the client strings requiring **Veil of Alaris** for guild plots. That bit is inferred, not verified.
4. `0x4ff4e0(permission, record, arg, ownerFlag)` ≠ 0 → **deny** (partly read: an owner with permission 2, 3, 5, or 8 passes; otherwise it checks against a Guild Plot ancestor).
5. If the record is a house (category 2) or crate (category 6), move to its containing plot (`0x4d6790(record, 0x88, 1)`); none → **deny**.
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
   - if a character value (`+0x2054` or `+0x205c`) equals record **`+0x28`**: the entry for the key built by `0x4fef40` (a club or fellowship style key; not read)
   - if the character's guild == record `+0x24`: the entry for the key built by `0x4feff0(guild, rank)` (a guild/rank key; not read)

### Access check `0x4ff210(list, group, permission)`

Tests one bit: `list.masks[group]` is a 64-bit mask stored as two u32s, and the bit tested is `permission`. So each property has **one 64-bit permission mask per group**, and groups are assigned by name, club key, or guild/rank key.

Also in `0x4ff4e0`: for guild-plot permissions 2–10 the character-side virtual slot 0 is called with **feature id 10**. If it returns 0, the client shows a "requires purchase" style message (`0x7dbc50(10, 1, 1, 1)` → `0x7db9c0`, the same mechanism as `0x66bf` code 13) and **denies**. **Feature 10 is an account membership feature**: the client's feature table (`0xa0aa28`) names it "access to a number of guild hall functions". It's evaluated by the membership manager (`0x7dbc50` → `0x7db9c0`), and the message comes from `0x7dbd10`: "Your %s %s been %s because you do not have at least a %s membership."

## Access-list messages (server → client)

Both use the list reader **`0x5b7a90`** (list object 0x48 bytes, constructor `0x5b5250`, then `0x5b5830`):

```
7 × { u32 maskLow, u32 maskHigh }        permission mask for groups 0..6 (group 6 = owner)
u32   count
count × { string name, u32 group }       name -> group; names include individual names and synthetic club / guild-rank keys
```

| Opcode | Payload | Effect | Evidence |
|---|---|---|---|
| **`0x485d`** | `u32 propertyID`, then the access list | **Set or replace** the property's access list (`0x5b7de0` → manager map `+0x54`); the Manage window refreshes before (`0x747ed0`) and after (`0x7465c0`) | case `0x5b2abd` |
| **`0x3146`** (client → server) | `u32 propertyID`, then that property's full access list (serializer `0x749420`) | **Set the access list**: the Manage window queues the plot id on every local edit to map `+0x54` (`0x5b0b10` set a name's/club's group, `0x5b0bb0` remove an entry, `0x5b0ad0` set a group's permission bit) and flushes the queue every 3 s (`0x747ed0`); the `0x485d` handler flushes it first | Manage `0x747ed0` |
| **`0x3968`** | `u32 propertyID`, an access list (parsed into a temporary and discarded), then `u32` | **Remove** the property's access list (`0x5b78d0`); Manage window refresh `0x7465f0`. The trailing u32 goes to `0x5b59a0` when global `[0xdcee30]` is 0 (not understood). | case `0x5b28eb` |

**Test preconditions this implies:** the character's expansion flags must include `0x10000` for any real-estate permission, and `0x20000` for guild plots. Non-owners need an access list entry: the server sends it with `0x485d` (and receives edits as `0x3146`).

**Group computation** (`0x4ff250(record, name, guild id, guild rank, fellowship id)`; callers pass character `+0x204c`, `+0x205c`, `+0x2054`): a house or crate uses its plot's record (`0x4d6790`); on a **player plot**, the owner's name -> **6 (Owner)**; on a **guild plot**, a character in the plot's guild (record `+0x24`) **with guild rank 1 -> 6 (Owner)**. Otherwise the plot's access list (map `+0x54`, by record `+0x14`) gives the **highest** of: the **name** entry (`0x4ff130`, map lookup `0x5b9eb0`); **`"_Fellowship"`** if the character's fellowship id (`+0x2054`) equals **record `+0x28` (the owner's fellowship id)**; **`"_Guild<rank>"`** if the character's guild (`+0x204c`) equals **record `+0x24`**. The reserved keys are built by `0x4fef40` (`"%c%s"`: `_Fellowship`) and `0x4feff0` (`"%c%s%d"`: `_Guild` + the guild rank).

## Guild rank permissions and account membership (Q9, Q21, Q22)

**Guild rank permissions** (`0x4241f0(permission)`, used for guild plots: **27 = buy, 28 = sell**): a GM bypass (character vtable `+0x90` = the spawn's GM flag `+0x25c`, Q7), otherwise `0x7d96c0`(character `+0x205c` = the player's **rank**, permission) on the **static guild object at `0xdd5cf8`**. Its rank table (`+0x9318`, 0x58 bytes, `0x7d9650`) holds **9 entries {u32 rank number, u32 permission mask} for ranks 0–8, all masks 0 by default**; a permission below 32 is bit (id) of the rank's mask. **So by default no rank may buy or sell guild plots.** The table is rebuilt from defaults when you leave the guild (`OP_GuildMemberDelete`) or the guild is deleted (`OP_GuildDeleteGuild`).

**Setting a rank permission: `OP_GuildUpdate` (`0x2958`) action 5**: payload `+0x00` u32 **action = 5**, `+0x50` u32 **rank**, `+0x54` u32 **permission id**, `+0x58` u8 **value** (1 grant, 0 revoke) -> `0x7d95e0`. **The server must send this for the ranks allowed to buy (27) and sell (28) guild plots.** EQEmu's `GuildUpdate_Struct` ({guild id, guild-list entry}) has no action-coded form, so this is a new struct. (Other actions share the frame: 3 guildmate achievement, 4 guild rename (`+0x50` guild id, `+0x54` name).) The guild object also has a key=value settings blob (`VERSION`, `GUID`, `URL`, `CHAN`, `PDS_FLAGS_VALID`, and "rank^mask" pairs; `0x7da560`, `0x7d9c40`), reached through its vtable; its trigger wasn't found and action 5 is sufficient.

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

**A server-only rule:** string 5426 ("…your guild only has %1 members. In order to purchase a guild plot, your guild must have at least %2 members.") is never shown by client code, so Live enforced a **minimum guild size for guild plots** on the server.
