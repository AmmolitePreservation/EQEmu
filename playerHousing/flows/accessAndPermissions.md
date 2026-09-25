# Flow: Access Lists and Permissions

**Status:** client side decoded (`../windows/RealEstateManageWnd.md`, chunks 4–5; `../permissions.md`).

## 1. Goal

A plot's owner (or someone with **Manage**) assigns people and the owner's clubs to access groups and decides which permissions each group has.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| A1 | The plot's **access list** (7 group masks + name/club → group entries) | `0x485d` (set) / `0x3968` (remove) | the grid, the lists, every permission check |
| A2 | The viewer's name and club ids (character `+0x204c`, `+0x2054`, `+0x205c`) and GM flag | character data | the viewer's group; may edit |
| A3 | The plot's owner name (record `+0x2c`) and type (player / guild) | records | owner mode; view defaults |

## 3. Sequence

| Action | Client (local) | Client → server |
|---|---|---|
| grid cell click | `0x5b0ad0` (set the group's permission bit) | queued → **`0x3146`** within 3 s |
| Add a name at a group | checks (`0x192b` already has access, `0x192c` above your group, `0x628` invalid) → `0x5b0b10` | `0x3146` |
| Remove a selected name | `0x5b0bb0` | `0x3146` |
| change a club's group | `0x5b0bb0` + `0x5b0b10` | `0x3146` |
| (server) | | **`0x485d`** new list (the client flushes its queue first), **`0x3968`** list removed |

## 4. Rules (client)

**Groups:** 0 Everyone, 1 Visitor, 2 Acquaintance, 3 Friend, 4 Trustee, 5 Co-Owner, 6 Owner. **Permissions 0–12:** Receive Buffs, Enter, Withdraw Upkeep, Remove Crate, Add Crate, Remove House, Add House, Mail Item, Remove Item, Add Item, Move Item, Interact Item, Manage. **May edit** = GM, owner, or Manage (12). **Grid:** a cell is editable only for groups below the viewer's group and permissions the viewer holds (owner and GM: all but the Owner column). **Add:** not above your own group (unless owner/GM); not Everyone. **Remove:** owner/GM, or Manage and a lower-group entry.

## 6. Failure branches

| Symptom | Cause |
|---|---|
| No Access tab | the viewer lacks Manage (12) and isn't a GM |
| Grid cells disabled | not may-edit, or the cell's group isn't below the viewer's, or the viewer lacks that permission |
| An edit disappears | made within 3 s of leaving the plot (the refresh clears the queue), or the server removed the list |

## 7. Server obligations

Apply `0x3146` with the **same rank rules** (the client can be bypassed); **validate names** (the client doesn't); answer with `0x485d`; enforce each permission where it matters (entering, buffs, mail, withdrawing, and so on).

## 8. Test cases

Owner edits everything; a Trustee with Manage edits only lower groups; adding above your group; adding an existing name; club group change; editing then walking off the plot within 3 s.
