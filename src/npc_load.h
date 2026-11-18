#pragma once
#ifndef CATA_SRC_NPC_LOAD_H
#define CATA_SRC_NPC_LOAD_H

#include <vector>

#include "item_location.h"

class npc;
class item;

// Xbro10 fork: with NPC autonomy on, an NPC carrying more than it can carry puts down junk it picked up
// itself, cheapest per kilo first, until it is back under its limit. Things it needs or was handed stay.
namespace npc_load
{
// Never put down: favourites (what the player gave or traded them), firewood, and anything that is or
// holds food, drink, medicine, or ammo and magazines for the gun in their hands. Clothes they wear and
// their weapon are never considered at all.
bool keeps( const npc &guy, const item &it );

// What they would put down right now, in order, to get back under their weight limit (empty when
// they aren't over it or have nothing they'd part with).
std::vector<item_location> what_to_drop( npc &guy );

// Called from npc::move when not in danger. Starts a drop activity and says so; true if it did.
// Checks at most once a minute per NPC.
bool lighten_load( npc &guy );
} // namespace npc_load

#endif // CATA_SRC_NPC_LOAD_H
