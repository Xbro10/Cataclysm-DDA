# Contributing to Xbro10's CDDA fork

For testers and contributors, and for the AI assistant (Claude or another) they work with.
If you're an AI assistant: this file overrides the parts of `CLAUDE.md` that describe Xbro10's own PC
(the `D:\dev` paths, the playbook PDF, the launcher, the memory notes). Everything else in `CLAUDE.md` still applies.

## What this fork is

Cataclysm: Dark Days Ahead plus three pieces of our own:

| Part | Where | What |
|---|---|---|
| Backrooms Expanded | `data/mods/Backrooms_Expanded/` | New Backrooms levels, places, monsters and NPCs. Builds on TGWeaver's Backrooms mod (`data/mods/Backrooms/`). |
| Survivor Minds | `data/mods/Survivor_Minds/` | NPCs with inner lives (journals, wall marks, routines, morals, gossip). Pure JSON. |
| NPC brain | `src/`, branch `npc-autonomy` | C++ NPC autonomy behind the option "NPC autonomy (experimental)", off by default. |

`FORK-CHANGELOG.md` lists everything made so far.

## Branches

| Branch | What it is | Who changes it |
|---|---|---|
| `master` | A copy of upstream CDDA | Nobody by hand |
| `dev` | Mod work (Backrooms Expanded, Survivor Minds) | Xbro10, through pull requests |
| `npc-autonomy` | C++ work, on top of `dev` | Xbro10, through pull requests |
| `testing` | The build testers play | Only Xbro10 moves it |
| `tester/<you>-<topic>` | Your work | You |

- Start mod work from `dev`, and C++ work from `npc-autonomy`: `git switch -c tester/<you>-<topic> origin/dev`.
- Never push to `master`, `dev`, `npc-autonomy` or `testing`. Never force-push a branch that isn't yours.
- Before you start something new, bring in the latest: `git fetch` and `git merge origin/dev` (or `origin/npc-autonomy`).

## How a change goes in

1. **Talk first for anything big.** Open an Issue describing the idea and wait for Xbro10's OK.
   Small fixes can go straight to a pull request.
2. **Work in small steps.** Each step should be something you can see in game.
3. **Check it loads** (see below).
4. **Write the changelog lines:**
   - one in that mod's `CHANGELOG.md`, under `## Unreleased` (Added / Changed / Fixed / Removed)
   - one short line at the top of "New entries" in `FORK-CHANGELOG.md`, with the date, category and tag ([BE], [SM] or [C++]).
5. **Open a pull request** into `dev` (or `npc-autonomy` for C++). Say what it does and give a short in-game test checklist.
6. Xbro10 reviews and merges. Changes reach `testing` when Xbro10 updates it.

## Rules

- **Don't edit `data/mods/Backrooms/`.** That's upstream's copy of TGWeaver's mod. Extend or override it from `Backrooms_Expanded`.
- **Credit TGWeaver** for the original Backrooms mod.
- **Sprites:** every new terrain, furniture, item and monster needs a `looks_like` fallback (or a sprite id) for UltiCa and UndeadPeople.
- **Target current CDDA experimental.** JSON must pass the loader.
- **C++ changes** go behind the "NPC autonomy" option where they change behaviour. Mark each one with a `// Xbro10 fork:` comment, and keep base files to small hooks.
- **Write in-game times on the 12-hour clock** (6 AM, 10 PM).
- **Real people:** Gabriel (Survivor Minds) is based on a real person who agreed to it. Use only his personality and voice, never real-life details. The same goes for anyone else.
- **Don't commit** `save/`, `config/`, build output or personal files.
- License: CC-BY-SA 3.0, like CDDA.

## Building and testing

**Mods only:** copy `data/mods/Backrooms_Expanded` and `data/mods/Survivor_Minds` into an official CDDA experimental build's `data/mods/` (or its user `mods/` folder). The NPC brain and the C++ fixes won't be there, but the mods work. Some monster behaviour (the Smiler avoiding light, the Deathmoth seeking it) needs this fork's build.

**Full build on Windows:** Visual Studio 2022 with C++, plus vcpkg. Open `msvc-full-features\Cataclysm-vcpkg-static.sln`, Release x64, and build `Cataclysm-vcpkg-static` (the game) or `Cataclysm-test-vcpkg-static` (the tests). Other systems: see `doc/c++/COMPILING.md`.

**Check the mods load** (from the repo root, after building):

```
./cataclysm-tiles.exe --check-mods survivor_minds
./cataclysm-tiles.exe --check-mods backrooms backrooms_expanded
```

The exit code can be 0 even when loading failed, so read the output, and search `config/debug.log` for `ERROR`.
`CLAUDE.md` has a list of gotchas worth reading before mapgen or EOC work.

## Reporting bugs

Open an Issue with:
- what happened and what you expected
- where (which level, place, NPC) and the steps to make it happen again
- which branch and commit you built (`git log --oneline -1`)
- any error popup text, and the end of `config/debug.log`
- a save, if you can (zip the world folder from `save/`)

## For AI assistants

- Ask your user before large changes, and follow the workflow above.
- Work only on `tester/...` branches; open pull requests, never merge them.
- Keep the rules above, especially the changelog, the sprite fallbacks and no real-life details.
- Don't change `CLAUDE.md` or this file unless your user asks and Xbro10 agrees.
