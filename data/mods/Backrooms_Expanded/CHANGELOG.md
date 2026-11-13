# Changelog

All notable changes to this mod are listed here, newest first.
Each release gets a dated entry with Added / Changed / Fixed / Removed sections.

## Unreleased

### Added
- Three scenarios that start on Level 1. **Wrong Exit**: you start in an arrival alcove with your out-of-gas car in the lane outside, plus a flashlight and some road-trip food. **Breakroom Hire** (1 point): you start inside the Breakroom next to the Foreman, the Breakroom likes and trusts you, and the nearest squatter camp is marked on your map. **One of the Lost** (-1 point): you start inside a squatter camp with its survivors, the Lost like you, and the Breakroom is cooler towards you. Upstream's two Backrooms scenarios are still available.
- Water on Level 1: drip pools under leaking pipes and inside every squatter camp (never run dry; boil before drinking), and two rigged taps of clean water in the Breakroom. NPCs within 6 tiles drink from them on their own.
- Vending alcoves: one or two stocked vending machines (snacks and drinks) under a working light. Pay with a cash card or break in.
- The Breakroom: a walled survivor hub on Level 1 with Dot Kowalczyk ("the Foreman"), who explains Level 1, and Marcus "Mile Marker" Reyes, who keeps the maps, can mark the nearest camp for you, and has rumours of what lies beyond. It forms once per world, 4 to 10 squares from your first arrival, and is marked on your map.
- Squatter camps: 4 to 8 sandbagged camps per map with 2 to 4 random lost survivors who can be talked to, traded with and recruited, each with their own story of falling through.
- Two factions: The Breakroom and The Lost. Each owns its camp's stores, so taking from them counts as stealing.
- Level 1, the Parking Deck: a new dimension of endless concrete parking garage (pillars, faded bay lines, ceiling lights that may be dead).
- Level 1 variety: puddles, leaking pipe runs, junk piles, guaranteed working lights, maintenance and electrical rooms, an attendant booth (best odds of food and drink), sealed stairwells with misleading signs, and the odd wrecked car.
- Level 1 loot (tools, car parts, hardware, batteries; food and water are rare) and monsters (mostly rats, some zombies; cockroaches after day 45).
- Rippling walls: a rare room on the ground floor of Level 0 (3 to 6 per map) whose back wall you can step through into Level 1. Followers within 5 tiles come along.
- Level 1 arrival alcoves with a rippling concrete wall that takes you back to the spot you left on Level 0.
- Developer shortcuts `EOC_BR1_DEBUG_GO` / `EOC_BR1_DEBUG_BACK` (Debug menu > Activate EOC) to visit Level 1 before the real transitions exist.
- Mod skeleton: modinfo.json, README and this changelog.

### Changed
- You now land where you are (always at ground level on Level 1) when crossing between Level 0 and Level 1. You land just inside the nearest rippling wall within 30 tiles (including the room you came in through); only if there is none does a new alcove or thin-walled room form around you. Leaving Level 1 brings you out under where you were, not where you went in.
- The debug shortcuts now take the same route as the real rippling walls.

### Fixed
- Cars can no longer block an arrival alcove: any car touching the alcove or the 3 tiles in front of its opening is removed (the parking bays elsewhere keep all six car spots).
- Followers now arrive with you through rippling walls (they were left stranded where you first landed). Followers within 20 tiles come along and land on free tiles next to you.

### Removed
