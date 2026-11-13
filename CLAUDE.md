# CLAUDE.md: CDDA Backrooms + Autonomous NPCs

This is Xbro10's fork of Cataclysm: Dark Days Ahead (CDDA). The project plan is the
playbook PDF at
`C:\Users\xande\Documents\CDDA documents for modding\CDDA Backrooms + Autonomous NPCs — Claude Code Playbook.pdf`
(Phases 0–4 plus testing/publishing prompts). Map every request to a playbook prompt.

## The project's parts

| Part | Where | What |
|---|---|---|
| **Backrooms Expanded** | `data/mods/Backrooms_Expanded/` (id `backrooms_expanded`) | Add-on to the in-game Backrooms mod (`data/mods/Backrooms`, id `backrooms`): new levels, structures, resources. Playbook Phase 1. |
| **Survivor Minds** | `data/mods/Survivor_Minds/` (id `survivor_minds`) | NPC autonomy mod in JSON/EOC: journals, wall marks, schedules, personality, gossip. Must work with or without the Backrooms. Playbook Phase 2. |
| **C++ NPC brain** | `src/`, branch `npc-autonomy` (not created yet) | Phase 3. Everything behind a game option "NPC autonomy (experimental)", default **off**. |

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
- Release tags are per mod: `survivor-minds-vX.Y.Z`, `backrooms-expanded-vX.Y.Z`.

## Fork changes to the game itself (watch these during upstream merges)

Search for `Xbro10 fork` in `src/` to find every C++ change.

| What | Files | Why |
|---|---|---|
| Character creator crash fix: past achievements are loaded before the first ImGui frame | `src/newcharacter.cpp` (`character_creator_ui::display`) | Their "please wait" popup redrew mid-frame and crashed in `GetCursorScreenPos`. Drop it if upstream fixes this. |
| Legacy sandbox world options restored (branch `sandbox-options`) | `src/options.cpp` (world option groups), `data/core/external_options.json` (7 entries set to `"stub": true`) | Upstream hid them or made them JSON-only (#79310, #80640). |
| City size / spacing / NPC spawn time as % world options | `src/regional_settings.cpp` (`region_settings_city::finalize`), `src/game.cpp` (`perhaps_add_random_npc`), `src/newcharacter.cpp` (`cities_enabled`) | Upstream moved them into region settings; ours scale the region's value. New ids `CITY_SIZE_SCALING`, `CITY_SPACING_SCALING`, `NPC_SPAWNTIME_SCALING`. |

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
