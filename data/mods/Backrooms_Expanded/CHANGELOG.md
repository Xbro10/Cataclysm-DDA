# Changelog

All notable changes to this mod are listed here, newest first.
Each release gets a dated entry with Added / Changed / Fixed / Removed sections.

## Unreleased

### Added
- Level 1, the Parking Deck: a new dimension of endless concrete parking garage (pillars, faded bay lines, ceiling lights that may be dead).
- Level 1 variety: puddles, leaking pipe runs, junk piles, guaranteed working lights, maintenance and electrical rooms, an attendant booth (best odds of food and drink), sealed stairwells with misleading signs, and the odd wrecked car.
- Level 1 loot (tools, car parts, hardware, batteries; food and water are rare) and monsters (mostly rats, some zombies; cockroaches after day 45).
- Rippling walls: a rare room on the ground floor of Level 0 (3 to 6 per map) whose back wall you can step through into Level 1. Followers within 5 tiles come along.
- Level 1 arrival alcoves with a rippling concrete wall that takes you back to the spot you left on Level 0.
- Developer shortcuts `EOC_BR1_DEBUG_GO` / `EOC_BR1_DEBUG_BACK` (Debug menu > Activate EOC) to visit Level 1 before the real transitions exist.
- Mod skeleton: modinfo.json, README and this changelog.

### Changed
- You now land where you are when crossing between Level 0 and Level 1. If no arrival alcove (or thin-walled room on the way back) is close, one forms around you. Leaving Level 1 brings you out under where you were, not where you went in.
- The debug shortcuts now take the same route as the real rippling walls.

### Fixed
- Cars can no longer block an arrival alcove: any car touching the alcove or the 3 tiles in front of its opening is removed (the parking bays elsewhere keep all six car spots).
- Followers now arrive with you through rippling walls (they were left stranded where you first landed). Followers within 20 tiles come along and land on free tiles next to you.

### Removed
