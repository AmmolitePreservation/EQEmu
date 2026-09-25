# Flow: Find a Plot and Follow the Wisp

**Status:** client side decoded (`../windows/RealEstatePlotSearchWnd.md`; `../opcode2691PropertySearch.md`).

## 1. Goal

A player inside a neighborhood finds plots by owner name (or their own plots, or plots holding their items) and follows a wisp to one.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| S1 | The server's current property is a **neighborhood** (definition type 5) | `0x6fd0` (sets manager `+0x8c` and `+0x90`) | the window only opens in a neighborhood |
| S2 | The /find system can path within the zone | the zone | the wisp |

## 3. Sequence

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | C -> S | `0x2691` type 4 (Search / My Plots) or 5 (My Items) | `+0x04` = the current neighborhood; owner name at `+0x5c`; `+0xdc` = 1 typed / 0 own plots |
| 2 | S -> C | `0x2691` mode 4 or 5 | rows: plot id, kind (4 Player / 8 Guild), `rowValue`, address, owner |
| 3 | C -> S | `OP_FindPersonRequest` (`0x5cea`), target type 4, `rowValue` | Go |
| 4 | S -> C | `OP_FindPersonReply` (`0x7e58`) | the path to the plot's center (the wisp) |

## 7. Server obligations

Answer request types 4/5 with reply modes 4/5 for the current neighborhood; choose `rowValue` as the id the server wants back for the find; implement `OP_FindPersonRequest` target type 4 (plots) as well as type 3 (placed items, from Locate).
