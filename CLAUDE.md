# CLAUDE.md: CDDA Backrooms + Autonomous NPCs

This is Xbro10's fork of Cataclysm: Dark Days Ahead (CDDA). The project plan is the
playbook PDF at
`C:\Users\xande\Documents\CDDA documents for modding\CDDA Backrooms + Autonomous NPCs — Claude Code Playbook.pdf`
(Phases 0–7 plus testing/publishing prompts; the 2026-11-16 version, the older one is kept beside it
as "(old, 2026-11-12)"). Map every request to a playbook prompt.

## Playbook progress

As of 2026-11-16. Update this each time a prompt is finished.
- Phases 0, 1 and 2: finished (Prompts 0.1 to 2.6). Don't redo, re-plan or re-verify them.
- Phase 3: 3.1, 3.2, 3.3 finished (3.3 tested in game 2026-11-16). **3.4 built in full**: 3.4a (get water) and 3.4b
  (get food) tested in game (2026-11-16); 3.4c (get warm) built 2026-11-16, waiting for the in-game test.
  Then 3.4 is finished. The planned "stocking up" step became Prompt 7.0 (stock count) in the new playbook.
- Fixes already made ahead of Phase 5: part of 5.3 (a follower drops a meal when left behind, guards eat
  on their post, water trips use upstream's follow and guard reach). Still open in 5.3: chats.
- Next after 3.4: Phase 5 in order (5.1 first), then Phase 6 (6.0 first), then 3.6, then Phase 7.
  Skip 3.5 and 4.1 (covered by 5.6), 4.5 (5.8 step B), 3.7 and 4.7 (Phase 7). 6.6 starts 4.6 (scouts).
- The playbook's `CHANGELOG-fork.md` is this repo's `FORK-CHANGELOG.md`.
- Phase 6 rule: stance, anchor, leash, wants and trips are saved NPC variables, not new enum entries.
  The follower rule `ally_rule::no_self_care` (added 2026-11-15) predates this rule.

**If you're not Xbro10** (a tester or contributor), read `FORK-CONTRIBUTING.md` first: the paths,
playbook and launcher below are on Xbro10's PC. `FORK-CHANGELOG.md` lists everything made so far;
every change adds a line under today's date there, in the right section (Backrooms, NPCs, Launcher, Bug fixes,
For developers; see "How to add an entry" at its end), as well as to its mod's `CHANGELOG.md`.

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
  ("push to testing"): merge the latest `npc-autonomy` (which already holds `dev`) into it, then push it. Never commit straight to it,
  except `TESTER-GUIDE.md` (the testers' guide), which lives only on `testing`; keep its "What's in this build" current when testing moves.
  The user's own files live outside git on purpose, in `D:\dev\Important Files\`: `TEAM-MANAGEMENT-GUIDE.txt`,
  `GABRIEL-SUMMARY.txt`, a read-only copy of `TESTER-GUIDE.md` (refresh it when testing's guide changes), and the in-game
  checklists `TESTING-CHECKLIST-BACKROOMS.md` (levels work) and `TESTING-CHECKLIST-NPCS.md` (NPC brain and Survivor Minds).
  Add each finished step's test items to the matching checklist.
- **Tester releases** (first: `testing-build-2026-11-14`): when testing moves and the user wants a build, make sure
  `src/` matches `origin/testing` and build; stage `git archive origin/testing data gfx doc README.md LICENSE*.txt TESTER-GUIDE.md`,
  plus the compiled `data/shaders/*.dxil|*.spv`, `gfx/UltimateCataclysm`, `gfx/MSX++UnDeadPeopleEdition` (git-ignored here),
  the exe and a `VERSION.txt`; check-mods it from the staged folder (create `config/` first), then delete first-run folders (including `data/cache`) and zip
  to `D:\dev\releases\`. Publish as a pre-release with tag `testing-build-YYYY-MM-DD` (`-2`, `-3` for more that day) targeting `testing`, with one asset named `CDDA-Xbro10-testing-*.zip`: the launcher looks for exactly that. The built-in browser can't
  attach files, so the user drags the zip in.
- A ruleset (PR required, no force-push or delete) covers `master`, `dev`, `npc-autonomy`, `testing`; Xbro10 bypasses it,
  so pushes still work and GitHub prints a "must be made through a pull request" note.
- Release tags are per mod: `survivor-minds-vX.Y.Z`, `backrooms-expanded-vX.Y.Z`.

## Fork changes to the game itself (watch these during upstream merges)

Search for `Xbro10 fork` in `src/` to find every C++ change.

| What | Files | Why |
|---|---|---|
| Character creator crash fix: past achievements are loaded before the first ImGui frame | `src/newcharacter.cpp` (`character_creator_ui::display`) | Their "please wait" popup redrew mid-frame and crashed in `GetCursorScreenPos`. Drop it if upstream fixes this. |
| Legacy sandbox world options restored (branch `sandbox-options`) | `src/options.cpp` (world option groups), `data/core/external_options.json` (7 entries set to `"stub": true`) | Upstream hid them or made them JSON-only (#79310, #80640). |
| City size / spacing / NPC spawn time as % world options | `src/regional_settings.cpp` (`region_settings_city::finalize`), `src/game.cpp` (`perhaps_add_random_npc`), `src/newcharacter.cpp` (`cities_enabled`) | Upstream moved them into region settings; ours scale the region's value. New ids `CITY_SIZE_SCALING`, `CITY_SPACING_SCALING`, `NPC_SPAWNTIME_SCALING`. |
| Light-aware monster flags `AVOIDS_LIGHT` (won't step from dark into light) and `SEEKS_LIGHT` (heads for the brightest light in view) | `src/monmove.cpp` (`will_move_to`, end of `monster::plan`, helpers near the top) | Backrooms Expanded's Smiler and Deathmoth. The mod defines the flags; the C++ checks them lazily, so official builds load the mod and just skip the behaviour. |

| Dimension travel keeps every nearby follower: busy followers (`NPCATT_ACTIVITY`, previously following) drop their chore and come; travellers filed under another overmap are found with `overmap_buffer.remove_npc` | `src/npctalk.cpp` (`f_travel_to_dimension`), `src/game.cpp` (`travel_to_dimension`) | Upstream bugs: a follower eating or picking something up, or one registered in a neighbouring overmap, was left behind (seen going through Level 0's rippling wall with 3 followers). |
| Debug "Spawn NPC follower" adds the NPC to the follower list | `src/debug_menu.cpp` (`spawn_npc_follower`) | Upstream bug: spawned followers were missing from the faction screen and radio contact until a reload. |
| NPCs saved mid-meal keep their food on load; items in pockets keep their uid across a load | `src/item_location.{h,cpp}` (`item_location_owner_scope`, `item_on_person::ensure_who_unpacked`), `src/savegame_json.cpp` (`Character::load` around `activity`/`backlog`; `item::deserialize` passes `keep_uids`), `src/item_contents.{h,cpp}` (`combine`, new `keep_uids`), `src/item.h` (`restore_uid`) | Two upstream bugs: an NPC is not registered with the game while it loads ("Failed to find item_location owner"), and loading copied every pocketed item, and a copied item gets a new uid ("UID not found in container contents"), so any saved activity on an item in a bag lost it, the player's too. Test `npc_meal_survives_save_and_load`. |
| NPC spell rating ignores harmless fields | `src/npc_attack.cpp` (`npc_attack_spell::evaluate_tripoint`) | Upstream bug: every empty tile in a spell's area scored for its field, harmful or not, so a wide harmless field (Mind Over Matter's Clairvoyance, `fd_clairvoyant`) out-rated melee and NPCs cast it instead of fighting. Now only fields with a dangerous intensity level count. |
| **NPC autonomy (Phase 3, branch `npc-autonomy`)**: game option `NPC_AUTONOMY` (General > NPC options, default off), behavior-tree predicate `npc_autonomy_on`, quests cancelled when a follower leaves, needs 0–100 (food, water, sleep, warmth, safety, social) stored as NPC vars `autonomy_need_*` and shown in the look-around panel, scores `npc_safety_urgency` / `npc_social_urgency`, mealtimes and socializing (goals `autonomy_meal` / `autonomy_socialize`), guards eat at mealtimes on their post (`autonomy_post_meal`), follower rule `ally_rule::no_self_care` (set = no water trips or mealtimes; texts in `npc_autonomy.json`), getting water (3.4a: goal `autonomy_get_water` drives upstream's `npc::execute_drink_water` from Water 50, search radius 20 via `npc_autonomy::water_search_radius()`, bottle refill at clean water, give up after 3 failed targets), getting food (3.4b: goal `autonomy_get_food` drives `npc::execute_eat_food` from Food 50;), getting warm (3.4c: goal `autonomy_get_warm` at Warmth 50: clothes, a hearth fire, or firewood to a hearth and light it via `npc::autonomy_light_hearth`; else `npc::execute_regulate_temperature`; water and food share `fetch()` / `wants_trip()` in `npc_autonomy.cpp`), weights in `data/json/npcs/npc_autonomy.json` | New `src/npc_autonomy.{h,cpp}`, `tests/npc_autonomy_test.cpp`, `data/json/npcs/npc_autonomy.json`; hooks in `src/options.cpp`, `src/npc.h` (`ally_rule::no_self_care` and its `ally_rule_strs` entry; `autonomy_has_firestarter` / `autonomy_light_hearth`, defined in `npcmove.cpp` next to `find_fire_spot`), `src/npc.cpp` (`npc::path_avoids` asks `npc_autonomy::avoids_trap`: visible harmful traps are walls), `src/behavior_oracle.cpp`, `src/character_oracle.{h,cpp}` (`autonomy_*`, `safety_urgency`, `social_urgency`), `src/npctalk_funcs.cpp` (`talk_function::leave`), `src/npc.cpp` (`npc_update_body`, `print_info`), `src/npcmove.cpp` (`npc::move` goal dispatch and commitment, `bt_goal_to_category`, radius in `find_nearby_water_sources`; in `find_nearby_food`, drinks and food within that radius, inside ground containers, in LIQUIDCONT furniture (drinks) and CONTAINER furniture (food); Very-thirsty and Very-hungry lines in `npc::intake_useful`; cheap checks before line of sight in both searches; a follower left behind drops the meal near the top of `npc::move` and, via `npc_autonomy::should_catch_up`, any eat or drink goal just before `adopt_goal`); `data/json/npcs/npc_behavior.json` (six children of `npc_priorities`) | Phase 3 logic lives in `npc_autonomy.*`; base files only get small hooks. With the option off, NPCs behave like upstream. |

Not restored on purpose: wandering hordes, character point pools, default region, initial
visibility distance, 3D FOV range.

## Build and run (Windows)

- Visual Studio 2022 + vcpkg at `D:\dev\vcpkg`. Solution: `msvc-full-features\Cataclysm-vcpkg-static.sln`, Release x64.
  Command-line build (game only; use target `Cataclysm-test-vcpkg-static` for tests):
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Cataclysm-vcpkg-static.sln /t:Cataclysm-vcpkg-static /p:Configuration=Release /p:Platform=x64 /m /v:minimal`
- Outputs in the repo root: `cataclysm-tiles.exe` (game), `Cataclysm-test-vcpkg-static-Release-x64.exe` (tests).
- This build's saves and config live in `save\` and `config\` (git-ignored).
- **The launcher** (`tools/launcher/`, on `dev`, shipped to testers on `testing`) installs builds into a folder of its own.
  The user's install is `D:\Games\CDDA MY FORK\` (`CDDA Fork Launcher.exe`, game in `Game\`, user data in `UserData\`;
  desktop shortcut "CDDA Fork Launcher"). The .exe only starts the script; rebuild it with `tools/launcher/src/build-exe.ps1`
  after changing `launcher.ico`. Files in `$script:LauncherAssets` (icons, .exe) ship with every self-update.
  Its "Builds from" picks Xbro10 tester builds (default) or official experimental; "Sync my mods from my fork" copies our mods
  into `UserData\mods`. It updates itself from `testing`: when the launcher changes, raise `$script:LauncherVersion` (a date).
  Never edit the copy in `D:\Games` by hand; change `tools/launcher` and copy it there.

## Performance

Benchmarks (hidden tests): `[npc_perf]` times an NPC turn with autonomy off and on, plus each check and search
(no mods needed). Real play: on quitting, `config/debug.log` gets a "Game Turn" timing tree (monmove,
process_npc_turn, build_map_cache...); read it after a slow session. Level 0 lights (needs the Backrooms mods):
`./Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_perf]"`.
It times the per-turn map cache rebuild for several ceiling-light layouts and prints the darkest floor
light level (10 = "brightly lit"). Light barely fades in open air (`LIGHT_RANGE`: brightness 120 reaches
about 92 tiles), so many bright static lights are the usual cost; Level 0's are overridden to 40.

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
- Math `rng(a, b)` returns a decimal: use `rand(n)` (a whole number 0..n) for picks you compare with `==`.
- `u_set_goal` / `assign_mission_target` / `target_params` ignore `"var"` unless `"om_terrain"` is also set (any id; the var wins), and use your own square instead. `"search_range"` only works with `"random": true`.
- A palette can't name furniture defined in a mod file that loads later ("invalid furniture id"); use `place_furniture` in the mapgen instead.
