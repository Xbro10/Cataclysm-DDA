# NPC & Mapgen Architecture Notes

Written 2026-11-12 against the CDDA experimental code in this fork (commit `a7303e0`).
Playbook Prompt 0.3. Plain-language map of how NPCs think and how the world is built,
so later phases start from facts instead of guesses.

> **Headline finding:** the current game already has a big chunk of Phase 3 built in.
> NPC decisions run through a **behavior tree with utility scoring**, defined in JSON
> (`data/json/npcs/npc_behavior.json`), and NPCs already pursue hunger, thirst, sleep and
> warmth on their own. Phase 3 should **extend this system**, not build a second brain
> next to it. Details in section 1.3 and section 4.

---

## 1. How an NPC decides what to do each turn

### 1.1 Where it starts

Every turn, each active NPC runs `npc::process_turn()` (`src/npc.cpp:3694`), which calls
**`npc::move()`** (`src/npcmove.cpp:1593`). `move()` picks one **`npc_action`**
(attack, flee, follow, pickup, sleep, pause, etc.) and hands it to
**`npc::execute_action()`** (`src/npcmove.cpp:2174`), which actually does it.

### 1.2 The decision order inside `npc::move()`

Read it top to bottom. **The first thing that matches wins.**

1. **Housekeeping.** Fleeing timers, sleep bookkeeping, then `regen_ai_cache()` (line 1441)
   rebuilds the NPC's view of the world: visible enemies, danger level, sounds, its target.
2. **Locked-in activities.** Surgery, spellcasting and closing a door finish without
   re-thinking (lines 1612–1621). NPCs with the `NPC_STASIS` trait do nothing.
3. **`act_on_danger_assessment()`** (line 1321). Decides whether things are scary enough to run.
4. **Self-care in progress.** If the NPC is already eating or sleeping, it keeps going unless
   it's in harm's way (lines 1639–1651).
5. **Turning hostile.** If its faction hates you, a follower mutinies and others pick
   KILL or FLEE based on `fear` vs `aggression + bravery` (lines 1681–1694).
6. **Emergencies, hard-coded in C++** (lines 1715–1791), in this order:
   - Standing in fire or a dangerous field → step away.
   - Explosives nearby → `npc_escape_explosion`.
   - Fleeing → `method_of_fleeing()`.
   - Asthma attack with an inhaler → `npc_heal`.
   - Has a target and danger > 0 → `method_of_attack()`.
   - Heard a sound → `npc_investigate_sound` (skipped by the `IGNORE_SOUND` trait).
7. **No emergency → the behavior tree** (lines 1792–2029). See 1.3. This is where needs,
   following, guard duty and camp work get decided.
8. **Fallbacks** (lines 2031–2111): continue a player-assigned activity, do camp work, pick up
   items (`find_item()`, line 4308), and finally `long_term_goal_action()` (line 3382), which
   is what wandering non-follower NPCs do.
9. **Followers defend you.** An idle follower near you attacks visible threats (lines 2118–2148).
10. `execute_action( action )`.

### 1.3 The behavior tree (already in the game)

Defined in **`data/json/npcs/npc_behavior.json`** (201 lines). Simplified:

```
npc_decision            (fallback = first child that applies wins)
├── npc_combat          if npc_in_danger → flee or fight
├── npc_investigate     if npc_has_sound_alerts
└── npc_priorities      (utility = highest score wins)
    ├── npc_needs       (utility)
    │   ├── npc_homeostasis   score: npc_temperature_urgency → regulate_temperature
    │   ├── npc_thirst        score: npc_thirst_urgency      → drink_water (have it, or go find it)
    │   ├── npc_hunger        score: npc_hunger_urgency      → eat_food   (have it, or forage)
    │   └── npc_sleepiness    score: npc_sleepiness_urgency  → go_to_sleep
    ├── npc_duty            guard posts / shifts
    ├── npc_follow_embarked / npc_follow / npc_player_order
    ├── npc_camp_work / npc_return_to_camp / npc_free_time
```

Real JSON node:

```json
{
  "type": "behavior",
  "id": "npc_thirst",
  "strategy": "sequential",
  "conditions": [ { "predicate": "npc_needs_water_badly" } ],
  "children": [ "npc_obtain_water" ],
  "score": "npc_thirst_urgency"
}
```

How the pieces split between JSON and C++:

