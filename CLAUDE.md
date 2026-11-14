# CLAUDE.md: CDDA Backrooms + Autonomous NPCs

This is Xbro10's fork of Cataclysm: Dark Days Ahead (CDDA). The project plan is the
playbook PDF at
`C:\Users\xande\Documents\CDDA documents for modding\CDDA Backrooms + Autonomous NPCs — Claude Code Playbook.pdf`
(Phases 0–4 plus testing/publishing prompts). Map every request to a playbook prompt.

**If you're not Xbro10** (a tester or contributor), read `FORK-CONTRIBUTING.md` first: the paths,
playbook and launcher below are on Xbro10's PC. `FORK-CHANGELOG.md` lists everything made so far;
every change adds a line at the top of its "New entries" as well as to its mod's `CHANGELOG.md`.

## The project's parts

| Part | Where | What |
|---|---|---|
| **Backrooms Expanded** | `data/mods/Backrooms_Expanded/` (id `backrooms_expanded`) | Add-on to the in-game Backrooms mod (`data/mods/Backrooms`, id `backrooms`): new levels, structures, resources. Playbook Phase 1. |
| **Survivor Minds** | `data/mods/Survivor_Minds/` (id `survivor_minds`) | NPC autonomy mod in JSON/EOC: journals, wall marks, schedules, personality, gossip. Must work with or without the Backrooms. Playbook Phase 2. |
| **C++ NPC brain** | `src/`, branch `npc-autonomy` (created 2026-11-13 from `dev`) | Phase 3. Everything behind a game option "NPC autonomy (experimental)", default **off**. |

- Don't edit `data/mods/Backrooms/` directly. It's upstream's copy (TGWeaver's mod, maintained in CDDA).
  Override or extend it from `Backrooms_Expanded` so upstream merges stay conflict-free.
  The original 2021 repo (github.com/TGWeaver/Backrooms-CDDA-Mod) has no license file, which is why we build on the in-game copy (CC-BY-SA 3.0).
- Always credit **TGWeaver** for the original Backrooms mod in the README and changelog.
- Read `NOTES-npc-architecture.md` before NPC or mapgen work. Key facts: NPCs already run a JSON
  behavior tree with utility scoring (`data/json/npcs/npc_behavior.json`). Phase 3 extends it
  rather than building a parallel brain. Backrooms levels are planned as separate **dimensions**
  modeled on the Netherum Labyrinth.

## Rules (from the user)

1. **Plan first.** Explain the plan and which files you'll touch, then wait for OK before any large change.
2. **Small, testable steps.** Each step ends with something visible in-game plus a test checklist.
3. **Commit after each working step.**
4. **UltiCa tileset.** Every new terrain, furniture, item and monster ID needs a `looks_like` fallback (or an UltiCa sprite id).
5. **Target current CDDA experimental.** JSON must pass the game's loader (see "Checking mods").
6. **Changelog.** Every change adds a line under `## Unreleased` in that mod's `CHANGELOG.md`
   (Added / Changed / Fixed / Removed). Every release turns it into a dated entry before publishing.
7. Ask "what can't this do?" for EOC work, and say early when something needs C++.

## Git layout

- `origin` = github.com/Xbro10/Cataclysm-DDA, `upstream` = CleverRaven/Cataclysm-DDA.
- `master` tracks upstream. Mod work lives on branch **`dev`**. C++ work goes on **`npc-autonomy`** (branched from `dev`).
- Sync with upstream (playbook T.3): update `master` from `upstream/master`, then merge `master` into `dev`.
- **`testing`** is the branch outside testers build from (created 2026-11-13). It only moves when the user asks
  ("push to testing"): merge the latest `npc-autonomy` (which already holds `dev`) into it, then push it. Never commit straight to it.
- Release tags are per mod: `survivor-minds-vX.Y.Z`, `backrooms-expanded-vX.Y.Z`.

## Fork changes to the game itself (watch these during upstream merges)

Search for `Xbro10 fork` in `src/` to find every C++ change.

