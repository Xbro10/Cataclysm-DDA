#pragma once
#ifndef CATA_SRC_NPC_MEMORY_H
#define CATA_SRC_NPC_MEMORY_H

// Xbro10 fork: NPC autonomy, step 5.6 A (place memory). Each NPC remembers up to 30 places
// (water, safe, danger, looted, camp), with where and when they last saw them, in which
// dimension. Kept in one saved NPC var ("autonomy_places") as a short text list, so it saves
// and loads with the NPC and old saves load with nothing remembered. Memories fade (a looted
// mark after a week, danger after 3 days) and, when full, the least important and oldest goes.
// Everything here does nothing with the NPC_AUTONOMY option off.

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "coords_fwd.h"
#include "coordinates.h"

class npc;

namespace npc_memory
{

enum class place_kind : int {
    water = 0,
    safe,
    danger,
    looted,
    camp,
    last
};

struct place {
    place_kind kind = place_kind::water;
    tripoint_abs_ms pos;
    // The dimension it's in (the game's dimension id; empty in the normal world).
    std::string dimension;
    // When they last saw it, in turns since the start of the game.
    int seen = 0;
};

constexpr int max_places = 30;
// Two memories of one kind closer than this are the same place.
int same_place_radius( place_kind kind );

/** What they remember, faded ones left out. */
std::vector<place> places( const npc &guy );
/** How many places they remember (for the look-around panel). */
int count( const npc &guy );
/** Note (or refresh) a place where they are now, in this dimension. Option off: nothing. */
void remember( npc &guy, place_kind kind, const tripoint_abs_ms &pos );
/** Drop a remembered place of this kind near pos (it wasn't there after all). */
void forget( npc &guy, place_kind kind, const tripoint_abs_ms &pos );
/** The nearest remembered place of this kind in the dimension they're in, skipping any that
 *  `usable` (if given) turns down, so an out-of-reach one doesn't hide a nearer usable one. */
std::optional<tripoint_abs_ms> nearest( const npc &guy, place_kind kind,
                                        const std::function<bool( const tripoint_abs_ms & )> &usable = nullptr );

// For tests: the saved text and back.
std::string encode( const std::vector<place> &list );
std::vector<place> decode( const std::string &text );

} // namespace npc_memory

#endif // CATA_SRC_NPC_MEMORY_H
