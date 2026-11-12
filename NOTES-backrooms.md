# How the Backrooms mod builds its world

Written 2026-11-12 for playbook Prompt 1.2, against the in-game copy of TGWeaver's mod
(`data/mods/Backrooms`, id `backrooms`) on this fork's experimental code.

> **Short version:** the mod doesn't add a new place. It **replaces what "the normal world" is**.
> It re-points the `default` dimension at its own region settings, and those settings say
> "every map square on floors −3 to +3 is a Backrooms chamber; every other floor is solid wall."
> Each chamber is then filled at random from about 400 hand-drawn room pieces.

---

## 1. The five layers, top to bottom

```
dimension "default"                       region_settings/dimensions.json
   │  (the mod overrides the vanilla one)
   ▼
dimension_region_layout "default_backrooms"   UNIFORM = one region everywhere
   ▼
region_settings "default_backrooms"           region_settings/regional_map_settings.json
   │  rivers, lakes, forests, roads, cities → all OFF
   │  default_oter → what EVERY empty map square becomes, per floor
   │  specials whitelist → only specials flagged BACKROOMS may spawn
   ▼
overmap_terrain "backfloorchamber"            overmap_terrain.json
   │  one 24×24-tile square on the world map
   │  monsters: GROUP_BACKROOMS, 40% chance, 2–15 of them
   │  map extras: "backrooms" collection
   ▼
mapgen for "backfloorchamber"                 overmap_terrain.json (+ rooms/*.json)
      carpet floor + 3 layers of nested room pieces (see section 2)
```

### 1.1 Dimension: how it takes over the whole world

`region_settings/dimensions.json`:

```json
{ "type": "dimension_region_layout", "id": "default_backrooms",
  "generation_mode": "UNIFORM", "uniform_region": "default_backrooms" },
{ "type": "dimension", "id": "default", "region_layout": "default_backrooms" }
```

Vanilla defines `dimension "default"` → region `default`. The mod **redefines the same id**,
so the starting world is generated with Backrooms rules. That's the whole trick. It's also why
the mod conflicts with other world-replacing mods (Innawood, Aftershock: Exoplanet …): only one
mod can own `default`.

### 1.2 Region settings: what fills empty space

`regional_map_settings.json`, `region_settings "default_backrooms"`:

- `rivers`, `lakes`, `ocean`, `forests`, `ravines`, `highways` → `null`; `cities: "no_cities"`;
  `place_roads: false`. Nothing natural or urban is placed.
- **`default_oter`** is a list of 21 entries, one per floor from z −10 to z +10
  (index = 10 + z, from `src/overmap.cpp:307`):

  | Floors | Filler terrain |
  |---|---|
  | z −10 … −4 | `backroom_border` (mapgen = solid `t_backroom_wall_border`) |
  | **z −3 … +3** | **`backfloorchamber`** (7 floors of Backrooms) |
  | z +4 … +10 | `backroom_border` |

  So the Backrooms is **7 stacked floors** with solid caps above and below.
- `feature_flag_settings.whitelist: [ "BACKROOMS" ]`: only overmap specials carrying the
  `BACKROOMS` flag can spawn. Today those are the six stairwell specials.
- `region_terrain_furniture` swaps generic "groundcover / shrub / tree" placeholders for
  carpet and drywall, so vanilla map extras (craters, ponds, crashed helicopters) still render
  in Backrooms materials.
- Weather: copied from default, with portal storms blacklisted. Two EOC overrides switch off the
  portal-storm timer and Balthazar progress tracking.

### 1.3 Overmap terrain and location

`overmap_terrain.json`:

- `overmap_location "backfloorchamber"` (and `"backrooms"` in `overmap_specials.json`) is the
  label that specials and start locations use for "a place that is Backrooms floor".
- `overmap_terrain "backfloorchamber"` carries the **monster spawns** and the **map-extras
  collection**. Both are attached to the terrain type, not the region. This matters for new
  levels (section 3).
- `backroom_stairs_bottom` / `_top`: the stairwell squares, placed in pairs by the specials
  `backroom_stairs_z-3` … `backroom_stairs_z2` (15–40 per overmap per floor pair). That's the
  only way between floors.
- Start: `start_location "sloc_backrooms"` on `backfloorchamber`; scenarios `backroom_dweller`
  and `backroom_dweller_starternpc`. A `SCENARIO_BLACKLIST` whitelist hides every other
  scenario.

### 1.4 Fast travel

