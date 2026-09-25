# Real Estate Result Codes (server → client)

String texts come from the owner's `eqstr_us.txt`. The client looks them up through `[0xdd25c4]->0x7d0660`; that this is the eqstr lookup was verified against the coin names, ids `0xfb6`–`0xfb9` = Platinum, Gold, Silver, Copper. Each message is a single `u32 code`.

## `0x1228` — item request result (`0x5ae1f0`)

- **-1:** ignored.
- **0: success.** If an item placement is active (`0x556d70`), placement mode ends (`0x557b60(1, 1)`) and the Items window refreshes (`0x733cb0`). No text is shown.
- **Unlisted codes:** default `0x1467`, "Your real estate item request failed."
- The placement commit (`0x5b1f90`) raises code `0x1c` locally when a placement lands on a player.

| code | string id | text |
|---|---|---|
| 9 | `0x146a` | The item was not found at the specified location.  It is likely that you are out of sync with the zone.  Try camping or zoning to fix the issue. |
| 10 | `0x1463` | You cannot add this item because the real estate is full. |
| 11 | `0x1479` | There are no items to transfer. |
| 12 | `0x1498` | You cannot add this item.  The vault is full. |
| 15 | `0x147a` | You cannot add another house to this yard. |
| 16 | `0x147b` | You cannot place another moving crate on this property. |
| 17 | `0x1464` | You can't put that item in this real estate. |
| 19 | `0x146d` | You can only do this with empty containers. |
| 20 | `0x146f` | You are too far away from the item.  Move closer to it and try again. |
| 21 | `0x149b` | This item must be placed in the zone for you to interact with it. |
| 22 | `0x1478` | You must have a moving crate placed in your yard or house for this operation. |
| 23 | `0x1483` | This can only be done by the owner of the property. |
| 24 | `0x1929` | You cannot do this because the upkeep on this plot has expired. |
| 25 | `0x192a` | You cannot do this because the upkeep on the item or house has expired. |
| 28 | `0x1475` | You cannot place this on top of a player. |
| 29 | `0x1499` | You cannot place this item here because the surface is not flat enough. |
| 31 | `0x1924` | You cannot place this here because you've reached the limit for that item. |
| 32 | `0x1925` | Non-empty houses and crates can only be placed in yards and houses.  They may not be put in the shed or the closet.  While non-empty houses are allowed in moving crates, non-empty crates are not allowed inside crates. |
| 33 | `0x1926` | You cannot pick this up because you may only have one non-empty house and crate in your inventory at a time. |
| 35 | `0x1482` | You cannot place that item in this zone. |
| 36 | `0x146e` | You can only do this with placeable items.  See the item's description. |
| 38 | `0x1928` | You cannot place this here because you have reached the pet limit for this yard. |
| 39 | `0x14a2` | Moving crates can only contain one non-empty house and this moving crate already has one. |
| 42 | `0x14a3` | You cannot interact with this NPC item because its NPC was not found in the zone. |
| 43 | `0x14ab` | You cannot add this item because it is No Storage. |
| 44 | `0x14ac` | You cannot add this item because it contains an augment that is No Storage. |
| 45 | `0x14c0` | Part of the stack was successfully moved. |
| 46 | `0x14c1` | You cannot place an item from that stack because the real estate does not have room for the item that would be added from splitting the stack. |
| 47 | `0x14c7` | This activity is disabled until you own the House of Thule expansion. |
| 48 | `0x14d3` | Your house swap failed because the house you are attempting to swap in does not have enough space to hold all of the placed house's items. |
| 49 | `0x14d4` | Your house swap failed because the house you are attempting to swap in does not have enough space to hold all of the placed house's archived items. |
| 50 | `0x14d5` | Your house swap failed because the house you are attempting to swap in does not have enough space to hold all of the placed house's NPC items. |
| 53 | `0x14d6` | Your house swap failed because you do not have a house placed. |
| 55 | `0x14d7` | Your house swap failed because the house you are attempting to swap in is already placed. |
| 59 | `0x14da` | Your house swap failed because you cannot swap houses while you have a placed crate in your placed house.  Move the crate to storage,  the yard or your inventory and try again. |
| 64 | `0x2db` | You cannot remove this item because it is no drop and you are not its owner. |
| 65 | `0x2dc` | You cannot remove this item because it is a lore item that you already have. |
| 66 | `0x2e7` | You cannot do that because you are the owner of the item. |
| 67 | `0x2e8` | You cannot do that because the item contains a real estate. |
| 68 | `0x2ed` | You cannot add another guild hall to this yard. |
| 70 | `0x2f8` | You must be guild member who has modify trophy access to do that. |
| 71 | `0x151c` | You cannot do that because you do not have add item access. |
| 72 | `0x151d` | You cannot do that because you do not have remove item access. |
| 73 | `0x151e` | You cannot do that because you do not have move item access. |
| 74 | `0x151f` | You cannot do that because you do not have interact item access. |
| 75 | `0x1520` | You cannot do that because you do not have add house access. |
| 76 | `0x1521` | You cannot do that because you do not have remove house access. |
| 77 | `0x1526` | You cannot do that because you do not have add crate access. |
| 78 | `0x1527` | You cannot do that because you do not have remove crate access. |
| 79 | `0x1525` | You cannot do that because you do not have mail item access. |
| 81 | `0x152a` | You cannot store this house in the desired moving crate because the house contains a placed moving crate.  The moving crate inside the house must either be moved to the closet or removed from the house before the house can be stored in the desired moving crate. |
| 82 | `0x1535` | This activity is disabled until you own the Veil of Alaris expansion. |