| Piece | Where it lives | Editable from JSON? |
|---|---|---|
| Tree shape, ordering, strategy (`fallback`, `sequential`, `utility`) | `npc_behavior.json` | **Yes** |
| **Predicates** (`npc_needs_water_badly`, `npc_has_food`, …) | registered in `src/behavior_oracle.cpp:41-65` | No. New ones need C++. |
| **Scores** (`npc_thirst_urgency`, …) | `src/behavior_oracle.cpp:76-86`, code in `src/character_oracle.cpp` | No. The formulas are C++. |
| **Goals** (`"drink_water"`, `"camp_work"`, …) | dispatched by string in `npc::move()` lines 1909–2005 | No. A new goal needs C++. |

Example score (`src/character_oracle.cpp:255`): thirst urgency = `thirst / 1200`, clamped
to 0–1, where 1200 is the dehydration-death point. Hunger scales the same way, toward
starvation. Those scores are numbers on a common 0–1 scale, which is what the utility
strategy compares.

**Goal commitment.** Once an NPC adopts a goal (say `eat_food`), it sticks with it until the
goal is satisfied, proves impossible, or something more urgent interrupts
(`should_interrupt()`, lines 1819–1906). This stops NPCs from flip-flopping every turn.

**Need executors and plans.** Eat, drink, sleep and warm-up goals run through
`execute_need_goal()`. Each keeps a small **plan** in `npc.h:623` (`need_plan`: goal, source
kind such as `ground_item`, `harvestable`, `water_terrain`, `camp_water` or `sleep_spot`,
plus target tile and progress), and a **failed-targets** set so the NPC doesn't retry a bad
spot. These are short-term and not a long-term memory.

**Debugging it.** Turn on debug mode with the `DF_NPC_NEEDS` filter. The log prints
`BT needs goal = …`, the chosen target and the executor result each turn (lines 1664–1677, 2007–2021).

### 1.4 What an NPC "remembers" today

- **Opinion of the player:** `npc_opinion` (`src/npc_opinion.h`): trust, fear, value, anger,
  owed, sold. Saved with the game.
- **Personality:** aggression, bravery, collector, altruism (−10..10).
- **Short-term caches:** combat memory (`npc_combat_memory_cache`, not saved), need plans,
  failed targets.
- **Missing:** place memory ("vending machine 3 rooms east"), event memory, or
  NPC-to-NPC opinions. **That gap is exactly Phase 3.5/3.6.**

---

## 2. What you can change from JSON and what needs C++

### 2.1 JSON (works on official builds, Phase 2)

| System | Files / docs | What you can do |
|---|---|---|
| **NPC classes** | `data/json/npcs/classes.json`, `NC_*.json`; doc `doc/JSON/NPCs.md` | Skills, traits, gear, personality bonuses (`bonus_aggression`, `bonus_altruism` …), shop inventories. |
| **Dialogue (`talk_topic`)** | `data/json/npcs/TALK_*.json`, `common_chat/` | Conversations, conditions, effects. "Can I read your journal?" goes here. |
| **Effect-on-conditions (EOCs)** | `doc/JSON/EFFECT_ON_CONDITION.md`, `data/json/effects_on_condition/` | Timers and event hooks: see 2.2. |
| **Missions** | doc `MISSIONS_JSON.md`, `FACTION_MISSIONS.md` | Quests NPCs give. |
| **Factions** | doc `FACTIONS.md` | Faction relations; EOC `u_set_fac_relation` / `npc_change_faction`. |
| **Behavior tree layout** | `data/json/npcs/npc_behavior.json` | Reorder priorities, remove or regroup nodes, reuse existing predicates, scores and goals. |
| **NPC rules** | EOC `set_npc_rule`, `set_npc_engagement_rule`, `set_npc_aim_rule` | Toggle follower behaviors (e.g. "don't pick up items"). |

### 2.2 EOC tools most useful for the NPC mod

- **Timers:** `"eoc_type": "RECURRING"` with `recurrence`. Add `"global": true, "run_for_npcs": true`
  to run it on **every NPC**, not just you. That's the basis of the morning journal (Prompt 2.1).
- **Event hooks:** `"eoc_type": "EVENT"` with `required_event`, e.g. `character_kills_character`,
  `character_dies`, `character_eats_item`, `character_wakes_up`, `character_finished_activity`
  (full list in `src/event.h`). That's the basis of event journal entries (Prompt 2.2).
- **`NPC_DEATH`** EOC type. Fires when an NPC dies (friend-died journal entries, mourning).
- **Variables:** `u_add_var` / `set_string_var` and `math` with `u_`/`n_` prefixes store
  per-NPC data. That's how journal lines and simple "memories" get stored.
