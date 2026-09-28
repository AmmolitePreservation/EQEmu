# `0x2698` — Real Estate Definitions and Costs (server → client)

Pushes the full real-estate definitions table and costs table to the client. The client knows no plot, house, or pricing data until it receives this, so it is a prerequisite for every other real-estate flow.

Decoded statically from the RoF2 client (`May 10 2013 23:30:08`). Every claim below cites the client code it came from.

## Framing

- The network layer reads the u16 opcode from the first two bytes of the application packet, then calls `HandleWorldMessage(opcode, buffer + 2, length - 2)` (`0x4623e0`). **All offsets below are payload-relative**, which matches EQEmu's `EQApplicationPacket` payload.
- `RealEstateManagerClient::HandleMessage` case `0x2698` (`0x5b278c`) wraps the payload in a stream cursor `{buffer, length, position = 0}` and hands the **same cursor** to two readers in sequence:
  1. Definitions (`0x7f5b60`) clears the table, then reads the definition list.
  2. Costs (`0x7fc6b0`) clears the table, then reads the cost list.
- Then `0x5aaad0` refreshes client state. It does not read the stream.

All integers are little-endian. There is no padding and no alignment.

**Send complete packets.** The reader bounds-checks each field: a read that would run past the end is skipped and the field keeps its prior value. A truncated **count**, however, falls back to a garbage value (the reader reuses whatever is in the destination register), which can drive the record loop far past the real data. A partial or short packet is never safe.

## Primitive types

| Type | Encoding |
|---|---|
| `u32` / `i32` / `f32` | 4 bytes LE |
| `u64` | 8 bytes LE |
| `string` | `u32 length` + `length` raw bytes, no terminator. A length ≤ 0 yields an empty string (`0x80c9b0`). |

## Payload layout

```
u32              definitionCount
Definition[definitionCount]
u32              costCount
CostElement[costCount]
```

Both counts are compared unsigned.

### `Definition` (reader `0x7f4c90`)

Wire order is **not** in-memory order. "Mem" is the client struct offset, for cross-reference with the accessors.

