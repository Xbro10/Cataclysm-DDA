# CLAUDE.md: CDDA Backrooms + Autonomous NPCs

This is Xbro10's fork of Cataclysm: Dark Days Ahead (CDDA). The project plan is the
playbook PDF at
`C:\Users\xande\Documents\CDDA documents for modding\CDDA Backrooms + Autonomous NPCs — Claude Code Playbook.pdf`
(Phases 0–10 plus testing/publishing prompts and a "Parked ideas" list; the 2026-11-18 version, which adds Phase 9
"Fighting smart" and Phase 10 "Life off-screen"; older ones are kept beside it as "(old, 2026-11-12)", "(old, 2026-11-16)"
and "(old, 2026-11-17)"). Map every request to a playbook prompt.

## Playbook progress

As of 2026-11-18. Update this each time a prompt is finished.
- Phases 0, 1 and 2: finished (Prompts 0.1 to 2.6). Don't redo, re-plan or re-verify them.
- Phase 3: 3.1 to 3.4 finished (3.4: water, food and warmth, all tested in game by 2026-11-17).
  The planned "stocking up" step became Prompt 7.0 (stock count). 3.6 comes after Phase 6.
- Phase 4: skipped for now. Don't start it.
- Phase 5: 5.1 finished 2026-11-17 (both mods v0.1.0, one combined GitHub release `mods-v0.1.0`). 5.2 (seek safety)
  finished 2026-11-18, tested in game with its follow-ups (Safety sizes threats up against the NPC's own strength,
  gunners stand and shoot, shopkeepers return to their spot, NPCs pick up what they dropped in a fight). 5.3 (chats keep to
  follow and guard rules) built 2026-11-18, waiting for the in-game test; then 5.3 is finished. User decision
  (2026-11-18): strangers only chat with their own friends, never come over to chat with the player or followers.
  5.4 (journals mention loneliness and fear) built 2026-11-18 (morning values only, user choice), waiting for the
  in-game test; test `[survivor_minds]` with `--mods=survivor_minds`. 5.5 (Survivor Minds traces in the Backrooms:
  overrun-camp journals, CAMP/WATER/DANGER/LOOTED marks, picked-over rooms, all from Backrooms Expanded's
  `mod_interactions/survivor_minds/` filling empty hooks in `survivor_minds_hooks.json`) built 2026-11-18, waiting for
  the in-game test; test `[backrooms_sm]` with `--mods=backrooms,backrooms_expanded,survivor_minds`. Next: 5.6.
- Order: the rest of Phase 5 in order, then Phase 6 (6.0 first), then 3.6, then Phase 9, then Phase 7, then Phase 8
  (it takes over the shelter half of 7.7; 8.6, Backrooms homes, goes on `dev`), then Phase 10 (last).
  Skip 3.5 and 4.1 (covered by 5.6), 4.5 (5.8 step B), 3.7 and 4.7 (Phase 7). 6.6 starts 4.6 (scouts).
- Notes for Phase 9 (from the 2026-11-18 review): its "what exists today" should include the gunner rules
  (`npc_autonomy::gunner_stance_for`, hooks in `npc::assess_danger` and `npc::act_on_danger_assessment`);
  decide in 9.0 whether they move under the "NPC combat tactics" sub-option. The game's long retreat lasts at
  least 20 turns (`std::max( ..., 20_turns )` in `act_on_danger_assessment`), not two. 9.3 and 9.7 can build on
  5.2's safe-spot scoring (`find_safe_spot`). 9.9 (light draws the dead at night) conflicts with 5.2's light
  bonus outside the Backrooms: settle it by 9.9 at the latest.
