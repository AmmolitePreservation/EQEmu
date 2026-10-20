# Flow: Browse, Search and Enter Neighborhoods

**Status:** client side decoded (`../windows/RealEstateNeighborhoodWnd.md`; `../opcode2691PropertySearch.md`).

## 1. Goal

A player finds a neighborhood (by browsing, by name, by player or guild, or their own plots/items), sees its player and guild plots, and enters it.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| N1 | Neighborhood rows (id, name, guild id, plot counts) | `0x2691` reply modes 0/1 | the list (the server can also open the window with `showWindow`) |
| N2 | Guild names for listed guild ids | the client's guild-name cache (it requests misses) | the Guild column, the guild plot list |
| N3 | Filter checkboxes ticked | the patcher's XML, which starts all six checked as Live's client does (PAT-2); stock RoF2's starts them unchecked | otherwise every row is hidden |

## 3. Sequence

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | C -> S | `0x2691` type 0 (name search) / type 1 (My Items) | the criterion in the player, guild or neighborhood name field |
| 2 | S -> C | `0x2691` mode 0/1 | neighborhood rows; `showWindow` opens the window |
| 3 | C -> S | `0x2691` type 2 (row click) | that neighborhood's details |
| 4 | S -> C | `0x2691` mode 2 | "Player Plots" strings and "Guild Plots" guild ids |
| 5 | C -> S | `0x218c` `{neighborhood id, -1, 0}` (Go) | |
| 6 | S -> C | the player joins the neighborhood's running copy (or a new one) at its template's zone-in point | `../serverPlan.md` D3 |

## 7. Server obligations

Answer the three request types with the matching reply modes (the client dispatches on the reply's mode, not the request); an empty name search lists every neighborhood (Live; the client sends it as-is); public neighborhoods carry guild -1; answer guild-name requests for listed guild ids; treat `0x218c` with record id -1 as "enter this neighborhood".
