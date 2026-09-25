# `0x2691` — Property Search / Neighborhood Results (server → client)

Handled by manager `0x5b0f80`. The client also sends `0x2691` as a request; that request is a fixed 224-byte struct (constructor `0x749500`), decoded separately. Wire types are as in the [`0x2698` spec](opcode2698DefinitionsAndCosts.md): little-endian, and `string` is a `u32` length plus bytes.

## Header

```
u32   (skipped by the client)
u8    kind
```

**`kind` is a definition type** (0x7f6030 is the type → category map shared with `0x7f60f0`/`0x7f61b0`; all three tables are identical). Category 3 is the plot category, so kinds 4 (Plot) and 8 (Guild Plot) take the single-property path. The mapping:

| kind | 1 | 2 | 3 | **4** | 5 | 6 | 7 | **8** | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| category | 1 | 2 | 2 | **3** | 4 | 5 | 6 | **3** | 0 | 8 | 9 |

Category 3 means a **single property**: kind 4 is a player property and kind 8 a guild property. The Plot Search window labels these kinds "Player" and "Guild".

## Single property (kind 4 or 8)

```
u32     (skipped)
u16     W
u32     propertyID          record +0x14
u32     parentID            record +0x1c: the **parent property id** (the hierarchy used by the containing-plot and neighborhood lookups); applied on create only
u32     definitionID        record +0x20 (create only)
u32     guildID             record +0x24, the owning guild (-1 = none); verified from the Purchase window's guild-plot check (0x74debd). Create: stored; update: passed to manager vtable +0x08
u32     V5                  record +0x28 (create only)
string  str1                (unused)
string  str2                record +0x38
string  str3                record +0x30
string  ownerName           record +0x2c (update goes through 0x5b4fe0)
u32 ×4  (skipped)
u8      openPurchase
```

The record is created if its id is new (`0x7fe1e0`, inserted through manager vtable `+0x24`). A new record gets `+0x18`, `+0x34`, `+0x3c`, `+0x40`, and `+0x44` set to 0 and an empty key/value map. For an existing record only `+0x30`, `+0x2c`, `+0x24`, and `+0x38` are updated.

If `openPurchase != 0` and `W > 0`, the Purchase window is filled from this record (`0x74dcb0`) and shown.

## List results (all other kinds)

```
u32   mode
```

- `mode` 4 or 5 → **Plot Search window** (`0x74c4c0`)
- `mode` 3 → ignored
- anything else → **Neighborhood window** (`0x74ab50`)

### Plot Search (mode 4 or 5), into `REPSW_PlotList`

```
u16     count
string  windowText                      set as the window's text
count × {
  u32     propertyID                    first column (%d)
  u32     kind                          only 4 ("Player") and 8 ("Guild") rows are shown; other rows are read and dropped
  u32     rowValue                      stored as the row's data value; **the /find target**: Go sends it in OP_FindPersonRequest (type 4, +0x08)
  string  a                             column (%s)
  string  b                             column (%s)
}
```

Columns in order: id, `a`, "Player"/"Guild", `b`. The list is then sorted (`0x74bb50`).

### Neighborhood window

All modes start with `u16 count`.

**Mode 0 or 1**, into `RENW_NeighborhoodList` (headings from the owner's XML: Neighborhood, Guild, PPlots, Used, GPlots, Used):

```
u8   showWindow                          if non-zero, the window is shown afterwards
if count > 0:
  u32  selected                          window +0x22c
  count × {
    u32     neighborhoodID               row data value
    string  name                         "Neighborhood"
    u32     guildID                      "Guild": name looked up by id; -1 means empty
    u32     playerPlots                  "PPlots"
    u32     playerPlotsUsed              "Used"
    u32     guildPlots                   "GPlots"
    u32     guildPlotsUsed               "Used"
  }
```

The three Neighborhood lists are cleared first, and the list is sorted (`0x749cd0`) at the end.

**Mode 2**, into the plot lists:

```
count × string                           RENW_PlayerList ("Player Plots")
u16   guildCount                         read only if RENW_GuildList exists (it always does with the stock XML)
guildCount × u32 guildID                 RENW_GuildList ("Guild Plots"), shown as guild names
```

## Client request (client → server `0x2691`, 224 bytes)

Constructor `0x749500`:

| Offset | Type | Default | Meaning |
|---|---|---|---|
| +0x00 | u32 | 0 | **request type** (all verified in code): **0 = name search** (Neighborhood window Search, the criterion in the field chosen by the type dropdown; My Plots = type 0 with your own name as the player); **1 = My Items** (your own name as the player: plots containing items you own); **2 = neighborhood details** (a neighborhood list click, id at `+0x04`); **4** = Plot Search by owner name (Search: the typed name, `+0xdc` = 1; My Plots: your own name, `+0xdc` = 0); **5** = Plot Search My Items (your own name); for types 4/5 `+0x04` = manager `+0x8c` (the current neighborhood) |
| +0x04 | u32 | -1 | neighborhood id (type 2); for types 0/1 the window's `+0x22c` (the "selected" id from the last neighborhood-list reply) |
| +0x08 | u32 | 0 | |
| +0x0c | char[80] | zero | neighborhood name criterion |
| +0x5c | char[64] | zero | player name criterion |
| +0x9c | char[64] | zero | guild name criterion |
| +0xdc | u16 | `0x0101` | constant |
| +0xde | 2 bytes | — | padding (sent uninitialized) |

This layout **matches the EQEmu docs' RoF/Live capture exactly** (type, id, unknown, 80/64/64-byte names, "always 1,1", then a 2-byte "132,0" that is the uninitialized padding). The reply types line up too: request type 2 → reply mode 2 (Player Plots / Guild Plots lists); request types 4/5 → reply modes 4/5 (Plot Search). All request types are now verified in code (Plot Search senders `0x74b920` (type 4) and `0x74be00` (type 5) build the struct inline).

## Entering a neighborhood

The Neighborhood window's **Go** button sends **`0x218c` `{u32 neighborhood id, u32 -1, u32 0}`** (the general interact/enter request; record id -1 = the property itself). The server should move the player into that neighborhood.

## Server obligation: guild names

Guild names in these lists come from the client's guild-name cache (`[0xdd5cf8]->0x425670`, fallback "Unknown Guild"). **On a cache miss the client sends its own request**, so the server must answer guild-name requests for any guild id it lists here. That is expected to be EQEmu's existing guild-name opcode; not verified.

## Open

- **`V5` (record `+0x28`) = the owner's fellowship id** (the permission group lookup gives fellowship members the `_Fellowship` entry, `permissions.md`). **`str2` (record `+0x38`) = the property's name** (the Purchase window title, the Manage window's property label, "You have entered %1.", the teleport prompts; the prior-research capture suggests a street address). **`str3` (record `+0x30`) is not read by any client code** (only assigned). Still open: the skipped fields (Live's Capacity/Vault/Pets). `parentID` (`+0x1c`) and `guildID` (`+0x24`) are resolved.
- What category values 1, 2, 5, 6, 8, and 9 mean, since the client dispatches on `mode` rather than on kind outside the single-property case.