- Gunner work the user moved to Phase 9 (2026-11-18), on top of the gunner rules from 5.2:
  1. Fire control: today an NPC aims until the target is inside its sure range (`npc_attack_gun::use`,
     `confident_gun_mode_range`), often only 3-5 tiles, and never checks what reaches it first (a TODO there).
     Fire sooner when the nearest enemy could reach them within about 2 turns or more aiming barely helps.
  2. Against several enemies, target the one that arrives first (distance and speed), not the biggest threat.
  3. Step back from the whole group, only to a tile further from all of them (5.2's spot scoring); else stand and shoot.
  4. Outgunned by a crowd: fall back toward the player or friends while still shooting (with 9.7), not the game's flee.
  5. Several gunners spread their fire (with 9.5).
  6. Gunners may range further from the player than melee followers (a longer follow leash while armed with a gun
     they may fire), to get a clear angle or keep their distance before firing, then rejoin. Followers still obey
     "hold the line" and follow orders.
- Squad roles (user request, 2026-11-18, inspired by squad-based survival games): followers in a group each take a
  role (medic, scout, fighter) that shapes how they fight and help. A big job; fold it into Phase 9 (9.5 "Fighting
  as a group" and 9.8 "Patching each other up" are the natural homes) when Phase 9 is planned.
- Note for Phase 10: 10.1 (eat, drink and sleep while away, from the NPC's own pack) fixes a problem players hit
  today (followers come back starving) and could come earlier in a simple form; ask the user before moving it.
- Runners for every camp (user request, 2026-11-18): any camp, with a faction or not, sends a runner out for supplies.
  Build it in Phase 10 next to 10.5 (trips that keep going), on the Breakroom's runners (`camps_runners.json`), which
  stay Breakroom-only until then as the working model. Ties in with the parked "Trade between camps" idea.
- Side-by-side survival test (user idea, 2026-11-18, very long term: after the phases are done): a small test map
  split in two, one NPC on each side with exactly the same body, skills, gear, terrain, loot and monsters, one with
  NPC autonomy on and one without (upstream behaviour). Run both for days or weeks and compare how long each survives
  and how well it does (food and water kept, injuries, kills, things built). It extends the long tests already
  planned (7.9 a month alone, 9.10 arena fights, 10.7 long absence) into a direct autonomy-vs-upstream comparison;
  build it from those tests' setups and report both NPCs side by side.
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
| **Additions** | `data/mods/Xbro_Additions/` (id `xbro_additions`) | Additions to the regular world (items, recipes, later structures and more), no Backrooms needed; its description counts what it adds (`Items: 2`...), keep it current; on by default in new worlds (`data/mods/default.json`). First: the Flamero-Mado spear (item ids keep their old `br_` prefix). Backrooms Expanded adds Backrooms spawns for it in `mod_interactions/xbro_additions/`, so it works with or without Additions. Its sprites are drawn by `tools/backrooms_expanded/draw_br_weapons.ps1`, `draw_br_held*.ps1` and `spear_shape.ps1`. |
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
- Mod releases (playbook P.1): each mod keeps its own version and tag (`survivor-minds-vX.Y.Z`, `backrooms-expanded-vX.Y.Z`),
  but the user wants **one combined GitHub release** (full release, not pre-release): tag `mods-vX.Y.Z`, title
  "Mods vX.Y.Z: Backrooms Expanded + Survivor Minds", both zips (`git archive --prefix=<Folder>/ <tag>:data/mods/<Folder>`
  into `D:\dev\releases\`), notes = install steps, a short list per mod and each changelog entry in a `<details>` block.
  First: `mods-v0.1.0` (2026-11-17). No `gh` CLI here: create it in the built-in browser; the user drags the zips in.
  The launcher only reads `testing-build-*` tags, so mod releases never show there.

## Fork changes to the game itself (watch these during upstream merges)

Search for `Xbro10 fork` in `src/` to find every C++ change.

| What | Files | Why |
|---|---|---|
| Character creator crash fix: past achievements are loaded before the first ImGui frame | `src/newcharacter.cpp` (`character_creator_ui::display`) | Their "please wait" popup redrew mid-frame and crashed in `GetCursorScreenPos`. Drop it if upstream fixes this. |
| Legacy sandbox world options restored (branch `sandbox-options`) | `src/options.cpp` (world option groups), `data/core/external_options.json` (7 entries set to `"stub": true`) | Upstream hid them or made them JSON-only (#79310, #80640). |
| City size / spacing / NPC spawn time as % world options | `src/regional_settings.cpp` (`region_settings_city::finalize`), `src/game.cpp` (`perhaps_add_random_npc`), `src/newcharacter.cpp` (`cities_enabled`) | Upstream moved them into region settings; ours scale the region's value. New ids `CITY_SIZE_SCALING`, `CITY_SPACING_SCALING`, `NPC_SPAWNTIME_SCALING`. |
| Food and drink flag `NPC_REFUSES`: NPCs never pick an item with it to eat or drink on their own | `src/npcmove.cpp` (`rate_food`, flag checked only if a mod defines it) | Backrooms Expanded's cloudy water (liquid pain) looks like almond water; NPCs drank it when thirsty. The mod defines the flag (`items/wiki_finds.json`), so official builds load the mod and ignore it. |
| Light burns monsters with `BURNS_IN_LIGHT`: at light 50+ on their tile (a flashlight, lamp or fire, not Level 0's 40-brightness ceiling lights) they lose 20-30% of max HP a turn, with smoke and "sizzles and smokes in the light!" | `src/monster.cpp` (`burn_in_light`, called at the top of `monster::process_turn`) | Backrooms Expanded's Smiler weakness (user request). The mod defines the flag; official builds load it and skip the burning. Test `smiler_burns_in_bright_light` (`[backrooms_entities]`). Next to it, `FEARS_LIGHT` (`fear_light`): at the same light level the monster's morale drops below 0, so it backs off while lit (Skin-stealer, Hound); the game's own `BRIGHT_LIGHT` fear only counts near-daylight. Test `skin_stealer_backs_off_from_bright_light`. |
| Light-aware monster flags `AVOIDS_LIGHT` (won't step from dark into light) and `SEEKS_LIGHT` (heads for the brightest light in view) | `src/monmove.cpp` (`will_move_to`, end of `monster::plan`, helpers near the top) | Backrooms Expanded's Smiler and Deathmoth. The mod defines the flags; the C++ checks them lazily, so official builds load the mod and just skip the behaviour. |

| Dimension travel keeps every nearby follower: busy followers (`NPCATT_ACTIVITY`, previously following) drop their chore and come; travellers filed under another overmap are found with `overmap_buffer.remove_npc` | `src/npctalk.cpp` (`f_travel_to_dimension`), `src/game.cpp` (`travel_to_dimension`) | Upstream bugs: a follower eating or picking something up, or one registered in a neighbouring overmap, was left behind (seen going through Level 0's rippling wall with 3 followers). |
| Debug "Spawn NPC follower" adds the NPC to the follower list | `src/debug_menu.cpp` (`spawn_npc_follower`) | Upstream bug: spawned followers were missing from the faction screen and radio contact until a reload. |
| NPCs saved mid-meal keep their food on load; items in pockets keep their uid across a load | `src/item_location.{h,cpp}` (`item_location_owner_scope`, `item_on_person::ensure_who_unpacked`), `src/savegame_json.cpp` (`Character::load` around `activity`/`backlog`; `item::deserialize` passes `keep_uids`), `src/item_contents.{h,cpp}` (`combine`, new `keep_uids`), `src/item.h` (`restore_uid`) | Two upstream bugs: an NPC is not registered with the game while it loads ("Failed to find item_location owner"), and loading copied every pocketed item, and a copied item gets a new uid ("UID not found in container contents"), so any saved activity on an item in a bag lost it, the player's too. A run of identical items on a tile or vehicle part is saved as one entry with one uid, so a location on any but the last falls back to its index while that item is still in such a run (`retrieve_rle_copy`). Tests `npc_meal_survives_save_and_load`, `item_location_on_map_survives_identical_items_reload`. |
| NPC spell rating ignores harmless fields | `src/npc_attack.cpp` (`npc_attack_spell::evaluate_tripoint`) | Upstream bug: every empty tile in a spell's area scored for its field, harmful or not, so a wide harmless field (Mind Over Matter's Clairvoyance, `fd_clairvoyant`) out-rated melee and NPCs cast it instead of fighting. Now only fields with a dangerous intensity level count. |
| A wound progresses at most once per hit | `src/bodypart.cpp` (`bodypart::add_or_worsen_wound`) | Upstream bug: a wound with two `wound_progression` entries (heavy bruising) whose rolls both passed looked for the wound the first had already replaced ("Tried to get wound bruise_bad from bodypart torso") and dereferenced null. Test `worsening_a_wound_with_two_progressions`. |
| Random looks give "no facial hair" as a starting trait | `src/character.cpp` (`Character::randomize_cosmetics`) | Upstream bug: it used `set_mutation`, which skips `my_traits`, so removing it in the character creator (to pick a beard) hit "my_traits and my_mutations were out of sync for FACIAL_HAIR_NONE" and failed. Now `toggle_trait`. Test `random_looks_without_a_beard_can_be_changed`. |
| A stranger running from monsters runs from them, not from the player | `src/npcmove.cpp` (`npc::act_on_danger_assessment`, flat-out retreat for non-allies) | Upstream bug: the retreat set `NPCATT_FLEE_TEMP`, an attitude about the player: `set_attitude` adds a 24-hour `npc_flee_player` effect, so a stranger who ran from zombies "fled from you" for a day, and each time a shorter run lapsed with something hostile still in view they said "is no longer afraid" and got scared again. A stranger who doesn't already fear the player now only gets `npc_run_away` for `run_away_for` (the flee branch and `should_flee` follow it). Tests `stranger_running_from_monsters_doesnt_flee_the_player_for_a_day`, `stranger_runs_from_zombies_then_stops`. |
| Additions on by default | `data/mods/default.json` (`xbro_additions` added to `dev:default`'s dependencies) | User request: the Flamero-Mado spear and later additions belong to the regular game, not only the Backrooms. Re-add it if an upstream merge rewrites the file. |
| `--check-mods` loads mod_interactions like a real world | `src/game_io.cpp` (`game::check_mod_data`: `load_mod_data_from_dir` for each mod, then `load_mod_interaction_data_from_dir`, with the throwaway world's `active_mod_order` set to the mod and its dependencies) | Upstream loaded every .json under a mod folder, `mod_interactions` included, so Backrooms Expanded's `mod_interactions/xbro_additions` failed the check ("item group id br_flamero_cache is unknown") without Additions. Real worlds always loaded them correctly. |
| Test runs turn off the stale-data check | `tests/test_main.cpp` (`init_global_game_state`, after `get_options().load()`) | The first test run after any JSON edit logged "Stale game data detected" as an error and failed with "error logged during initialization". `WARN_ON_MODIFIED` is set to false there; `--option WARN_ON_MODIFIED:true` overrides it. |
| Ambient and effect volume apply as soon as you leave Options | `src/options.cpp` (`options_manager::save`, `options_manager::load`: `last_ambient_volume` / `last_effect_volume`) | Upstream only updated music volume live; ambient loops kept the volume they started with, so lowering Ambient sound volume did nothing until a restart. A change fades all channels and `sfx::do_ambient` restarts the loops. |
| A character template whose scenario comes from a mod this world lacks loads quietly with the generic scenario | `src/avatar.h` (`loading_template`), `src/newcharacter.cpp` (`avatar::load_template`), `src/savegame_json.cpp` (`avatar::load`, scenario fallback) | Upstream popped up "Tried to use non-existent scenario" whenever the "Last Character" template from a Backrooms world was offered in a plain world. Saves still report a missing scenario. Test `template_with_a_missing_scenario_loads_quietly`. |
| Discord "Playing" card (Rich Presence): option `DISCORD_PRESENCE` (General > Discord: off / basic / detailed, default basic); a small IPC client (Windows named pipe `discord-ipc-N`, Unix socket elsewhere, no library), at most one `SET_ACTIVITY` per 15 s; text, pictures, button and the Discord application id in `data/discord_presence.json` (id empty = nothing sent; `default:<mod>` places win while that mod is active) | New `src/discord_presence.{h,cpp}`, `data/discord_presence.json`, `tests/discord_presence_test.cpp`, `tools/backrooms_expanded/draw_discord_art.ps1` (the card picture, from the launcher icon); hooks in `src/main.cpp` (`run_game_loop`: `show_menu`, `game_started`), `src/do_turn.cpp` (`game::do_turn`: `update`), `src/options.cpp` | User request: a better Discord card than Discord's own "Cataclysm: Dark Days Ahead" guess. Art and setup steps in `D:\dev\Important Files\discord-art\`. |
| Picking a scenario in the character creator keeps stats, skills, profession, backgrounds and chosen traits the new scenario allows | `src/newcharacter.cpp` (`reset_scenario`) | User request: upstream reset everything (stats to 8, first profession, no traits or looks, no skills) on every scenario pick. |
| Character creator sprite preview: the tileset sprite (base + `get_overlay_ids()` + the profession's clothes) on the TRAITS tab (4x) and at General Info's right end (2x) | `src/cata_tiles.{h,cpp}` (`character_preview`, `cata_tiles::get_character_preview`), `src/newcharacter.cpp` (`draw_character_sprite_preview`, calls in `draw_top_bar` and `draw_traits`) | User request: see what cosmetic traits look like. Tiles builds only; overlays a tileset lacks are skipped. |
| Overloaded NPCs put down junk (NPC autonomy on): over their weight limit and out of danger, at most once a minute, they drop loose things they picked up, cheapest per kg first, keeping favourites, food, drink, medicine, firewood and ammo or magazines for their gun; what you trade to a follower becomes a favourite (as "give" already did) | New `src/npc_load.{h,cpp}`, `tests/npc_load_test.cpp`; hooks in `src/npcmove.cpp` (`npc::move`, after the self-care block) and `src/npctrade.cpp` (`npc_trading::transfer_items`) | Upstream switched NPC dropping off ("drop_items is still broken"), so an overloaded NPC just stayed slow. Items step 2. |
| **NPC autonomy (Phase 3, branch `npc-autonomy`)**: game option `NPC_AUTONOMY` (General > NPC options, default off), behavior-tree predicate `npc_autonomy_on`, quests cancelled when a follower leaves, needs 0–100 (food, water, sleep, warmth, safety, social) stored as NPC vars `autonomy_need_*` and shown in the look-around panel, scores `npc_safety_urgency` / `npc_social_urgency`, mealtimes and socializing (goals `autonomy_meal` / `autonomy_socialize`; 5.3: a chat partner must pass `npc::judge_option` as allowed, with the reach from `obligation_reach()`, and a chat gives way in `should_catch_up`; guards on shift chat via `npc_autonomy_post_socialize`, weight 0.5 over duty 0.45), guards eat at mealtimes on their post (`autonomy_post_meal`), follower rule `ally_rule::no_self_care` (set = no water trips or mealtimes; texts in `npc_autonomy.json`), getting water (3.4a: goal `autonomy_get_water` drives upstream's `npc::execute_drink_water` from Water 50, search radius 20 via `npc_autonomy::water_search_radius()`, bottle refill at clean water, give up after 3 failed targets), getting food (3.4b: goal `autonomy_get_food` drives `npc::execute_eat_food` from Food 50;), getting warm (3.4c: goal `autonomy_get_warm` at Warmth 50: clothes, a hearth fire, or firewood to a hearth and light it via `npc::autonomy_light_hearth`; else `npc::execute_regulate_temperature`; water and food share `fetch()` / `wants_trip()` in `npc_autonomy.cpp`), seeking safety (5.2: goal `autonomy_seek_safety`, predicate `npc_autonomy_wants_safety`, score `npc_autonomy_safety_urgency` = weight x Safety; at Safety 50+ outside the BT's `npc_combat` branch, walk to the best-scored tile in `obligation_reach()` or 20 tiles (lit by the `AVOIDS_LIGHT` test, friends within 3, distance from seen hostiles) and keep at it until Safety < 25 and 2 minutes after last scared (`autonomy_safe_scared_at`); Safety rises when the hostiles seen within 10 tiles together match `own_strength()` (defence + `evaluate_weapon`, a gun a tenth, by health), at any range, and from the game's own odds unless the NPC outclasses all it sees; `is_safe()` (daily life) also waits while anything hostile is within 3 tiles; `arm_up()` wields the best weapon carried every 3 turns with hostiles in view, only when the hands are empty or hold a non-weapon (never swaps a drawn weapon: it fought the attack choice)), gunners stand and shoot (`gunner_stance_for()`: none / hold / step_back; hooks in `npc::assess_danger` (aim at the nearest hostile within the gun's sure range and 10 tiles) and `npc::act_on_danger_assessment` (no back-off on hold, no long retreat on step_back)), shopkeepers and sheltering strangers return to their spot (`stays_put()`, NPC var `autonomy_home_spot` set the first time they stand calm, goal `autonomy_return_home`; seeking safety stays within 6 tiles of it), picking things back up after a fight (`track_fight_items()` tags the weapon, worn items and their contents with item var `autonomy_carried_by` at a fight's start and untags what is still carried at its end; goal `autonomy_recover_items` fetches tagged items within 10 tiles for 30 minutes; `grab_dropped_weapon()` takes their own weapon back at once from their tile or next to it, mid-fight too), weights in `data/json/npcs/npc_autonomy.json` | New `src/npc_autonomy.{h,cpp}`, `tests/npc_autonomy_test.cpp`, `data/json/npcs/npc_autonomy.json`; hooks in `src/options.cpp`, `src/npc.h` (`ally_rule::no_self_care` and its `ally_rule_strs` entry; `autonomy_has_firestarter` / `autonomy_light_hearth`, defined in `npcmove.cpp` next to `find_fire_spot`), `src/npc.cpp` (`npc::path_avoids` asks `npc_autonomy::avoids_trap`: visible harmful traps are walls), `src/behavior_oracle.cpp`, `src/character_oracle.{h,cpp}` (`autonomy_*`, `safety_urgency`, `social_urgency`), `src/npctalk_funcs.cpp` (`talk_function::leave`), `src/npc.cpp` (`npc_update_body`, `print_info`), `src/npcmove.cpp` (`npc::move` goal dispatch and commitment, `bt_goal_to_category`, radius in `find_nearby_water_sources`; in `find_nearby_food`, drinks and food within that radius, inside ground containers, in LIQUIDCONT furniture (drinks) and CONTAINER furniture (food); Very-thirsty and Very-hungry lines in `npc::intake_useful`; cheap checks before line of sight in both searches; a follower left behind drops the meal near the top of `npc::move` and, via `npc_autonomy::should_catch_up`, any eat or drink goal just before `adopt_goal`); `data/json/npcs/npc_behavior.json` (ten children of `npc_priorities`; `npc_homeostasis` steps aside while `npc_autonomy_wants_warmth`, whose goal scores `npc_autonomy_warmth_urgency`; the hand-off is in `npc::move`'s `regulate_temperature` commitment check) | Phase 3 logic lives in `npc_autonomy.*`; base files only get small hooks. With the option off, NPCs behave like upstream. |

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
./cataclysm-tiles.exe --check-mods xbro_additions
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
