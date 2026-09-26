# Trophy Tribute: Data and Messages

**Status:** done (`windows/TributeTrophyWnd.md`, flow `flows/trophies.md`).

**Trophy tribute** is House of Thule's tribute variant for trophies: the character (and the guild) keep **active trophy benefits** chosen from available ones, shown in `TributeTrophyWnd`, which the Items window's **Trophies** button opens. It runs on the same **tribute manager** as ordinary tribute (a static object at `0xabfcd0`), but with its own messages and data.

## Link to housing

- **Personal available trophy benefits are computed from the trophies placed in the character's plots** (below). So every placed-item change marks the trophy window for refresh (`+0x28c` = 1): entry added (`0x5abd20`), removed (`0x5ab910`), replaced (`0x5ab4b0`, `0x1097`), container cleared (`0x5ab3a0`), and several real-estate manager messages (`0x5b2500`).
- The **opt-in checkbox's enabled state** reads the real-estate manager (chunk 3).
- The Items window's **Trophies** button and the housing slash command (`0x4e4e60`) toggle the window.
- Opening it requires **House of Thule** and `profileType` 0 (`0x483b10` on the tribute manager); otherwise "You must own the House of Thule expansion to display this window." (`0x152e`).

## The personal trophy slots

The character profile holds **10 personal trophy benefit slots at `+0x880c`**, 8 bytes each **{u32 benefit id, u32 tier}** (accessor `0x7c4920(slot)`, which does not bounds-check; the handlers do). They arrive in **`OP_PlayerProfile` right after the 5 personal tribute slots** (`+0x87e4`; reader `0x801060`). **EQEmu's RoF2 encoder sends all 10 as empty** (`{0xFFFFFFFF, 0}`, commented "Guild Tribute Count ?").

## Server → client messages (none named in EQEmu)

Handled by the tribute manager (`0x485990`); after each one the manager sets `+0x30`, refreshes the tribute master window's list (`[0xf75a58]`, `0x782db0`) and flags the trophy window.

