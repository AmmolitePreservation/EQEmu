# TributeTrophyWnd ("Trophy Tribute")

**Status:** done (19 / 19 bound controls). Data and messages: `../trophyTribute.md`. Usage map: `TributeTrophyWnd.usage.md` (19 bound controls, 59 sites).

## Lifecycle

| Piece | Function | Behavior |
|---|---|---|
| Global | `[0xf75ad8]` | created by the UI setup (`0x49a297`) |
| Constructor | `0x7881d0` | binds the 19 controls; list columns (`0x787f40`) |
| **Activate** (vtable `+0xdc`) | `0x788fe0` | House of Thule gate (`0x483b10` on the tribute manager `0xabfcd0`; refusal `0x152e`), then sends **`0x51a9`** |
| **Deactivate** (vtable `+0xe0`) | `0x787d40` | in the guild view (`+0x2a4`) and under a condition (`[0xddf04a]`), sends `0x7c1d` and `0x7a41` (chunk 3) |
| **Per frame** (vtable `+0xc4`) | `0x789100` | while visible: refresh (`0x788d80`) when `+0x28c` is set (or `+0x28d` in the guild view); button states (`0x788e40`) when they change |
| **Notifications** (vtable `+0x88`) | `0x789170` | view toggles, Modify / Save, Upgrade, Downgrade, opt-in, list selection (chunks 2–3) |

## State

| Field | Meaning |
|---|---|
| `+0x2a4` | **view**: 0 = Personal, 1 = Guild (the view checkboxes set it) |
| `+0x28c` | **refresh**: set by placed-item changes, real-estate manager messages, and the tribute and trophy messages; cleared by the refresh |
| `+0x28d` | guild refresh: set by the guild handler (`0x425a10`, a member leaving) |
| `+0x264`, `+0x278` | the two benefit arrays the refresh builds the lists from (`0x782af0`, shared with the tribute windows) |
| `+0x2a0` | the last button-state snapshot |

## Openers

The Items window's **Trophies** button (`0x73d86c`, toggles by visibility) and the housing slash command (`0x4e4e60`).

## Lists and description (chunk 2)

Refresh `0x788d80`: the available list (`0x788a10`, into `+0x278`, count `+0x288`), then the active list (Personal `0x788ca0` / Guild `0x788be0`, into `+0x264`, count `+0x274`), then the listboxes (`0x5cbd90` via the helper at `+0x220`: name, tier and cost columns; the selection is restored). Sources and rules: `../trophyTribute.md`, *The lists*. Selecting an active row (`0x789548` → `0x5cb3d0`) requests the benefit's description with `OP_SelectTribute` / `OP_GuildSelectTribute` when it isn't cached, and updates the buttons (`0x5cb690`).

## Buttons (chunk 3)

| Control | Binding | Behavior |
|---|---|---|
| `TTW_ViewPersonal_Btn` | `+0x2b4` | Personal view (`+0x2a4` = 0); if the guild edit lock is held, first sends **`0x7c1d`** (the edited guild set) and **`0x7a41`** (release) |
| `TTW_ViewGuild_Btn` | `+0x2b8` | Guild view (`+0x2a4` = 1) |
| `TTW_LockButton` ("Modify" / "Save") | `+0x240` | Guild view only; enabled by guild permission 25; Modify sends **`0x7a41`** (lock 1), Save sends **`0x7c1d`** then **`0x7a41`** (lock 0) |
| `TTW_UpgradeButton` / `TTW_DowngradeButton` | `+0x230` / `+0x234` | Personal: **`0x4b65`**; Guild: edits the local set while the lock is held, otherwise `0x1531`; double-clicking an available / active row does the same |
| `TTW_StatusButton` (opt-in) | `+0x23c` | Guild view; sends **`0x2bc`**; enabled rule in `../trophyTribute.md` |
| `TTW_StatusText` | `+0x238` | "Opted In" caption (shown in the Guild view) |
| Labels (`+0x244`–`+0x260`) | | **EQType-bound** (297–300, 117, 118, 122); shown or hidden per view (`0x5cb130`) |

Button states are recomputed every 500 ms (`0x788e40`, `+0x2a0`).

## Close-out

**Sends:** `0x51a9`, `0x4b65`, `0x7a41`, `0x7c1d`, `0x2bc`, `OP_SelectTribute`, `OP_GuildSelectTribute`. **Receives:** `0x7697`, `0x4b65`, `0x08bf`, `0x51ae`, `0x7dd2`, `0x7db5`, `0x7a41`, plus the ordinary tribute messages and the housing item messages that trigger a refresh. **Static controls** (never referenced): `TTW_ViewPersonal_Label`, `TTW_ViewGuild_Label`, `TTW_View_Layout`, `TTW_ActiveTrophyLabel`, `TTW_ActiveTrophyBenefitArea`, `TTW_AvailableTrophyLabel`, `TTW_AvailableTrophyBenefitArea`, `TTW_BenefitButtonsWnd`, `TTW_TimeText`, `TTW_CostText`, `TTW_CurrentPointsText`, `TTW_LabelWnd`.
