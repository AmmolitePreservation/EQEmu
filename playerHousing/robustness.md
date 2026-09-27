# Robustness Review: What Bad Server Data Does to the Client

**Status:** done for every housing server → client handler (register section 4). Method: `robustnessScan.py` flags candidates mechanically (output `robustnessScan.md`), and every flagged site was then read by hand.

## Scope and method

**31 handlers:** the receive table in `COVERAGE.md` section 2, plus the world layer (`0x1045`, `0x2a9e`, `0x2523`, `0x49df`), placement (`0x626e`), the anchor teleport (`0x7bb1`), the polls (`0x15e2`, `0x39bb`, `0x3311`, `0x6860`, `0x7497`, `0x52b5`, `0x593a`, `0x7a4d`), the guild neighborhood notice (`0x1d3c`), `OP_GuildUpdate` action 5, the housing container (`OP_ClickObjectAction`) and `OP_SendMembershipDetails`. Each handler is scanned with the housing functions it calls, two levels deep; handlers inlined in the dispatchers are scanned as address ranges.

The scanner flags four patterns: a lookup result used before a null check; an allocation whose size isn't a constant; a memory write indexed by a register; a window pointer used before a null check.

## Results

| Category | Candidates | Result |
|---|---|---|
| **Lookup results** (record, placed entry, definition, spawn, poll, poll results, world object, access list) | 103 calls | **all null-checked.** The same detector finds 1 unchecked lookup in the rest of the binary (`0x4222d0`, not housing), so it works. A message that refers to something the client doesn't have is skipped, not a crash. |
| **Allocations sized from data** | 22 | **all overflow-safe:** `count × size` saturates to `0xFFFFFFFF` on overflow (`mul` / `seto` / `neg` / `or`, and `add` / `setb` for headers), so a huge count can't wrap to a small buffer. It makes the allocation fail, and the container code then throws (`0x8db1da`), which most likely ends the client. |
| **Indexed writes** | 85 | 37 bounds-checked, 21 appends after a grow, the rest hash-bucket clears, heap sorts, bounded copies and fixed-size searches (a 36-slot container, fixed permission ids), **except one: `OP_GuildUpdate` action 5** (below). |
| **Window pointers** | 10 | **all guarded**, some by an earlier check the scanner didn't pair (`cmp reg, reg-holding-0`, a check at the function's start) or by running inside that window's own method. |

**Reads:** the stream reader checks every read against the packet length; a read past the end leaves the destination unchanged. Strings copy at most N−1 characters, and a string that would run past the end comes out empty without moving the position. So **short packets and missing terminators don't overrun**, but every later field then comes from the wrong position, and a destination the handler didn't initialize keeps whatever was on the stack.

## The one real risk: `OP_GuildUpdate` action 5

`0x7d95e0(rank, permission, value)` writes **bit (permission mod 32) of dword `table + 4 + rank × 8 + (permission / 32) × 4`** with no bounds check. The table has 9 entries (ranks 0–8) of 8 bytes, and only the second dword of each is the mask. **A rank above 8, or a permission id of 32 or more, writes into memory past (or inside the wrong field of) the table.** The server must send only ranks 0–8 and permission ids 0–31 (guild plots use 27 and 28).

## Server rules this implies

1. **Counts must equal the entries actually sent**, and stay reasonable: a large count fails an allocation and likely crashes the client.
2. **Never send a truncated packet**: nothing overruns, but later fields shift and some default to stack garbage.
3. **Always terminate strings** within the packet.
4. **`OP_GuildUpdate` action 5: rank 0–8, permission 0–31.**
5. **Always append `0x626e`'s extra region pair** (`placementMode.md`).
6. Ordering mistakes (a record before its definition, an item before its property) are **silently ignored** rather than crashing, which makes them look like "nothing happened" in testing (`testingNotes.md`).

## Q11: the house-interior zone-in crash

The prior research's first attempt at adding house interior zones crashed the client. **The static review finds no crash path in any housing handler**, and the client has **no code specific to the housing zones** (the only compares are zone-table bounds, ids 1–767). The crash therefore most likely comes from the zone level:

- **The zone's files.** The client registers these housing zones (`0x7e0de0`); each needs its zone files in the RoF2 install:

| Zone ids | Short names | Title |
|---|---|---|
| 712 | `neighborhood` | Sunrise Hills |
| 714–719 | `phinterior3a1`, `phinterior1a1`, `phinterior3a2`, `phinterior3a3`, `phinterior1a2`, `phinterior1a3` | House Interior |
| 720, 723 | `phinterior1b1`, `phinterior1d1` | Dragon House Interior |
| 739–744 | `plhogrinteriors1a1`, `1a2`, `3a1`, `3a2`, `3b1`, `3b2` | One / Three Bedroom House Interior |
| 745–750 | `plhdkeinteriors1a1`, `1a2`, `1a3`, `3a1`, `3a2`, `3a3` | One / Three Bedroom House Interior |
| 766 | `phinteriortree` | Evantil's Abode |
| 737, 738, 751 | `guildhalllrg`, `guildhallsml`, `guildhall3` | Grand, Greater, Modest Guild Hall |

- **Ids 721 and 722 are not registered** in this client; zoning a player into either would reference an unknown zone.
- **The prior research's "house interiors 714–766" range is wrong:** 724–736 and 752–765 are ordinary zones (Veil of Alaris, Rain of Fear). Sending a player to one of those as a "house" would load an unrelated zone.
- **The server's zone-in data** for zones EQEmu doesn't normally serve (zone header, spawns, doors), or a non-housing message during zone-in.

**Test plan:** confirm the files above exist in the install; zone in with **no** housing messages at all; then add the housing messages one at a time. If it still crashes, the crash address (from the client's crash log) will say which code path it is.

**On Peridot, steps 1 and 2 pass:** the RoF2 install has every zone above (`.eqg` and `.zon`), and with the zone rows added (`sql/housingZones.sql`), `#zone phinterior3a3` loads the three-room stone interior with no housing messages at all. So the zone level isn't the crash; step 3, the housing messages one at a time, remains.