| What | Files | Why |
|---|---|---|
| Character creator crash fix: past achievements are loaded before the first ImGui frame | `src/newcharacter.cpp` (`character_creator_ui::display`) | Their "please wait" popup redrew mid-frame and crashed in `GetCursorScreenPos`. Drop it if upstream fixes this. |
| Legacy sandbox world options restored (branch `sandbox-options`) | `src/options.cpp` (world option groups), `data/core/external_options.json` (7 entries set to `"stub": true`) | Upstream hid them or made them JSON-only (#79310, #80640). |
| City size / spacing / NPC spawn time as % world options | `src/regional_settings.cpp` (`region_settings_city::finalize`), `src/game.cpp` (`perhaps_add_random_npc`), `src/newcharacter.cpp` (`cities_enabled`) | Upstream moved them into region settings; ours scale the region's value. New ids `CITY_SIZE_SCALING`, `CITY_SPACING_SCALING`, `NPC_SPAWNTIME_SCALING`. |
| Light-aware monster flags `AVOIDS_LIGHT` (won't step from dark into light) and `SEEKS_LIGHT` (heads for the brightest light in view) | `src/monmove.cpp` (`will_move_to`, end of `monster::plan`, helpers near the top) | Backrooms Expanded's Smiler and Deathmoth. The mod defines the flags; the C++ checks them lazily, so official builds load the mod and just skip the behaviour. A candidate for an upstream PR (then the flags move to `data/json/monsters/monster_flags.json`). |

| Debug "Spawn NPC follower" adds the NPC to the follower list | `src/debug_menu.cpp` (`spawn_npc_follower`) | Upstream bug: spawned followers were missing from the faction screen and radio contact until a reload. A candidate for an upstream PR. |
| **NPC autonomy (Phase 3, branch `npc-autonomy`)**: game option `NPC_AUTONOMY` (General > NPC options, default off), behavior-tree predicate `npc_autonomy_on`, quests cancelled when a follower leaves, needs 0–100 (food, water, sleep, warmth, safety, social) stored as NPC vars `autonomy_need_*` and shown in the look-around panel, scores `npc_safety_urgency` / `npc_social_urgency`, mealtimes and socializing (goals `autonomy_meal` / `autonomy_socialize`, weights in `data/json/npcs/npc_autonomy.json`) | New `src/npc_autonomy.{h,cpp}`, `tests/npc_autonomy_test.cpp`, `data/json/npcs/npc_autonomy.json`; hooks in `src/options.cpp`, `src/behavior_oracle.cpp`, `src/character_oracle.{h,cpp}` (`autonomy_*`, `safety_urgency`, `social_urgency`), `src/npctalk_funcs.cpp` (`talk_function::leave`), `src/npc.cpp` (`npc_update_body`, `print_info`), `src/npcmove.cpp` (`npc::move` goal dispatch and commitment, `bt_goal_to_category`); `data/json/npcs/npc_behavior.json` (two children of `npc_priorities`) | Phase 3 logic lives in `npc_autonomy.*`; base files only get small hooks. With the option off, NPCs behave like upstream. |

Not restored on purpose: wandering hordes, character point pools, default region, initial
visibility distance, 3D FOV range.

## Build and run (Windows)

- Visual Studio 2022 + vcpkg at `D:\dev\vcpkg`. Solution: `msvc-full-features\Cataclysm-vcpkg-static.sln`, Release x64.
  Command-line build (game only; use target `Cataclysm-test-vcpkg-static` for tests):
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Cataclysm-vcpkg-static.sln /t:Cataclysm-vcpkg-static /p:Configuration=Release /p:Platform=x64 /m /v:minimal`
- Outputs in the repo root: `cataclysm-tiles.exe` (game), `Cataclysm-test-vcpkg-static-Release-x64.exe` (tests).
- This build's saves and config live in `save\` and `config\` (git-ignored).
- The official build (for pre-release checks) is installed by the launcher at
  `D:\Games\CDDA MY FORK\` (`Start CDDA Launcher.bat`, game in `Game\`, user data in `UserData\`).
  Its "Sync my mods from my fork" copies our mods into `UserData\mods`.

## Checking mods

Run from the repo root. Exit code 0 means it loaded clean:

```
./cataclysm-tiles.exe --check-mods survivor_minds
./cataclysm-tiles.exe --check-mods backrooms backrooms_expanded
```

This loads the base game plus the listed mods and reports JSON errors, then exits.
**The exit code can be 0 even when loading failed** ("Error loading data: …" is printed to stdout),
so always read the command's output as well as the exit code.
The details (including mapgen overlap errors) are in `config/debug.log`; grep it for `ERROR` after a run.
`WARN_ON_MODIFIED` is set to false in this build's `config/options.json`, so editing mod files
doesn't trigger "stale game data" errors.

## Gotchas learned so far

- `predecessor_mapgen` needs the full overmap terrain id, including the rotation suffix for
  rotatable terrain (`backfloorchamber_north`, not `backfloorchamber`).
- Sibling `place_nested` chunks that may overlap need `ERASE_FURNITURE_BEFORE_PLACING_TERRAIN`
  (or `ERASE_ALL_BEFORE_PLACING_TERRAIN`) or the loader reports an error.
- In nested mapgen, a space in `rows` means "leave what's underneath", as long as the palette doesn't define `" "`.
- Furniture `looks_like` should point at furniture, terrain at terrain. Check UltiCa coverage with
  `grep '"<id>"' "D:\Games\CDDA MY FORK\Game\gfx\UltimateCataclysm\tile_config.json"`.
- UltiCa (`gfx/UltimateCataclysm`) is **not** in this source checkout; it only ships with release builds.
- In an EOC run "as" an NPC (`run_for_npcs`, `u_run_npc_eocs`, event EOCs for NPCs), `u_message` is silently dropped.
  Use the effect `"message"` instead (it always shows to the player; `<u_name>` still means the NPC).
  A `"global": true` field on `u_message` does nothing.
- Math has no `&&` / `||`: use `{ "and": [ … ] }` / `{ "or": [ … ] }` conditions.
- A `mod_tileset`'s `"file"` is relative to the mod's root folder, not the JSON's folder (`gfx/br1_overmap.png`).
