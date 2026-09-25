# Slash Commands and Key Bindings

**Status:** done. Every slash command and key binding in the client was listed and the housing ones traced.

## How the client registers them

- **Slash commands:** a static table at `0xacd868`, **366 records of 28 bytes** (`+0x00` name, `+0x08` handler, then flag and level fields), plus 23 commands registered elsewhere (`/who`, `/say`, `/afk`, …). None of the 23 is housing.
- **Key bindings:** **484 named bindings** (name table `0xaa7000`, id = position), dispatched by `0x4d7230` (a switch over ids 0–499).

`/pizza` is not an EverQuest command in this client (it was EverQuest II's).

## Housing slash commands

| Command | Handler | Behavior |
|---|---|---|
| **`/realestate items`** | `0x4e4e60` | toggles the Real Estate Items window |
| **`/realestate manage`** | | toggles the Manage window |
| **`/realestate trophies`** | | toggles the Trophy Tribute window (its House of Thule gate applies) |
| **`/realestate plot`** | | toggles the Plot Search window (its "must be in a neighborhood" gate applies) |
| `/realestate` with anything else | | `0x1d57` "Format: /realestate <items\|manage\|trophies\|plot>" |
| **`/leaverealestate`** | `0x4df640` | sends **`0x1da9`** (2-byte opcode plus 4 bytes the client never fills: a bare request). **Nothing else follows on the client**: no local state change, and no receive handler or compare for `0x1da9` anywhere. The server decides what leaving means (*inferred from the name:* leave the property you're in). **Not named in EQEmu. Marked for a Live comparison** (`testingNotes.md`). |

Subcommands are matched case-insensitively; the command itself has no gates. The inventory window's **Plot Items** button and the Purchase window's **Manage** and **Items** buttons call the same handler.

**Related:** `/find` drives the ordinary find window; housing's own locate and Plot Search Go use `0x5cea` (`worldLayer.md`). **There is no `/vote` or `/poll`**: polls open from the Purchase window, the Polls window, "pollid=" chat links, server `0x7a4d` and the auto-open (`polls.md`).

## Housing key bindings

| Id | Binding | Action |
|---|---|---|
| 429 (`0x1ad`) | `CMD_REAL_ESTATE_ITEMS_WINDOW` | `/realestate items` |
| 430 (`0x1ae`) | `CMD_REAL_ESTATE_MANAGE_WINDOW` | `/realestate manage` |
| 431 (`0x1af`) | `CMD_REAL_ESTATE_TROPHIES_WINDOW` | `/realestate trophies` |
| 432 (`0x1b0`) | `CMD_REAL_ESTATE_PLOT_SEARCH_WINDOW` | `/realestate plot` |
| 433 (`0x1b1`) | `CMD_REAL_ESTATE_HELP_WINDOW` | opens in-game help at topic `0x51` (`0x776ef0`) |
| 434 (`0x1b2`) | `…ITEM_PLACEMENT_TOGGLE_MODE` | cycle the placement mode (`0x557330(-1)`) |
| 435–438 (`0x1b3`–`0x1b6`) | `…SET_MODE_COLLISION` / `_WALL` / `_CURSOR` / `_FREE` | set placement mode 0 / 1 / 2 / 3 (`0x557330`) |
| 439–441 (`0x1b7`–`0x1b9`) | `…RESET_ITEM_MOUSE_X_ROTATION` / `_MOUSE_Y_` / `_WHEEL_` | reset one rotation axis (`0x558970`) |
| 442 (`0x1ba`) | `…RESET_ITEM_SIZE` | reset the scale (`0x558b30`) |
| 443–445 (`0x1bb`–`0x1bd`) | `…ROTATE_ITEM_…_90_DEGREES` | rotate one axis by 90° (`0x558820`) |
| 446 (`0x1be`) | `CMD_REAL_ESTATE_LEAVE` | `/leaverealestate` (sends `0x1da9`) |
| 447 (`0x1bf`) | `CMD_VOTE_RESULTS` | toggles the Polls window (`[0xd1fd54]`) |
| 356 (`0x164`) | `TOGGLE_TRIBUTEBENEFITWIN` | toggles the Tribute Benefit window (it has trophy tabs; `trophyTribute.md`) |

The placement bindings act on the placement state (`0x558540`) and do nothing unless an item is being placed (`placementMode.md`).

## Server obligations

Handle **`0x1da9`** (leave real estate). Everything else here is client-side window and placement control.
