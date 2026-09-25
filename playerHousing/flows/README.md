# Flow Specs (Test Plans)

One file per player-facing feature (Purchase, Sell, Transfer, Upkeep and escrow, Place/Move/Stow, Moving crate, Layouts, Neighborhood and plot search, Interaction menu, Access and permissions). A flow is the unit of live testing: *set up this state, run this sequence, expect this*.

## Flows

| Flow | File | Status |
|---|---|---|
| Purchase a plot | `purchase.md` | draft (predates the gate and Q1 answers; needs a refresh) |
| Place a new item | `placeItem.md` | client side decoded |
| Edit, move, store or locate a placed item | `editPlacedItem.md` | client side decoded |
| Store All, moving crate, house swap | `movingCrate.md` | client side decoded |
| Layouts (save, load, details) | `layouts.md` | client side decoded |
| Escrow and upkeep | `escrowAndUpkeep.md` | client side decoded |
| Access lists and permissions | `accessAndPermissions.md` | client side decoded |
| Browse, search and enter neighborhoods | `neighborhoods.md` | client side decoded |
| Find a plot and follow the wisp | `plotSearch.md` | client side decoded |
| Polls, property contests, winner teleport | `polls.md` | client side decoded |
| Trophies and trophy tribute | `trophies.md` | client side decoded |
| Transfer a plot | `purchase.md` (Transfer section) | the Manage and Purchase windows share it (`0x507f` via `0x5b1470`); refresh with the Purchase draft |

## Template

1. **Goal** — what the player does and sees.
2. **Client state checklist** — every precondition, each traced to its origin (server message and field, client action, or initialization step). This includes expansion flags, the character, current property, windows, and latches.
3. **Sequence** — the ordered messages in both directions, including requests the client sends on its own (for example on window open). Each step: sender, opcode, key fields, and the client state it changes.
4. **Decision trees** — for every gate and control involved, expanded to data leaves (see `../windows/`).
5. **Expected result per step** — what the UI shows and which controls are enabled.
6. **Failure branches** — for each check: what the client shows or does when it fails (silent no-op, a refusal string id, a request), so a symptom seen in testing maps back to the missing precondition.
7. **Server obligations** — what the server must store and send for this flow (feeds `../serverWork.md`).
8. **Test cases** — the happy path plus one case per failure branch.
9. **Status and open questions.**