`teleport.json` adds the Red Room **waycircle** (`f_waycircle`, `TRANSLOCATOR`) and the
**waystone** item, which casts `prim_warp` to any linked waycircle. Red Rooms are map
extras (`mx_teleporter_node`, the most common extra at 5000 weight).

---

## 2. How one hallway chunk gets generated

When you first come near an unvisited `backfloorchamber` square, its mapgen runs once.
Every square is built from scratch like this:

```
 STEP 0  fill_ter            STEP 1  "whole" layout       STEP 2  0–2 BIG rooms      STEP 3  0–3 SMALL rooms
 (24×24)                     (1 of 6, 100%)               (8×8 or 11×11, x,y 0–13)   (4×4 or 6×6, x,y 0–18)

 ........................    ---Q---Q---oo---Q----Q--    ---Q---Q---oo---Q----Q--    ---Q---Q---oo---Q----Q--
 ........................    -        -    -   Q         -        -    -   Q         -        -    -   Q
 ........................    Q        Q    Q     q q     Q   ----?-?----   q q       Q   ----?-?----   q q
 ........................    ---Q---  -    -  ---- -     ---Q-$   n  $ -  ---- -     ---Q-$   n  $ -  ---- -
 ........................    Q   -    -    -     Q -     Q   -    h    -     Q -     Q   -    h    -  ------
 ........................    -   -         ------- --    -   -  J h bB -  ----- --    -   -  J h bB -  -.l,-
 ........................    -                           -   -  J h bB -              -   -  J h bB -  -,,,D
 ........................    -   -----Q---Q------        -   D         d---------     -   D         d--d---
 ........................    ...                         ...                          ...
   all carpet                  hallways + wall segments     a breakroom stamped on top   a micro bathroom too

 Each symbol is then resolved through the PALETTE (backrooms_palette):
   "-"  → t_backroom_wall              "Q" → 100:20 wall vs carpet  (sometimes a gap)
   "o"  → 75:25 carpet vs wall         "O" → carpet, or a door 15%
   "?"  → mostly wall, else a door      "$" / "@" → a nested 2×2 "micro" chunk (loot corner)
 Then the overmap terrain adds:  monsters (40%: 2–15 from GROUP_BACKROOMS)
                                 map extra (30%: one from the "backrooms" collection)
```

The real mapgen (`overmap_terrain.json`) is three `place_nested` layers:

```json
"place_nested": [
  { "chunks": [ [ "mx_backrooms_whole", 100 ] ], "x": 0, "y": 0 },
  { "chunks": [ [ "null", 250 ], [ "mx_backroom_big_common", 100 ], [ "mx_backroom_big_breakroom", 25 ], … ],
    "x": [ 0, 13 ], "y": [ 0, 13 ], "repeat": [ 0, 2 ] },
  { "chunks": [ [ "null", 250 ], [ "mx_backroom_small_common", 100 ], … ],
    "x": [ 0, 18 ], "y": [ 0, 18 ], "repeat": [ 0, 3 ] }
]
```

Why it never looks the same twice:

1. **Weighted picks**: each layer rolls a room type (common > empty/generic > storage/breakroom >
   bathroom), and `null` wins most often, so many squares are just hallways.
2. **Several variants per type**: e.g. 20 different big breakrooms, 86 micro common corners.
3. **Rotation**: room pieces have `"rotation": [0, 3]`, so each lands in one of 4 orientations.
4. **Per-symbol randomness**: `Q`, `o`, `?`, `O` flip between wall, carpet and door, so even the
   same layout opens differently.
5. **`ERASE_ALL_BEFORE_PLACING_TERRAIN`**: each room stamps cleanly over what's under it. Rooms
   can overlap; the README calls the odd leftovers "harmless artifacts".

Piece counts (from `rooms/*.json`):

| Piece | Size | Variants |
|---|---|---|
| `mx_backrooms_whole` (base hallway layout) | 24×24 | 6 |
| `mx_backroom_big_*` (common, generic, storage, breakroom, bathroom, empty, special) | 8×8 or 11×11 | 82 |
| `mx_backroom_small_*` | 4×4 or 6×6 | 51 |
| `mx_backroom_micro_*` (loot corners via palette `$` `@`) | 2×2 | 221 |

---

## 3. Adding Level 1, Level 2 …: three options

### Option A: one **dimension** per level (recommended)

Model: the vanilla **Netherum Labyrinth** (`data/json/region_settings/region_settings/dimensions/`,
`data/json/effects_on_condition/nether_eocs/labyrinth_effect_on_condition.json`).

