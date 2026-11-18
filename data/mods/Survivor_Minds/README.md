# Survivor Minds

A Cataclysm: Dark Days Ahead mod that gives NPCs inner lives: journals, wall
markings, daily routines, moral reactions and things to say about the world
around them.

Pure JSON / effect_on_condition, so it runs on official experimental builds.
It works with or without the Backrooms.

**Version 0.1.0** (2026-11-17). Full details in `CHANGELOG.md`.

## What it adds

**Survivor's journals.** Every NPC near you carries a journal and writes in it:
a line each morning about how they slept, food, water and mood, and an entry the
moment something big happens (a fight, killing a person, eating after starving,
being rescued by you, a friend dying). Ask a follower "Can I read your journal?";
strangers keep theirs to themselves, but a journal can be taken, traded or found
on a body and read.

**Survivor marks.** Chalk floor markings for whoever comes next: LOOTED, CAMP,
DANGER, and WATER with an arrow. Draw them from the construction menu with chalk,
a marker or a spray can, examine one to read it, scrub it off when it's wrong.
Chalk can be crafted from limestone or quicklime. Followers mark places too: ask
them to, or they chalk LOOTED on their own when you leave a square you took
things from (you can tell them to stop).

**Routines.** Followers get sleepy and go to bed at night (10 PM to 4 AM) with
a goodnight, check their pack for food each morning, and say a line that fits
their mood every hour or two, with extra lines at night and in the Backrooms.

**Moral reactions.** Followers who see you kill someone who wasn't fighting back,
or eat human flesh, react to it, each by their own kindness and ruthlessness, and
write it down. Disapproval builds up and fades slowly: they warn you, then leave.
Sharing your food makes them like and trust you more. Ask "How do you feel about
me?" to see where you stand.

**Danger warnings.** Followers watch for hostiles they can see and speak up when
something new shows up or the odds get worse: what it is, how many, where, and
whether your side can take it. You hear it nearby, or over a two-way radio.

**Gossip.** Ask "Seen anything around here?" and a follower tells you what's
really around: hostiles they can see, the nearest drinkable water, and any
LOOTED mark nearby, with a compass direction.

**Gabriel.** A unique NPC: a veteran brawler with a morningstar, short on words,
who hates anyone who gangs up on people and never snitches. He has his own talk,
look and sprites. For now he only comes in through a developer shortcut; his
place in the world comes later.

All of it sits under one **"Got a minute?"** entry in the follower menu.

Panicked people are also a quarter as common in city crowds, so towns feel less
like a stampede.

## Install

Copy the `Survivor_Minds` folder into your game's `data/mods/` (or your
user folder's `mods/`), then enable **Survivor Minds** when creating a world.

## Requirements

- A current CDDA experimental build.
- Made with the UltiCa tileset in mind (UndeadPeople is supported too); every
  new ID has a `looks_like` fallback, so other tilesets work.
- Optional: the **Backrooms** and **Backrooms Expanded** mods. Followers have
  their own mood lines for the Backrooms.

## Developer shortcuts

Run these with Debug → Game → Activate EOC:

- `EOC_SM_DEBUG_ROUTINE`: followers say a routine line and check their food now.
- `EOC_SM_DEBUG_MARK_LOOTED`: followers try to mark LOOTED now.
- `EOC_SM_DEBUG_DANGER`: each follower's danger numbers; clears past warnings.
- `EOC_SM_DEBUG_GOSSIP_RESET`: resets the daily gossip tip.
- `EOC_SM_DEBUG_MORALS_SHOW`, `EOC_SM_DEBUG_WITNESS_HELPLESS`,
  `EOC_SM_DEBUG_WITNESS_CANNIBAL`, `EOC_SM_DEBUG_MORALS_RESET`: moral reactions.
- `EOC_SM_DEBUG_SPAWN_GABRIEL`: Gabriel next to you.
- `EOC_SM_DEBUG_WATCH_GABRIEL`: watch mode, to see Gabriel act on his own.

## License

CC-BY-SA 3.0, the same as Cataclysm: Dark Days Ahead.
