# Tester guide

Thanks for helping test Xbro10's fork of Cataclysm: Dark Days Ahead.
This `testing` branch is the build to play. It only changes when Xbro10 updates it.

## What's in this build

- **Backrooms Expanded** (mod): new Backrooms levels (the Parking Deck, Pipe Dreams), places, monsters, NPCs and jobs.
  It builds on TGWeaver's Backrooms mod. Every level has a **Survival Difficulty Class** (from the Backrooms Wiki),
  shown in the scenario descriptions, on a card when you first arrive and in a field guide you start with; Level 2
  (Class 3) has twice the monsters, almost no food and bursting steam pipes. A hard start, **Pipe Rat**, alone on
  Level 2. Exits and the Breakroom aren't marked or explained: find them yourself, or listen to the people down there.
  The **Breakroom** is bigger and comes in one of two layouts (a lit, fenced **Checkpoint** or a sandbagged
  **Bunker**), with a door guard, a **runner board** and its own larder.
  Level 0 has the rare **Manila Room** (always food and almond water), **archway rooms**, **blackout zones** and
  **pillar halls**; Level 1 has **the Exchange** (a fenced market with three traders) and **the All-Nite diner**.
  The Breakroom has two archers with bows and a supply depot that runners restock; Backrooms entities are one
  faction (they never fight each other and attack zombies).
  New this time (start a **new world** to see most of it): a **trading overhaul** at the Exchange and the diner:
  traders and the cook sell only their stall stock (never their own gun or gear), each has a clear trade with deep
  piles of a few goods, says what they sell ("What do you sell?", "What's cooking today?"), has a stall sign or menu
  board, and buys only their own kind of goods. The **runners** are the Breakroom's best fighters now (machete,
  pistol, first aid kit) and come home early when badly hurt. The **Flamero-Mado**, a fuel-burning spear the
  Redeemer can drop, with its own sprite. Fixed: runners really leave (they used to stay home, "Done for today"),
  each Exchange stall shows only its own stock, Bunker archers stand in sandbag nests, and archers can draw their
  bows and use a knife up close instead of dropping the bow.
- **Survivor Minds** (mod): NPCs with inner lives: journals, chalk wall marks, routines, reactions to what you do,
  gossip and danger warnings, and **Gabriel**, a unique recruitable NPC. Gossip no longer marks places on your map,
  and you can tell a follower to stop marking places as looted ("Got a minute?").