```
dimension "default"  = Level 0 (existing mod, unchanged)
dimension "backrooms_level_1" → region_layout → region_settings "backrooms_level_1"
                                  default_oter: level-1 filler on its own floors
                                  whitelist: BACKROOMS_L1 specials only
```

Getting there is an EOC on a rare transition structure (an exit door, a "no-clip" wall, a
hole in the floor). The Labyrinth does it in three moves:

1. `u_location_variable` saves where you were (for the way back).
2. `u_travel_to_dimension: "backrooms_level_1"` (`npc_travel_radius` + `npc_travel_filter:
   "follower"` bring your followers along).
3. `u_location_variable` with `target_params: { om_terrain, om_special }` finds the level's
   arrival structure, then `u_teleport` drops you there.

| Pros | Cons |
|---|---|
| Each level gets its **own** filler, specials whitelist, map extras, weather, z-range. | Only the current dimension is loaded. NPCs left on another level are **frozen** until you come back. Off-screen NPCs barely simulate anyway, so in practice this isn't a new limit. |
| Level 0 stays byte-for-byte TGWeaver's mod; our levels can't break it. | Every level needs a region_settings block (~60 lines to copy). |
| Fits Backrooms lore: you *no-clip* into a level, you don't walk there. | Travel is EOC-scripted; we must write the "way back" ourselves. |
| All JSON, so it works on official builds. | |

### Option B: z-level bands inside the same world

`default_oter` is per floor, so Level 1 could fill z −10 … −5 with its own `backrooms_l1_chamber`
and keep a solid band at −4 between the levels. A rare "hole in the floor" special drops you down.

| Pros | Cons |
|---|---|
| One world, so NPCs on every level live in the same save. | Only 21 floors total; 7 are already used, so there's room for 2 or 3 more levels at most. |
| Monsters, loot and map extras still differ per level (they hang off the terrain type). | Weather, region swaps and the specials whitelist are **shared** with Level 0. |
| No EOC travel code. | It means editing the mod's `default_oter`, i.e. **overriding TGWeaver's region settings**, which is fragile when upstream changes it. |

### Option C: random regions in the same world (`generation_mode: "RANDOM"`)

Each 180×180-square overmap picks a region at random, so Level 1 appears as large patches next
to Level 0. Easy, but the borders are abrupt, you "walk" between levels instead of no-clipping,
and it also means overriding Level 0's region layout. Not recommended.

### Recommendation

**Option A** for real levels (Level 1 parking garage, Level 2 pipe dreams …), entered through rare
transition structures. Inside a level, use extra **floors** (like Level 0's 7) and rare
**overmap specials** for variety. Level 0 stays untouched; everything new lives in
`Backrooms_Expanded`.

Next steps in the playbook:

- **1.3**: first level as a dimension: terrain, furniture, mapgen pieces, loot group, monster group,
  arrival structure, transition structure on Level 0 (flag `BACKROOMS` so Level 0's whitelist
  allows it), and the travel EOC with a way back.
- **1.4**: structures are fixed `overmap_special`s with hand-drawn mapgen; on Level 0 they need
  the `BACKROOMS` flag and `"locations": [ "backrooms" ]`.
- **1.5**: Level 0 already has `f_watercooler` in its palette. Check whether NPC thirst
  executors recognize it before inventing new water furniture (`NOTES-npc-architecture.md`, 1.3).

---

## 4. File map (in-game Backrooms mod)

| What | File |
|---|---|
| Takes over the default dimension | `region_settings/dimensions.json` |
| World rules (no rivers/cities, floor filler, whitelist, extras, weather) | `region_settings/region_settings/regional_map_settings.json` |
| Chamber terrain, monster spawns, base mapgen | `overmap_terrain.json` |
| Stairwell specials between floors | `overmap_specials.json`, `rooms/overmap_special_stairwells.json` |
| Room pieces | `rooms/rooms_*.json` |
| Symbol → terrain/furniture/loot | `palettes.json` |
| Custom terrain/furniture (carpet, drywall …) | `furniture_terrain.json` |
| Loot | `itemgroups.json` |
| Monsters | `monstergroups.json` |
| Map extras (Red Rooms, autodoc rooms, nests …) | `map_extras.json`, `rooms/rooms_backrooms_map_extra_overrides.json`, `rooms/rooms_special.json` |
| Red Room teleport network | `teleport.json` |
| Scenarios and start location | `scenarios.json` |