- **Opinion math:** `n_npc_trust()`, `n_npc_fear()`, `n_npc_anger()`, `n_npc_value()`. Read and
  write (doc `NPCs.md:1474`). That's the basis of moral reactions (Prompt 2.5).
- **Location memory (lite):** `u_location_variable` / `npc_location_variable` save a tile
  position into a variable, e.g. "water source here".
- **Make an NPC do something:** `npc_assign_activity`, `npc_set_goal` (send to an overmap
  location), `npc_set_guard_pos`, `npc_message`.

### 2.3 C++ only (Phase 3)

- The emergency cascade in `npc::move()` (fire, explosives, flee, attack).
- New **predicates**, **scores** and **goals** for the behavior tree (see 1.3 table).
- Need executors: *how* an NPC finds and walks to food or water (`execute_need_goal`,
  `consume_food`, `find_item`).
- Any saved data structure: place memory, event memory, NPC-to-NPC relationships.
- Multi-step planning (GOAP) beyond what tree sequences can express.
- Adding the "NPC autonomy (experimental)" game option.

**Bottom line for Phase 2.4 ("simple schedules"):** EOCs can *nudge* (set vars, assign an
activity, send messages, set guard positions), but the moment-to-moment choice is the
behavior tree's. Sleep, eat and drink are already handled by it, so Phase 2.4 is mostly
idle mood lines and morning routines, not rebuilding sleep logic.

---

## 3. How the world is built: overmap, mapgen, overmap specials

### 3.1 The layers

```
dimension                 "which world am I in"     dimensions.json
  └─ dimension_region_layout                       dimension_regions.json
       └─ region_settings  what fills empty space,  netherum_labyrinth.json
            │               cities on/off, roads,
            │               which specials may spawn
            ├─ overmap_terrain (OMT)  one 24×24-tile   overmap_terrain_*.json
            │    square on the world map
            ├─ overmap_special  groups of OMTs placed  overmap_special/*.json
            │    as a unit (fixed or "mutable")
            └─ mapgen           what tiles go inside   mapgen/**/*.json
                 one OMT, generated on first visit
```

Every **overmap terrain** id needs at least one **mapgen** whose `om_terrain` matches it.
Mapgen only runs when you first get close, so the world map can be huge but cheap.

### 3.2 Dimensions: the clean way to do Backrooms levels

The game now supports **separate dimensions**, each with its own world generation. The
in-game **Netherum Labyrinth** already works this way and is the closest model to the
Backrooms:

- `data/json/region_settings/region_settings/dimensions/dimensions.json` registers e.g.
  `{ "type": "dimension", "id": "netherum_labyrinth", "region_layout": "netherum_labyrinth" }`.
- `netherum_labyrinth.json` (region settings) turns off rivers, forests, cities and roads,
  fills the world with `deep_rock` plus a filler terrain, and whitelists only specials flagged
  `NETHERUM_LABYRINTH`.
- The player gets there with the EOC effect **`u_travel_to_dimension`**
  (`data/json/effects_on_condition/nether_eocs/labyrinth_effect_on_condition.json`), which can
  bring followers along (`npc_travel_radius`, `npc_travel_filter: "follower"`).

**Proposal for Phase 1.2/1.3:** make each Backrooms level its own dimension
(`backrooms_level_0`, `backrooms_level_1`, …) and use structures with an EOC as the
"no-clip" transitions between them. This is better than the 2021 mod's approach (replacing
the normal world), because Level 0 and Level 1 can each have their own filler, specials and
rarity. It's also all JSON, so it works on official builds. **Check this against the
original mod in Prompt 1.2 before committing to it.**

### 3.3 Mapgen: two styles

**Hand-drawn rows + palette**: `data/json/mapgen/netherum/labyrinth2_overgrown_labyrinth.json`:

```json
{
  "type": "mapgen",
  "om_terrain": [ "labyrinth_overgrown_entrance" ],
  "object": {
    "fill_ter": "t_nl_metal_floor",
    "rows": [ "vvvvvvvvv#    #vvvvvvvvv", "...24 rows of 24 chars..." ],
    "palettes": [ "labyrinthine_overgrown_palette" ],
    "place_nested": [ { "chunks": [ "labyrinth_overgrown_9x9_cave_nw" ], "x": 0, "y": 0 } ]
  }
}
```

Each character in `rows` maps, through the palette, to terrain, furniture, items or monsters.
This is the format for Prompt 1.4 (survivor camp): draw it in ASCII first, then write it.