| # | Wire type | Mem | Meaning | Evidence |
|---|---|---|---|---|
| 1 | u32 | +0x00 | **definition id** (table key) | key of the lookups at `0x7f4f70`, `0x7f4fa0` |
| 2 | u32 | +0x1c | **region id**: the `0x49df` region volume that belongs to properties of this definition (region → property lookup `0x4b30f0`; the yard-shape lookup `0x740560` in the Items window also uses it) | `0x4b30f0`, `0x740560` |
| 3 | u32 | +0x20 | **purchase cost id**: `CostElement.costID` for the price combobox | Purchase window fill `0x74de3b` → `0x74d240` |
| 4 | u32 | +0x24 | **upkeep cost id**: `CostElement.costID` for the upkeep combobox. **Correction:** this is not the price; the price is `+0x20`. | Purchase window fill `0x74de4c`; Manage window upkeep (`0x741470`, `0x742d82`) |
| 5 | u32 | +0x28 | **max items** | accessor `0x7f5040`; validation strings |
| 6 | u32 | +0x2c | **max non-real-estate items** (must be ≤ max items) | accessor `0x7f5080`; validation clamps to +0x28 |
| 7 | u32 | +0x30 | **max houses**: the houses and guild halls a yard holds. Placement rule 9 (`0x5b48b0`) counts the yard's children that aren't moving crates (`0x7f61b0`) and refuses once the count reaches this (`0x7f5100` on the yard's definition), with `0x0f` for a house or `0x44` for a guild hall | accessor `0x7f5100` |
| 8 | u32 | +0x34 | unknown | |
| 9 | u32 | +0x38 | unknown | read at `0x5b46e0` |
| 10 | u32 | +0x3c | **definition type** (enum below) | accessor `0x7f4fa0` (50 callers); name table `0xb616a0` |
| 11 | u32 | +0x40 | **category**: a dbstr category 27 string id shown as `REPW_Category_Label` | `0x74e084` → `0x4866c0(id, 0x1b)` |
| 12 | string | +0x0c | unknown string (≤ 600 chars in the file format) | |
| 13 | u32 | +0x44 | **icon**: Live's `RealEstateDefinition::icon`, in the same place in Live's field order (between the category and the pet cap). Live gives each plot and guild plot its street's icon (3668-3674) and every other definition 0; its plot search list carries the icon in the first column, where RoF2 prints the row's first number (`opcode2691PropertySearch.md`). **RoF2 never reads it.** | the constructor's default 336 (`0x7f520c`); no accessor (those at `0x7f4fa0`-`0x7f51b0` cover the type, the caps, max houses, the origin and the heading); none of the 7 callers of `0x7f4f70` reads it; no icon control in any `EQUI_RealEstate*.xml` |
| 14 | u32 | +0x48 | **max pet (yard NPC) items** (must be ≤ max items) | accessor `0x7f50c0`; validation clamps to +0x28 |
| 15–17 | f32 ×3 | +0x4c..+0x54 | **layout origin (x, y, z)**: saved layout positions are relative to it (verified: the layout writer subtracts it, the loader adds it back) | accessor `0x7f5140`; `0x5ae900`, `0x5b0380`, `0x5b0680` |
| 18 | f32 (read as raw u32) | +0x58 | **layout heading** (EQ heading units): saved headings are relative to it, and Load adds it plus the Rotate Yard choice | accessor `0x7f5190`; `0x5ae900`, `0x5b0380` |
| 19 | u32 | +0x5c | **child count** | |
| 20 | u32 × count | +0x60 | **child definition ids** (the parent → child hierarchy the client file format keeps in `RealEstateDefRel.txt`) | "Tried to add RealEstateDefinition %d to parent %d" |

Fields marked unknown are wire-certain but their meaning isn't pinned yet. They'll resolve as the flows that read them get decoded. Send 0 for them until then.

**Not on the wire:** the file format also carries a name (+0x04), a description (+0x08), and a second vec3 (+0x10..+0x18). The stream never sends them. The client's display names come from elsewhere, probably string tables by id; still to be confirmed.

**Definition type enum** (`0xb616a0`; lookups on an unknown id return 10):

| Value | Type |
|---|---|
| 0 | None |
| 1 | Regular Zone |
| 2 | Guild Hall |
| 3 | Player House |
| 4 | Plot |
| 5 | Neighborhood |
| 6 | Town |
| 7 | Moving Crate |
| 8 | Guild Plot |

### `CostElement` (reader `0x7fc2b0`)

```
u32         costID                     referenced by Definition +0x24
u32         optionCount
CostOption[optionCount]
```

### `CostOption` (reader `0x7fbdb0`)

One selectable pricing option. The Purchase window adds one combobox entry per option, with the option key as the entry's value (`0x74d240`).

```
u32         optionKey                  combobox value; the file format keys on it
u32         itemCount
CostItem[itemCount]
```

### `CostItem` (16 bytes on the wire)

| Wire type | Meaning | Evidence |
|---|---|---|
| u32 | **currency**: 0 = coin, otherwise an alternate currency id | `0x5aa5e0` branches on 0 into the coin formatter `0x4dc5f0` |
| u32 | **currency subKey**: matched together with currency in the character's alt-currency table | `0x7f7e70` looks up by (currency, subKey) |
| u64 | **amount**. For coin it's in copper: the formatter splits by 1000/100/10 into plat/gold/silver/copper. | `0x4dc5f0` |

A price with several currencies (for example coin plus an alt currency) is one `CostOption` with several `CostItem`s.

## Client data files: not needed

On construction (`0x5b21e0`), `RealEstateManagerClient` tries to read `RealEstateDefs.txt`, `RealEstateDefRel.txt`, `RealEstateCosts.txt`, and `RealEstateItemGroups.txt`. **Neither the RoF2 client nor Live ships them**, and **`RealEstateErrors.txt` is written only for invalid rows in a file that was loaded** (every reference to its name, from `0x7f569c` on, is in the loaders' row validation, such as the item caps check before `0x7f569c`), so a missing file is silently skipped: neither install has the log.

- **Definitions and costs:** fully replaced by `0x2698`.
- **Item groups** (`(int key, int group, bool flag)` rows): **the client never reads them.** The item-groups class has only a constructor, file loader, insert, clear, and destructor, with no lookup. The only read of its pointer (manager `+0x7c`) is the destructor, and none of the 194 singleton call sites touch it. It's server-side data that happens to be linked into the client.

## Still open

- Meanings of the unknown definition fields (+0x34, +0x38, the string at +0x0c); +0x30 is max houses. (+0x1c = region id, +0x20 = purchase cost id, +0x40 = category label, +0x44 = icon are resolved.)
- When the server should send `0x2698`: on zone-in, or on the first real-estate window open. Decide once the request flows are decoded.