- **NPC autonomy** (built into the game, off by default): NPCs with needs who eat at mealtimes, seek company and go
  and get water or food when they're very thirsty or hungry (up to 20 tiles, in cupboards and fridges too). Cold NPCs
  get warm: warmer clothes, a burning fireplace or stove, or firewood gathered (carried by hand if it won't fit the
  pack) for a hearth fire sized to the day's cold, kept going. They never walk into a harmful trap they can see. The
  follower rule **leave food and water to me** turns their own trips off.
  New: **scared NPCs seek safety**: they back into the light (where Smilers won't follow), near friends and away
  from what scares them, and size threats up against their own weapon and armour. **Gunners stand and shoot**
  instead of backing away. Shopkeepers and guards go back to their spot once calm.
- **The base game** is updated to the latest CDDA experimental (bone setting and wound fixes, crafting faults, and more).
- **Fixes in the game itself**: followers no longer get left behind when you change levels; saving while you or an
  NPC is eating or using an item in a bag no longer loses the item on load; NPCs with Mind Over Matter powers fight
  instead of casting a harmless spell; a heavy bruise getting worse no longer crashes the game; facial hair can be
  changed again in the character creator; changing Ambient sound volume in Options takes effect at once.
- **New in the game itself**: the character creator shows your character's **sprite** while you pick traits and
  looks, and a **Discord "Playing" card** shows where you are (Options > General > Discord status: off, basic or
  detailed).
- **The launcher** (`tools/launcher/`): installs this build, tells you when a new one is out, and keeps your saves.
  New: a **Sound** page that downloads the **CC-Sounds** or **@'s soundpack** sound packs and switches the game to
  the one you pick.

What changed is listed in each mod's `CHANGELOG.md` (`data/mods/Backrooms_Expanded/`, `data/mods/Survivor_Minds/`).

## Getting the build

**Easiest (Windows): the launcher.** It downloads the newest tester build, tells you when there's a new one, and
keeps your saves, settings and mods when it updates.

1. Make a folder with a short path, for example `C:\Games\CDDA` (not inside Program Files).
2. From this branch, copy the `tools/launcher` folder's contents into it: `CDDA Fork Launcher.exe` and the
   `Launcher` folder. (Green **Code** button > **Download ZIP** gets you the whole branch; you only need those two.)
3. Double-click `CDDA Fork Launcher.exe`. If Windows SmartScreen warns you, click **More info > Run anyway** (it
   isn't signed). Press **Install / update**, then **PLAY**. **Settings > Create desktop shortcut** adds it to your desktop.

From then on, open the launcher instead of the game. When Xbro10 posts a new build, it says so when it starts
and offers to install it. The launcher also updates itself. Your saves live in the launcher's `UserData` folder;
**Back up my saves now** (Saves & backups page) zips them. Settings also has the theme (Black, Dark blue, Light).
If you set the launcher up before it had an .exe, your old `Start CDDA Launcher.bat` still works, and the next
launcher update puts the .exe next to it.

**Without the launcher:** download the newest **Testing build** zip from the
[Releases page](https://github.com/Xbro10/Cataclysm-DDA/releases), unzip it anywhere (not inside Program Files)
and run `cataclysm-tiles.exe`. If SmartScreen warns you, click **More info > Run anyway**.
You'll have to check the Releases page for new zips yourself.

**Building it yourself, or on another system:**

1. Download this branch (switch the branch menu from `master` to `testing` first; `master` is plain CDDA): the green **Code** button > **Download ZIP**, or `git clone -b testing https://github.com/Xbro10/Cataclysm-DDA.git`.
2. Pick one:
   - **Full build (Windows):** Visual Studio 2022 with C++ and vcpkg. Open `msvc-full-features\Cataclysm-vcpkg-static.sln`,
     choose Release x64 and build `Cataclysm-vcpkg-static`. The game is `cataclysm-tiles.exe` in the main folder.
     Other systems: `doc/c++/COMPILING.md`.
   - **Mods only:** copy `data/mods/Backrooms_Expanded` and `data/mods/Survivor_Minds` into an official CDDA
     experimental build's `data/mods/`. You won't get NPC autonomy or the light-shy Smiler, but everything else works.

## Setting up a world

- New world > Mods: add **Backrooms**, **Backrooms Expanded** and **Survivor Minds**.
- For NPC autonomy: Options > General > **NPC autonomy (experimental)** > on (full build only).
- Tileset: **UltiCa** or **UndeadPeople** show the new sprites.
- To start in the Backrooms, pick a scenario like **Wrong Exit**, **Breakroom Hire** or **One of the Lost**.

## What to test

Play normally, and keep an eye out for:
- error popups or crashes
- things that look wrong (missing sprites, walls in odd places, NPCs stuck)
- anything too easy, too hard or boring
- NPC lines that don't fit what's happening

Xbro10 may post a list of things to test with each update.

## Reporting a bug

Open an **Issue** on GitHub (the **Issues** tab) and include:
- what happened, and what you expected
- where (which level, place or NPC) and the steps to make it happen again
- the error popup text, if any
- which build you have (the launcher shows it at the top, for example "tester 2026-11-14")
- the last part of `config/debug.log` (with the launcher it's in `UserData\config\`; otherwise in the game folder)
- a save if you can: zip your world's folder from `save/` (`UserData\save\` with the launcher)

One bug per Issue. Ideas and feedback are welcome as Issues too.

## Please don't

- push to any branch: only Xbro10 changes them
- share the build as your own; credit Xbro10 and TGWeaver
