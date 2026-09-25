# In-World Placement / Edit Mode

The client-side mode for placing a new real-estate item, or moving, rotating or scaling one that's already placed. It is a **cross-cutting state**: its "is active" check has 40 call sites (37 outside real-estate code), and every window must be checked against it (`COVERAGE.md`, *Cross-cutting mode*, Q25). This file maps the mode itself.

## Feature surface (client strings)

- **Sub-modes:** Cursor, Collision, Wall, Free, plus a toggle (`CMD_REAL_ESTATE_ITEM_PLACEMENT_SET_MODE_{CURSOR,COLLISION,WALL,FREE}`, `…_TOGGLE_MODE`).
- **Rotation:** 90° steps on three axes (mouse X, mouse Y, wheel) with resets (`…_ROTATE_ITEM_{MOUSE_X,MOUSE_Y,WHEEL}_ROTATION_90_DEGREES`, `…_RESET_ITEM_{MOUSE_X,MOUSE_Y,WHEEL}_ROTATION`), and a **size reset** (`…_RESET_ITEM_SIZE`).
- **Help overlay text:** "Cursor Mode:", "Collision Mode:", "Free Mode:", "Alt Rotate Heading:", "Change Height (free mode only):", "Cycle Placement Mode:", "Cancel Placement:" (overlay code in the display layer, `0x48a5b0`).
- **Options:** `ItemPlacementDefaultModeCursor`, `ItemPlacementHideUI`, `ItemPlacementShowOverlay` (options window checkboxes `OGP_ItemPlacement…`).

## State object

A lazily created singleton: getter **`0x558540`**, object **0x2c bytes at `[0xe63980]`**.

| Field | Initial | Meaning (verified unless marked) |
|---|---|---|
| `+0x00` | 0 | **owner's callback interface**: the object that started placement (for real estate, the manager's placement interface, vtable `0x9d90f0`); begin refuses if a different owner is active |
| `+0x04` | 2 | *inferred:* current sub-mode (the default is 2) |
| `+0x08` | 0 | **active** (byte); `0x556d70` returns it |
| `+0x0c` | 1 | reset to 0 on begin |
| `+0x10` | 0 (u16) | reset on begin (byte); read by the per-mode methods |
| `+0x11` | — | cleared on end |
| `+0x14` | 0 | the item being placed (read by nearly every method) |
| `+0x18`, `+0x1c`, `+0x20` | -1, -1, -1 (u16) | **the item's inventory/storage location**: the client's item location struct (u32 location type, then three u16 slot indexes), set on begin (and by `0x556e10`); -1 means none |
| `+0x24` | 0 | **editing an existing placed spawn** (byte), set on begin; also written by `0x4b3950` and by `0x21ea` |
| `+0x28` | 0 | **spawn id** of the item being edited (set on begin, and by `0x21ea` when the spawn arrives) |

## API (methods called on the singleton)

| Method | Uses | Role |
|---|---|---|
| `0x556d70` | 40 | **is active** (`+0x08`) |
| **`0x558bb0`** | 1 | **begin** (see below) |
| **`0x557b60(commit)`** | 6 | **end** (see below) |
| `0x556d80(guid)` | 6 | is this GUID the item being placed |
| `0x556dc0` | 1 | property/record match |
| `0x556e10(guid, location, a, b)` | 3 | update location; copy entry fields into the item |
| `0x557330`, `0x558820`, `0x558970`, `0x558600`, `0x558a70`, `0x558b30`, `0x5572e0`, `0x5585f0` | 1–5 | read the owner, flags and item; *inferred:* per-mode operations (to read) |
| `0x4b3950` | 2 | writes `+0x24` |

### Begin: `0x558bb0(owner, …, location[3], editFlag, …, spawnID, …)`

1. If another owner is active and differs → **fail**.
2. Set owner (`+0x00`), reset `+0x0c`/`+0x10`; if already active, end the previous placement (`0x5571d0(0)`).
3. Set **active** (`+0x08` = 1), the **location triple** (`+0x18`–`+0x20`), and **editFlag** (`+0x24`).
4. **Overlay on:** `[0xdd2660]->0x48f1f0(1)`.
5. **Editing an existing item** (editFlag set, the item's `+0x2c` set, spawn id given): `GetSpawnByID(spawnID)` → its actor (vtable `+0x4c`); unless a later flag is set, the actor gets `vtable +0x08(1)` and `+0x28` = spawn id. Otherwise (a flag argument set): allocate a 0xc0-byte object (*inferred:* a preview actor; not read).

**Begin's only caller is `0x4d07ee`, inside `HandleWorldMessage`'s second switch, in the case for server → client `0x626e` (handler `0x4d058a`). Verified: the client does not enter placement mode by itself; the server's `0x626e` (the reply to the client's `0x626e` place request) starts it.**

