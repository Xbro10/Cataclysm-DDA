# Tester guide

Thanks for helping test Xbro10's fork of Cataclysm: Dark Days Ahead.
This `testing` branch is the build to play. It only changes when Xbro10 updates it.

## What's in this build

- **Backrooms Expanded** (mod): new Backrooms levels (the Parking Deck, Pipe Dreams), places, monsters, NPCs and jobs.
  It builds on TGWeaver's Backrooms mod. New this time: Level 1 lights that flicker, fail and black out lobbies;
  the Faceling, Skin-stealer and Partygoer; Breakroom runners who travel Level 1 and can go missing; Breakroom
  standing, a supply locker and job rewards; loot caches.
- **Survivor Minds** (mod): NPCs with inner lives: journals, chalk wall marks, routines, reactions to what you do,
  gossip and danger warnings. New: **Gabriel**, a unique recruitable NPC, and one "Got a minute?" entry in the
  follower menu for all of it.
- **NPC autonomy** (built into the game, off by default): NPCs with needs who eat at mealtimes, seek company and,
  new this time, go and get water when they're very thirsty.
- **The launcher** (`tools/launcher/`): installs this build, tells you when a new one is out, and keeps your saves.

What changed is listed in each mod's `CHANGELOG.md` (`data/mods/Backrooms_Expanded/`, `data/mods/Survivor_Minds/`).

## Getting the build

**Easiest (Windows): the launcher.** It downloads the newest tester build, tells you when there's a new one, and
keeps your saves, settings and mods when it updates.

1. Make a folder with a short path, for example `C:\Games\CDDA` (not inside Program Files).
2. From this branch, copy the `tools/launcher` folder's contents into it: `Start CDDA Launcher.bat` and the
   `Launcher` folder. (Green **Code** button > **Download ZIP** gets you the whole branch; you only need those two.)
3. Double-click `Start CDDA Launcher.bat`. If Windows asks, allow it. Press **Install / update**, then **PLAY**.

From then on, open the launcher instead of the game. When Xbro10 posts a new build, it says so when it starts
and offers to install it. The launcher also updates itself. Your saves live in the launcher's `UserData` folder;
**Back up my saves now** (Folders & backups tab) zips them.

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
