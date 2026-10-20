# Flow: Trophies and Trophy Tribute

**Status:** client side decoded (`../trophyTribute.md`, `../windows/TributeTrophyWnd.md`).

## 1. Goal

A character places trophies in their plot or house; each trophy offers a **trophy benefit**; the character activates up to 10 of them (personal), and a guild up to 4 (guild), paid for with favor on a timer.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| T1 | House of Thule (and Veil of Alaris for guild) expansion bits, `profileType` 0 | `OP_PlayerProfile` | the window's gate and the guild opt-in |
| T2 | Trophy items: placeable bits and **trophy benefit id** (`+0x5ec`) in the item data | item serialization | what counts as a trophy |
| T3 | The trophies placed in the character's plots (and their houses) | `0x1cef` item lists, property records, character list A (`0x2aa7`) | the personal available list |
| T4 | Tribute definitions for every benefit id, with `unknown128` = 1 | tribute info messages | names, tiers, costs |
| T5 | Personal slots, active flag, timer | `OP_PlayerProfile`, `0x7697`, `0x4b65`, `0x51ae` | active list, labels |
| T6 | Guild slots, available list, active flag, favor; rank permission 25 | `0x7dd2`, `0x7db5`, `OP_GuildUpdate` action 5 | guild view |

## 3. Sequence

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | C → S | `0x51a9` | window opened (Items window Trophies button or `/realestate trophies`) |
| 2 | S → C | `0x7697` (and guild `0x7dd2`, `0x7db5`) | current state |
| 3 | C → S | `0x4b65` {slot, id, tier} | personal Upgrade / Downgrade |
| 4 | S → C | `0x4b65` | authoritative slot |
| 5 | C → S / S → C | `0x7a41` (lock 1) / `0x7a41` (granted) | guild Modify |
| 6 | C → S | `0x7c1d`, `0x7a41` (lock 0) | guild Save |
| 7 | S → C | `0x7dd2` | the saved guild set |
| 8 | C → S | `0x2bc` | guild opt-in / out |

## 7. Server obligations

See `../trophyTribute.md`, *Server obligations*. **Trophies are the build's last step**, outside the first build: until then the trophy opcodes are unregistered (logged as unhandled), trophy items place as ordinary items with no benefit, and the guild opt-in, when it comes, is accepted from a member anywhere.
