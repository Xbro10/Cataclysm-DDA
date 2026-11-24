# Fork changelog

Everything Xbro10's fork adds to Cataclysm: Dark Days Ahead, newest first. Backrooms Expanded builds on
TGWeaver's Backrooms mod. Each mod's own `CHANGELOG.md` has the full detail
(`data/mods/Backrooms_Expanded/`, `data/mods/Survivor_Minds/`).

| Tag | Part of the fork |
|---|---|
| **[BE]** | Backrooms Expanded mod: levels, places, monsters, items, jobs |
| **[SM]** | Survivor Minds mod: NPC journals, wall marks, routines, gossip |
| **[AD]** | Additions mod: things added to the regular world, no Backrooms needed (on by default) |
| **[C++]** | A change to the game itself (the NPC brain, options, bug fixes) |
| **[Launcher]** | The launcher in `tools/launcher/` |

**Jump to:** [Nov 24](#november-24-2026) · [Nov 23](#november-23-2026) · [Nov 21](#november-21-2026) · [Nov 20](#november-20-2026) · [Nov 19](#november-19-2026) · [Nov 18](#november-18-2026) · [Nov 17](#november-17-2026) · [Nov 16](#november-16-2026) · [Nov 15](#november-15-2026) · [Nov 14](#november-14-2026) ·
[Nov 12–14, the first version](#november-1214-2026-the-first-version) · [How to add an entry](#how-to-add-an-entry)

---

## November 24, 2026

### NPCs
- [C++] **A follower's favourites look different from yours:** what you hand or trade to a follower becomes their
  favourite (so they never drop it), and its `*` now shows in light red, the nearest the game has to orange. Your own
  favourites keep the plain `*`; a follower's older favourites get the light red mark when the game loads them. When such an item comes back to you it stops being a favourite (before, your
  inventory slowly filled with favourites you never set); something you had favourited yourself stays yours.
- [C++] **Pickup rules by category:** in a follower's "Set up pickup rules." or the new **Pickup rules** entry when you examine a follower (`k`) (and your own auto-pickup rules), press
  `g` for a **Categories** checklist: each category is Take, Never or "-" (your other rules decide). Rules can also be
  typed as `c:food` or `c:ammo,guns`. Material rules (`m:kevlar`) now work for followers too (they were ignored).
  An Exclude rule now always wins: a follower no longer picks up an excluded pipe or plank because it beats their fists.
- [C++] **Followers leave junk:** with NPC autonomy on and no pickup rules, a follower no longer takes scrap, pipes,
  planks, rags, nails, rocks, empty cans, vehicle parts or corpses, nor anything a kilogram or more worth almost nothing.
  Food or drink they need, ammo for their gun and their own arrows always count. They also go for the best item in
  reach instead of the farthest one.

## November 23, 2026

### Backrooms
- [BE] **Level 0 is quieter:** a Level 0 chamber now has monsters half as often (a 20% chance per map square instead of
  upstream's 40%). Places you've already been keep the monsters they had.
- [BE] **Level 0 places:** 12 small named rooms to come across while exploring Level 0, each shown on the map by its
  name, 2-4 of each per map. Intact: a cubicle farm, a waiting room and a janitor's closet. Ruined: a collapsed
  office, a burned break room and a flooded conference room. Ones that make no sense: a cosy living room, a birthday
  party with nobody there, a room of ten doors opening onto closets, a tipped room with all its furniture heaped in
  one corner, stairs that climb into the ceiling and stop, and an office overgrown with moss. Loot is small; most
  hold junk, a few a little food.
- [BE] **A bit more food on Levels 1 and 2, still scarce:** Level 1 has loading docks (a shut crate sometimes still
  holds canned food or snacks) and attendants' break rooms (the odd snack in the cupboards), plus the odd shopping
  cart with groceries spilled round it. Level 2 has crew messes, where the lockers hold lunchboxes, and now and then a
  lunchbox left on a pipe ledge in the tunnels.

### Bug fixes
- [C++] **No error eating gum from a nearby tile:** chewing gum (or another medicine you chew or use) eaten from a
  tile or container next to the eater, as NPCs do, popped up "Couldn't find item caffeinated chewing gum to form
  item_location".
- [C++] **No crash drawing a bow or gun:** pressing fire with empty hands and picking a bow in a quiver (or a gun in a
  holster, or one on a shoulder strap) from "Draw what?" crashed the game. The menu's choices pointed at a list that
  was already gone (an upstream bug).

### NPCs
- [C++] **Faster NPC turns with autonomy on:** an NPC's safety checks looked over every creature in sight and rated
  their own weapon several times per action; now once per action. In the benchmark, autonomy's extra cost per NPC turn
  is about gone, and an NPC turn with zombies in view takes roughly half as long.

### For developers
- [C++] Debug menu > Player > **Remove all wounds** (`H`) clears every wound on every body part of you (or the NPC
  picked) at once; before, the medical screen's debug option removed them one at a time.
- `[npc_perf]` also times NPC turns with 6 zombies in view. Loading an NPC saved mid-meal no longer logs
  "Invalid or misplaced field name item_type".

### Mods
- **No Wounds** (new, optional, off by default; `data/mods/No_Wounds/`): hits no longer give body parts wounds
  (scratches, cuts, lacerations, bruises), the limb wounds upstream added from experimental 2026-10-25. Damage and
  bleeding work as before. It sets the game's `WOUND_CHANCE` option to 0.

## November 21, 2026

### Backrooms
- [BE] **Breakroom jobs, runners and finds:** handing in a job now says what you earned. The Hound den's pack is 3 or 4
  Hounds that defend their den (bolder, and a flashlight doesn't drive them off). Moth jelly also cleans dirty cuts and
  scratches, as saline does. A runner getting tired on a run heads home to sleep instead of lying down out in the lanes.

### NPCs
- [SM] **Gabriel's two loadouts:** he now spawns as either the Brawler (morningstar, chainmail under his cyan hoodie,
  nasal helmet) or the Spearman (the Additions mod's Flamero-Mado spear, or a steel spear without it, leather
  armour and a wool cloak), picked at random. Same look, talk and personality either way.
- [C++] **NPCs value bows properly:** NPCs took a pair of scissors over a compound bow and refused a bow they were asked
  to use. The game rated a draw bow by its sights, and bows have none, so every shot counted as a pure gamble; and a
  bow holds no arrow until it's fired, so it was rated as a club. Now it's rated by the aim the archer really gets
  (aiming by eye improves with archery skill, as when they shoot) and the arrows they carry. An archer with a quiver
  rates a compound bow above a katana, a recurve or longbow above a combat knife, a short bow about level with one.
  With no arrows it's still a club. Greatbows still need the strength to draw them. All NPCs, not only with NPC
  autonomy on.

### Launcher
- **"Already up to date" pops up:** installing a sound pack you already have says so in a pop-up, not only in the
  status line at the bottom.

### Bug fixes
- [BE] **Level 2 valve junctions** no longer log a mapgen error when a dripping pool's lightstick lay where the
  junction puts its ceiling light.

### For developers
- [C++] **One test for every Backrooms Expanded place:** `backrooms_places_generate_cleanly` (`[backrooms_runner]`,
  about 4 minutes) generates each of the mod's 22 overmap terrains 40 to 200 times and fails on any mapgen error,
  naming the place. It replaces the separate Level 0, garage and tunnel tests; the Level 0 one had been skipping itself
  (it looked up `backfloorchamber`, but the rotatable terrain's id is `backfloorchamber_north`). Add new places to its list.
- [SM] **Gabriel's loadouts:** templates `sm_gabriel` (class `NC_SM_GABRIEL`, Brawler) and `sm_gabriel_spear`
  (`NC_SM_GABRIEL_SPEAR`, Spearman). Survivor Minds' first `mod_interactions/xbro_additions/` file replaces the
  Spearman's weapon group with the Flamero-Mado. Debug `EOC_SM_DEBUG_SPAWN_GABRIEL_BRAWLER` / `_SPEAR`.

## November 20, 2026

### NPCs
- [C++] **Archers and gunners keep their distance:** with NPC autonomy on, an NPC with a bow or gun they may fire and
  something slower right beside them steps to a free tile further from everything hostile, then shoots again, instead
  of clubbing it with the bow. They stay within their follow distance or near their post; against something as fast
  as them, or with nowhere to go, they fight as before.
- [C++] **NPCs pick up the arrows they shot:** a shot arrow or bolt (or something thrown) lands as a new item, so it
  never counted as theirs and was only picked up by chance. Now it does, they look up to 20 tiles away (was 10), and a
  follower may walk up to 15 tiles from you to get their things (their follow distance is only a few tiles).
- [C++] **Archer followers really work now (fixed again):** played out in a test, an archer follower with default
  rules never shot their bow and clubbed zombies with it. Four causes, all fixed:
  - **Bows count as quiet:** "use silent weapons only" (on by default) now allows bows and crossbows; a short bow's
    twang counted as loud, so followers weren't allowed to shoot it at all.
  - **They loose sooner:** with NPC autonomy on, an NPC stops aiming once something could reach them within about two
    turns (they aimed while a zombie walked up, waiting for a "sure" shot at 3 tiles).
  - **They nock the next arrow:** with an empty bow or gun, enemies in view and none beside them, they reload at once
    (the game rated walking up and clubbing above reloading).
  - **Kiting:** something slower right beside them, they step away and shoot again; the game's own step back (which
    made them flee a zombie two tiles off instead of shooting) is off for them. A follower may back off up to 10
    tiles from you to do it, and walk up to 15 tiles from you to pick their arrows up without turning back.
- [C++] **Archers no longer walk back up to the zombie (N18):** after stepping away they shoot from there. Four causes:
  past their follow distance the "follow close" rule made the game ignore the zombie and walk them back to you, past
  it; at two tiles the game rated swinging the bow above a shot; its "don't hit a friend" side-step picked the tile
  nearest the zombie; and a bow counted for almost nothing in their own strength, so one zombie made them "outgunned"
  and every gunner rule switched off. When there's nowhere left to step (10 tiles from you, a wall behind), they shoot
  point-blank instead of clubbing.
- [C++] **Every arrow picked back up (N26):** arrows that stuck in a zombie end up in its corpse, and they take them
  out of it now. One arrow they can't reach no longer makes them stop looking for the rest, they also go for arrows
  that flew on into the dark, and a shot at something further off than they watch still counts as theirs.
- [C++] **Plan line for meals and chats (N27):** the look-around panel says "Plan: have a meal" or "Plan: have a
  chat" too. Chores (camp work, smashing corpses) already show after "Follower;" on the line above.
- [C++] **NPCs remember places (step 5.6, part A):** with NPC autonomy on, each NPC remembers up to 30 places: water
  they drank from (a sink, cooler or pool, not a bottle they found), spots they sheltered at when scared, and where
  they got badly hurt. A thirsty NPC with no water in sight walks back to water they remember ("Name heads for water
  they remember."), within their follow or guard limits, and forgets it if it's gone. Memories fade (danger after 3
  days, water after a month) and the least important go first when full; they save with the NPC, and older saves
  start with nothing remembered. The look-around panel shows "Remembers 7 places".

### Bug fixes
- [C++] **No "item_location UID not found" on loading (N28):** two stacks of the same thing in a bag can merge into one
  when a save loads, and an activity on the stack that merged away (an NPC nocking an arrow) lost its item with that
  error. Saves now note what the item was, and loading falls back to that kind of item in the same place (never a
  different item, as the first try at this fix could). Saves made before this still show the error once.
- [C++] **No more error for a job with nowhere to go:** a stranger's ordinary job aimed at a house can't be done inside
  a Backrooms level; it no longer logs "Unable to find and assign mission target" or "Mapgen update function called
  with invalid tripoint".
- [C++] **Mod sprites for the other tileset aren't errors:** each of our mods has one sprite file for UltiCa and one for
  UndeadPeople, and the game logged an error for whichever didn't match your tileset. Now it's a plain log line.

### For developers
- [C++] The hidden test `Character_attacking_a_zombie` (`[.melee]`) no longer crashes: its unarmed case read the
  wielded item without checking there was one (an upstream test bug, on master too). It runs now and misses two of
  its expected hit chances by a little (48% for 50-70%, 74.6% for 75-85%): upstream's numbers drifted; left as is.

## November 19, 2026

### Additions
- [AD] **Specter (shock lance):** the Flamero-Mado's sibling, a rare end-game spear that runs on a medium battery. Switched on,
  every hit adds electric damage, about one hit in three stuns, and forked blue arcs crackle and lash out from its head (animated, both
  tilesets, on the ground and in your hands). Labs, military gear, and police and prison armouries; crafted from its schematic.
  [BE] With the Backrooms too: Level 1 gun safes and the Exchange's rare find.
- [C++] **Switched-on weapons keep their special effects:** turning a weapon on never gave it the "on" form's
  built-in effects, so the lit Flamero-Mado never actually set things on fire. Now it does, and the Specter stuns.

### Backrooms
- [BE] **Tally tokens are the Backrooms' money:** brass tokens, five to a bottle of almond water. Every Exchange
  stall, the diner, the Breakroom and the Level 2 crews deal in them and keep a stack to pay you with; Breakroom jobs
  and standing rewards pay in tokens instead of almond water, and loose tokens replace cash cards in Level 1 loot.
- [BE] **Light tools against the dark things:** a **work lamp** (a battery floodlight to set down, far brighter than a
  flashlight), a **lightstick bundle** (five glowsticks taped together: about ten minutes of very bright light) and a
  **UV torch** whose violet beam [C++] doubles the burn on a Smiler (wielded, switched on, within 10 squares and in
  view). All three are craftable, sold at the Exchange's gear stall and found with other lights; sprites for both
  tilesets, the lit bundle and the UV beam animated. A plain flashlight still burns Smilers too.
- [BE] **Weaknesses that work in play:** [C++] Skin-stealers and Hounds back off from a flashlight, lamp or fire
  (`FEARS_LIGHT`; the game's bright-light fear only counted near-daylight). Deathmoths really burn in fire (powdery
  wings, and no insect fire fear; a ground fire barely hurt flyers). Fewer brown rats on Level 1, which slowed it
  down. [AD] The Flamero-Mado spear no longer sets things alight after you put it out.
- [BE] **Hound dens and Breakroom job fixes:** Hound dens are a Level 1 place with four layouts (den, abandoned, a
  survivor's last stand, feeding ground); the den job no longer errors, "Who they were" always has its body, the
  3- and 6-job rewards wait for Dot, door guards spear zombies over the fence, the board shows a runner resting at
  home, and almond water rewards no longer error.
- [BE] **Breakroom jobs are handed in:** a job no longer finishes out in the lanes (the Fuel run took the gas from
  your pack on the spot). Go back and pick "About that job..." with Dot or Marcus to hand it in and get the reward;
  Hound kills count from when you take the job.
- [BE] **Guards hold their posts, runners rest indoors:** the Breakroom's door guard and archers and the Exchange's
  guards go back to their own post after a fight or a meal; a runner back from a run goes inside to its own spot to
  rest and heal instead of standing in the yard. [C++] Arrows, bolts and thrown things fly through chain-link, bars
  and arrow slits (bullets already did), and NPCs shoot through them instead of walking round or climbing the fence:
  the Breakroom archers can fire out of the fenced yard. Jobs that send you somewhere you haven't seen (the Hound
  den, "Who they were") no longer fail on a revealed map ("Unable to find and assign mission target").
- [BE] **Ruth, the Breakroom's medic:** in the infirmary (or by the Bunker's bunks). Every 10 minutes she treats the
  Breakroom's own people for free; you pay in tally tokens (5 to patch up, 20 for a full treatment with 2 hours on her
  cot; 3 and 10 after 3 jobs), and while one of hers is badly hurt she only patches you up.

