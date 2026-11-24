# CLAUDE.md: CDDA Backrooms + Autonomous NPCs

This is Xbro10's fork of Cataclysm: Dark Days Ahead (CDDA). The project plan is the
playbook PDF at
`C:\Users\xande\Documents\CDDA documents for modding\CDDA Backrooms + Autonomous NPCs — Claude Code Playbook.pdf`
(Phases 0–10 plus testing/publishing prompts and a "Parked ideas" list; the 2026-11-20 version; older ones are kept
beside it as "(old, 2026-11-12)", "(old, 2026-11-16)", "(old, 2026-11-17)" and "(old, 2026-11-18)", and "MOST UP TO
DATE.pdf" is a copy of the current one). The PDF is an export of the Claude doc "CDDA Backrooms + Autonomous NPCs —
Claude Code Playbook" (https://claude.ai/artifact/G7ZKKDAthN3upUXGrznUQX): to update the playbook, edit that doc
with the Claude Docs tools, export it as a PDF and save it here the same way. The roadmap page "Backrooms Fork Roadmap"
(https://claude.ai/artifact/Eszyf1oUojRwdUxuXozHrg) is published from `D:\dev\Important Files\fork-roadmap.html`:
read the live page first (other sessions publish to it too), edit the file, publish to that URL. Map every request to
a playbook prompt.

## Playbook progress

As of 2026-11-18. Update this each time a prompt is finished.
- Phases 0, 1 and 2: finished (Prompts 0.1 to 2.6). Don't redo, re-plan or re-verify them.
- Phase 3: 3.1 to 3.4 finished (3.4: water, food and warmth, all tested in game by 2026-11-17).
  The planned "stocking up" step became Prompt 7.0 (stock count). 3.6 comes after Phase 6.
- Phase 4: skipped for now. Don't start it.
- Phase 5: 5.1 finished 2026-11-17 (both mods v0.1.0, one combined GitHub release `mods-v0.1.0`). 5.2 (seek safety)
  finished 2026-11-18, tested in game with its follow-ups (Safety sizes threats up against the NPC's own strength,
  gunners stand and shoot, shopkeepers return to their spot, NPCs pick up what they dropped in a fight). 5.3 (chats keep to
  follow and guard rules) finished 2026-11-19, tested in game. User decision
  (2026-11-18): strangers only chat with their own friends, never come over to chat with the player or followers.
  Follow-up to 5.2 (user request 2026-11-19, built, waiting for the in-game test): followers and guards stand with the
  player and only run with the follower rule `allow_fall_back` (an `ally_rule` bit like `no_self_care`, so the rules
  window lists it; not a Phase 6 stance). Journals stay as they are (user decision 2026-11-19): later steps add a
  journal line only where it's cheap, never as their main output.
  5.4 (journals mention loneliness and fear) built 2026-11-18 (morning values only, user choice), waiting for the
  in-game test; test `[survivor_minds]` with `--mods=survivor_minds`. 5.5 (Survivor Minds traces in the Backrooms:
  overrun-camp journals, CAMP/WATER/DANGER/LOOTED marks, picked-over rooms, all from Backrooms Expanded's
  `mod_interactions/survivor_minds/` filling empty hooks in `survivor_minds_hooks.json`) built 2026-11-18, waiting for
  the in-game test; test `[backrooms_sm]` with `--mods=backrooms,backrooms_expanded,survivor_minds`.
  5.4 tested 2026-11-20 (lonely and scared lines, guard catch-up). 5.6 (place memory, then reading chalk marks; covers
  3.5 and 4.1): step A (place memory, `src/npc_memory.{h,cpp}`) built 2026-11-20, waiting for the in-game test; step B
  (marks) next, after the user's OK. Step C (optional, proposed): "Seen anything around here?" map tips from real
  memories, which brings back the switched-off gossip tip.
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
- Cata++ (user idea, 2026-11-20, on the to-do list, not scheduled): add Noctifer's Cataclysm++ mod
  (github.com/Noctifer-de-Mortem/nocts_cata_mod, folder `nocts_cata_mod_DDA`, mod id `Cata++`, Unlicense, so it can be
  shipped) as an optional mod, not on by default. Plan agreed in outline: import a pinned commit on `dev` with credits
  and a list of our edits; `--check-mods Cata++`, then with Backrooms, Backrooms Expanded, Survivor Minds and Additions;
  then a Backrooms Expanded `mod_interactions/Cata++/` folder, because Cata++ adds its monsters and items to vanilla
  groups (`GROUP_ZOMBIE`, canned food, clothing, lighting) that Backrooms loot and spawns use, and its buildings and
  start scenarios don't exist in a Backrooms world (the Backrooms mod replaces the default dimension). It has UltiCa
  sprites, none for UndeadPeople. Ask the user before downloading it.
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
| Death drops with flag `FRESH_DROP` are as fresh as the kill | `src/monster.cpp` (`drop_items_on_death`: birthday and rot reset instead of `randomize_rot`; flag checked only if a mod defines it) | Upstream makes death drops at the start of the cataclysm and ages them, so Backrooms Expanded's moth jelly dropped rotten weeks in. Test `moth_jelly_is_as_fresh_as_the_kill` (`[backrooms_items]`). |
| Light burns monsters with `BURNS_IN_LIGHT`: at light 50+ on their tile (a flashlight, lamp or fire, not Level 0's 40-brightness ceiling lights) they lose 20-30% of max HP a turn, with smoke and "sizzles and smokes in the light!" | `src/monster.cpp` (`burn_in_light`, called at the top of `monster::process_turn`) | Backrooms Expanded's Smiler weakness (user request). The mod defines the flag; official builds load it and skip the burning. Test `smiler_burns_in_bright_light` (`[backrooms_entities]`). Next to it, `FEARS_LIGHT` (`fear_light`): at the same light level the monster's morale drops below 0, so it backs off while lit (Skin-stealer, Hound); the game's own `BRIGHT_LIGHT` fear only counts near-daylight. Test `skin_stealer_backs_off_from_bright_light`. Item flag `UV_LIGHT` (`in_uv_beam`): someone within 10 tiles who sees the monster and wields a switched-on item with it doubles the burn, even in dim light (Backrooms Expanded's UV torch); test `uv_torch_doubles_the_burn_on_a_smiler` (`[backrooms_items]`). |
| Light-aware monster flags `AVOIDS_LIGHT` (won't step from dark into light) and `SEEKS_LIGHT` (heads for the brightest light in view) | `src/monmove.cpp` (`will_move_to`, end of `monster::plan`, helpers near the top) | Backrooms Expanded's Smiler and Deathmoth. The mod defines the flags; the C++ checks them lazily, so official builds load the mod and just skip the behaviour. |

| Dimension travel keeps every nearby follower: busy followers (`NPCATT_ACTIVITY`, previously following) drop their chore and come; travellers filed under another overmap are found with `overmap_buffer.remove_npc` | `src/npctalk.cpp` (`f_travel_to_dimension`), `src/game.cpp` (`travel_to_dimension`) | Upstream bugs: a follower eating or picking something up, or one registered in a neighbouring overmap, was left behind (seen going through Level 0's rippling wall with 3 followers). |
| Debug "Spawn NPC follower" adds the NPC to the follower list | `src/debug_menu.cpp` (`spawn_npc_follower`) | Upstream bug: spawned followers were missing from the faction screen and radio contact until a reload. |
| Arrows, bolts and thrown things fly through chain-link, bars and arrow slits; NPCs shoot through them | `src/ballistics.{h,cpp}` (`projectile_passes_mesh`: terrain TRANSPARENT + PERMEABLE + THIN_OBSTACLE, or furniture TRANSPARENT + SHORT + MOUNTABLE such as a half sandbag wall, no vehicle; the stop check in `projectile_attack`), `src/npc_attack.cpp` (`has_obstruction`), `tests/projectile_test.cpp` (`projectiles_through_obstacles` now expects a slow shot through chain-link, a wall still stops it), `tests/npc_attack_test.cpp` (`NPC_shoots_through_chain_link`, `NPC_shoots_over_sandbags_and_through_chain_link`) | User request: only bullets passed chain-link, and NPCs treated any impassable tile as blocking, so the Breakroom archers never fired out of the fenced yard and climbed the fence to fight instead. |
| A `cant_see` mission target falls back to a seen place whose map isn't made yet | `src/mission_util.cpp` (`find_or_create_om_terrain`, end of `get_target_position`) | Upstream: on a revealed map nothing is unseen, so the Hound den and "Who they were" jobs failed ("Unable to find and assign mission target", then "Mapgen update function called with invalid tripoint"). Test `hound_den_job_on_a_revealed_map` (`[backrooms_jobs]`). |
| NPCs saved mid-meal keep their food on load; items in pockets keep their uid across a load | `src/item_location.{h,cpp}` (`item_location_owner_scope`, `item_on_person::ensure_who_unpacked`), `src/savegame_json.cpp` (`Character::load` around `activity`/`backlog`; `item::deserialize` passes `keep_uids`), `src/item_contents.{h,cpp}` (`combine`, new `keep_uids`), `src/item.h` (`restore_uid`) | Two upstream bugs: an NPC is not registered with the game while it loads ("Failed to find item_location owner"), and loading copied every pocketed item, and a copied item gets a new uid ("UID not found in container contents"), so any saved activity on an item in a bag lost it, the player's too. A run of identical items on a tile or vehicle part is saved as one entry with one uid, so a location on any but the last falls back to its index while that item is still in such a run (`retrieve_rle_copy`). Tests `npc_meal_survives_save_and_load`, `item_location_on_map_survives_identical_items_reload`. A location inside a container also saves `item_type`; when its uid merged away on load (two stacks that become identical combine), `item_location::deserialize` falls back to an item of that type in the same container, the one at its old index first, never another type (N28, 2026-11-20; saves without `item_type` keep the error). Test `item_location_on_a_stack_that_merged_on_load`; `item_location_in_container_uid_miss_becomes_nowhere` still passes. |
| Archers keep their distance and shoot (N18, NPC autonomy on) | `src/npcmove.cpp`: `npc::assess_danger` (`must_retreat` off while `npc_autonomy::shoots_from_kite_range`), `npc::evaluate_best_attack` (best shot replaces a melee pick while `npc_autonomy::shoots_from_here`), `npc::avoid_friendly_fire` (no side-step to within 2 tiles of the target); `src/npc_autonomy.cpp` (`own_strength`: a gun or bow they can fire counts at least half a shot's damage, `shot_damage`) | Reported 2026-11-20: an archer stepped away (`kite_step`) and walked straight back. "Follow close" past the follow distance makes upstream skip every monster as a target; a close shot rates x0.85 and the held bow costs a time penalty, so clubbing won at 2 tiles; the friendly-fire side-step sorts nearest-the-target first; and the game rates a one-shot bow by its one arrow (about 0.7), so one zombie outgunned a skilled archer. Played-out test `an_archer_follower_kites_and_collects_arrows` (`approached <= 2`, `clubbed <= 2`). |
| NPCs rate draw bows by the aim they really get and the arrows they carry | `src/ranged.cpp` (`Character::gun_value`: `most_accurate_aiming_method_limit` instead of `sight_dispersion().second`), `src/melee.cpp` (`Character::evaluate_weapon_internal`: a `RELOAD_AND_SHOOT` weapon with no shot counts compatible ammo carried, `find_ammo( weapon, false, -1 )`) | Reported 2026-11-21: NPCs took scissors over a compound bow and refused a bow. Bows have no sights (3000+ dispersion, "pure gamble") and are nocked only when fired, so they rated as clubs (1-2.4). Now an archer (archery 4, quiver) rates a compound bow 22, recurve 13, short bow 9 (combat knife 9, katana 15, crossbow 11). Not behind the autonomy option. Test `an_archer_rates_a_bow_above_scissors`. |
| Drawing from a holster, quiver or strap via "Draw what?" doesn't crash | `src/character_attire.cpp` (`outfit::fire_options`: actions capture the `item_location` by value) | Upstream bug (since #89473, on master): the lambdas captured the loop variable by reference from a temporary `worn_locations` list, so picking a bow from a quiver crashed in `item_location::impl::ensure_unpacked` (crash dump 2026-11-21). Test `drawing_a_strapped_gun_from_the_fire_menu` (the holster branch opens a menu tests can't answer). |
| Eating a medicine with a use action (chewing gum) uses the caller's item location | `src/consumption.cpp` (`consume_med` takes the location; `Character::consume` overloads share `consume_item`) | Upstream bug: `consume_med` re-found the item only on the eater and their own tile, so gum eaten from the next tile (NPCs eat from nearby containers) logged "Couldn't find item ... to form item_location" (2026-11-23). Test `chewing_gum_from_the_next_tile`. |
| Debug "Remove all wounds" (Debug menu > Player, key `H`) | `src/debug_menu.cpp` (`D_REMOVE_WOUNDS`) | User request 2026-11-23 with the No Wounds mod: clear wounds already taken. Upstream only toggles one wound per body part from the medical screen in debug mode. |
| Hidden melee test doesn't crash | `tests/melee_test.cpp` (`full_attack_details`) | Upstream test bug (on master too): the unarmed case dereferenced an empty wielded item. It runs now, but two hit chances are just outside its ranges (upstream drift, not chased). |
| NPC spell rating ignores harmless fields | `src/npc_attack.cpp` (`npc_attack_spell::evaluate_tripoint`) | Upstream bug: every empty tile in a spell's area scored for its field, harmful or not, so a wide harmless field (Mind Over Matter's Clairvoyance, `fd_clairvoyant`) out-rated melee and NPCs cast it instead of fighting. Now only fields with a dangerous intensity level count. |
| Bows and crossbows count as quiet for the follower rule "use silent weapons only" | `src/npc.{h,cpp}` (`quiet_enough_for_follower_rules`: silent, or the gun fires arrows or bolts), used in `src/melee.cpp` (`Character::evaluate_weapon_internal`), `src/npcmove.cpp` (`npc::evaluate_best_attack`) and `src/npc_autonomy.cpp` (`ready_gunner`) | Reported 2026-11-20: the rule's cutoff is noise under 5 and a short bow is 6, so with default rules an archer follower never shot and clubbed zombies with the bow. Not behind the autonomy option. |
| A wound progresses at most once per hit | `src/bodypart.cpp` (`bodypart::add_or_worsen_wound`) | Upstream bug: a wound with two `wound_progression` entries (heavy bruising) whose rolls both passed looked for the wound the first had already replaced ("Tried to get wound bruise_bad from bodypart torso") and dereferenced null. Test `worsening_a_wound_with_two_progressions`. |
| Random looks give "no facial hair" as a starting trait | `src/character.cpp` (`Character::randomize_cosmetics`) | Upstream bug: it used `set_mutation`, which skips `my_traits`, so removing it in the character creator (to pick a beard) hit "my_traits and my_mutations were out of sync for FACIAL_HAIR_NONE" and failed. Now `toggle_trait`. Test `random_looks_without_a_beard_can_be_changed`. |
| A stranger running from monsters runs from them, not from the player | `src/npcmove.cpp` (`npc::act_on_danger_assessment`, flat-out retreat for non-allies) | Upstream bug: the retreat set `NPCATT_FLEE_TEMP`, an attitude about the player: `set_attitude` adds a 24-hour `npc_flee_player` effect, so a stranger who ran from zombies "fled from you" for a day, and each time a shorter run lapsed with something hostile still in view they said "is no longer afraid" and got scared again. A stranger who doesn't already fear the player now only gets `npc_run_away` for `run_away_for` (the flee branch and `should_flee` follow it). Tests `stranger_running_from_monsters_doesnt_flee_the_player_for_a_day`, `stranger_runs_from_zombies_then_stops`. |
| NPCs drop a corpse-smashing job to fight | `src/npcmove.cpp` (`npc::move`: `ACT_PULP` abandoned while `in_harms_way()`; `npc::address_needs`: no pulping while in harm's way; `npc::assess_danger`: an unseen hostile within 1 tile on the same level still counts) | Upstream bug (reported 2026-11-19): the pulp activity stayed through a fight and resumed whenever the fight logic didn't act, so a follower smashed a corpse while a zombie chewed on her. Test `npc_stops_pulping_to_fight_a_zombie_on_top_of_them` (`tests/npc_pulp_test.cpp`). |
| A job with no target fails quietly in another dimension | `src/mission_util.cpp` (`find_or_create_om_terrain`: outside the `default` dimension a missing target is a `D_INFO` log line, not a `debugmsg`; `set_update_mapgen`: no map update at an invalid target) | Seen 2026-11-19: a random stranger's vanilla "house" job inside a Backrooms level logged "Unable to find and assign mission target house" and then "Mapgen update function called with invalid tripoint". The job is still offered; it just has no target. |
| A mod tileset for another tileset is an info line, not an error | `src/tileset_loader.cpp` (mod tileset loop: `D_INFO` instead of `D_ERROR`) | Our mods ship one sprite file per tileset (UltiCa, UndeadPeople), so every start logged "Mod tileset ... is not compatible" as an error for the other one. |
| Melee damage in item info lists damage types in their defined order | `src/item_info.cpp` (`item::melee_combat_info`: a vector in `damage_type::get_all()` order instead of `std::map<damage_type_id, int>`) | Upstream bug: `string_id`'s `<` compares the order ids were first created, which changes with link order, so the fork's build showed "Pierce" before "Bash" and `weapon_attack_ratings_and_moves` failed. |
| Switching an item into another form takes the new form's relic effects | `src/item.cpp` (`item::convert`: copies `type->relic_data` unless the item's relic data differs from its old type's) | Upstream copied relic data (passive effects, on-hit spells) only when an item was created, so a weapon switched on by `transform` or `transform_item` kept the "off" form's none: the lit Flamero-Mado never set things on fire and the shock lance never stunned. Tests in `[backrooms_items]` switch them on and check `is_relic()`. |
| Additions on by default | `data/mods/default.json` (`xbro_additions` added to `dev:default`'s dependencies) | User request: the Flamero-Mado spear and later additions belong to the regular game, not only the Backrooms. Re-add it if an upstream merge rewrites the file. |
| `--check-mods` loads mod_interactions like a real world | `src/game_io.cpp` (`game::check_mod_data`: `load_mod_data_from_dir` for each mod, then `load_mod_interaction_data_from_dir`, with the throwaway world's `active_mod_order` set to the mod and its dependencies) | Upstream loaded every .json under a mod folder, `mod_interactions` included, so Backrooms Expanded's `mod_interactions/xbro_additions` failed the check ("item group id br_flamero_cache is unknown") without Additions. Real worlds always loaded them correctly. |
| Test runs turn off the stale-data check | `tests/test_main.cpp` (`init_global_game_state`, after `get_options().load()`) | The first test run after any JSON edit logged "Stale game data detected" as an error and failed with "error logged during initialization". `WARN_ON_MODIFIED` is set to false there; `--option WARN_ON_MODIFIED:true` overrides it. |
| Ambient and effect volume apply as soon as you leave Options | `src/options.cpp` (`options_manager::save`, `options_manager::load`: `last_ambient_volume` / `last_effect_volume`) | Upstream only updated music volume live; ambient loops kept the volume they started with, so lowering Ambient sound volume did nothing until a restart. A change fades all channels and `sfx::do_ambient` restarts the loops. |
| A character template whose scenario comes from a mod this world lacks loads quietly with the generic scenario | `src/avatar.h` (`loading_template`), `src/newcharacter.cpp` (`avatar::load_template`), `src/savegame_json.cpp` (`avatar::load`, scenario fallback) | Upstream popped up "Tried to use non-existent scenario" whenever the "Last Character" template from a Backrooms world was offered in a plain world. Saves still report a missing scenario. Test `template_with_a_missing_scenario_loads_quietly`. |
| Discord "Playing" card (Rich Presence): option `DISCORD_PRESENCE` (General > Discord: off / basic / detailed, default basic); a small IPC client (Windows named pipe `discord-ipc-N`, Unix socket elsewhere, no library), at most one `SET_ACTIVITY` per 15 s; text, pictures, button and the Discord application id in `data/discord_presence.json` (id empty = nothing sent; `default:<mod>` places win while that mod is active) | New `src/discord_presence.{h,cpp}`, `data/discord_presence.json`, `tests/discord_presence_test.cpp`, `tools/backrooms_expanded/draw_discord_art.ps1` (the card picture, from the launcher icon); hooks in `src/main.cpp` (`run_game_loop`: `show_menu`, `game_started`), `src/do_turn.cpp` (`game::do_turn`: `update`), `src/options.cpp` | User request: a better Discord card than Discord's own "Cataclysm: Dark Days Ahead" guess. Art and setup steps in `D:\dev\Important Files\discord-art\`. |
| Picking a scenario in the character creator keeps stats, skills, profession, backgrounds and chosen traits the new scenario allows | `src/newcharacter.cpp` (`reset_scenario`) | User request: upstream reset everything (stats to 8, first profession, no traits or looks, no skills) on every scenario pick. |
| Character creator sprite preview: the tileset sprite (base + `get_overlay_ids()` + the profession's clothes) on the TRAITS tab (4x) and at General Info's right end (2x) | `src/cata_tiles.{h,cpp}` (`character_preview`, `cata_tiles::get_character_preview`), `src/newcharacter.cpp` (`draw_character_sprite_preview`, calls in `draw_top_bar` and `draw_traits`) | User request: see what cosmetic traits look like. Tiles builds only; overlays a tileset lacks are skipped. |
| Overloaded NPCs put down junk (NPC autonomy on): over their weight limit and out of danger, at most once a minute, they drop loose things they picked up, cheapest per kg first, keeping favourites, food, drink, medicine, firewood and ammo or magazines for their gun; what you trade to a follower becomes a favourite (as "give" already did) | New `src/npc_load.{h,cpp}`, `tests/npc_load_test.cpp`; hooks in `src/npcmove.cpp` (`npc::move`, after the self-care block) and `src/npctrade.cpp` (`npc_trading::transfer_items`) | Upstream switched NPC dropping off ("drop_items is still broken"), so an overloaded NPC just stayed slow. Items step 2. |
| A follower's favourites show apart from the player's: items given or traded to a follower get item var `npc_favorite` with the favourite (`mark_npc_favorite`, not if already the player's favourite); the prefix `* ` is light red for them; `Character::try_add` on the avatar removes the var and the favourite (`unmark_npc_favorite`) | `src/npc.{h,cpp}` (`mark_npc_favorite`, `unmark_npc_favorite`), `src/npctrade.cpp` (`transfer_items`), `src/talker_npc.cpp` (give/carry), `src/item_tname.cpp` (`pre_asterisk` only: the suffix asterisk is part of sort and pickup-rule keys), `src/character_inventory.cpp` (both `try_add`) | User request 2026-11-24: an NPC's favourites looked like the player's, and nothing cleared the flag when items came back, so the player's inventory filled with favourites. Not behind the autonomy option. Test `a_followers_favourite_shows_apart_from_yours` (`tests/npc_pickup_rules_test.cpp`). |
| Pickup rules by category (`c:food`, `c:ammo,guns`) and a Categories checklist (key `g`, action `CATEGORY_RULES`, Take / Never / -, rules inserted at the top so typed item rules win); NPC rules checked against the item (`npc_settings::create_rule( const item & )`), so material and category rules count for followers; `npc::item_blacklisted`: an Exclude rule beats the "better weapon" exception in `npc::wants_take_that` | `src/auto_pickup.{h,cpp}` (`check_special_rule` takes the item category, `edit_category_rules`, help text), `data/raw/keybindings.json` (`CATEGORY_RULES`), `src/dialogue.cpp` (`npc_item_rule`, `item_whitelisted`, `item_blacklisted`), `src/npc.h`, `src/npcmove.cpp` (`wants_take_that`) | User request 2026-11-24: followers with pickup allowed took junk; the existing per-follower rules ("Set up pickup rules.") had names and materials only, materials never reached NPCs, and a pipe or plank got past Exclude rules as a better weapon than fists. Not behind the autonomy option. Test `follower_pickup_rules_by_category`. |
| Followers without pickup rules leave junk (NPC autonomy on): `npc_autonomy::worth_carrying` in `npc::wants_take_that` skips categories spare_parts, container, other, veh_parts, corpses (by `get_category_of_contents`) and anything 1 kg+ under 25 cents/kg (`npc::value`); food or drink they need, ammo or magazines for their gun and their own tagged things pass; `npc::find_item` keeps the best-valued item (nearest of equals) instead of the last (farthest) | `src/npc_autonomy.{h,cpp}`, `src/npcmove.cpp` (`wants_take_that`, `find_item`'s `consider_item`) | User request 2026-11-24. Upstream's only test was value over 10-30 cents, and the scan picked the farthest passing item. Test `a_follower_without_rules_leaves_junk`. |
| NPC place memory (5.6 A, NPC autonomy on): up to 30 places per NPC (kinds water, safe, danger, looted, camp; position, dimension, last seen), in saved NPC var `autonomy_places` (`kind,x,y,z,turn,dimension;...`, no save-format change, old saves empty); same kind within 3 tiles (danger 5) is one place, refreshed; fades (danger 3 days, looted 7, safe 10, water and camp 30); when full the lowest importance minus days since seen goes. Filled by `npc_autonomy`: water at a lasting source after drinking (`fetch`), safe where they hold in `seek_safety`, danger when Safety 50+ and HP under 50% (`on_npc_turn`). `wants_water` also says yes for remembered water in the loaded map and in reach (else 60 tiles); `walk_to_remembered_water()` goes there when the game's planner finds nothing, forgets it within 2 tiles or if unreachable. Panel: "Remembers N places" after the needs | New `src/npc_memory.{h,cpp}`, `tests/npc_memory_test.cpp`; hooks only in `src/npc_autonomy.cpp` | Playbook 5.6 step A (covers 3.5's places). Step B adds marks read from Survivor Minds furniture. |
| Autonomy checks cached per NPC action (speed): hostile scans (`watched_threats`, `sees_hostile_within`) and `own_strength` are kept for one `npc::move`, dropped when the NPC steps, changes weapon or the turn changes; outside a scope (tests calling checks directly) nothing is cached | `src/npc_autonomy.{h,cpp}` (`action_cache_scope`, `cache_for`), `src/npcmove.cpp` (top of `npc::move`), `tests/backrooms_perf_test.cpp` (zombies-in-view timing) | Real play logged about 1 ms per NPC turn; `safety_need` and `is_safe` were asked several times per action, each with line-of-sight checks on every creature and a weapon rating. |
| **NPC autonomy (Phase 3, branch `npc-autonomy`)**: game option `NPC_AUTONOMY` (General > NPC options, default off), behavior-tree predicate `npc_autonomy_on`, quests cancelled when a follower leaves, needs 0–100 (food, water, sleep, warmth, safety, social) stored as NPC vars `autonomy_need_*` and shown in the look-around panel, scores `npc_safety_urgency` / `npc_social_urgency`, mealtimes and socializing (goals `autonomy_meal` / `autonomy_socialize`; 5.3: a chat partner must pass `npc::judge_option` as allowed, with the reach from `obligation_reach()`, and a chat gives way in `should_catch_up`; guards on shift chat via `npc_autonomy_post_socialize`, weight 0.5 over duty 0.45), guards eat at mealtimes on their post (`autonomy_post_meal`), follower rule `ally_rule::no_self_care` (set = no water trips or mealtimes; texts in `npc_autonomy.json`), followers stand with the player (2026-11-19: `stands_with_player()` = an ally without the follower rule `ally_rule::allow_fall_back`: no `wants_safety`, no long retreat in `npc::act_on_danger_assessment`, the short step back only while `may_step_back()` (inside `obligation_reach()`), a running ally outside it stops; with the rule set, `falls_back_to_player()` makes `npc::good_escape_direction` regroup on the player; tests in `tests/npc_fall_back_test.cpp`), getting water (3.4a: goal `autonomy_get_water` drives upstream's `npc::execute_drink_water` from Water 50, search radius 20 via `npc_autonomy::water_search_radius()`, bottle refill at clean water, give up after 3 failed targets), getting food (3.4b: goal `autonomy_get_food` drives `npc::execute_eat_food` from Food 50;), getting warm (3.4c: goal `autonomy_get_warm` at Warmth 50: clothes, a hearth fire, or firewood to a hearth and light it via `npc::autonomy_light_hearth`; else `npc::execute_regulate_temperature`; water and food share `fetch()` / `wants_trip()` in `npc_autonomy.cpp`), seeking safety (5.2: goal `autonomy_seek_safety`, predicate `npc_autonomy_wants_safety`, score `npc_autonomy_safety_urgency` = weight x Safety; at Safety 50+ outside the BT's `npc_combat` branch, walk to the best-scored tile in `obligation_reach()` or 20 tiles (lit by the `AVOIDS_LIGHT` test, friends within 3, distance from seen hostiles) and keep at it until Safety < 25 and 2 minutes after last scared (`autonomy_safe_scared_at`); Safety rises when the hostiles seen within 10 tiles together match `own_strength()` (defence + `evaluate_weapon`, a gun a tenth, by health), at any range, and from the game's own odds unless the NPC outclasses all it sees; `is_safe()` (daily life) also waits while anything hostile is within 3 tiles; `arm_up()` wields the best weapon carried every 3 turns with hostiles in view, only when the hands are empty or hold a non-weapon (never swaps a drawn weapon: it fought the attack choice)), gunners stand and shoot (`gunner_stance_for()`: none / hold / step_back; hooks in `npc::assess_danger` (aim at the nearest hostile within the gun's sure range and 10 tiles) and `npc::act_on_danger_assessment` (no back-off on hold, no long retreat on step_back); `kite_step()` in `npc::move` after the field escape: something slower beside a ready gunner, a step to a free tile further from every seen hostile (a follower within 10 tiles of the player, a guard inside `obligation_reach()`), and the game's own step back is off for ready gunners; `ready_the_shot()` right after: an empty gun or bow with hostiles in view and none beside them is reloaded at once; `fire_now()` in `npc_attack_gun::use` (`src/npc_attack.cpp`): no more aiming once a hostile could reach them within two turns; `fetches_own_things()` zeroes `npc_following_urgency` (`src/character_oracle.cpp`) while a follower picks up their own things within 15 tiles of the player; played-out test `an_archer_follower_kites_and_collects_arrows`), shopkeepers and sheltering strangers return to their spot (`stays_put()`, NPC var `autonomy_home_spot` set the first time they stand calm, goal `autonomy_return_home`; seeking safety stays within 6 tiles of it), picking things back up after a fight (`track_fight_items()` tags the weapon, worn items and their contents with item var `autonomy_carried_by` at a fight's start and untags what is still carried at its end; goal `autonomy_recover_items` fetches tagged items within 20 tiles for 30 minutes, loose or in a corpse (an arrow that stuck in a monster), seen or not, giving up only on a spot it fails to walk to 3 times (NPC var `autonomy_recover_failed`) (a follower up to 15 tiles from the player; shots and throws in a fight are tagged by `tag_shot_ammo()`, hooked in `src/ranged.cpp`: `make_gun_projectile` and `Character::throw_item`); `grab_dropped_weapon()` takes their own weapon back at once from their tile or next to it, mid-fight too), weights in `data/json/npcs/npc_autonomy.json` | New `src/npc_autonomy.{h,cpp}`, `tests/npc_autonomy_test.cpp`, `data/json/npcs/npc_autonomy.json`; hooks in `src/options.cpp`, `src/npc.h` (`ally_rule::no_self_care` and its `ally_rule_strs` entry; `autonomy_has_firestarter` / `autonomy_light_hearth`, defined in `npcmove.cpp` next to `find_fire_spot`), `src/npc.cpp` (`npc::path_avoids` asks `npc_autonomy::avoids_trap`: visible harmful traps are walls), `src/behavior_oracle.cpp`, `src/character_oracle.{h,cpp}` (`autonomy_*`, `safety_urgency`, `social_urgency`), `src/npctalk_funcs.cpp` (`talk_function::leave`), `src/npc.cpp` (`npc_update_body`, `print_info`), `src/npcmove.cpp` (`npc::move` goal dispatch and commitment, `bt_goal_to_category`, radius in `find_nearby_water_sources`; in `find_nearby_food`, drinks and food within that radius, inside ground containers, in LIQUIDCONT furniture (drinks) and CONTAINER furniture (food); Very-thirsty and Very-hungry lines in `npc::intake_useful`; cheap checks before line of sight in both searches; a follower left behind drops the meal near the top of `npc::move` and, via `npc_autonomy::should_catch_up`, any eat or drink goal just before `adopt_goal`); `data/json/npcs/npc_behavior.json` (ten children of `npc_priorities`; `npc_homeostasis` steps aside while `npc_autonomy_wants_warmth`, whose goal scores `npc_autonomy_warmth_urgency`; the hand-off is in `npc::move`'s `regulate_temperature` commitment check) | Phase 3 logic lives in `npc_autonomy.*`; base files only get small hooks. With the option off, NPCs behave like upstream. |

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
- Every new Backrooms Expanded overmap terrain goes in the list in `backrooms_places_generate_cleanly` (`tests/backrooms_runner_test.cpp`, `[backrooms_runner]`): it generates each place many times and fails on any mapgen error, such as a chunk setting terrain over an earlier chunk's items (fix: `ERASE_ALL_BEFORE_PLACING_TERRAIN` on the later chunk). A test looking up a missing id should fail, not skip: the old Level 0 test had been skipping itself on `backfloorchamber` (the id is `backfloorchamber_north`).
