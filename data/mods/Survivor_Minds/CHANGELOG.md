# Changelog

All notable changes to this mod are listed here, newest first.
Each release gets a dated entry with Added / Changed / Fixed / Removed sections.

## Unreleased

### Added
- Journal event entries, written the moment they happen with the day and time of day: fights (at most one every 4 hours), killing a person, eating after starving (at most every 12 hours), being rescued by you (a best guess: you kill a monster within 6 tiles of an NPC hurt in the last 2 minutes; at most every 6 hours), and a follower dying (the other followers write about them by name, and the dead follower's own journal ends with "[The writing stops here.]"). Four wordings for each.
- Developer shortcut `EOC_SM_DEBUG_JOURNAL_EVENTS` makes followers write one line of each event type right away.
- Survivor's journal: every NPC near you carries one and writes a line each morning (06:00-09:59) about how they slept, food, water and mood, with several wordings for each. Days spent away from you show as blank pages.
- Followers have a new option "Can I read your journal?" Strangers keep theirs to themselves, but a journal can be taken, traded or found on a body and read by activating it.
- Developer shortcut `EOC_SM_DEBUG_JOURNAL_NOW` (Debug menu > Activate EOC) makes followers write a line right away.
- Mod skeleton: modinfo.json, README and this changelog.

### Changed
- The survivor's journal now shows a book sprite in UltiCa instead of the placeholder.
- Panicked people are a quarter as common in city civilian spawns and come in smaller crowds.

### Fixed

### Removed