### NPCs
- [C++] **Followers stand with you:** with NPC autonomy on, followers and your guards no longer run off on their own
  when a fight goes badly (no "is panicking!" retreat, no walking off to a safe spot). They fight or hold beside you,
  stepping back a tile or two from a crowd only while still within their follow distance or near their post. A new
  follower rule, "fall back when a fight goes badly", lets them retreat again, and then they regroup on you instead
  of running anywhere. Strangers still run as before.
- [SM] **Guards far away still keep their journal:** a guard you left at a distant post all morning wrote nothing that
  day (entries only counted from 6 to 10 AM, and NPCs out of range don't write). Now they write it as soon as you're
  back near them. New debug shortcut `EOC_SM_DEBUG_JOURNAL_NOW` makes followers, guards and camp workers write an
  entry right away, to test the lonely and scared lines.

### Launcher
- [C++] **Discord card: days survived and followers:** every card now says how long you've survived
  ("First day", "Survived 12 days") and, with followers, how many ("Survived 2 days · 1 follower"). Detailed still
  adds sleeping or driving.

### Bug fixes
- [C++] **NPCs stop smashing corpses to fight:** a follower could keep pulping a dead zombie while a live one chewed on
  her; the corpse job stayed through the fight and came back whenever the fight logic paused. Now an NPC drops it
  (and won't start one) with something hostile close by, and a hostile right beside them counts even in the dark.
- [BE] **Moth jelly is fresh:** [C++] a Deathmoth's jelly is as fresh as the kill (the game aged every death drop
  from the first day, so weeks in it dropped rotten). **The royal ration** no longer freezes the debug spawn menu (it
  listed two vitamins the game no longer has, and each one popped an error every redraw). Level 1's attendant
  booth and the garage's lights, puddles and junk no longer cause mapgen errors where they land on each other.
- [C++] **Archers shoot over sandbags:** arrows, bolts and thrown things fly over low cover you can see over and climb
  (a half sandbag wall), as bullets already did, and NPCs count the shot as clear. The Breakroom archers in their
  sandbag nests now shoot through the yard's chain-link instead of standing there.
- [C++] **A weapon's melee damage always lists Bash, Cut, Pierce in order:** the order depended on how the game
  was built, so the same spear could show "Pierce" before "Bash".

### For developers
- [C++] **Synced with upstream CDDA** (66 commits: partial dodge and limb shielding, wound and quest fixes, cash
  cards retired, `map.cpp` and `npctalk.cpp` split into more files). No conflicts. Upstream's use actions now take
  an `item_location`, so the NPC hearth-lighting code and the Backrooms item tests were updated to match.

## November 18, 2026

### Backrooms
- [BE] **The Exchange has three guards:** one at the gate and two in the back corners, in body armour and army
  helmets, with a machete, steel spear or metal bat and a loaded Glock, strong and skilled enough to use them.
- [BE] [SM] **Survivor Minds traces in the Backrooms:** with both mods on, an overrun camp's dead survivor carries
  their journal (four stories of a camp's last days), CAMP and WATER marks lead to squatter and tunnel crew camps,
  DANGER marks edge the dead zones, and LOOTED marks sit in picked-over places (about 1 in 5 Level 1 maintenance
  rooms, booths and security offices come emptied). Either mod still works alone.
- [BE] **Backrooms entities have weaknesses:** [C++] Smilers burn in bright light (switch on a flashlight mid-fight
  and one dies in a few turns; Level 0's ceiling lights are too dim). Hounds break when you stand your ground,
  Deathmoths fly into fires and burn, Wretches take double damage from burning weapons, Skin-stealers back off from light.
- [AD] **Additions, a new mod on by default:** additions to the regular world, no Backrooms needed. First in it is the
  **Flamero-Mado spear**, a rare end-game spear you fill with kerosene or gasoline and light; lit, it burns as well as
  pierces and about one hit in three sets the target on fire. Found in museums and mansion collections; crafted only
  from its schematic. Sprites for UltiCa and UndeadPeople. [BE] With both mods on, it also turns up in Level 2 crew
  stores, at the Exchange and on the Redeemer; Backrooms Expanded works with or without Additions.
- [BE] **Entity loot and proper butchery:** butchering a Hound, Deathmoth or the Redeemer gave human flesh (the game's
  default); now they give mutant meat, fat, bone and hide. Hounds drop fangs, Smilers sometimes their teeth, Facelings
  their clothes and pocket things, Skin-stealers the skin they wore, Partygoers a red balloon and party hat, the
  Redeemer a cracked insulator. The Exchange buys the trophies.
- [BE] **More for the Backrooms finds:** the lit Flamero-Mado spear sets what it hits on fire about one hit in three;
  craftable firesalt torches (twice as long as a fuel torch) and firesalt twists (a quick fire-starter); the Exchange
  sells moth jelly, royal rations and firesalt, Level 2 crews carry firesalt to trade, and no trader buys cloudy water.
- [C++] **NPCs never drink cloudy water** (liquid pain) on their own: a food or drink flagged `NPC_REFUSES` by a mod
  is never picked. [SM] NPCs you hand moth jelly, royal rations or cloudy water write about it in their journal.
- [BE] **Loot comes packed:** the All-Nite serves chili and soup in a bowl and coffee in a cup (chili used to spawn
  as a loose puddle), and the security office pistol has its magazine in it. A new test spawns every mod loot table
  and fails on anything loose that should be packed.
- [BE] **Backrooms Wiki finds:** moth jelly from Deathmoths (food that heals a little), firesalt on Level 2 (burns
  as lamp fuel, so it also fills the Flamero-Mado spear), rare royal rations (most of a day's food, never spoil) and
  cloudy water, liquid pain in an almond water bottle that hurts to drink. Each has a sprite, and the Flamero-Mado
  spear now shows in your character's hands, burning when lit. Breakroom runners' Glocks start loaded.
- [BE] **Trading overhaul (the Exchange and the All-Nite diner):** traders and the cook sell only their stall stock,
  never the gun or gear they carry; each has a clear trade with a few kinds of goods in deep piles, says what they
  sell ("What do you sell?", "What's cooking today?"), has a sign or menu board, buys only their own kind of goods
  and pays extra for it.
- [BE] **Breakroom archers can draw their bows:** every bow needs Strength 9 and some archers rolled less, so they
  kept trying to wield it ("needs at least strength 9"). Archers get +5 Strength, and enough skill for their knife.
- [BE] **Exchange stalls keep their own stock:** each trader has a stall faction and their own loot zone, so the
  supplies trader no longer lists the gear stall's tools; stall crates no longer roll camp loot; the diner cook sells
  from the counter. **Stronger runners:** top combat skills, a machete, a Glock 19 with spare magazines and a first aid
  kit; a badly hurt runner heads home early.

### NPCs
- [SM] **Journals mention loneliness and fear (step 5.4):** with NPC autonomy on, a follower's morning journal entry
  adds a line when they've been alone a long time ("Haven't talked to anyone in days.") or are scared as they write
  ("Couldn't settle.  Kept watching the dark."), four wordings each. Official builds never show it.
- [C++] **Shopkeepers go back to their booth:** with NPC autonomy on, a shopkeeper (like the Exchange traders) or a
  sheltering stranger scared off their spot looks for safety only within 6 tiles of it, and walks back once calm,
  as guards return to their post. They remember the spot where they first stood calm, so a trader already standing
  in the aisle in an old save takes the aisle as their spot.
- [C++] **A well-armed NPC isn't scared of one weak monster, even up close:** Safety now only rises when what an NPC
  sees matches or beats what it could fight on its own, at any range (a katana fighter read Safety 50 from a single
  Hound). Meals, chats and trips still wait while anything hostile is within 3 tiles.
- [C++] **NPCs draw a weapon when trouble shows up:** empty-handed (or holding a tool or wood) with hostiles in view,
  they draw the best weapon they carry within a few turns (before, only when idle or once an hour, so a katana handed
  over mid-chase stayed in the pack). A weapon already in hand is never swapped by this: an earlier version pulled a
  pistol over a drawn machete while the game's attack choice picked the machete for a zombie next to them, so a
  Breakroom runner swapped back and forth every turn and never attacked.
- [C++] **NPCs pick their things back up after a fight:** with NPC autonomy on, a weapon knocked away or set down
  mid-fight, arrows and bolts they shot, or anything else they carried that ended up on the ground nearby is
  fetched once the fight is over (within 10 tiles they can see and may go to, for 30 minutes), the weapon back into
  their hands. "Name picks their things back up." Things used up are just gone, and wood carried for a fire stays put.
  A weapon that lands right beside them is grabbed straight back, mid-fight too ("Name grabs their weapon back.").
- [C++] **Chats keep to follow and guard rules (step 5.3):** with NPC autonomy on, a follower only walks over to chat
  with someone within their follow distance, and follows you instead if you walk off on the way; a guard on shift
  chats with someone by their post (before, guard duty always won and guards never chatted, even side by side); an NPC you sent somewhere goes there first. Strangers with no orders still walk up to
  15 tiles to chat. Meals, water and food trips, warmth and seeking safety already kept to these limits.
- [C++] **Overloaded NPCs lighten their load (with NPC autonomy on):** an NPC carrying more than it can puts down
  the cheapest junk it picked up itself ("Too heavy.  Leaving the ... here."). It keeps its clothes, weapon, ammo,
  food, drink, medicine, firewood and favourites, and what you trade to a follower counts as a favourite, so a
  follower you load up on purpose just walks slower.

### Bug fixes
- [C++] **Picking a scenario no longer starts your character over:** stats, skills, traits, looks (hair, eyes,
  skin) and your profession and backgrounds stay, as long as the new scenario allows them. Only what it doesn't
  allow is dropped, plus traits that came with the old scenario.
- [C++] **Ambient volume changes apply in game:** lowering Ambient sound volume or Sound effect volume and leaving
  Options fades the playing ambient loops (rain, wind, night) so they come back at the new volume; before, they kept
  the old volume until a restart.
- [C++] **No scenario pop-up in plain worlds:** the "Last Character" template from a Backrooms world no longer
  pops up "Tried to use non-existent scenario" when you start a world without the Backrooms; it just uses the default
  scenario.
- [C++] **A stranger who ran from monsters no longer flees from you for a day:** running from zombies switched them
  to the game's "fleeing" attitude, which also meant fleeing the player for 24 hours, so they kept running from you
  after you had killed what scared them. Now they just run from the monsters for that run, without the "fleeing"
  attitude, so no more "is no longer afraid" over and over while they keep running (unless they already feared you).

### For developers
- [BE] **Optional hooks for another mod:** `survivor_minds_hooks.json` defines empty nested chunks (`br_sm_mark_*`, weight 0, so
  they exist but never place anything) and the item group `br1_overrun_body`; `mod_interactions/survivor_minds/` gives
  them content when Survivor Minds is loaded, plus weighted picked-over copies of three Level 1 rooms. Hidden test
  `[backrooms_sm]` (`--mods=backrooms,backrooms_expanded,survivor_minds`).
- [C++] **`--check-mods` handles mod_interactions like a real world:** it loaded every file in a mod's folder,
  interaction folders included, so a mod with an interaction folder for another mod failed the check without it.
- [C++] **Test runs no longer fail after a data file changes:** the first run of the test program after any JSON
  edit logged "Stale game data detected" as an error per changed file, so it ended with "Treating result as failure
  due to error logged during initialization" even when every test passed. The test program now turns that check off
  (`--option WARN_ON_MODIFIED:true` turns it back on), as this build's own config already does.
- [C++] **NPC autonomy checks are 15-25 times cheaper:** the Safety check looked for hostiles twice per call and
  checked line of sight to every creature in view, friends included, several times per NPC turn. It now scans once,
  only checks sight to hostiles, and rates the NPC's weapon only when a hostile is in view (`[npc_perf]`: Safety
  0.023 to 0.001 ms; an autonomous NPC's turn about 0.7 to 0.2-0.3 ms). Same answers as before.
- [C++] The four "anything in reach?" answers kept for a few turns share one helper (`recent_answers`) that drops
  stale answers, instead of keeping one for every NPC ever met.

## November 17, 2026

**First releases:** Backrooms Expanded v0.1.0 and Survivor Minds v0.1.0, together in one release on the GitHub
Releases page ("Mods v0.1.0", tag `mods-v0.1.0`), with a download for each mod. Both READMEs now list everything
built so far.

### Backrooms
- [BE] **Breakroom archers and supply depot:** two archers with bows guard the front of each Breakroom layout, and a
  depot of food, water, medicine and tools fills back up every time a runner gets home.
- [BE] **Backrooms entities are their own faction:** they never fight each other, always attack zombies and other
  outsiders, and leave Facelings alone.
- [BE] **Breakroom fixes from play:** runners really leave now (they looked only 12 squares away for a camp, lobby or
  generator room, found nothing and stayed home; now 30 squares, or a scavenging run through the lanes). The Bunker's
  archers stand in sandbag nests, and archers draw a knife and sling the bow when something gets close instead of
  dropping the bow.

### NPCs
- [C++] **Scared NPCs head for safety (step 5.2):** with NPC autonomy on, an NPC who is scared (Safety 50+) but not
  fighting or fleeing yet walks to the safest spot it may reach: a lit one (where Smilers won't follow), near friendly
  people and away from the hostiles it can see. "Name backs into the light." It holds there until calm again (Safety
  under 25, at least two minutes). Followers stay within follow distance, guards near their post. The look-around
  panel shows "Plan: stay safe (…)".
- [C++] **Safety notices more, but sizes things up:** hostiles an NPC can see within 10 tiles (it was 3) make Safety 50+
  when together they match or beat what the NPC could fight on their own (armour, dodge and the weapon in hand, by
  the game's own ratings). Unarmed, a zombie 8 tiles off is scary; with a katana it isn't, but three are, and a zombie
  brute is until they're skilled; a hulk always is. Anything within 3 tiles still counts. Meals, chats and trips wait
  while scared.
- [C++] **No more pacing back and forth** between two hostiles: once an NPC heads for a safe spot it keeps going until
  calm (Safety under 25 and two minutes since it was last scared), and only switches to a clearly better spot.
- [C++] **Gunners stand and shoot:** with NPC autonomy on, an NPC holding a gun it may fire (loaded, or ammo to
  reload) that outguns what it sees no longer backs away from walkers it should be shooting. It aims at the nearest
  hostile within its gun's sure range and 10 tiles (a follower within your engagement rules) and fires before it gets
  close; it only takes a short step back from something slower within 2 tiles. Out of ammo, too slow, or
  outgunned (a rifle beats zombies and brutes, not a hulk), it fights or flees as before. Followers still only use
  loud guns when your rules allow it.
### Launcher
- [C++] **Discord status card:** while you play, Discord shows which level or world you're in (with the launcher picture),
  how long you've played and a **Get the fork** button. Options > General > Discord: Off, Basic or Detailed (adds
  the day, your followers, and sleeping or driving). Needs the Discord desktop app.
- [Launcher] **Sound packs:** a new **Sound** page downloads and updates **CC-Sounds** (music and effects) and
  **@'s soundpack** into `UserData\sound`, where game updates never touch them, and **Use this pack** makes the game
  use it. **Menu sounds only** switches back. Launcher version 2026-11-17.

### Bug fixes
- [BE] **Breakroom runners** now really leave at the time on the runner board (the schedule couldn't find them).
  New worlds only.
- [BE] **Exchange traders and the All-Nite cook** lay out their whole stock to trade, not one or two items.
- [C++] **No more "item_location lost its target item during a save/load cycle" pop-ups** after loading: something
  being used from a pile of identical items on the ground or in a vehicle (eating, picking up, hauling) lost track of
  its item, because the save keeps only one id for the whole pile.
- [C++] **Character creator: facial hair can be changed again.** Taking off "Facial hair: none" to pick a beard showed
  "my_traits and my_mutations were out of sync" and did nothing, because random looks gave it the wrong way.
- [C++] **NPC autonomy: a plank carried for the fire is put down straight away** once the NPC no longer needs the
  fire (warmed up, or called away), and they take their weapon back out. Before, they could hold it for up to a
  game minute.

### For developers
- [C++] `src/discord_presence.{h,cpp}`: a small Discord IPC client (named pipe or Unix socket, no library),
  hooked in `main.cpp` and `game::do_turn`, at most one update per 15 s. Settings and pictures per dimension in
  `data/discord_presence.json` (application id empty = off). Card picture (the launcher icon): `tools/backrooms_expanded/draw_discord_art.ps1`.
  Test `[discord_presence]`.

---

## November 16, 2026

### Backrooms
- [BE] **New sprites** (UltiCa and UndeadPeople): the Manila Room's walls, deep and dark carpet, pooled fluid, and map
  signs for the Exchange and the All-Nite diner.
- [BE] **The All-Nite diner:** a diner on Level 1 with a cook who sells hot meals, a server full of gossip,
  regulars in the booths, and a stocked kitchen where NPCs come to eat.
- [BE] **The Exchange:** a fenced market on Level 1 with three traders (supplies, gear, oddities), a guard and
  a locked storeroom. Stock refreshes every few days, and almond water is the best thing to pay with.
- [BE] **Level 0 room variety:** archway rooms (rings of arches over deep, slow carpet), blackout zones (unlit rooms
  with ankle-deep pools, where Smilers wait) and pillar halls (whole squares of open floor and pillars).
- [BE] **The Manila Room:** a rare, small, dry room on Level 0 with manila walls. It always has food in the cupboard
  and almond water on the table, plus a few random finds.
- [BE] **Breakroom larder:** the Breakroom keeps its own food on the kitchen counters and water in the fridge, enough
  for a few days. With NPC autonomy on, its people eat from it; taking from it yourself is stealing. Each runner who
  gets home brings a fresh batch. Its crates and lockers are stocked far more often too.
- [BE] **Breakroom lockers** no longer crowd the doorways.

### NPCs
- [C++] **Getting warm (step 3.4c):** cold NPCs (Warmth 50+) put on warmer clothes, walk to a fire burning in a
  fireplace, wood stove or fire pit and stay until warm, or, with a lighter or matches, gather firewood (never
  anything owned), carry it to an unlit hearth and light it. They only ever light fires in a hearth. They say
  "goes to gather wood", "carries the wood to the fireplace" and "lights a fire", and give up after 3 failed tries.
  When a cold NPC can't warm up, the look-around panel says why (nothing to light a fire with, no fireplace close
  enough, no firewood in sight, or no room to carry it), on its own line under their needs. While warming up
  they keep the fire going, bigger the colder the day (1 to 3 pieces of wood on it: 10 °C or warmer, 0-10 °C,
  freezing), adding wood from their pack or fetching some first ("adds wood to the fire"), and say "warms up by
  the fire" once. Wood too big for their pack (planks) goes in their hands, one piece a trip, their weapon put
  away meanwhile and taken back out after; a plank they fight with is never burned. They keep fetching wood
  until the fire is big enough (before, one step towards more wood turned them back to the fire), also when Very
  cold (the game's own warming code used to take over then, which never adds wood and shows no plan). A cold NPC also puts on
  warmer clothes they carry, instead of waiting until Very cold.
- [C++] **Debug: owned items:** `EOC_AUTONOMY_DEBUG_OWNED_AREA` (Debug → Game → Activate EOC) makes every item lying
  in your map square belong to the Free Merchants, to check that NPCs leave owned food, water and wood alone.
- [C++] **NPCs avoid traps:** they never step onto a harmful trap they can see (pits, spikes, bear traps), so a
  hungry NPC no longer walks through spiked pits to reach food.
- [C++] **Getting food (step 3.4b):** very hungry NPCs (Food 50+) eat from their pack, or walk to food they can
  see up to 20 tiles away (in containers, cupboards and fridges too, never anything owned) and eat it there.
  They say "goes to get food", and after 3 failed tries they give up for an hour.
- [C++] **Followers keep up:** a follower you walk away from follows first and eats once close again, even when
  hungry or underweight. Only Food or Water 100 still makes them eat on the spot.
- [C++] **Safety notices close danger:** a hostile within 3 tiles now raises Safety (a lone zombie right next to
  them used to read as Safety 2).
- [C++] **No stale plans:** food and water trips they've given up on are dropped, so the look-around panel only
  shows what they're really doing.
- [SM] Followers only chalk **LOOTED** after you've taken something from the square (0.5 kg or more).

### Bug fixes
- [BE] **Level 2 tunnels** no longer show a mapgen error when a ceiling light lands on a junk pile.
- [BE] **Breakroom runners' deaths** are recorded again (the game refused to run their death script).
- [C++] **No more "Tried to get wound bruise_bad" pop-ups** in fights: when heavy bruising got worse in two ways at
  once (severe bruising and torn skin), the game lost track of the bruise. It could also crash.

### For developers
- [C++] **Synced with upstream CDDA** (30 commits: faults, wounds, factions linked to monster factions). No
  conflicts; every fork change is intact.
- [C++] The NPC autonomy tests now start each case at 20 C. Upstream's `tree_tick_full_returns_score` forces
  -30 C and never undoes it, so the water and food in later tests froze and 3 of them failed in a full test run.
- [C++] **Character creator sprite preview:** with a tileset on, the creator shows your character's sprite (hair,
  facial hair, eye colour, skin, other traits with a sprite, and your profession's clothes), large on the TRAITS
  tab and small at the right end of General Info on every tab. It changes as you pick traits, gender or outfit.
  `cata_tiles::get_character_preview` plus `draw_character_sprite_preview` in `newcharacter.cpp`.

---

## November 15, 2026

### Backrooms
- [BE] **Survival Difficulty Classes** from the Backrooms Wiki: Level 0 and 1 are Class 1, Level 2 is Class 3.
  You see the class in scenario descriptions, on a card when you first arrive and in a new field guide.
- [BE] **The class means something:** Level 2 has about twice the monsters, much less food and bursting steam
  pipes. Level 0 gets a few rare Backrooms entities.
- [BE] **Pipe Rat**, a new scenario: a hard start alone on Level 2 with almost nothing.
- [BE] **A bigger Breakroom** in one of two layouts (Checkpoint: a lit, fenced yard; Bunker: an airlock and a
  sandbag line), with a door guard and a **runner board** showing when Jonah and Rosa leave each day.
- [BE] The Breakroom is **no longer marked on your map** when you reach Level 1. You find it by exploring.
- [BE] **Level 0 lights** sit in neat rows every 4 tiles (a few dead) instead of being scattered at random.
- [BE] **Level 0 runs faster:** its ceiling lights are dimmer but it's still brightly lit everywhere, and lighting
  takes about 40% less time.

### NPCs
- [C++] **Getting water finds more:** drinks in a bottle or other container on the floor, and water coolers,
  heaters and tanks (up to 20 tiles). "Gives up looking for water" only counts water they couldn't reach.
- [C++] **Faster NPC turns:** searching for water and food is far cheaper, which matters with many NPCs around.
- [C++] **Followers don't stand eating while you leave:** past their follow distance they stop and follow.
- [C++] **Guards eat on their post** at mealtimes, from their pack.
- [C++] **New follower rule "leave food and water to me"** (Follower rules screen): no water trips or mealtimes
  of their own. Off by default.
- [SM] **"Got a minute?"** can turn a follower's automatic LOOTED marks off and on.
- [SM] **Gossip no longer marks places on your map** (it knew far-off places even for brand-new NPCs). It comes
  back with NPC memory.

### Launcher
- [Launcher] **The CDDA Fork Launcher is in the fork** (`tools/launcher/`). It installs and updates Xbro10 tester
  builds, says when a new one is out, and updates itself. Official CDDA experimental builds are one click away.
- [Launcher] **New look:** a sidebar with the infinity-hexagon icon, Game / Mods / Saves & backups / Settings
  pages, and Black, Dark blue and Light themes.
- [Launcher] **CDDA Fork Launcher.exe** replaces `Start CDDA Launcher.bat`: it carries the icon and opens the
  launcher with no console window.

### Bug fixes
- [C++] **Followers no longer get left behind** when changing levels or dimensions: one busy with a chore (eating,
  picking something up) drops it and comes along, and one filed under a neighbouring overmap is found.
- [C++] **NPCs fight instead of casting harmless spells:** NPCs with Mind Over Matter powers used to cast
  Clairvoyance over and over instead of using their weapon.
- [BE] **Breakroom runners** never left on a run, and the "Find the runner" job marked the wrong place.

### For developers
- [C++] `EOC_AUTONOMY_DEBUG_LONELY` / `EOC_AUTONOMY_DEBUG_NOT_LONELY` set Social to 100 / 0 for NPCs within
  30 tiles, to test chatting.
- [C++] The water and food searches check the tile before line of sight: a thirsty NPC's water search went from
  about 3 ms to 0.06 ms, and the game's own food search is about 15 times faster. The water search also runs at
  most every 5 turns.
- [BE] Level 0's ceiling lights are brightness 40 instead of 120. Benchmark: `tests/backrooms_perf_test.cpp`.

---

## November 14, 2026

### Backrooms
- [BE] **Level 1 lights:** lane lights flicker and slowly fail for good, and lobbies sometimes black out for up to
  an hour (a Smiler may come).
- [BE] **Breakroom runners:** two named runners travel Level 1 on errands for Dot, bring back extra for the
  supply locker, and sometimes go missing, which opens a "Find the runner" job.
- [BE] **Faceling, Skin-stealer and Partygoer:** a harmless blank-faced wanderer, a skin-wearing mimic that calls
  for help, and a very rare, very fast grinning partygoer, each with its own sprite.
- [BE] **The Redeemer** (a boss, not placed in the world yet): a car-sized three-headed beast with power poles on
  its back, shock storms and an electric bite, with its own large sprite.
- [BE] **Breakroom standing:** 3 jobs earn Dot's supply key (a weekly ration locker), 6 jobs earn Marcus's
  hand-drawn map and extra almond water. Ask Dot "Where do I stand with you lot?". Job rewards now fit each job.
- [BE] The Level 1 **security office** has a gun safe (about 2 in 5 hold a sidearm and a little ammo).
- [BE] **Oil-stained notes:** activate one on Level 1 to mark a generator room 3–8 squares away on your map.
- [BE] **Loot is a themed handful:** every crate, locker, heap and shelf holds 3–6 items of one kind instead of
  whole stores.

### NPCs
- [SM] **Gabriel**, a unique recruitable NPC (class Brawler): a veteran fighter with a morningstar and a hatchet,
  short on words, hates teamers, never snitches. Ask "Want to team up?" and tell him you trust him.
- [SM] **Gabriel's look:** light skin, long blond hair, green eyes, a cyan hoodie and a blue wool scarf, with his
  own sprite in UltiCa and UndeadPeople.
- [SM] **Gabriel is never Timid:** his personality is fixed (Freedom fighter, Brave and Hero), to fit "I don't run".
- [SM] The follower menu has **one "Got a minute?" entry** for all Survivor Minds options (journal, gossip, marks,
  food, feelings, Gabriel's chat) instead of six.
- [C++] **Getting water (step 3.4a):** very thirsty NPCs (Water 50+) walk to water they can see up to 20 tiles
  away, drink, and fill an empty bottle at clean water. After 3 failed tries they give up for an hour.
- [C++] The **Food** need only counts body weight once an NPC is underweight, so it drops as soon as they eat.

### Bug fixes
- [C++] **Saving mid-meal no longer loses the food:** saving while anyone is using an item in a bag kept it on
  load, with no more "item_location" error pop-ups.

### For developers
- [SM] `EOC_SM_DEBUG_SPAWN_GABRIEL` spawns Gabriel (his real spawn point comes later).
- [SM] `EOC_SM_DEBUG_WATCH_GABRIEL` spawns an unrecruited Gabriel and hides you completely (invisible, silent,
  no scent, no damage, sees through walls) so you can watch him; `EOC_SM_DEBUG_WATCH_STOP` ends it.
- [BE] Debug spawn for the Redeemer: `mon_br_redeemer` (its sprite is 128x96).

---

## November 12–14, 2026: the first version

### Backrooms levels and travel
- [BE] **Level 1, the Parking Deck:** an endless concrete parking garage, with pillars, bay lines and ceiling
  lights that may be dead.
- [BE] **Level 2, Pipe Dreams:** hot, narrow maintenance tunnels laid out as a maze, lit by the odd caged bulb.
- [BE] **Level 0 → 1:** rare rippling walls you step through. You land at the nearest rippling wall, or a new
  alcove forms. Followers come along.
- [BE] **Level 1 → 2:** about 1 stairwell in 4 is open, and it collapses behind you. **2 → 1:** rare service
  ladders up to a floor hatch.
- [BE] **Level 1 → 0:** only a rare elevator ride (about 1 in 50).
- [BE] **Level 2 climate:** about 30 °C all year, with humid air, sweating, thirst and heat stroke. Steam leaks
  make it hotter still.

### Backrooms places
- [BE] Level 1 arrival alcoves (sealed behind you), with a torn note from someone who came before.
- [BE] **The Breakroom:** a walled survivor hub, one per world, marked on your map (unmarked since Nov 15).
- [BE] **Squatter camps** (4–8 per map) and **overrun camps** (blood, a body, gear, a last note).
- [BE] **Dead zones:** 2x2 patches where every light has died. More danger, better stashes.
- [BE] **Elevator lobbies** (3–6 per map): safe, lit, with a lost-and-found bin. The panel goes to any floor,
  back or home, and rides can go wrong in 6 ways.
- [BE] A **security office**, **generator rooms** (2 L of gas lights the area for a week) and padlocked
  **storage cages**.
- [BE] Vending alcoves, attendant booths, maintenance and electrical rooms, junk piles, leaking pipes, sealed
  stairwells, and wrecked and parked cars.
- [BE] "The exit ramp that isn't" (up into a brick wall), and graffiti with arrows and warnings.
- [BE] Level 2 **boiler rooms** (hot, with tool lockers): normal, cold and pump room.
- [BE] Level 2 **valve junctions** (turn the BLEED wheel to shut steam leaks for 2 days): burst main and control
  closet.
- [BE] Level 2 **flooded dips** (knee-deep warm water): drowned traveller and draining.
- [BE] Level 2 **Tunnel Crew camps** in old pump rooms.

### Backrooms monsters
- [BE] **Smiler:** a grin in the dark that avoids bright light.
- [BE] **Hound:** packs of 2–3 that hunt by sound.
- [BE] **Deathmoth:** swarms that gather at working lights.
- [BE] **Wretch** (day 30+): a survivor who stayed too long.
- [BE] Level 1 and 2 spawns: rats, Hound packs, Smilers in the dark, Deathmoths at bulbs and a few zombies; later
  cockroaches and Wretches.

### Backrooms items, people and jobs
- [BE] **Almond water** on Level 1 (rare). It also gives a calm mood boost.
- [BE] **Water:** drip pools (boil before drinking) and clean taps in the Breakroom. NPCs drink from them on
  their own.
- [BE] **Siphonable cars:** about 1 in 4 still has a few litres of fuel.
- [BE] **Notes:** arrival notes, crumpled notes and sweat-stained Level 2 notes (not all true).
- [BE] **Loot** for Level 1 (tools, car parts, batteries; rare food) and Level 2 (pipe, scrap, tools; even less
  food).
- [BE] **Factions:** The Breakroom, The Lost and the Tunnel Crew. Their stores count as stealing.
- [BE] **Dot Kowalczyk** ("the Foreman") and **Marcus "Mile Marker" Reyes** run the Breakroom.
- [BE] **Lost survivors** in camps, with their own fall-through stories. They can be recruited and traded with.
- [BE] **Tunnel Crew** (2–4 per camp): they talk, trade, can be recruited, and know rumours of a way further down.
- [BE] **Jobs from Dot:** Fuel run, Thin the Hounds, The Hound den, Who they were.
- [BE] **Jobs from Marcus:** Supply drop, Ride the lifts. Rewards are supplies, almond water and standing.
- [BE] **Scenarios:** Wrong Exit, Breakroom Hire and One of the Lost, all starting on Level 1.

### NPC behaviour (Survivor Minds)
- [SM] **Journals:** every NPC carries one, with a morning entry about sleep, food, water and mood, plus entries
  for fights, kills, rescues and deaths. It can be read, traded or found on a body.
- [SM] **Wall marks:** a craftable stick of chalk writes LOOTED, CAMP, DANGER or WATER (with an arrow).
  Followers mark looted places on their own.
- [SM] **Routines:** bedtime between 10 PM and 4 AM, a morning food check, and idle mood lines (extra ones at
  night and in the Backrooms).
- [SM] **Morals:** followers react to killing someone helpless or to cannibalism. Disapproval builds up, and at
  10 they leave you.
- [SM] **Sharing food** builds trust. New option "How do you feel about me?".
- [SM] **Gossip:** "Seen anything around here?" covers hostiles, water and looted marks, plus a daily map tip
  (switched off on Nov 15).
- [SM] **Danger warnings:** followers call out threats with a verdict, by voice or over the radio.
- [SM] Panicked civilians are a quarter as common.

### NPC brain (option "NPC autonomy (experimental)", off by default)
- [C++] **Step 3.1:** the game option, and quests are cancelled when a follower leaves.
- [C++] **Step 3.2:** needs from 0 to 100 (food, water, sleep, warmth, safety, social), shown when you look at
  an NPC.
- [C++] **Step 3.3:** NPCs eat at breakfast, lunch and dinner, and lonely NPCs walk over to chat.

### Look (sprites)
- [BE] Overmap sprites for Level 1 and 2 places, redrawn in UndeadPeople style. They show in UltiCa and
  UndeadPeople.
- [BE] Level 0 walls and floors: yellow wallpaper, damp carpet, light panels.
- [BE] Sprites for the Smiler, Hound, Deathmoth and Wretch.
- [SM] A journal book sprite.

### Game options and fixes
- [C++] **Sandbox world options are back** (game world, monsters, world time, character options).
- [C++] **City size, city spacing and NPC spawn time** are back as % world options.
- [C++] Fixed a crash when opening character creation.
- [C++] Debug "Spawn NPC follower" now adds the NPC to your follower list.
- [BE] Followers come along through Backrooms portals, and cars no longer block arrival alcoves.

### For developers
- [C++] New monster flags `AVOIDS_LIGHT` and `SEEKS_LIGHT`, used by the Smiler and the Deathmoth.
- [C++] Unit tests for NPC autonomy (`tests/npc_autonomy_test.cpp`).
- [SM] Debug EOCs: `EOC_SM_DEBUG_DANGER`, `EOC_SM_DEBUG_GOSSIP_RESET`, `EOC_SM_DEBUG_MORALS_*`,
  `EOC_SM_DEBUG_ROUTINE`, `EOC_SM_DEBUG_MARK_LOOTED`.

---

## How to add an entry

1. Find today's date heading (`## Month D, YYYY`) at the top. If there isn't one yet, add it above the newest day,
   with a `---` line between days, and add it to "Jump to".
2. Put the line under the right section, in this order: **Backrooms**, **NPCs**, **Launcher**, **Bug fixes**,
   **For developers**. Add the section if the day doesn't have it yet.
3. Write it for a player: the tag, a **bold name**, then a sentence or two. One change per line. Debug ids,
   timings and file paths go under "For developers".
4. Also add the detailed line to that mod's `CHANGELOG.md` under `## Unreleased`.
