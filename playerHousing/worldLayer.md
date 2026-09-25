# Placed Items in the 3D World

How placed items exist, render and are interacted with in the world. **Status: done** (steps 1–6).

## Two representations

| Representation | What | Fed by | Code |
|---|---|---|---|
| **World objects** | placement objects (the 0xc0-byte layout that `0x626e` also carries) held in a client-side manager | **`0x1045`** (add/update: find by GUID, then insert) and **`0x2a9e`** (remove: iterate, then remove by GUID); neither is named in EQEmu's RoF2 table | manager singleton `0x5061e0` (`[0xdcfc70]`, a list head) |
| **NPC items** (the client's term: placed items backed by an NPC spawn) | ordinary spawns tagged with a placed item | the normal spawn messages, then **`0x21ea`** (spawn id, GUID → spawn `+0x378`, `+0x198`, `+0x420`) | spawn fields read by the functions below |

(The earlier note "placed items render as spawns" is only half the picture; see `placedItemsAndProperties.md`.)

## World-object manager (`[0xdcfc70]`, getter `0x5061e0`)

| Method | Size | Role | Callers |
|---|---|---|---|
| `0x506030` | 9 | insert | `0x1045` handler (`0x4d0940`) |
| `0x5060c0` | 38 | find by GUID (16 hex chars; all "0" = none) | `0x1045` handler; placement (`0x5571d0`, `0x558bb0`); Items window (`0x730de0`) |
| `0x5061a0` | 62 | remove by GUID (destroys the object, `0x504530`) | `0x2a9e` handler (`0x4d09de`) |
| `0x506120` | 23 | **find by (property id, record id)** | `0x2a9e` handler; `0x65d740` (from `0x53e4d0`) |
| `0x506160` | 12 | *inferred:* pick (ray test over the list) | world click `0x537d30` |
| `0x506080` | 21 | clear all | zone cleanup `0x532640` (from `0x543100`); teardown `0x4939b0` (from `0x53bdd0`/`0x540000`/`0x540d30`) |
| (getter only) | | not yet classified | `0x489cb0`, `0x51c890` |

## NPC items (spawn side)

The distinctive spawn field **`+0x1e54`** is read by: `0x504b60` (the anchor function placement uses), `0x58f750` (12 callers), `0x590a60` (spawn cleanup; cancels placement for such spawns), `0x5961e0`, `0x599330`, `0x4877e0`. (`+0x378` is too common an offset across unrelated classes to use as an anchor.)

## Step 2: world objects

### Messages

| Opcode | Direction | Payload | Handler behavior |
|---|---|---|---|
| **`0x1045`** | server -> client | **a placement object** (the same format as the `0x626e` begin-placement object; reader `0x505400`) | allocate a 0xc0-byte object (`0x504460`), read it; if `+0x2c` is set, set `+0x98` = 1 (`0x4b3940` just returns `+0x2c`); **find by GUID**: found -> the existing object **updates itself** (`0x504fd0`) and the new copy is discarded; not found -> **insert**, then build its body (`0x504b60`(isBeingPlaced, 0, 0, 0)); if it's the item being placed, `0x4b3950(1)` |
| **`0x2a9e`** | server -> client | the item reference: **GUID `+0x00`**, **property id `+0x20`**, **record id `+0x24`** (the common request-header layout) | **find by (property id, record id)** (`0x506120`), **remove and destroy** (`0x5061a0`); if it's the item being placed, `0x4b3950(0)` |
| **`0x2523`** | server -> client | the same item reference, with the **location at `+0x14`** | update the **item being placed** (`0x556e10`: its storage location and ids), like `0x1097` does; no effect otherwise |

### The placement object (wire order, reader `0x505400`), with the world meanings

| # | Wire | Field | Meaning | Evidence |
|---|---|---|---|---|
| 1 | cstring ≤ 63 | `+0x3c` | **model name**: an actor definition name for ordinary items (for example an item's "IT…" model); for NPC items, **its first 3 characters are the race tag** | actor build `0x504b60` -> `0x48b710` ("IT", "BLOB_ACTORDEF", "BBBOARD") / `0x8dc4f0`(3) -> `0x50f070` -> `0x50a370` |
| 2 | u8 | `+0x2c` | **NPC item** (the item is drawn as a spawn) | `0x504b60`; `0x4b3940` |
| 3 | f32 (read as u32) | `+0xbc` | **the NPC's size** (0 = default) | `0x5a4ec0` on the NPC item's spawn |
| 4 | u32 | `+0x0c` | **not used by the client**: only read (`0x505400`) and copied (`0x504fd0`, `0x5056b0`); a server-defined value | |
| 5 | GUID (16 hex) | `+0x10` | the item's GUID | |
| 6, 7 | u32, u32 | `+0x24`, `+0x28` | property id, record id | |
| 8 | f32 ×7 | `+0x7c`–`+0x94` | **scale, heading, pitch, roll, then position (three coordinates)** | scale: `0x504830(+0x7c)` (resize); rotation: actor `+0x74`(`+0x80`, `+0x84`, `+0x88`); heading `+0x80` also sets an NPC item's spawn heading (`0x5024b0`) |
| 9 | u8 ×3 | `+0x98`, `+0x99`, `+0x9a` | `+0x98`: set for NPC items (the actor's flags get `|= 5`); `+0x99`/`+0x9a`: allowed placement sub-modes (`placementMode.md`) | `0x504b60`; `0x5ae160` |
| 10 | u32 | `+0x9c` | surface type (1 = wall) | `0x5ae160` |
| 11 | f32 ×7 (read as u32) | `+0xa0`, `+0xa4`, `+0xa8` | **minimum scale, maximum scale, default scale** | resize clamps to `+0xa0`/`+0xa4` (`0x558a70`); reset size returns to `+0xa8` (`0x558b30`) |
| | | `+0xac` | **a heading offset** (EQ heading units, 512 = 360°): added to the heading the placement code gets from `[0x15d46b4]` (vtable `+0x18`) | `0x557070`; `0x5043e0` treats it as a heading (quadrant test ((`+0xac` + 64) / 128) & 1) |
| | | `+0xb0`, `+0xb4` | **tilt angles for surface (wall) mounting**: `0x5043e0` compares the model's footprint (width vs depth, from its bounding box) with the heading quadrant and picks one, which `0x504660` applies as the second or third rotation angle | `0x557070`, `0x5043e0`, `0x504660` |
| | | `+0xb8` | **light type** (non-zero -> a light source at the item's position) | `0x48b270(+0xb8, position)` -> `+0x34` |

Runtime-only fields: **`+0x30`** spawn id (an NPC item's spawn), **`+0x34`** the light, **`+0x38`** the actor (or the NPC item's spawn actor).

**Float order differs between messages:** the placement object (`0x626e`, `0x711a`, `0x1045`) is **scale, heading, pitch, roll, position**; the placed-item entry (`0x1cef`) and the layout file are **heading, pitch, roll, scale, position**.

### Building the body (`0x504b60`)

1. **Tear down** any previous body: an NPC item's spawn is removed from the spawn manager (`[0xe641d0]` -> `0x8cf790`); otherwise the actor is removed from the display (`0x490a10`); a light is removed (`0x48b520`).
2. **Ordinary item** (`+0x2c` = 0): **`[0xdd2660]->0x48b710(model name, position, heading, pitch, …)`** creates a display actor from the model name.
3. **NPC item** (`+0x2c` = 1): clone a given spawn (`0x599d00`), or **create a client-side spawn** (`[0xe641d0]->0x599bd0`) of the race named by the model's first 3 characters; apply the size (`+0xbc`); optionally snap to the floor (floor height + the spawn's `+0x138`); place it (position + heading, `0x5024b0`); then **mark it: spawn `+0x1e54` = 1**, and store its spawn id in `+0x30`. (**`+0x1e54` = "a spawn the client created for a placed NPC item"**; spawn cleanup cancels placement for such spawns.)
4. For either: actor `+0x2c`(1 or 9, the type code) and **`+0x28`(the placement object)** (the back-pointer the ray filter compares against); if `+0x98`: actor flags `|= 5` (no surface, no collision); an NPC item without a spawn anchor snaps to the floor using its bounding box (`0x48c5b0`, `0x4907f0`, `0x5045e0`); rotation (actor `+0x74`), scale (`0x504830`), visibility (actor `+0x08`); **a light if `+0xb8` ≠ 0**.

**Update in place** (`0x504fd0`): copies every wire field from the new object and re-applies position, rotation and scale to the existing actor.

**So placed NPC items can be drawn by a spawn the client creates itself** (from the model name's race tag), which is separate from a server spawn bound by `0x21ea` (step 3).

### Lifecycle

| Event | What happens | Code |
|---|---|---|
| **zone teardown** | clears the ground objects (`0x4dc1b0` -> `0x4dbed0`) and **all placed-item world objects** (`0x506080`) | `0x489cb0` (reached through a function pointer) |
| **zone-change cleanup** | clears the placed-item world objects ("Cleanup 10/11") | `0x532640` <- `0x543100` |
| **game teardown** | clears them | `0x4939b0` <- `0x53bdd0` / `0x540000` / `0x540d30` |
| **display rebuild** | recreates other actors, then **rebuilds the body of every world object that has none** (`+0x38` = 0 -> `0x504b60`) | `0x51c890` (function pointer; *inferred:* display reset / zone graphics reload) |
| **zone entry** | if the server's current property (`0x6fd0`) is known, prints **"You have entered %1."** (`0x3045`) with its record name (`+0x38`); for properties that aren't neighborhoods it clamps the camera distance (`[0xde0d7c]` + `0x34`) to 53 (see *Neighborhood camera*); runs the manager tick | `0x53cca0` |

**So world objects don't survive zoning**: the server must send `0x1045` for every placed item again after each zone-in. The actor factory `0x48b710` copies the model name (up to 64 characters) and special-cases names matching "PLAYER_1"; item models use their actor definition names.

**Handed to step 4 (resolved there):** `0x65d740` is the container manager's world-container range and existence check, not targeting.

## Step 3: NPC items

**Which body an NPC item gets** (from the body builder `0x504b60`'s arguments: 1 = visible, 2 = create a spawn, 3 = a spawn id to clone, 4 = snap to the floor):

| Situation | Body | Evidence |
|---|---|---|
| **a placed NPC item in the world** | **the server's spawn** (an ordinary spawn from the spawn messages, instantiated from an NPC type like any other), **bound to the item by `0x21ea`**; the item's world object (`0x1045`, `+0x2c` = 1) creates **no body** of its own | `0x1045` calls `0x504b60`(isBeingPlaced, **0**, 0, 0) |
| **placing or editing an NPC item** | the preview: **a clone of the server spawn** (`0x599d00`), or, with no spawn yet, **a client-created spawn of the model name's race tag** (`0x599bd0`), marked **`+0x1e54` = 1** | placement begin `0x5056b0` -> `0x504b60`(0, **1**, spawnID, 0) |

The world object and the server spawn are **correlated only by the item's GUID** (`0x21ea` never touches the world-object manager).

**`0x21ea` fields, now named:** payload `+0x18` -> spawn **`+0x198` = property id**; payload `+0x1c` -> spawn **`+0x420` = record id** (verified: the world right-click builds the `0x21ec` menu request from exactly these two). Spawns start unbound (`0x590dd0`, `0x5a0000` reset them) and clones copy them (`0x598480`).

**Client-created preview spawns (`+0x1e54`):** exempt from the **12-second staleness check** (`0x2ee0` ms without an update; `0x58f750`, `0x4877e0`; `+0x1e55` exempts too), since they never get server updates; **removing one ends placement** (spawn cleanup `0x590a60`). Removing a *bound* spawn that is the item being edited only unlinks it from placement.

**Right-clicking a bound NPC item's spawn** (world click `0x537d30`, spawn `+0x198` ≠ -1):
1. If the player is beyond a distance threshold (`[0x9c57f4]`), the click only counts when **the property the player is standing in (manager `+0x94`) is the spawn's property (`+0x198`)**.
2. If the spawn's class (`+0xeb8`) is a **guildmaster (20–35, `0x59ef30`)** or one of the **merchant / banker service classes (40–74 table, `0x59eee0`)** and the player is **within 20 units**: the **ordinary NPC interaction** (`0x5a06f0`: shop, bank, training).
3. Otherwise: send the **`0x21ec` interaction-menu request** with the spawn's property id, record id, GUID and the mouse position.

**The `0x21ec` menu's NPC link:** menu entry types `0xc` / `0xd` take the NPC's **spawn id from the reply's `+0x2fc`** (window `+0x38c`; the client's own request doesn't carry it, so the server must fill it), look it up (`GetSpawnByID`) and run the NPC interaction (`0x536d80`, which references the merchant window). **So the server's menu reply for an NPC item carries the NPC's spawn id at `+0x28`.** If the spawn isn't found, nothing happens (the result text `0x14a3` "…its NPC was not found in the zone" is a result code the server can return).

## Step 4: interaction

### Housing tradeskill containers (the movable forge): answered

**A placed item becomes a usable forge through the same world-container mechanism as a zone forge, identified by its property and record ids instead of a ground-object id.** Traced backwards from the container UI:

1. **Open:** the server sends **`OP_ClickObjectAction` (`0x0c1e`)**; the handler `0x4c19a0` refuses if a world container is already open (`0x5a3` "ERROR. Tried to open a world container when you already have one open."), builds a temporary container item (0x150 bytes, vtable `0x9caf74`, definition `0x4d57d0`) and opens it through the container manager **`[0xd1fd84]`** (`0x65d890` -> `0x65cae0`), which records the world container as **(drop id, property id, record id)** at `+0xa8` / `+0xac` / `+0xb0`.

| Payload | EQEmu RoF2 (`ClickObjectAction_Struct`) | EQEmu sends | Client meaning |
|---|---|---|---|
| `+0x00` | `drop_id` | the object's drop id | ground-object id (`+0xa8`) |
| **`+0x04`** | `unknown04` ("Seen -1") | **-1** | **property id of a placed container** (`+0xac`) |
| **`+0x08`** | `unknown08` ("Seen -1") | **-1** | **record id of a placed container** (`+0xb0`) |
| `+0x0c` | `type` | object type | container / combine type (read as a byte; 0x35 special-cased) |
| `+0x10` | `unknown16` | 0 | (overwritten by the client) |
| `+0x14` | `icon` | icon | icon |
| `+0x18` | `unknown24` | (not set) | container definition `+0xec` |
| `+0x1c` | `object_name[64]` | name | the container's name |

**So a housing forge needs no new message: the server sends `OP_ClickObjectAction` with the placed item's property and record ids instead of EQEmu's hardcoded -1s** (a small encoder change).

2. **Opened:** the client sends **`0x06c8`** (12 bytes: the container's location; a world container is location type 4; not named in EQEmu's table).
3. **Combine:** **`OP_TradeSkillCombine`** (24 bytes) carries the container's **location**, not the placed-item ids.
4. **Close:** the client sends **`OP_ClickObjectAction` with no payload** (`0x65d180`).
5. **Auto-close:** every 500 ms the container manager (`0x65d740`, from the per-frame code) checks the world container still exists and the player is in range: **for a placed container, the world object must still exist (found by property and record id) and the player must be within 20 units of it** (`0x5051b0`); otherwise it closes (and sends the close).

**Server obligations:** decide when a placed item opens as a container (for example in reply to `0x218c` / a menu entry); send `OP_ClickObjectAction` with the ids; **remember which container each player has open**, since combine and close don't say.

**Edit mode:** while placing, world clicks go to placement, so a forge can't be clicked open then. Editing an open forge keeps it open (its world object stays, hidden, with the same ids), but the range check uses the world object's position, which only changes when the server confirms the move (`0x1045` update); **moving it more than 20 units away, storing it or removing it closes the container** at the next check. *(The range-during-edit detail is inferred from the check's inputs.)*

### Click arbitration and targeting

**Placement mode captures clicks first**: the mouse dispatcher `0x53a200` and the input routing (`0x5fd0b0`) check "placement active" before anything else (`placementMode.md`, *Global effects*).

**Left click** (`0x5384d0`), in order:
1. placing: a quick click **confirms** (`0x5572e0`);
2. **a spawn under the cursor** (`0x5190a0`) -> **target it** (`0x536310`, `[0xdd2648]`); placed NPC items are spawns, so they're targeted like any NPC;
3. **the item on the cursor** (type from item definition `+0x1d4`, `0x7affa0`): types **12, 33 (key), 39** -> use it on the world (`0x51ecb0`: a key on a door -> `OP_ClickDoor`); a placeable item -> **place it** (`0x626e`);
4. **placed items** (`0x5428c0` / `0x542a20`, ray-box test `0x5423e0`): a hit -> **`0x218c`**; otherwise within 20 units -> **`0x050c`**;
5. **ground objects** (object manager) -> **`OP_ClickObject`** (`0x4aa1`).

**Right click** (`0x537d30`): **placed world objects first** (manager pick `0x506160`): a hit -> **`0x21ec` menu request** with the object's ids; **then spawns** (`0x5190a0`): a bound NPC item follows the rules in *Step 3* (beyond **60 units** (`[0x9c57f4]`) only from inside the same property).

**Doors:** ordinary door clicks come through the input and actor-update path (`0x5fd0b0` -> `0x49ca10` -> `0x48d530` -> `0x512690` -> `OP_ClickDoor`), which placement mode suppresses; `0x48d530` is the per-frame actor update (also reached from the main loop, `0x49d410`), so bumping into doors goes the same way. A key on the cursor goes through step 3 above, and the keybind handler (`0x4d7ee0`) also reaches `0x51ecb0`.

**Targeting:** **only spawns are targeted**. Placed world objects are never targets: clicking them sends the placed-item messages instead. No hover or tooltip behavior specific to placed items was found.

**Placed-item interaction check** (`0x5b0200`, also reached through the wrapper `0x577f60`): too far (`0x14`), not placed in the zone (`0x15`), no interact access (`0x4a`), NPC not found (`0x2a`).

### Neighborhood camera

The camera (`[0xde0d7c]`, distance at `+0x34`) allows **a maximum distance of 212 inside a neighborhood and 53 elsewhere** (`0x518a70` from the mouse-move handler, using manager `+0x90`); **on zone entry into a property that isn't a neighborhood, a distance above 53 is clamped back to 53** (`0x53cca0`). So players can pull the camera much farther back in neighborhoods (to see houses).

## Step 5: houses and yards in the world

**Houses are placed items outside and ordinary zones inside.** The client has **no house-interior-specific code**: the only compares against the interior / guild-hall zone ids are zone-table bounds checks (ids 1–767, `0x7fe300`, `0x7fefe0`, `0x606280`) and one short list of special zones that includes guild hall 738 (`0x4326b0`, unrelated rule). The client's zone table (`0x7e0de0`) gives the housing zones exactly: neighborhood 712; house interiors **714–720, 723, 739–750, 766** (721/722 unregistered; 724–736 and 752–765 are ordinary zones); guild halls 737/738/751 (`robustness.md`). The prior research's "714–766" range is too broad.
- **Outside:** a house is a placed item (house category) whose body is built from its model like any world object (`0x1045`).
- **Entering:** a **server action** (zoning the player into an interior zone), triggered however the server chooses, for example from `0x218c` on the house or a menu entry (or an ordinary door by server choice). The client has entry messages the server can use: `0x1920` "You cannot enter %1 because the upkeep on it has expired."; guild halls: `0x17e1` "Your Guild Hall is loading, please try again in 10-15 seconds.", `0x17e2` "You are unable to enter the Guild Hall at this time."
- **Inside:** an ordinary zone for the client, so **the real-estate data must be sent there too**: the current property (`0x6fd0`, the house, which gives "You have entered %1."), the regions (`0x49df`), the definitions and records, and the item lists. Houses also hold **teleport anchors** (strings `0x14e1`–`0x14e7`, `0x154a`/`0x154c`: an anchor's house must be placed in a yard; teleports only from zones connected to its neighborhood).
- **Q11 (the interior zone-in crash from the prior research) can't be resolved statically**: there's no interior-specific client path, so the likely cause is data the client expects but didn't get (or inconsistent data) in the interior zone. **Capture the crash address during testing** and map it to the code documented here.

**Guild halls:** zones 737 / 738 / 751. `/outputfile realestate` writes the real-estate export (`0x5b37d0`, "%s-RealEstate.txt"); `/outputfile guildhall` requires being in a guild (character `+0x204c` ≠ -1) (`0x4febd0`). Dumping the real-estate guild hall has client messages `0x153e`–`0x1540` (in a guild, on the guild plot or in the hall, modify item access).

**Placement overlay (`ItemPlacementShowOverlay`):** read at display setup (`0x491c20`) into display `+0x2d80`; **placement begin and end switch a render layer** (`0x48f1f0`: renderer `[0x15d46a4]` vtable `+0xd0` / `+0xd4`) when it's on.

**Region boxes:** the client can draw **every `0x49df` region as a translucent red box** (color `0x22ff0000`, `0x4b2920`, run every frame from `0x53e4d0`) when the region object's `+0x10` flag is set, but **nothing in this build sets that flag** (searched: the housing and region code, the `0x49df` message and loader, and slash-command strings; the only write clears it at teardown, `0x540fd5`). The region object is built inline at game init (`0x535950`), whose constructor sets `+0x10`/`+0x11` to 0, and a data-flow search (every write through a register loaded from `[0xdcee30]`, including `setcc` forms) finds no other write. *Inferred:* a debug feature with no way to enable it. **Not to be confused with the placement preview's red tint**: when a position is invalid, the item being placed is tinted red (valid: green) by the validity check on every move (`0x557170`, `placementMode.md`, *Validation*); that's the active "can't place here" feedback, drawn on the item, not the region.

**/find wisp:** Locate (target type 3) and Plot Search Go (target type 4) use the client's ordinary /find path display; nothing housing-specific in how the path is drawn.

## Step 6: the world flow (close-out)

| Stage | Server -> client | Client result | Client -> server |
|---|---|---|---|
| **zone-in** | property data (`0x6fd0` current property, definitions `0x2698`, records, regions `0x49df`, item lists `0x1cef`) | "You have entered %1."; camera distance 212 in neighborhoods / 53 elsewhere | |
| **ordinary placed items** | **`0x1045`** per item (the placement object: model name, transform (scale-first), scale limits, mounting angles, light) | a display actor from the model name, plus a light if `+0xb8` ≠ 0; **re-sent after every zone-in** (zoning clears them) | |
| **NPC items** | the **server spawn** (ordinary spawn messages) + **`0x21ea`** (GUID, property id, record id); `0x1045` with `+0x2c` = 1 for the item | the spawn is the body (the world object makes none); correlated by GUID | |
| **changes** | `0x1045` (update in place by GUID), `0x2a9e` (remove by ids), `0x2523` (item being placed moved) | actor updated / destroyed; placement's "editing existing" flag follows (`0x4b3950`) | |
| **left click** | | target a spawn; place the cursor item; **placed item: `0x218c` (hit) / `0x050c` (≤ 20 units)**; ground object: `OP_ClickObject` | `0x218c`, `0x050c`, `0x626e`, `OP_ClickObject` |
| **right click** | `0x21ec` menu (entries; **`+0x2fc` = the NPC spawn id** for NPC items) | placed item -> menu request; NPC item: service classes within 20 units -> their NPC interaction, others -> menu | `0x21ec` |
| **tradeskill container** | **`OP_ClickObjectAction`** with **`+0x04`/`+0x08` = property id / record id** (EQEmu sends -1) | a temporary world container, closed beyond 20 units or when the item disappears | `0x06c8` (opened), `OP_TradeSkillCombine` (location only), `OP_ClickObjectAction` (close, no payload) |
| **placement preview** | (begin `0x626e`) | a clone of the NPC item's spawn or a client-created spawn (`+0x1e54`), tinted green/red | `0x711a` |

**Resolved in the close-out:**
- **Actor vtable `+0x08` = set hidden:** placement end calls it with 0 on the world object's actor (`0x5571d0`), while begin, `0x21ea` and `0x1045` (for the item being placed) call it with 1: **the real item is hidden while its preview is edited and shown again at the end**.
- **Actor type codes** (actor `+0x2c`, from every constant use in `eqgame.exe`): **9 is set only for placed items** (their own category); 3 for doors (`0x510960`); 5 and 7 for static world objects (`0x503e20`, "%s_ACTORDEF", "GRASS_"); 1 at spawn sites and for a placed NPC item's spawn body.
- **Spawn `+0x1e55` is never set** in this build (only zeroed by the spawn reset and copied by the clone, `0x599330`); only `+0x1e54` (the NPC-item preview) is ever 1.

**Actor flags, resolved with `EQGraphicsDX9.dll`:** the graphics DLL's actor class (vtables such as `0x10137074`) stores the back-pointer at actor `+0x74` (set `+0x28`, get `+0x20`/`+0x24`), the **type code at `+0x78`** (set `+0x2c`, get `+0x30`) and the **flags at `+0x148`** (**get `+0x54`, set `+0x50`**); hide (`+0x08`) sets or clears a bit (mask `[0x10174ba8]`) in the actor's scene node flags (`+0x5c`, `0x100c4050`). **The DLL never interprets the flags; `eqgame.exe` does:**
- **bit 0 (`0x1`) = ignored by floor-height and ray queries**: the ray/surface filter callback `0x476280` (passed to the floor-height query `0x48c5b0` and to `0x507230`, `0x440000`) skips such actors; the same callback also skips **a placed item (type 9) that is the caller's excluded object** (so the item being placed doesn't hit itself);
- **bit 2 (`0x4`) = ignored by movement collision**: the collision candidate filter `0x475ac0` (vtable `0x9cf81c`, built in the per-frame spawn movement update `0x508070`, reached from `0x509050` <- `0x4877e0` / `0x49ca10`) skips such actors; otherwise it applies type rules (spawns: spawn checks; doors: collide).

**So a placed NPC item's spawn (flags `|= 5`) neither blocks movement nor acts as a surface; ordinary placed items get neither bit, so they collide and can be stood on.** (Bit 1, cleared by doors, and bit 3 aren't used for placed items and stay unnamed.)

**Open items:**
- **Q11** (the house-interior crash) needs a crash address from testing.

## All world-entity paths (for comparison)

Every path the client uses to put something in the world, with its handler and manager. **Housing uses only the last two rows**; see *Hooks into housing* for the proof.

| Kind | Server -> client (handler) | Client -> server | Manager / first calls |
|---|---|---|---|
| **Spawns** (NPCs, players, corpses) | `OP_ZoneSpawns` `0x5237` (`0x4caea0`), `OP_NewSpawn` `0x6097` (`0x4cae8e`), `OP_DeleteSpawn` `0x7280` (`0x4cc82a` -> `0x4b8380`); appearance: `OP_SpawnAppearance` `0x0971`, `OP_Illusion` `0x312a`, `OP_WearChange` `0x7994`, `OP_LevelAppearance` `0x3bc9` | | the spawn manager (the create path lies past the handlers' first instructions; to trace in step 3) |
| **Doors** | `OP_SpawnDoor` `0x7291` (`0x4ca32c`), `OP_MoveDoor` `0x08e8`, `OP_RemoveAllDoors` `0x700c` | `OP_ClickDoor` `0x3a8f` | door manager `[0xdcfd00]` (add `0x511990`/`0x510850`, move `0x510740`, remove all `0x510bd0`) |
| **Objects** (ground spawns; tradeskill objects such as forges and kilns) | `OP_GroundSpawn` `0x6fca` (`0x4d0438`), `OP_ClearObject` `0x7a11`, `OP_ClickObjectAction` `0x0c1e`, `OP_WorldObjectsSent` `0x5ae2`, `OP_TradeSkillCombine` `0x579a` | `OP_ClickObject` `0x4aa1` (sent by the world click `0x5384d0`) | object manager `[0xdcf120]`/`[0xdcf124]` (`0x4dbbd0`, `0x4dbd50`, `0x4dc1b0`, `0x4dbb10`) |
| **Traps** | **none**: `OP_RemoveTrap`, `OP_SenseTraps`, `OP_DisarmTraps` have no handler | | (server-side only) |
| **Zone points** | `OP_SendZonepoints` `0x69a4` (-> `0x5133e0`) | | |
| **Auras** | `OP_UpdateAura` `0x1456` (-> `0x466e60`) | | |
| **Placed items: world objects** | **`0x1045`** add/update, **`0x2a9e`** remove, **`0x2523`** update the item being placed (see *Step 2*) | | the placed-item manager |
| **Placed items: NPC items** | the spawn messages above, then **`0x21ea`** | | spawns |

`0x410c40` is shared by `OP_GroundSpawn`, `0x1045` and four other message cases: a generic step (*inferred:* a stream or item reader), not a housing hook.

## Hooks into housing

**No door, object, zone-point, aura or spawn-creation code calls into the real-estate code, and the real-estate code calls none of theirs** (checked: references to the door and object managers and their functions from the real-estate ranges; callers of the real-estate manager getter `0x5b3540` from outside). The 30 outside callers of the manager getter are:

| Group | Functions | Step |
|---|---|---|
| message dispatch | `0x4cb870` (forwards the real-estate opcodes), `0x4d0050` (`0x626e`), **`0x4d2820` (`0x7bb1`, unknown: Q23)** | 2 / Q23 |
| world clicks and interaction | **`0x537d30`**, **`0x5384d0`** (world click), **`0x577f60`** (placed-item interaction check), **`0x518a70`** (unknown) | 4 |
| zone and game lifecycle | **`0x53cca0`** (zone loading), `0x540d30` (main loop / cleanup), `0x5629c0` (login) | 2 |
| guild halls | **`0x4febd0`** ("guild", "guildbank", "guildhall") | 5 |
| items and inventory | `0x5b83f0` (item display "Placed"/"Stored"), `0x6a06b0`, `0x6a0880`, `0x57a890`–`0x57ae80`, `0x788a10`, `0x788e40`, `0x793f00`, `0x67e560` | (item display; not rendering) |
| other UI | `0x4d9180` (keybind commands), `0x52e510` ("EQ Players") | — |

**So the client draws housing through two paths of its own.** Doors, objects and ordinary spawns can still be part of a house **by server choice** (for example a house entrance as an ordinary door), and the client would handle them generically. Step 4 covers how the world click arbitrates between placed items, objects (`OP_ClickObject`) and doors.

## Plan

| Step | Scope |
|---|---|
| **2. World objects** | `0x1045` / `0x2a9e` wire formats; what zone loading (`0x53cca0`) does for housing; insert/update/remove; how the actor and model are built (`0x5056b0`-style construction, which field names the model, scale); per-frame update/draw (`0x65d740`); zoning (`0x532640`); the two unclassified getters |
| **3. NPC items** | `0x21ea` and spawn `+0x198` / `+0x420` / `+0x1e54`; the functions above; which items are world objects vs NPC items; the NPC side (merchant / banker paths, "its NPC was not found in the zone") |
| **4. Player interaction** | picking (`0x506160`, `0x5428c0`/`0x542a20`, `0x5423e0`), targeting (can world objects be targeted?), hover, cursor, left/right click, range, the routes to `0x218c` / `0x050c` / `0x21ec`; **how the world click arbitrates between placed items, objects (`OP_ClickObject`), doors and spawns**; `0x577f60`, `0x518a70` |
| **5. Houses and yards in the world** | house models and entering houses (a placed house item? a door by server choice?); guild halls (`0x4febd0`); region display (`ItemPlacementShowOverlay`); the /find wisp display |
| **6. Close-out** | a world flow (server data → what the player sees and clicks), open questions, updating `placedItemsAndProperties.md` |

**Case to answer throughout: placed tradeskill containers (a forge you can move).** A housing forge may be a different kind of thing from a world forge while still ending in a usable forge interface, so **don't assume the object path** (`OP_ClickObject` -> `OP_ClickObjectAction`). **Method: trace backwards from the UI**: find every function that opens the tradeskill / container window (for a world object, for an inventory container such as a collapsible forge, or otherwise), list all their callers, and follow the ones reachable from a placed item, the interaction menu (`0x21ec` entry types) or other real-estate code. Then answer: (a) which representation a housing forge uses and which path opens its interface; (b) edit mode: clicking it while placing or editing something else (world clicks go to placement), editing it while it's open or in use by someone else, and whether an open interface follows or closes when the item moves.