| Opcode | Payload | Effect |
|---|---|---|
| **`0x7697`** | **u32 active, u32 ids[10], u32 tiers[10]** (84 bytes) | **sets all 10 slots**; manager `+0x31` = **personal trophy tribute active** (the timer runs while it's set) |
| **`0x4b65`** | u32 slot (0–9, checked), u32 benefit id, u32 tier | **sets one slot**; id -1 clears it (`0x486620`) and marks the character for recalculation (`+0x2dd4` = 1, `+0x2f6c` = 0) |
| **`0x08bf`** | none read | **clears all 10 slots**; manager `+0x31` = 0 |
| **`0x51ae`** | u32 | **the personal trophy timer** (character `+0x20c8`, milliseconds; counts down each frame while manager `+0x31` is set, `0x483050`) |

### Guild trophy messages (guild tribute handler `0x485bb0`; none named in EQEmu)

| Opcode | Payload | Effect |
|---|---|---|
| **`0x7dd2`** | `+0x04` u32 **guild favor** (`[0xdd5fb4]`, also written by `OP_GuildTributeFavorAndTimer`), `+0x08` u32 (`[0xddf040]`), `+0x0c` u8 **guild trophy tribute active** (`[0xddf049]`; the guild trophy timer runs while it's set), **`+0x14` u32 ids[4]**, **`+0x24` u32 tiers[4]** | **the guild's 4 active trophy slots** (guild object `0xdd5fcc` / `0xdd5fd0`, flag `[0xddf048]`); refreshes the tribute and trophy windows and marks the character for recalculation |
| **`0x7db5`** | a u32 (skipped), **u32 count, u32 ids[count]** | **the guild's available trophy benefits** (tribute manager `+0x34`) |
| **`0x7a41`** | u8 | the **guild trophy edit lock** `[0xddf04a]` (Modify / Save and Upgrade / Downgrade require it); the client also sends `0x7a41` (chunk 3) |

## The lists (`windows/TributeTrophyWnd.md`)

| List | Personal view | Guild view |
|---|---|---|
| **Active** | the 10 profile slots (`0x788ca0`) | the guild's 4 slots (`0x788be0`) |
| **Available** | **computed from placed trophies** (`0x5b1b50`, below) | `0x7db5`'s list (`0x485180`) |

**Personal available benefits (`0x5b1b50`):** start from the character's **property list A** (character data `+0x29ac`, filled by `0x2aa7` / `0x76c6`); keep the properties whose definition type is **4 (Plot) or 8 (Guild Plot)** (`0x7f6030`); for each, **walk the property tree** (`0x5b64c0`: the plot and, recursively, its children such as houses; a record with `+0x40` set is skipped) and, for each property of the right category (`0x7f5f40` / `0x7f5f50`), visit **every placed item in its container** (`0x5b55d0`, `0x5b4d80`). **A placed item counts if it is a trophy** (`0x7b0ba0`: **item definition `+0x5ec` ≠ -1 AND `+0x5ac` placeable ≠ 0**); its benefit id is **`+0x5ec`** (`0x7b0c50`). Duplicates are removed.

**Rows:** each benefit id is looked up in the tribute manager's **tribute definitions** (personal `0x484d50`, guild `0x484d80`): the same definitions ordinary tribute uses, from the tribute info messages. A row (0x54 bytes) takes the definition's **name** (`+0x04`), and for the tier: **cost** (`+0x38 + tier × 4`) and a second per-tier value (`+0x10 + tier × 4`). **Descriptions** are requested on selection with the ordinary tribute messages: **`OP_SelectTribute`** (`0x79fc`, personal, `0x484e20`) and **`OP_GuildSelectTribute`** (`0x649f`, guild, `0x485260`), only when the definition's description isn't cached.

**The trophy flag on definitions:** the tribute info reader stores definition **`+0x88` = (the u32 after the tiers ≠ 0)**; that u32 is **`unknown128` in EQEmu's RoF2 `TributeAbility_Struct`, which the encoder always sets to 0**. Flagged definitions are **hidden from the ordinary tribute windows** and shown by the trophy window (which includes definitions either way).

## Item data: the trophy benefit id

Item definition **`+0x5ec`** is read, after the Heirloom byte, following: u32 `+0x5ac` (placeable), u8 `+0x5b0`, u32 × 4, u32 `+0x5c4` (upkeep cost id), u32 `+0x5c8`, a string (≤ 32 bytes, `+0x5cc`), u8 `+0x608`, then **u32 `+0x5ec`** (bytes 31–34 after Heirloom when the string is empty). **With stock EQEmu it lands on `unknown30`, which the encoder sets to -1**, so no item is a trophy (placeable is 0 anyway). **A server that corrects the item tail for placeable and upkeep must also write `+0x5ec`: -1 for ordinary items, the benefit id for trophies**; otherwise every placeable item becomes a trophy with benefit id 0.

## Client → server messages

| Opcode | Payload | Sent when |
|---|---|---|
| **`0x51a9`** | none | the window opens (`0x788fe0`) |
| **`0x4b65`** | **u32 slot, u32 benefit id, u32 tier** (12 bytes) | a **personal** Upgrade (`0x484f50`: a free slot at tier 0; if all 10 are full, `0x1486` "…all of the active trophy slots are full") or Downgrade (`0x4830d0`); the client updates its own slot first and the server's `0x4b65` sets the authoritative value |
| **`0x7a41`** | 80 bytes: **`+0x00` u8 lock** (1 = start editing, 0 = release), **`+0x14` u32 ids[4]**, **`+0x24` u32 tiers[4]** (the guild slots) | Modify (1); Save, switching to Personal while editing, and closing while editing (0) |
| **`0x7c1d`** | 68 bytes: **`+0x14` u32 ids[4]**, **`+0x24` u32 tiers[4]** | **Save**, and leaving the Guild view or closing while editing: **the edited guild set** |
| **`0x2bc`** | 80 bytes: **`+0x00` u32 the local spawn's `+0x34c`**, **`+0x04` u32 opted in** (the checkbox), **`+0x08` the character name** | the Guild view's opt-in checkbox |
| `OP_SelectTribute`, `OP_GuildSelectTribute` | the ordinary tribute requests | selecting a benefit whose description isn't cached |

In `0x7a41`, `0x7c1d` and `0x2bc`, **every byte not listed is stack leftover**; the server must ignore it. The server grants the guild edit lock with its own `0x7a41` (u8). (`0x14bf`, sent near the trophy code, is a trade message, not trophies.)

## Permissions, openers and the GUI folder

- Guild edits need **guild permission 25, "Trophy Tribute: Change Active Benefit"** (Modify, Upgrade, Downgrade); the guild window also checks **24, "Trophy Tribute: Change for Others"**. Placing guild trophies needs **29, "Real Estate: Modify Trophies"** (`permissions.md`).
- **Personal trophies** are placeable in houses or yards; **guild trophies** in guild halls or guild yards (the `+0x5ac` bitmask, `windows/RealEstateItemsWnd.md`).
- Openers: the Items window's Trophies button and **`/realestate trophies`**.
- The Guild Management window's **"Trophy Tribute Opt Out"** button (`TrophyTributeOptOutButton`, tribute page) is **not bound by the client** in this build.

## Labels (EQType bindings)

The labels have no code of their own; the XML binds them to game data: **297** personal trophy time left (character `+0x20c8`), **298** personal trophy active cost (the tribute manager's sum of the active tiers' costs, `0x4851f0`), **299** guild trophy time, **300** guild trophy cost; **117** current favor, **118** career favor, **122** guild favor (shared with ordinary tribute). The **Tribute Benefit window** has trophy tabs bound to the same types (`TBWT_*`, `TBWGT_*`).

## Buttons and rules (`windows/TributeTrophyWnd.md`)

- **Modify / Save** (Guild view): enabled by **guild permission 25**; the caption follows the edit lock (`0x1530` "Modify" / `0x152f` "Save").
- **Upgrade / Downgrade** (and double-clicking a row): Personal: free; Guild: requires the edit lock, otherwise `0x1531` "In order to modify Guild Trophy Tribute Benefits, you must have \"Trophy Tribute: Change Active Benefit\" access… and first click the Modify button."
- **Opt-in** (Guild view): **enabled when already checked (opting out is always allowed), or when the character is in a guild, has Veil of Alaris and `profileType` 0 (`0x483b70`), and is either in a zone with flag bit 31 (`0x7dc760`; *inferred:* guild hall zones) or standing on a plot under their guild's Guild Plot (`0x67f860`).**

## Server obligations

1. Send trophy benefit definitions through the tribute info messages with `unknown128` = 1; answer the select-tribute requests with descriptions.
2. Personal: the 10 slots in `OP_PlayerProfile`, `0x7697` / `0x4b65` / `0x08bf` updates, `0x51ae` timer; validate each `0x4b65` request (the benefit is available from placed trophies, the tier and the cost).
3. Guild: `0x7dd2` (active set, favor, active flag), `0x7db5` (available), `0x7a41` (edit lock, one editor at a time); validate `0x7c1d` against permission 25 and the lock; handle `0x2bc` opt-in with the same location rule.
4. Item data: trophy benefit id `+0x5ec` and the placeable bits (`windows/RealEstateItemsWnd.md`).
