# Changelog

All notable changes to this mod are listed here, newest first.
Each release gets a dated entry with Added / Changed / Fixed / Removed sections.

## Unreleased

### Added
- Danger warnings: followers watch for hostile monsters they can see within 30 tiles and speak up on their own when something new shows up or the odds get worse. They name the most dangerous one, say how many there are and where, and weigh the game's own monster threat ratings against your side (you and nearby followers, by melee and gun skill and how hurt you are): "We can take them", "Doable if we stick together", "That's risky" or "Too many. We should go." You hear it in the log when within 12 tiles, or as `[Radio]` when you both carry a two-way radio. Followers you left waiting or guarding a spot report in too. Only one follower reports the same sighting.
- Developer shortcut `EOC_SM_DEBUG_DANGER` prints each follower's raw numbers (hostiles seen, threat, our side, verdict, distance from you, who has a radio) and clears past warnings.
- Gossip (lite): "Seen anything around here?" in the follower menu. Followers report what is really around right now, with a compass direction: the hostiles they can see (named, with a verdict), the nearest drinkable water (Breakroom tap, drip pool, pump, stream or shallow water) and any LOOTED mark nearby. Once a day they also mark a place on your map: a nearby shop or service building, or in the Backrooms a squatter camp (Level 1) or a rippling-wall room (Level 0).
- Developer shortcut `EOC_SM_DEBUG_GOSSIP_RESET` lets followers give their daily map tip again.
- Moral reactions: followers who see you kill someone who wasn't fighting back, or eat human flesh, react to it (kind ones are shocked and trust you less, ruthless ones may approve, followers with the Cannibal trait don't mind), and they write it in their journal. Disapproval builds up and fades by 1 a day; at 7 a follower warns you, at 10 they leave you. Each follower has a kindness and ruthlessness score of their own.
- "Here, have some of my food." in the follower menu: pick a food and it is put at their feet. Sharing makes them like and trust you more (counts once every 6 hours).
- "How do you feel about me?" in the follower menu.
- Developer shortcuts `EOC_SM_DEBUG_MORALS_SHOW`, `EOC_SM_DEBUG_WITNESS_HELPLESS`, `EOC_SM_DEBUG_WITNESS_CANNIBAL` and `EOC_SM_DEBUG_MORALS_RESET`.
- Follower routines: between 10 PM and 4 AM a follower who is getting sleepy is nudged to bedtime (so the game's own sleep logic puts them to bed at night) and says goodnight once; each morning they check their pack and say whether there's food; and every hour or two an idle follower says a line that fits their mood, with extra lines at night and in the Backrooms. Respects the follower rule "allow sleep".
- Developer shortcut `EOC_SM_DEBUG_ROUTINE` makes followers say a routine line and do the food check right away.
- Survivor marks: chalk floor markings for whoever comes next: LOOTED, CAMP, DANGER, and WATER with an arrow (north, east, south or west). Draw them from the construction menu with chalk, a permanent marker, a survival marker or a spray can; examine to read; scrub off with "Deconstruct Simple Furniture" (no tools, 10 seconds).
- Stick of chalk: craftable from limestone or quicklime.
- Followers mark places: "Mark this place as looted." in the follower menu, and when you leave a map square you spent 15+ minutes in, your nearest follower indoors chalks LOOTED where they stand (never within 10 tiles of another LOOTED mark).
- Developer shortcut `EOC_SM_DEBUG_MARK_LOOTED` makes followers try to mark LOOTED right away.
- Journal event entries, written the moment they happen with the day and time of day: fights (at most one every 4 hours), killing a person, eating after starving (at most every 12 hours), being rescued by you (a best guess: you kill a monster within 6 tiles of an NPC hurt in the last 2 minutes; at most every 6 hours), and a follower dying (the other followers write about them by name, and the dead follower's own journal ends with "[The writing stops here.]"). Four wordings for each.
- Survivor's journal: every NPC near you carries one and writes a line each morning (6 AM to 10 AM) about how they slept, food, water and mood, with several wordings for each. Days spent away from you show as blank pages.
- Followers have a new option "Can I read your journal?" Strangers keep theirs to themselves, but a journal can be taken, traded or found on a body and read by activating it.
- Mod skeleton: modinfo.json, README and this changelog.

### Changed
- The survivor's journal now shows a book sprite in UltiCa instead of the placeholder.
- Panicked people are a quarter as common in city civilian spawns and come in smaller crowds.

### Fixed
- No more "<follower> gives you a survivor's journal" popup when a follower who had no journal yet writes their first entry during a conversation (they were handed one behind the scenes).
- The "chalks a big X" message now appears when a follower marks a place LOOTED (scripted NPC messages were being dropped).

### Removed