### Server → client `0x626e`: begin placement (partly decoded)

Wire order (the stream cursor is the usual `{buffer, length, position}`):

| # | Wire | Destination | Meaning |
|---|---|---|---|
| 1 | **OBJ**: the **placement object** (`0x4d5510` → `0x505400`) | local object (constructor `0x504460`, 0xc0 bytes) | see *Placement object* below |
| 2 | **LOC** (`0x4d6780` → `0x4d60e0`) | begin's location | the **item location**: `u8 locationType` (stored as u32), then `u16 × 3` slot indexes |
| 3 | u8 **A** | begin arg 4 | **editFlag** → state `+0x24` (editing an existing placed spawn) |
| 4 | u8 **B** | begin arg 5 | **create the preview object** (the 0xc0-byte placement object; see *Begin: object creation*) |
| 5 | u8 **C** | begin arg 6 | **unused**: begin never reads it (its stack slot is reused for float temporaries, `0x558fe8`/`0x559066`); send 0 |
| 6 | u32 **D** | begin arg 7 | **screen X** where a new item's preview first appears (the constructor's ray sweep start, `fild`); *inferred:* the server echoes the request's click X (request `+0x34`) |
| 7 | u32 **E** | begin arg 8 | **screen Y** (as D; request `+0x38`) |
| 8 | u32 **F** | begin arg 9 | **spawn id** of the placed item being edited (`GetSpawnByID` in begin; state `+0x28`) |
| — | — | — | if `[0xdcee30]` is null: clean up, **no placement** |
| 9 | (`0x4d5b90` → `0x5aac90`: **clears the map**, verified), then **u32 count**, then `count × {u32 regionID, u32 code}` | **the placement permission map**: `[0xdcee30]->0x4b3c50()` returns `&world+0x14` (id → code hash map) and `0x8d6c50(&code, &regionID)` inserts it | **where the player may place**: code 0 = allowed (see *Validation*) |
| 10 | `{u32 id, u32 code}` | same map, same insert path | **one more entry after the counted list**: an explicit extra read after a normal counted loop (not an off-by-one), inserted identically. **The server must always send it** (if missing, the bounds-safe reads insert `{0, 0}`); its meaning can't be told from the client (*inferred:* the property the item belongs to) |

#### Placement object (OBJ, reader `0x505400`)

| # | Wire | Field | Meaning / cross-check |
|---|---|---|---|
| 1 | cstring (≤63 characters) | `+0x3c` | **model name** (actor definition; for NPC items the first 3 characters are the race tag; `worldLayer.md`) |
| 2 | u8 | `+0x2c` | **NPC item** (drawn as a spawn) |
| 3 | u32 (float) | `+0xbc` | **NPC size** |
| 4 | u32 | `+0x0c` | not used by the client |
| 5 | cstring (≤15 characters), validated as the **item GUID** (16 hex characters, `0x505a50` → `0x80c920`) | `+0x10` | the commit `0x711a` sends this GUID |
| 6 | u32 | `+0x24` | **property id** the item belongs to (sent in `0x711a`; rewritten by `0x1097` via `0x556e10`) |
| 7 | u32 | `+0x28` | **record id** of the placed-item entry (same) |
| 8 | u32 × 7 (floats) | `+0x7c`–`+0x94` | **transform: scale (`+0x7c`), heading (`+0x80`), pitch (`+0x84`), roll (`+0x88`), then position (`+0x8c`–`+0x94`)** (verified from the world-object body build, `worldLayer.md`). `0x711a` sends exactly these (the four as-is; the position adjusted when anchored to a spawn). End reads the position from `+0x8c`–`+0x94`. **Note the order differs from the placed-item entry and layouts (heading first).** |
| 9 | u8 × 3 | `+0x98`, `+0x99`, `+0x9a` | `+0x98` NPC item (actor flags: no surface, no collision); `+0x99`/`+0x9a` sub-mode gates |
| 10 | u32 | `+0x9c` | surface type (1 = wall) |
| 11 | u32 × 7 | `+0xa0`–`+0xb8` | **min / max / default scale**, **heading offset**, **two wall-mount tilts**, **light type** (`worldLayer.md`, *The placement object*) |

**So the server gives the client the item's current transform in `0x626e`, and the client returns the edited transform in `0x711a`.** The fields not marked are known on the wire but not in meaning.

#### Begin: object creation (decoded)

