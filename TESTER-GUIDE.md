# Tester guide

Thanks for helping test Xbro10's fork of Cataclysm: Dark Days Ahead.
This `testing` branch is the build to play. It only changes when Xbro10 updates it.

## What's in this build

- **Backrooms Expanded** (mod): new Backrooms levels (the Parking Deck, Pipe Dreams), places, monsters, NPCs and jobs.
  It builds on TGWeaver's Backrooms mod.
- **Survivor Minds** (mod): NPCs with inner lives: journals, chalk wall marks, routines, reactions to what you do,
  gossip and danger warnings.
- **NPC autonomy** (built into the game, off by default): NPCs with needs who eat at mealtimes and seek company.

What changed is listed in each mod's `CHANGELOG.md` (`data/mods/Backrooms_Expanded/`, `data/mods/Survivor_Minds/`).

## Getting the build

1. Download this branch: the green **Code** button > **Download ZIP**, or `git clone -b testing https://github.com/Xbro10/Cataclysm-DDA.git`.
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
- the last part of `config/debug.log` (in the game folder)
- a save if you can: zip your world's folder from `save/`

One bug per Issue. Ideas and feedback are welcome as Issues too.

## Please don't

- push to any branch: only Xbro10 changes them
- share the build as your own; credit Xbro10 and TGWeaver
