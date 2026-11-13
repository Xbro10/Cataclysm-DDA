#pragma once
#ifndef CATA_SRC_NPC_AUTONOMY_H
#define CATA_SRC_NPC_AUTONOMY_H

// Xbro10 fork: "NPC autonomy (experimental)", playbook Phase 3.
// Everything here does nothing unless the NPC_AUTONOMY game option is on,
// so with the option off NPCs behave exactly like upstream.

class npc;

namespace npc_autonomy
{

/** True when the "NPC autonomy (experimental)" option is on. */
bool enabled();

/**
 * Called when a follower leaves the player (dialogue or the EOC "leave" effect).
 * With autonomy on, quests the player got from them are cancelled (failed),
 * since nobody is left to hand them in to.
 */
void on_follower_leaves( npc &guy );

} // namespace npc_autonomy

#endif // CATA_SRC_NPC_AUTONOMY_H