State `+0x14` becomes a 0xc0-byte **placement object** built by **`0x5056b0(source, flags…, D, E, spawnID)`** (it copies the source's GUID, `+0x2c`, `+0x30`, `+0x98`–`+0x9c`; D/E seed the placement ray; the spawn id anchors it through `0x504b60`):

| Path | When | Source | Effect |
|---|---|---|---|
| 0 | editing (A) + OBJ `+0x2c` + spawn id F, spawn found | OBJ | **hides the spawn's actor** (unless the last argument is set) and records F in `+0x28`; if B: the preview takes the **spawn's position (`+0x64`) and heading (`+0x80`)** and is **anchored to the spawn** (`0x504b60(0, 1, F, 0)`) |
| 1 | editing (A), non-spawn item found by GUID (`0x5061e0(&OBJ+0x10)` → `0x5060c0`), OBJ `+0x2c` set, B | OBJ | **hides the existing world item's actor**; the preview is built from OBJ |
| 2 | as path 1 but OBJ `+0x2c` clear | **the existing world object** | hides it; the preview is a copy of the world object |
| 3 | not editing, or no object yet | OBJ, **D, E**, F | a new preview seeded at screen (D, E) |

**Editing hides the real item and moves a preview; the commit then applies the preview's transform.**

**Default sub-mode (set by begin when B == 0 path continues):** **Wall (1)** if the object's `+0x9c` == 1; otherwise **Cursor (2)** if the option `ItemPlacementDefaultModeCursor` (`[0xde0bd2]`) is on; otherwise **Collision (0)**.

#### Begin, other steps

Camera-relative preview positioning and floor snapping (see *Begin's distance handling*).

Then **manager `0x5adb60` clears the request latch**, and **begin(owner = manager `+0x84` (placement interface, vtable `0x9d90f0`), &OBJ, &LOC, A, B, C, D, E, F, 0)**.

### End: `0x557b60(commit)`

1. If not active → nothing. Otherwise clear **active**.
2. If `commit` and there's an item: read the item actor's position (`+0x8c`, `+0x90`, `+0x94`) and pass it to `[[0xdd2660]+0x118]` vtable `+0x74`.
3. **Call the owner's interface slot `+0x0c`(commit)** (for the manager: vtable `0x9d90f0` slot 3 = `0x5d7540`), then clean up (`0x5571d0`), call `0x519e30`, clear `+0x11`, and turn the **overlay off** (`0x48f1f0(0)`).

(The placement commit message `0x711a` is sent from the owner's interface slot `[2]` (`0x5b1f90`); which placement action invokes slot `[2]` is still to trace.)

## Input: keys, mouse, confirm, cancel

### Keybind commands (command handler `0x4d9180`, case per command, key-down only)

| Command (by call pattern) | Call | Notes |
|---|---|---|
| set sub-mode | `0x557330(mode)` with **-1 (toggle), 0, 1, 2, 3** | see *Sub-modes* below |
| rotate 90° | `0x558970(1,0,0)`, `(0,1,0)`, `(0,0,1)` | one axis each: mouse X, mouse Y, wheel |
| reset rotation | `0x558820(x, y, z)` with the constant `[0x9c732c]` on one axis | three commands |
| reset size | `0x558b30()` | |
| **cancel placement** | if active **and the manager request latch (`+0xa4`) is clear** → `end(0)` | **cancel is ignored while a real-estate request is pending** |
| (mouse-look toggle) | blocked while placement is active | |

(The expansion checks inside this handler, `0x4da119`/`0x4da133`, test bits `4`/`8` for an **unrelated** command.)

### Sub-modes (state `+0x04`)

| Mode | Name (client string) | Keybind | Allowed when (manager `0x5ae160(mode, placement object)`) | Behavior |
|---|---|---|---|---|
| 0 | "Collision Mode" (`0x1492`) | `SET_MODE_COLLISION` | unless OBJ `+0x99` ≠ 0 **and** OBJ `+0x9c` == 1 | normal surface checks |
| 1 | "Wall Mode" (`0x14a6`) | `SET_MODE_WALL` | only if OBJ **`+0x9c` == 1** (wall-mountable) | rotate-90 and the validation orientation checks branch on it |
| 2 | "Cursor Mode" (`0x1493`) | `SET_MODE_CURSOR` | as mode 0 | **the default** (`+0x04` starts at 2; option `ItemPlacementDefaultModeCursor`) |
| 3 | "Free Placement Mode" (`0x1494`) | `SET_MODE_FREE` | only if OBJ **`+0x9a` == 0 and `+0x2c` == 0** | **skips the surface check** in validation; translate has its own path (the height change, "free mode only") |
| 4, 5 | — | — | never | the toggle cycles 0–5 but these are always refused |

The keybind order is confirmed by the command-name table (`0xaa76c8` TOGGLE, `0xaa76cc` COLLISION, `0xaa76d0` WALL, `0xaa76d4` CURSOR, `0xaa76d8` FREE) and by the mode strings.

**Mode announcement (`0x556e70`):** applying modes 0–3 flashes the mode's name on screen for 3 s (`0x1492` "Collision Mode", `0x14a6` "Wall Mode", `0x1493` "Cursor Mode", `0x1494` "Free Placement Mode"; overlay text `0x41f3a0`); modes 4 and 5 show nothing.

**Set mode `0x557330(mode)`** (needs an owner and an item): **toggle (-1)** tries `(mode + 1) mod 6` until the owner allows one. For an explicit mode: if allowed → `+0x04` = mode, then `0x556e70` (apply); otherwise **`0x556f50(mode)`**, which prints a message containing the mode's name (*inferred:* not available for this item).

**So the server controls which modes a placement offers**, through placement-object fields in `0x626e`: `+0x9c` (surface type; 1 = wall), `+0x99`, `+0x9a`, and `+0x2c` (also begin's spawn-takeover flag).

**Other operations:** rotate 90° (`0x558970`) and rotate-by/reset (`0x558820`) apply through the actor (`0x5043e0`/`0x504660`) and then call translate; `0x558a70` (mouse) and reset size (`0x558b30`) use `0x504830` (*inferred:* scale) and then translate. **Translate (`0x558600`) runs the validation path** (ray-cast `0x4907f0`, `0x558030`, `0x557c40`, `0x557630`, then `0x557030` → the owner's validity callback), so **every operation re-validates** and re-tints the preview.

### Mouse

- **Movement** (`0x517f50`): applies deltas through `0x558600` (translate), `0x558820` (rotate), and `0x558a70` (one scaled value; *inferred:* height/size).
- **Mouse/camera input** (`0x539e60`, called from `0x53a6c0`): placement being active blocks several mouse modes (gated on `[0xddf702]` and `[0x15d3d00]`); one path cancels placement (`end(0)`). It records the mouse-down time in `[0xddf7a0]`.
- **World click** (`0x5384d0`):
  - **Confirm = a quick click:** placement active **and** (now − mouse-down time) **< 500 ms** → **`0x5572e0` → owner slot `+0x08` (manager `0x5b1f90`) → send `0x711a`** (the commit), called with (item, &state `+0x18` storage location, &state `+0x0c`).
  - **Place from the cursor:** builds a place request (struct base `esp+0xdc`, constructor `0x5424d0`) whose item location is **slot `0x21` (the cursor)**, `+0x30` u8 = 1, and **the click's screen X/Y at `+0x34`/`+0x38`**, then **sends `0x626e`** (60 bytes; layout in `windows/RealEstateItemsWnd.md`, *Request structs*), the same request as the Items window's Place. **The manager request latch is set (`0x5b1d40`) on every path**, including when the send fails.

### Validation while placing (green/red preview)

**When:** on begin and **every time the item moves** (`0x558600`, driven by mouse movement and keybinds) → `0x557030` → **owner slot `[1]` = manager `0x5ae010`(item, sub-mode, &state+0x0c)**. With no owner, item or actor, the code is forced to 1. **There is no per-frame re-validation**: it only runs when the item moves.

**Code** (written to state `+0x0c`; 0 = valid):
1. Ray-cast from the item's actor; read its position and rotation.
2. **`0x4b2bf0(world [0xdcee30], position, rotation, …)`**, the **boundary check** (below). Nonzero → that code.
3. Otherwise, unless in sub-mode 3 (Free): a surface-orientation check against the item's `+0x9c` (wall vs floor) and the sub-mode; out of tolerance → **`0x1d`**.
4. `0x505210(item)` true → **`0x1c`** ("not on top of a player").

**Feedback:** `0x557170` asks owner slot `[4]` (manager `0x5aa530`, which **always returns true**) and tints the item's actor **green (`0xff00ff00`) when the code is 0, red (`0xffff0000`) otherwise**.

**Commit is refused locally when the code is nonzero**: the commit (owner slot `[2]` = `0x5b1f90`) receives `&state+0x0c`, and for a nonzero code prints the local result text (`0x5ae1f0`) instead of sending `0x711a`. When it does send, it **sets the manager request latch** (`0x5b1d40`).

### Boundary check `0x4b2bf0` (world region object `[0xdcee30]`)

1. Build the **8 corners of the item's oriented bounding box** (position, rotation, extents).
2. For each **region volume** in the world object: if its id is in the **id → code map** (`+0x14`/`+0x18`) and the region **contains the whole box** (`0x8d2b00`), take its code; **code 0 → return 0 (allowed) immediately**. A containing region that isn't in the map → 1.
3. If **no region contains the box**: fall back to **manager `+0x8c`** (the server's current property from `0x6fd0`) and return its code from the map, or **1** if it isn't in the map.

**So validity depends on the item's box and the server-provided id → code map, not on where the player stands.** The map is filled by the server's `0x626e` (the pair list); nothing else found writes it.

## Region volumes: server → client `0x49df`

**Region volumes are sent by the server, not read from zone files.** Opcode **`0x49df`** (first dispatcher switch, case `0x4ca7a5`; not named in EQEmu's conf):
1. **Reset** (`[0xdcee30]->0x4b2740`): clears the region list (`0x8d3030`) **and the placement permission map** (`+0x14` → `0x5aac90`).
2. **Load** (`0x4b2ab0(stream)`): `u32 count`, then `count` regions; each becomes a 0x5c-byte object (constructor `0x8d2970`, vtable `0x9ca6a8`), parsed by **`0x8d2e30`**, and stored in the region hash by its id.

**Region wire format (`0x8d2e30`):**

| Wire | Field | Meaning |
|---|---|---|
| u32 | `+0x04` | **region id** (the key used by `0x626e`'s permission pairs; equals the definition `+0x1c` of the properties it belongs to) |
| 16 × u32 (4 × `0x8d3200`, four u32 each) | `+0x08`–`+0x44` | **a 4×4 world transform matrix** (floats) that maps the **unit cube [-1, 1]³** onto the region: position, rotation and half-extents |
| u32 × 5 | `+0x48`–`+0x58` | **not used by any housing code**: containment, the standing-in lookup and the region drawing never read them (the functions once listed here, `0x8d23b0` / `0x8d2790` / `0x8d2b50`, operate on other classes). The server can send zeros. |

**Region = oriented box (verified):**
- **Point in region** (`0x8d2ad0`): invert the matrix (`0x505ba0`), transform the point (`0x505f60`), and require **|x| ≤ 1, |y| ≤ 1, |z| ≤ 1**.
- **Box in region** (`0x8d2b00`): all 8 corners inside.
- **Region size** (used by the Items window's yard-rotation choice, `0x73fd80`): the lengths of the matrix's axis vectors (a square yard has equal width and depth).

**Consequences:**
- **Placement validity needs both messages:** `0x49df` (region shapes) and then `0x626e` (the permission code per region). With no regions, every item box falls through to the `+0x8c` fallback.
- **`0x49df` also clears the permission map.** Sent **during** placement, it makes every position invalid (red, commit refused locally) until the next `0x626e`. **That's a server-side way to revoke placement mid-placement** (the placement stays active, but can't be committed).
- **Order matters:** `0x49df` then `0x626e`. The reverse leaves the permission map empty.
- **The "standing in" lookup uses these regions (verified).** The tick sets manager `+0x94` = `[0xdcee30]->0x4b3040(local player)` → **`0x4b2f40(player position, player +0x284)`**:
  1. The default is manager `+0x8c` (the server's current property, `0x6fd0`).
  2. For each `0x49df` region that **contains the player's position** (`0x8d2ad0`): `0x4b30f0(regionID, +0x8c)` finds the property; if it exists **and is plot-category** (Plot/Guild Plot), return its id.
  3. Otherwise return the default.
- **Region id → property** (`0x4b30f0(regionID, context)`): scan every known property record, and match the one whose **definition's `+0x1c` == regionID**. If the context (`+0x8c`) is -1, the first match wins; otherwise the match must be the context property or sit **under it in the parent chain** (`+0x1c`).
- **So the Place button (and Sell, Transfer, and everything driven by `+0x94`/`+0x98`) depends on:** `0x49df` regions whose ids equal the plots' **definition `+0x1c`** values (from `0x2698`), the plots' property records, and `0x6fd0`'s current property being an **ancestor** of those plots (for example the neighborhood). Without regions, `+0x94` falls back to `0x6fd0`'s property.

## Begin's distance handling (not a rejection)

Begin doesn't enforce a maximum distance. It **positions the preview**: using the camera's position and direction (`[0xdd2660]+0x118`, vtable `+0x40`/`+0x44`), if the camera is **inside the item's bounding sphere** it pushes the item out along the view direction (a sweep in 0.1 steps, `0x4767a0`, then `0x5045e0`). It then ray-casts from the item's actor, uses the anchor spawn's ground offset when the item is anchored (item `+0x30` → `GetSpawnByID` → `+0x138`), and **snaps to the floor height** (`0x48c5b0`; -2000 = no floor found).

## Scenario: the player moves around, or crosses boundaries, during placement

**What the client does by itself:**
- **Placement is not cancelled by player movement or by the player's location changing.** None of the seven end/cancel paths is tied to the manager tick or the location ids (`+0x94`/`+0x98`); the tick keeps updating those for the windows only.
- **The item does not follow the player.** Its position only changes through mouse-movement and keybind handlers (`0x558600` callers: `0x517f50`, `0x558820`). No per-frame placement update was found. *(If a camera move while in Cursor mode also moves it, that path hasn't been found; to confirm in testing.)*
- **Validity is re-checked only when the item moves**, and depends on the item's box against the **region permission map from the last `0x626e`**, plus the fallback on `+0x8c`. Crossing a boundary on foot therefore changes nothing until the item is moved; once moved, it's valid wherever the server's map marks a containing region with code 0, **regardless of where the player stands**.
- The "too far away from the item" check (code `0x14`) belongs to **interacting** with placed items (`0x5b0200`), not to placement validity. Begin's distance math only positions the preview (see *Begin's distance handling*); **there is no maximum placement distance on the client.**

**If the server sends nothing to revoke rights:**
- Placement stays active indefinitely. The allowed regions are whatever the last `0x626e` provided.
- A quick click with a valid (green) position sends `0x711a` and sets the latch. The client then waits:
  - `0x1228` code 0 → finalize.
  - Any other code → print its text; **placement continues** (the player can move and retry).
  - **No reply** → the latch clears after 5 s; placement stays active; commit and cancel work again.
- While the latch is set, the cancel keybind is ignored, and the purchase/sell/transfer gates and the Place button refuse.

**How a server can revoke or stop placement (messages found):**
- **Reject the commit** with a `0x1228` code (placement continues; the player sees the reason).
- **Cancel it**: `0x1baf` removing the entry being placed (cancel with message `0x1468`), removing the edited item's spawn (the spawn-cleanup path; cancels when the spawn has `+0x1e54`), or a zone change.
- **Restart it** with a new `0x626e` (begin ends any previous placement first), which also delivers a new region permission map.
- **Invalidate it:** **`0x49df`** clears the permission map (and reloads regions), so every position turns red and commit is refused locally until a new `0x626e`.
- **Not found:** a dedicated "you lost placement rights" message. `0x1228` code 0 **ends placement as a success**, so it must not be used to cancel. Access-list (`0x485d`) and property-record updates do **not** affect the placement validity check; they gate starting placement (the Place button, permissions).

**Test cases this implies:** place, then walk off the plot without moving the item (should stay active, no change); drag the item out of the allowed region (should turn red, commit refused locally); commit with no server reply (the latch for 5 s, then active again); reject with a code (placement continues); cancel with `0x1baf` (ends with message `0x1468`).

### Clicking a placed item in the world (`0x5384d0`)

A click on a placed item **does not start editing directly. It sends an interaction message:**
- The item is picked by ray against placed items (`0x5428c0`/`0x542a20`). The picked object uses the placement-object layout (`+0x24` real-estate id, `+0x28` record id, `+0x8c`–`+0x94` position).
- **If the click ray hits the item's box** (`0x5423e0` → `0x7a4b90`; threshold `[0x9c3388]` ≈ 1.2e-7, i.e. an actual intersection): send **`0x218c` {u32 `+0x24`, u32 `+0x28`, u32 0}**.
- **Otherwise, if the player is within interaction distance** (`0x5051b0`: item position to player position `+0x64`–`+0x6c` ≤ **20.0** (`[0x9c4ec0]`)): send **`0x050c` {u32 `+0x24`, u32 `+0x28`, u32 -1}**.

These are the same opcodes and 12-byte layout as the interaction menu's entry types 1 (`0x218c`) and 8 (`0x050c`). **The edit path is therefore** (server obligations in bold): click → `0x218c`/`0x050c` → **server sends the interaction menu `0x21ec`** (its entries include the move/place actions) → the player picks a type-2 entry → client sends `0x626e` → **server replies `0x626e` with editFlag = 1 and the item's spawn id** → edit mode (begin takes over the spawn). *(What the server replies to `0x218c`/`0x050c` is a server design choice; the client handles `0x21ec` as the menu.)*

**Press timing in the world-click handler:**
- Nothing runs while mouse-look (`[0xddf702]`) is active.
- **While placing:** release within **500 ms** = **confirm** (commit `0x711a`); release after 500 ms = **ignored** (holding is handled by the mouse-movement handler, `0x517f50`; the release is skipped at `0x538593` → `0x539451`).
- **While not placing:** a world click is only processed when released within **750 ms** (`0x2ee`).

**Placing from the cursor** (same handler): the cursor item (slot `0x21`) must be placeable (`0x7b0b10`: definition `+0x5ac`). The request is built when manager `+0x94` (standing in a property) ≠ -1; when it is -1, an extra UI-state check (`[0x15d3d00]->0x875bd0()` bit 1) and an item check (`0x7aff80`) decide instead (not fully read).

## Everything that ends placement (`0x557b60`)

| Caller | Commit | Meaning |
|---|---|---|
| **`0x5ae1f0`: server `0x1228` result, code 0** | **1** | **the server accepted the commit** → finalize (reads the final position, notifies the owner) |
| `0x5afe80`: server `0x1baf` (item removed) | 0 | cancel ("Item placement has been cancelled because the item being placed has been removed…") |
| `0x4d0a30` (second switch; references `.eqg`/`.s3d`) | 0 | cancel (*inferred:* zone/asset change) |
| `0x5585f0` ← game loop `0x540d30` | 0 | cancel from the game loop |
| `0x590a60` ← `0x5982b0` (spawn cleanup) | 0 | if the spawn has `+0x1e54` → cancel; otherwise, if the spawn carries `0x21ea`'s fields (`+0x198`, `+0x420`) and its GUID (`+0x378`) is the placed item → clear the state's spawn link (`+0x24`, `+0x28`) |
| `0x4d9180` (cancel keybind) | 0 | see above (only while the latch is clear) |
| `0x539e60` (mouse input) | 0 | a mouse path cancels |

### The full cycle (as far as mapped)

1. **Client → server `0x626e`** (place request): from the Items window's Place (a listed item) **or** by clicking in the world with a placeable item on the cursor. The request latch is set.
2. **Server → client `0x626e`**: clears the latch and **begins placement** with the placement object (current transform), the item's storage location, the edit flag, and the spawn id when editing an existing item.
3. The player moves, rotates and resizes with the mouse and keybinds and changes sub-modes; the overlay is shown.
4. **Quick click → client → server `0x711a`** (commit, with the new transform).
5. **Server → client `0x1228` code 0** → placement ends as committed. Other codes print their result text (`resultCodes.md`); placement ends only on 0 or on a cancel.
6. Cancel paths: the cancel keybind (latch clear), item removed (`0x1baf`), the spawn going away, zone/asset change, the game loop, mouse input.

**No House of Thule or membership check has appeared anywhere on the placement path so far** (begin, end, the API, the keybinds, the mouse handlers, the commit). The gates found elsewhere (Place button enabled state, permissions) run **before** the request is sent.

## Global effects while placing (verified unless marked)

**Placement is treated like mouse-look.** Nearly every outside user of "is active" (`0x556d70`, 40 call sites in 20 functions) checks it **together with** the mouse-look flag `[0xddf702]`, and takes the same path for both:
- **UI mouse-over/cursor handling is suspended.** The main loop (`0x53aac0`, `0x53a760`), `0x53e4d0` and the zoning path (`0x541710`) only call `[0x15d3d00]->0x8779c0()` when neither mode is active.
- **Mouse input routing skips its normal handling** (`0x5fd0b0` ×3, `0x5fcd80`, `0x5fca00`, `0x5f9c90`; the last also when the UI mode is 3). *Consequence (to confirm in testing):* **UI windows don't receive mouse clicks while placing.**
- Mouse-look itself can't be toggled while placing (command handler `0x4d9180`); the world-click and mouse handlers (`0x53a200`, `0x5384d0`, `0x539e60`, `0x537d30`, `0x53a1c0`, `0x518910`) route clicks to placement instead of normal targeting.
- `0x662310` (a window function) takes a different path (its virtual `+0x94`) while placing.
- **Begin/end UI hooks:** begin → `0x519da0` decrements a UI-manager counter (`[0x15d3d00]+0x8c`, clearing `+0x68` at zero) and captures the cursor position; end → `0x519e30` restores the cursor state.

**`ItemPlacementHideUI`** (options key `0x9d3104` → global **`[0xde0bd1]`**): at begin, if on, the global **UI mode `[0xd1f3b8]` is set to 3 (UI hidden)**; at end, if the mode is 3 it is set back to **2 (normal UI)**. UI modes: 1 = character select/loading, **2 = normal in-game UI**, **3 = UI hidden** (the same state as the client's own hide-UI toggle, `0x48d030`), 5 = transition. *Side effect:* a player who had already hidden the UI gets it back when placement ends.

**Other options:** `ItemPlacementDefaultModeCursor` → `[0xde0bd2]` (begin's default-mode choice). `[0xde0bd0]` is **not** a placement option (it's `PetOwnerNames`). `ItemPlacementShowOverlay` is read directly by the display layer (`0x491c20`); how it gates the overlay isn't read.

**Expansion / membership:** no House of Thule, Veil of Alaris or membership check appears in any of these paths.

### Couplings with the windows (step 2 of the cross-cutting plan)

| Coupling | Effect while placing | Evidence |
|---|---|---|
| **Request latch** | place requests and commits set manager `+0xa4` → the Purchase, Sell and Transfer gates, the Place button (can-place returns -1) and the cancel keybind refuse until the server replies or 5 s pass | `0x5b1d40` (world-click place, commit `0x5b1f90`); gates; tick `0x5b0a10` |
| **`ItemPlacementHideUI`** | on: **all windows hidden** during placement (UI mode 3), restored at end | `0x519da0`, `0x519e30` |
| **Mouse capture** | off or on: the UI gets no mouse-over updates, and (to confirm) no clicks, while placing | the "is active + mouse-look" pairs above |
| **Location ids** (`+0x94`/`+0x98`) | **unchanged by placement**: computed from the player's position and `0x6fd0`, not from the item | tick `0x5b0a10`, `0x4b2f40` |

**Net effect on the Purchase window:** its own logic doesn't read placement state, but **while placing it is either hidden (HideUI on) or effectively not clickable (mouse captured), and its gates refuse while a placement request or commit is pending.** It returns to normal when placement ends.

## Upkeep expiring during placement (and what happens to items)

**How upkeep reaches the client:** the property record's **`+0x40` is the upkeep-current flag** (non-zero = paid). `0x5b4780` checks it for the containing plot (walking up from a house or crate), and the placement rules check it for the house or crate being placed. **Property updates (`0x63fd`) carry it**; the handler compares `+0x40` between updates to decide what to refresh. **The client doesn't count upkeep down itself** (no timer in the manager tick), so it only learns of expiry from the server.

**Designed server behavior (the client's own strings):**
- Upkeep is charged **daily** from the property's escrow. If escrow can't cover an item, the server warns (`0x191f`: "…or **the item will be moved back to the shed or closet** after the next upkeep charges are applied") and then **stores the item**.
- An expired house **can't be entered** (`0x1920`), and anchor teleports to it fail (`0x14e4`, `0x14e7`, `0x154a`).
- These messages (`0x191f`, `0x1920`) are **server-sent**: no client code pushes them.

**Client behavior when upkeep expires mid-placement:**
- **Placement is not ended by expiry.** None of the end/cancel paths is tied to upkeep or to `0x63fd`.
- **The commit does not re-check upkeep.** The commit (`0x5b1f90`) only checks the local validity code (the `0x1c` "on top of a player" check) before sending `0x711a`; the placement rules (`0x5b69b0`, with codes `0x18`/`0x19`) run only for the **Place button's** enabled state and before the place request. **So the server must reject the commit** with `0x1228` code **`0x18`** ("…the upkeep on this plot has expired") or **`0x19`** ("…on the item or house has expired"). The client prints it and **stays in placement** (only code 0 ends it).
- The Place button is re-evaluated every 500 ms, but codes `0x18`/`0x19` **don't disable it**, so it stays clickable and shows the reason when used.

**Client behavior when the server acts on expiry by storing the item being edited:**

| Server sends | Client effect on the placement |
|---|---|
| **`0x1097`** (replace the entry; placed flag → 0, new location) | **placement continues**: `0x556e10` rewrites the placement's storage location (`+0x18`–`+0x20`) and the placement object's property/record ids (`+0x24`/`+0x28`) |
| **`0x1baf`** (remove the entry) | **placement is cancelled**, message `0x1468` ("…cancelled because the item being placed has been removed…") |
| the item's spawn is removed | spawn cleanup (`0x590a60`): cancels if the spawn has `+0x1e54`; otherwise **clears the spawn link** (`+0x24`/`+0x28`) and placement continues without the anchor |
| **`0x49df`** (new regions) | clears the permission map: every position turns red until the next `0x626e` |
| `0x63fd` with `+0x40` = 0 | no effect on the placement itself; the windows refresh (and the next Place attempt reports `0x18`/`0x19`) |

**So a server that stores an item because of upkeep should decide explicitly what happens to a player editing it:** `0x1baf` ends the edit cleanly with a message; `0x1097` alone leaves the player editing an item that's now stored (the next commit carries the new ids).

## Still to map

Nothing left for the client analysis; confirming at runtime that windows receive no clicks while placing is a test item (Q25).

(Resolved: the region's five trailing values (unused); OBJ's fields and the body/anchor function `0x504b60` in `worldLayer.md`; C is unused and D/E are the screen position; the mode announcement; the extra `0x626e` pair is an explicit extra read. Begin (`0x558bb0`) takes ten arguments: owner (0 from the server), OBJ, LOC, A, B, C, D, E, F, and a tenth byte that `0x626e` always passes as 0, meaning an NPC item's spawn actor is hidden while editing.)

## Placement options (Options window, General page)

Three client settings, stored under the names below: **`ItemPlacementHideUI`** (auto-hide the UI when placement starts), **`ItemPlacementDefaultModeCursor`** (start in Cursor mode instead of the default), **`ItemPlacementShowOverlay`** ("display the real estate text overlay on item placement"). Nothing the server sends affects them; they explain differences between testers' clients.
