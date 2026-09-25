# Server Data Model (draft, derived from the wire formats)

Entities and fields the server must be able to produce, inferred **only** from what the client reads. Field names are provisional; meanings marked *unknown* are known on the wire but not yet understood. This is not a schema design yet; it becomes one once the flows are complete.

| Entity | Fields (source) | Notes |
|---|---|---|
| **Definition** | id, type (0–8), purchaseCostID (+0x20), upkeepCostID (+0x24), categoryLabelID (+0x40, dbstr 27), maxItems, maxNonRealEstateItems, maxPetItems, layout vec3 and float, child definition ids, and seven unknown u32s plus one string (`0x2698`) | the plot/house/yard catalog; hierarchy via child ids |
| **Cost element / option / item** | costID → options (optionKey) → items {currency (0 = coin), subKey, amount} (`0x2698`) | the price and upkeep choices shown in comboboxes |
| **Property** | id, parentID (+0x1c), definitionID, owningGuildID (+0x24, -1 = none), ownerName, two more strings, upkeepOptionKey (+0x3c), upkeepCurrent (+0x40), votingOptOut (+0x44), name (+0x38), balance per currency {key → f64} (key 0 = coin), several unknown u32s and flags (`0x6fd0`/`0x4f09`/`0x63fd`/`0x2691`) | one per plot or house |
| **Placed item** | recordID (unique per property), a second id, flag byte, u32, seven floats (transform), two strings (the first used as a grouping key), the full serialized item (`0x1cef` and related) | the item portion reuses EQEmu's RoF2 item serialization |
| **Spawn binding** | spawnID ↔ placed item GUID, plus two u32s (`0x21ea`) | placed items exist in the world as spawns |
| **Character property lists** | three id lists (`0x2aa7`, `0x3e00`, `0x66df`) with single-id updates (`0x0c13`, `0x76c6`) | meaning of each list still inferred |
| **Per-item upkeep** | time left, cost, reduction, limiting factor (Manage window upkeep list columns) | each item's upkeep option comes from its **item definition's upkeep cost id** (item `+0x5c4`; zeroed by EQEmu's RoF2 item struct, critical finding #12); totals are computed by the client (`flows/escrowAndUpkeep.md`) |
| **Interaction menu** | per item: up to 40 entries {type, b, labelID (dbstr category 28), d (submenu key)} (`0x21ec`) | |
| **Access list** (per property) | 7 × 64-bit permission masks (groups 0–6, 6 = owner) plus name → group entries (individuals, club key, guild/rank key) (`0x485d` set, `0x3968` remove) | permission names still to map |
| **Guild names** | answered on client request | the client asks for names it lacks |
| **Neighborhood** | id, name, guild, player/guild plot totals and used counts (`0x2691` modes 0/1) | |

Every row still needs its full set of flows before it's a design input.
