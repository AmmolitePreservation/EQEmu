# Flow: Escrow and Upkeep

**Status:** client side decoded (`../windows/RealEstateManageWnd.md`, chunks 2–3; `../opcode2aa6PropertyAccount.md`).

## 1. Goal

A player standing on an owned plot opens **Manage**, deposits currency into the plot's escrow, sets the plot's upkeep option, sees how long the escrow lasts, prepays upkeep, and (if allowed) withdraws.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| U1 | On a plot (`+0x98`) with a known, **owned** record (player plot with an owner name, or guild plot with a guild) | regions + records | the window opens and stays open |
| U2 | The plot record's **balance map** (currency → f64) and **upkeep option** (`+0x3c`) and **upkeep-current flag** (`+0x40`) | `0x63fd` / `0x2691` | balances, time left, expiry rules elsewhere |
| U3 | The cost table (`0x2698`): the plot definition's **upkeep cost id** (`+0x24`) and its options; **each placeable item's upkeep cost element** | `0x2698` | Daily Cost, the upkeep list, escrowable currencies |
| U4 | Each item's **upkeep cost id** (item definition `+0x5c4`, bytes 21–24 after Heirloom) | item serialization (**EQEmu writes 0 there; must be fixed**) | which items appear in the upkeep list |
| U5 | The character's alt-currency table includes every currency used in upkeep costs | character data | otherwise that currency is left out of the totals and time left |

## 3. Sequence

| Action | Client → server | Server → client |
|---|---|---|
| deposit (click a balance row holding coin / an alt-currency item) | `0x2aa6` type 0 | the updated record (balance map) |
| withdraw (click a row, choose an amount) | `0x2aa6` type 1 | coin to the cursor (`0x2aa6` types 0/1/3 per `../opcode2aa6PropertyAccount.md`) + record |
| Withdraw All | `0x2aa6` type 7 | as above |
| set upkeep option | `0x2aa6` type 2 | the updated record (`+0x3c`) |
| add a day / week (all, or one row's upkeep kind) | `0x2aa6` types 3 / 4 / 5 / 6 | *inferred:* the upkeep cost moved into escrow; the updated record |

## 4. Computations (client)

**Daily cost** = the plot's selected upkeep option + every placed item's upkeep option, per currency, **net** (negative cost items are reductions). **Time left** per currency = balance ÷ net daily cost; overall = the minimum ("%dw %dd"); **Limited By** = the currency (per row) / the item or "Plot Upkeep" (summary).

## 6. Failure branches

| Symptom | Cause |
|---|---|
| "You must be inside a plot or house to use that." | not on a plot, or the plot's record is unknown |
| Window closes by itself | left the plot, or the plot isn't owned |
| Only coin can be deposited | no alt currencies in the `0x2698` cost table |
| Upkeep list empty | items carry no upkeep cost id (EQEmu default), or no matching cost element |
| Time left ignores a currency | the character's alt-currency table lacks it |

## 7. Server obligations

Validate every `0x2aa6` action (**the client checks nothing**; withdrawals need permission **2 Withdraw Upkeep**); keep balances and the upkeep option in the property record; charge upkeep daily and store items / expire upkeep when escrow runs out (`../placementMode.md`, *Upkeep*); write items' upkeep cost ids.

## 8. Test cases

Deposit coin and an alt currency; withdraw with and without permission 2; Withdraw All; change the upkeep option; a negative-cost (reduction) item; add day/week; escrow running out (`+0x40` = 0).
