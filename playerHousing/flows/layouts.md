# Flow: Layouts (Save, Load, Details)

**Status:** client side decoded (`../windows/RealEstateItemsWnd.md`, *Layouts*; `../windows/RealEstateLayoutDetailsWnd.md`).

## 1. Goal

A player saves the arrangement of their yard and/or house as a named layout (a local file) and later loads it, on the same or another plot, optionally rotated, using placed items, inventory items, and storing extras.

## 2. Client state checklist

| # | State | Origin | Why |
|---|---|---|---|
| L1 | Yard (`+0x98`) and/or house (`+0x9c`) exist | records, regions | Save/Load scope |
| L2 | **Each plot's definition has a meaningful layout origin (`+0x4c`..`+0x54`) and heading (`+0x58`)** | `0x2698` | transforms are saved relative to it and restored from it |
| L3 | May-manage gate passes; for Use Inventory, **permission 9, 6 or 4** (or GM) | expansions, access lists | Load; Use Inventory is silently turned off otherwise |
| L4 | No request pending | client | Save/Load |

## 3. Sequence

**Save** (local only): write `.\userdata\RE_Layout_<n>.xml` (`RealEstateLayout`: item ids, names, flags and transforms relative to the yard). Overwrite asks first (dialog `0x68`). **No message.**

**Load:**

| Step | Direction | Message | Notes |
|---|---|---|---|
| 1 | client | save the current arrangement as **"Auto Save"** | |
| 2 | client | plan: match layout entries to real items **by item id** (placed → other sources → inventory if allowed); stack merges; Store Extra stores unmatched placed items | the Details window shows the same plan as a dry run |
| 3 | C → S | **`0x5b41`**: u32 yard id, u32 house id, u16 count, records: **tag 1** place (GUID, inventory location, source property/record, **transform heading/pitch/roll/scale/x/y/z in world space**, destination property), **tag 0** move to storage (source property/record, quantity (0 = all), destination) | latch set; a chat summary follows |
| 4 | S → C | per-item results: entry updates (`0x1097`/`0x2cf7`/`0x1baf`), spawns, inventory changes | |

**World transform** = yard origin + saved position rotated by (yard heading + Rotate Yard); heading = saved heading + yard heading + Rotate Yard (0/90/180/270°, index × 128 heading units).

## 6. Failure branches

| Symptom | Cause |
|---|---|
| Load buttons dark | no layout selected, the layout lacks that part, the location doesn't exist, the gate fails, or a request is pending |
| Loaded items land in the wrong place/orientation | the plot definition's layout origin/heading (L2) is wrong or zero |
| Inventory items never used | Use Inventory silently off (no permission 9/6/4) |
| Nothing sent | the plan produced no records |

## 7. Server obligations

Parse `0x5b41`; **validate every record with the same rules as individual placements and moves** (no need to reproduce the planning); apply them; send the per-item updates. Provide the layout origin and heading in each plot definition.

## 8. Test cases

Save/Load on the same plot; Load onto a different plot (origin/heading); each Rotate Yard option; Use Placed only / Use Inventory only / Store Extra; a layout with more items than available (Details shows the shortage); a mergeable stack.