**Random assembly from nested chunks**: `data/json/mapgen/netherum/labyrinth2_generic.json`:

```json
"place_nested": [
  { "chunks": [ [ "labyrinth2_backdrop", 100 ] ], "x": 0, "y": 0 },
  { "chunks": [ [ "null", 500 ], [ "labyrinth2_hall", 250 ] ], "x": 0, "y": 0 },
  { "chunks": [ [ "null", 250 ], [ "labyrinth2_small_room", 250 ] ],
    "x": [ 0, 18 ], "y": [ 0, 18 ], "repeat": [ 0, 3 ] },
  { "chunks": [ [ "null", 49 ], [ "labyrinth2_treasure_room", 1 ] ], "x": 0, "y": 0 }
]
```

Weighted chunks dropped at random spots give endless non-repeating rooms, which fits
Backrooms hallways well. The `[ "null", 49 ], [ "...", 1 ]` pattern means a 2% chance.

### 3.4 Overmap specials

`data/json/overmap/overmap_special/netherum_labyrinth.json`, `labyrinthine_structure`:

- `"subtype": "mutable"`: grows a structure piece by piece, joining pieces through named
  `joins` (`"passage_to_passage"`), using pieces like `crossroads`, `tee`, `corner` and `dead_end`.
  That's how you get a maze of connected halls across many OMTs.
- `"locations"`: which overmap terrain it may be placed on.
- `"occurrences": [ 99, 100 ]`: how many per overmap.
- `"flags"`: e.g. `NETHERUM_LABYRINTH`, which the region's whitelist matches.

A **fixed** special (the default subtype) lists exact OMT positions instead. That's the
right fit for a single structure like the breakroom camp.

---

## 4. What this means for the playbook

| Playbook step | Adjusted plan |
|---|---|
| 1.2 / 1.3 Levels | Strongly consider **one dimension per level**, modeled on the Netherum Labyrinth. |
| 1.4 Structures | Fixed `overmap_special` + hand-drawn mapgen with a palette. |
| 1.5 Water and vending | Existing need executors already look for `water_terrain` and `ground_item` sources, so Backrooms sinks and coolers should use existing water flags so NPCs find them automatically. |
| 2.1 / 2.2 Journal | Global recurring EOC with `run_for_npcs`, plus EVENT EOCs, with lines stored in NPC variables. |
| 2.4 Schedules | Sleep, eat and drink are already autonomous. Focus on mood lines and routines. |
| 3.2 Needs | Hunger, thirst, sleep and warmth already exist as scores. Add **safety** and **social** as new scores in `character_oracle.cpp` instead of a parallel needs system. |
| 3.3 Utility AI | Already exists (`"strategy": "utility"`). Add new nodes to `npc_behavior.json` and new goals in `npc::move()`. Tunable **weights** in JSON are a real gap: today the scores are fixed formulas. |
| 3.4 GOAP | Tree `sequential` nodes cover simple chains. A true planner is new C++. |
| 3.5 / 3.6 Memory and opinions | Not present. This is the biggest genuinely new C++ work. |
| 3.1 Option toggle | Gate your new tree nodes and predicates behind the option, so vanilla behavior stays untouched when it's off. |

---

## 5. Key files cheat sheet

| What | File |
|---|---|
| Turn loop | `src/npc.cpp` → `npc::process_turn()` |
| Decision logic | `src/npcmove.cpp` → `npc::move()`, `execute_action()`, `address_needs()` |
| Behavior tree data | `data/json/npcs/npc_behavior.json` |
| Tree engine | `src/behavior.cpp`, `src/behavior_strategy.cpp` |
| Predicates and scores registry | `src/behavior_oracle.cpp` |
| Score formulas | `src/character_oracle.cpp` |
| NPC data (plans, opinion, personality) | `src/npc.h`, `src/npc_opinion.h` |
| Dialogue engine | `src/npctalk.cpp` |
| EOC engine | `src/effect_on_condition.cpp` |
| Mapgen engine | `src/mapgen.cpp` |
| Overmap engine | `src/overmap.cpp` |
| Docs | `doc/JSON/NPCs.md`, `EFFECT_ON_CONDITION.md`, `MAPGEN.md`, `OVERMAP.md`, `REGION_SETTINGS.md`, `FACTIONS.md`, `MISSIONS_JSON.md` |
| Model for Backrooms | `data/json/region_settings/region_settings/dimensions/`, `data/json/mapgen/netherum/`, `data/json/overmap/overmap_special/netherum_labyrinth.json` |
