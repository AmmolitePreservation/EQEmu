# `0x2aa6` — Property Account Message (both directions)

One message format carries deposits, withdrawals, upkeep option changes, and upkeep purchases for a property. The same structure is serialized by the client (`0x748ce0`) and deserialized by it (`0x748700`); the two are exact mirrors. The object class has constructor `0x7486a0` and add-cost-item helper `0x7488c0`.

Wire types are as in the [`0x2698` spec](opcode2698DefinitionsAndCosts.md), plus one type that is new here:

- **`cstring`**: NUL-terminated bytes. On read, the client copies at most N−1 characters into an N-byte buffer and **advances past the full string and its terminator**. If the copy would run past the end of the packet, the field comes out empty and the position does not move.

## Layout

| # | Wire | Field | Default (constructor) |
|---|---|---|---|
| 1 | u32 | **type** (table below) | 8 |
| 2 | u32 | **property id** (the real-estate id) | 0 |
| 3 | u32 | **argument** (type-dependent) | 0 |
| 4 | cstring (≤63 chars) | string A | empty |
| 5 | cstring (≤31 chars) | string B | empty |
| 6 | u8 | **flag** | 0 |
| 7 | u32 | unknown | 0 |
| 8 | u32 | cost item count | 0 |
| 9 | CostItem × count | `{u32 currency, u32 subKey, u64 amount}` (reader `0x7fb870`, writer `0x7fbe90`) | none |

No client path writes string A, string B, or field 7, and no client path reads them. The client always sends them empty or zero.

### Coin encoding in this message

Currency 0 means coin, and **`subKey` selects the denomination: 0 = copper, 1 = silver, 2 = gold, 3 = platinum** (jump table `0x74191c`). The amount is a count of that denomination. This differs from the costs table in `0x2698`, where a coin amount is a copper total. Alternate currencies use their currency id with `subKey` taken from the character's alt-currency table (normally 1).

## Types

| Type | Direction | Meaning | Source (client) | Fields used |
|---|---|---|---|---|
| 0 | C → S | **Deposit from cursor.** Coin: the cursor coin (one denomination) is sent and the cursor is cleared. An alt-currency item on the cursor is matched to its currency and its stack count is sent. | `0x741ee0` cursor paths (`0x7422fe`, `0x742373`) | property id; one cost item |
| 0 | S → C | **Coin to cursor** (flag must be 0). The first cost item must be coin; its amount goes into the cursor coin field for its denomination. | `0x741670` | cost items |
| 1 | C → S | **Withdraw an amount.** A `REMW_Balance_List` click opens the quantity window. Its result (notification `0x24`) sends the chosen currency and amount. | `0x741ee0` via `WndNotification` `0x746581` | property id; one cost item |
| 1 | S → C | **Coin to cursor** (flag must be 1), same handling as type 0. | `0x741670` | cost items |
| 2 | C → S | **Set upkeep option.** Sent when the upkeep combobox changes (Manage `0x745985`, Purchase `0x74da60`); the client applies the change locally first. | | property id; argument = option key |
| 2 | S → C | **Upkeep option changed**: sets the property record's `+0x3c` (selected upkeep option key) and refreshes the Manage window's upkeep combobox. | `0x5aee30`, `0x741470` | property id; argument |
| 3 | C → S | **`REMW_AddDay_Btn`**, "Add day to all items." (XML tooltip) | `0x745a51` | property id |
| 4 | C → S | **`REMW_AddWeek_Btn`**, "Add week to all items." | `0x745aa0` | property id |
| 5 | C → S | **"Add day to this item."** `REMW_AddDayTemplate_Btn` is instantiated into column 0 of every `REMW_Upkeep_List` row (`0x742cd0`: `0x870040`, then `0x857630(row, 0, wnd)`) | `0x746068` | property id; argument = **the row item's upkeep cost id** (item definition `+0x5c4`, `0x7b0b80`), not an item instance id |
| 6 | C → S | **"Add week to this item."** `REMW_AddWeekTemplate_Btn`, column 1 | `0x7460be` | as type 5 |
| 7 | C → S | **`REMW_WithdrawAll_Btn`** | `0x745a03` | property id |
| 8 | — | default (constructor) | | |

Server → client types other than 0, 1, and 2 are ignored by the client handler, and so is any mismatch between type and flag.

### Verified details

- **Cursor coin fields** are character profile `+0x33c8` (platinum), `+0x33cc` (gold), `+0x33d0` (silver), and `+0x33d4` (copper). The pairing is verified against the ground-drop code's `PLAT/GOLD/SILVER/COPPER_ACTORDEF` models in `0x526450`. After setting them, the client sets a "cursor coin kind" byte at `[0xdd2660]+0x11` to 1–4 (plat/gold/silver/copper) for the highest nonzero denomination.
- **Property balance.** The property record's `{u32 key → f64 value}` map (see [`placedItemsAndProperties.md`](placedItemsAndProperties.md)) holds the **balance per currency**, with key 0 meaning coin. The withdraw path uses it to cap the quantity window: coin is converted to the selected denomination (`0x542800`); alt currencies are also capped by the character's alt-currency data; everything is capped at 1,000,000,000.
- The Manage window's managed property is manager `+0x98` (the plot); the balance list reads that record's balance map directly.
- **No client-side permission or pending-request check** on types 0, 1 and 7: the only check is that the plot's record is known and the plot is owned (`0x5b41d0`). Withdraw All (type 7) has no confirmation. **The server must enforce who may withdraw.**
- **Escrowable currencies = the currencies used by the `0x2698` cost table** (list derived by `0x5aaad0` at manager `+0x78`, tested by `0x5aa720`): the balance list only offers those.

### Inferred, not verified

- **Resolved:** upkeep is charged per item plus the plot's own upkeep option; each item's price is the cost element keyed by its upkeep cost id (item definition `+0x5c4`), and negative cost amounts are reductions (`windows/RealEstateManageWnd.md`, *Upkeep*). *Inferred:* types 3–6 pay days of upkeep into escrow (the client keeps no per-item paid time).
- **Earlier note (superseded):** upkeep is paid per item. `REMW_Upkeep_List` rows are placed items, with columns (from the owner's XML) [day button], [week button], —, Name, Cost, Reduction, Time Left, Limited By. Types 3 and 4 add a day or a week to all items; 5 and 6 add one to a single item.
- **The flag (struct `+0x6c`)** is never set by the client (every request sends 0), so the pairing isn't an echo. The reply handler (`0x741670`) honors **flag ≠ 0 only with type 1** (coin to the cursor) and, **with flag 0, type 0** (coin to the cursor) **or the non-coin types (type 2)**; everything else is ignored. *Server interpretation:* the flag means **"the request succeeded"**: a failed deposit is refunded with **type 0, flag 0**; a completed withdrawal pays out with **type 1, flag 1**; an upkeep option change is confirmed with **type 2, flag 0**. A successful deposit or a failed withdrawal needs no coin reply (the balances arrive in the property record).