## `0x66bf` — sell plot result (`0x5ad930`)

Code 0 shows nothing; unlisted codes show `0x149e`. Code 13 builds its text with `0x7dbd10(…, 10, 1, 1, 1)` instead of a string id (not decoded).

| code | string id | text |
|---|---|---|
| 2 | `0x149d` | Your attempt to sell this plot failed because you are not its owner. |
| 3 | `0x1923` | You already have a non-empty moving crate in your inventory or bank so you may not sell this plot.  To sell this plot you must remove all items from it or you remove the crate from your inventory. |
| 4 | `0x149c` | Your attempt to sell this plot failed because the plot does not contain a placed crate with enough available space to hold the plot's items. |
| 5 | `0x14a1` | Your attempt to sell this plot failed because the placed crate cannot add another house containing items.  Crates are limited to one house containing items. |
| 6 | `0x152b` | Your attempt to sell this plot failed because you do not have permission to remove the crate that will contain the plot's items. |
| 7 | `0x14c9` | You must either empty all of the items from this plot or purchase the House of Thule expansion before you can move out (otherwise you would end up with a crate full of items that you could not access). |
| 8 | `0x1537` | You may not sell a guild plot until you purchase the Veil of Alaris expansion. |
| 9 | `0x2ef` | You may not sell a guild plot if you are not a member of its guild! |
| 10 | `0x2f0` | You may not sell this guild plot because it has funds in escrow and you do not have withdraw upkeep access. |
| 11 | `0x152c` | You may not sell this plot currently because the active moving crate on it, which you would receive by selling the plot, is No Trade and you are not its owner. |
| 12 | `0x152d` | You may not sell this plot currently because the active moving crate on it, which you would receive by selling the plot, is a lore item that you already own. |
| other | `0x149e` | Your attempt to sell this plot has failed. |

## `0x68ba` — transfer plot result (`0x5adad0`)

Code 0 shows nothing.

| code | string id | text |
|---|---|---|
| 2 | `0x149f` | Your attempt to transfer this plot failed because you are not its owner. |
| 3 | `0x1dd5` | Your attempt to transfer this plot failed because there is a NO TRADE house or moving crate in the plot.  Please remove it and try again. |
| other | `0x14a0` | Your attempt to transfer this plot failed. |

## Other client strings referenced by real-estate messages

| Where | string id | text |
|---|---|---|
| `0x1baf` removes the item being placed | `0x1468` | Item placement has been cancelled because the item being placed has been removed from the real estate. |
| entry update hits the item being placed (`0x087f`, `0x2cf7`, `0x1097`) | `0x1469` | The item you are placing has been moved. |
| `0x5a63` prompt | `0x1dd4` | Do you wish to accept %1 from %2? |
| declining the `0x5a63` offer | `0x1dd6` | The real estate transfer has been declined. |
| coin names used by `0x2aa6` | `0xfb6`–`0xfb9` | Platinum, Gold, Silver, Copper |
