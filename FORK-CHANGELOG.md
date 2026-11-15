# Fork changelog

Everything this fork adds to Cataclysm: Dark Days Ahead, sorted by kind.
Tags: **[BE]** Backrooms Expanded mod, **[SM]** Survivor Minds mod, **[C++]** a change to the game itself, **[Launcher]** the launcher in `tools/launcher/`.
The full detail for each mod is in its own `CHANGELOG.md` (`data/mods/Backrooms_Expanded/`, `data/mods/Survivor_Minds/`).
Backrooms Expanded builds on TGWeaver's Backrooms mod.

**Adding entries:** put each new change at the top of "New entries" with the date and the right category and tag,
and add the detailed line to that mod's `CHANGELOG.md` too.

---

## New entries

*(newest first)*

- 2026-11-15, Developer tools: [Launcher] **CDDA Fork Launcher** is now in the fork (`tools/launcher/`). It installs and updates **Xbro10 tester builds** (the `testing-build-*` releases) by default, says when a new one is out, and updates itself from the `testing` branch. Official CDDA experimental builds are still one click away.
- 2026-11-14, NPC brain: [C++] **Step 3.4a, getting water:** very thirsty NPCs (Water 50+) walk to water they can see up to 20 tiles away, drink, and fill an empty bottle at clean water. They say "goes to get water", the look-around panel shows the plan, and after 3 failed tries they give up for an hour.
- 2026-11-14, NPCs and factions: [SM] **Gabriel is never Timid:** his personality is fixed instead of rolled (Freedom fighter, Brave and Hero), to fit "I don't run".
- 2026-11-14, NPC behaviour: [SM] The follower menu has one **"Got a minute?"** entry for all Survivor Minds options (journal, gossip, marks, food, feelings, Gabriel's chat) instead of six.
- 2026-11-14, Game options and fixes: [C++] Saving in the middle of eating (or any action on an item in a bag) no longer loses the item on load: NPCs are found while they load, and items in pockets keep their identity across a save (no more "item_location" error pop-ups).
- 2026-11-14, NPC brain: [C++] The **Food** need now only counts body weight once an NPC is underweight, so it drops as soon as they eat (a normal-weight NPC read about 46 and never went down).
- 2026-11-14, NPCs and factions: [SM] **Gabriel's look:** light skin, long blond hair, green eyes, a cyan hoodie and a blue wool scarf (its own sprite in UltiCa and UndeadPeople).
- 2026-11-14, Developer tools: [SM] **Watch Gabriel:** `EOC_SM_DEBUG_WATCH_GABRIEL` spawns an unrecruited Gabriel nearby and hides you completely (invisible, silent, no scent, no damage, sees through walls) so you can watch him on his own; `EOC_SM_DEBUG_WATCH_STOP` ends it.
- 2026-11-14, NPCs and factions: [SM] **Gabriel**, a unique recruitable NPC (class Brawler): a veteran fighter with a morningstar and a hatchet, short on words, hates teamers, never snitches. Ask "Want to team up?" and tell him you trust him. Debug spawn `EOC_SM_DEBUG_SPAWN_GABRIEL` (his real spawn point comes later).
- 2026-11-14, NPCs: [BE] **Breakroom runners**: two named runners really travel Level 1 on errands for Dot, bring back extra for the supply locker, and sometimes go missing, which opens a "Find the runner" job.
- 2026-11-14, Levels: [BE] **Level 1 lights**: lane lights flicker and slowly fail for good, and lobbies sometimes black out for up to an hour (a Smiler may come).
- 2026-11-14, Monsters: [BE] **Faceling, Skin-stealer and Partygoer**: a harmless blank-faced wanderer, a skin-wearing mimic that calls for help, and a very rare, very fast grinning partygoer, each with its own sprite.
- 2026-11-14, Monsters: [BE] **The Redeemer** (boss, not placed yet; debug spawn `mon_br_redeemer`): a car-sized three-headed beast with power poles on its back, shock storms and an electric bite, with its own large 128x96 sprite.
- 2026-11-14, Missions and jobs: [BE] **Breakroom standing.** 3 jobs earn Dot's supply key (a weekly ration locker in the Breakroom); 6 jobs earn Marcus's hand-drawn map and extra almond water. Ask Dot "Where do I stand with you lot?". Job rewards now fit each job.
- 2026-11-14, Structures and places: [BE] The Level 1 **security office** has a gun safe (about 2 in 5 hold a sidearm and a little ammo).
- 2026-11-14, Items and resources: [BE] **Oil-stained notes:** activate one on Level 1 to mark a generator room 3-8 squares away on your map.
- 2026-11-14, Items and resources: [BE] **Loot is a themed handful:** every crate, locker, heap and shelf holds 3-6 items of one kind instead of whole stores.

---

## Everything so far (Nov 12–14, 2026)

### Levels and dimensions
- [BE] **Level 1, the Parking Deck:** an endless concrete parking garage. Pillars, bay lines, ceiling lights that may be dead.
- [BE] **Level 2, Pipe Dreams:** hot, narrow maintenance tunnels laid out as a maze, lit by the odd caged bulb.
- [BE] **Level 0 → 1:** rare rippling walls you step through. You land at the nearest rippling wall, or a new alcove forms. Followers come along.
- [BE] **Level 1 → 2:** about 1 stairwell in 4 is open, and it collapses behind you. **2 → 1:** rare service ladders up to a floor hatch.
- [BE] **Level 1 → 0:** only a rare elevator ride (about 1 in 50).
- [BE] **Level 2 climate:** about 30 C all year, with humid air, sweating, thirst and heat stroke. Steam leaks make it hotter still.

### Structures and places
- [BE] Level 1 arrival alcoves (sealed behind you), with a torn note from someone who came before.
- [BE] The **Breakroom**: a walled survivor hub, one per world, marked on your map.
- [BE] **Squatter camps** (4–8 per map) and **overrun camps** (blood, a body, gear, a last note).
- [BE] **Dead zones:** 2x2 patches where every light has died. More danger, better stashes.
- [BE] **Elevator lobbies** (3–6 per map): safe, lit, with a lost-and-found bin. The panel goes to **any floor**, **back** or **home**, and rides can go wrong in 6 ways.
- [BE] **Security office**, **generator rooms** (2 L of gas lights the area for a week), padlocked **storage cages**.
- [BE] **Vending alcoves**, attendant booths, maintenance and electrical rooms, junk piles, leaking pipes, sealed stairwells, wrecked and parked cars.
- [BE] "The exit ramp that isn't" (up into a brick wall), and graffiti with arrows and warnings.
- [BE] Level 2 **boiler rooms** (hot, with tool lockers), in variants: normal, cold, and pump room.
- [BE] Level 2 **valve junctions** (turn the BLEED wheel to shut steam leaks for 2 days), in variants: burst main and control closet.
- [BE] Level 2 **flooded dips** (knee-deep warm water), in variants: drowned traveller and draining.
- [BE] Level 2 **Tunnel Crew camps** in old pump rooms.

### Monsters
- [BE] **Smiler:** a grin in the dark that avoids bright light.
- [BE] **Hound:** packs of 2–3 that hunt by sound.
- [BE] **Deathmoth:** swarms that gather at working lights.
- [BE] **Wretch** (day 30+): a survivor who stayed too long.
- [BE] Level 1 and 2 spawn lists: rats, Hound packs, Smilers in the dark, Deathmoths at bulbs, a few zombies; later cockroaches and Wretches.
- [C++] New monster flags `AVOIDS_LIGHT` and `SEEKS_LIGHT`, used by the Smiler and the Deathmoth.

### Items and resources
- [BE] Almond water on Level 1 (rare). It now also gives a calm mood boost.
- [BE] Water: drip pools (boil before drinking) and clean taps in the Breakroom. NPCs drink from them on their own.
- [BE] Siphonable cars: about 1 in 4 still has a few litres of fuel.
- [BE] Notes: arrival notes, crumpled notes, sweat-stained Level 2 notes (not all true).
- [BE] Loot tables for Level 1 (tools, car parts, batteries; rare food) and Level 2 (pipe, scrap, tools; even less food).
- [SM] **Stick of chalk** (craftable from limestone or quicklime).
- [SM] **Survivor's journal** (every NPC carries one; it can be read, traded or found on a body).

### NPCs and factions
- [BE] Factions: **The Breakroom**, **The Lost** and the **Tunnel Crew**. Their stores count as stealing.
- [BE] **Dot Kowalczyk** ("the Foreman") and **Marcus "Mile Marker" Reyes** in the Breakroom.
- [BE] Random lost survivors in camps, with their own fall-through stories. They can be recruited and traded with.
- [BE] Tunnel Crew (2–4 per camp): they talk, trade, can be recruited, and know rumours of a way further down.

### Missions and jobs
- [BE] **Dot:** Fuel run, Thin the Hounds, The Hound den, Who they were.
- [BE] **Marcus:** Supply drop, Ride the lifts. Rewards are supplies, almond water and standing.

### NPC behaviour (Survivor Minds, JSON)
- [SM] **Journals:** a morning entry about sleep, food, water and mood, plus event entries (fights, kills, rescues, deaths).
- [SM] **Wall marks:** chalk LOOTED / CAMP / DANGER / WATER (with an arrow). Followers mark looted places on their own.
- [SM] **Routines:** bedtime between 10 PM and 4 AM, a morning food check, and idle mood lines (extra ones at night and in the Backrooms).
- [SM] **Morals:** followers react to killing someone helpless or to cannibalism. Disapproval builds up; at 10 they leave you.
- [SM] **Sharing food** builds trust. New option "How do you feel about me?".
- [SM] **Gossip:** "Seen anything around here?" covers hostiles, water and looted marks, plus a daily map tip.
- [SM] **Danger warnings:** followers call out threats with a verdict, by voice or over the radio.
- [SM] Panicked civilians are a quarter as common.

### NPC brain (C++, branch `npc-autonomy`; option "NPC autonomy (experimental)", off by default)
- [C++] **3.1:** the game option, the behavior-tree gate, and quests are cancelled when a follower leaves.
- [C++] **3.2:** needs from 0 to 100 (food, water, sleep, warmth, safety, social), shown when you look at an NPC.
- [C++] **3.3:** NPCs eat at breakfast, lunch and dinner; lonely NPCs walk over to chat.

### Scenarios
- [BE] **Wrong Exit**, **Breakroom Hire** and **One of the Lost**, all starting on Level 1.

### Sprites and tilesets
- [BE] Overmap sprites for Level 1 and 2 places, redrawn in UndeadPeople style. They show in UltiCa and UndeadPeople.
- [BE] Level 0 walls and floors: yellow wallpaper, damp carpet, light panels.
- [BE] Entity sprites for the Smiler, Hound, Deathmoth and Wretch.
- [SM] A journal book sprite.

### Game options and fixes
- [C++] Legacy sandbox world options restored (game world, monsters, world time, character options).
- [C++] City size, city spacing and NPC spawn time are back as % world options.
- [C++] Fixed a crash when opening character creation.
- [C++] Debug "Spawn NPC follower" now adds the NPC to your follower list.
- [BE] Followers come along through Backrooms portals. Cars no longer block arrival alcoves.

### Developer tools
- [SM] Debug EOCs: `EOC_SM_DEBUG_DANGER`, `EOC_SM_DEBUG_GOSSIP_RESET`, `EOC_SM_DEBUG_MORALS_*`, `EOC_SM_DEBUG_ROUTINE`, `EOC_SM_DEBUG_MARK_LOOTED`.
- [C++] Unit tests for NPC autonomy (`tests/npc_autonomy_test.cpp`).
