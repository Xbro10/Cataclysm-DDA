#pragma once
#ifndef CATA_SRC_NPC_AUTONOMY_H
#define CATA_SRC_NPC_AUTONOMY_H

// Xbro10 fork: "NPC autonomy (experimental)", playbook Phase 3.
// Everything here does nothing unless the NPC_AUTONOMY game option is on,
// so with the option off NPCs behave exactly like upstream.

#include <string>

#include "calendar.h"

class npc;

namespace npc_autonomy
{

/** True when the "NPC autonomy (experimental)" option is on. */
bool enabled();

// ---- Needs (step 3.2) ----

/**
 * Needs, 0 (fine) to 100 (desperate). Food, water and sleep follow the levels the
 * game shows (Peckish/Hungry, Thirsty/Dehydrated, Tired/Exhausted); warmth mirrors
 * the behavior tree's temperature urgency; safety and social are new.
 * With "No NPC Food" (eats == false) food and water are always 0.
 * update_needs() stores all six as plain NPC vars (autonomy_need_food, ...),
 * so they survive saving, load fine in builds without this code, and can be
 * read from JSON/EOCs as u_autonomy_need_social etc.
 */
struct needs_t {
    bool eats = true;
    int food = 0;
    int water = 0;
    int sleep = 0;
    int warmth = 0;
    int safety = 0;
    int social = 0;
};

/** Per-turn hook (npc::npc_update_body). Updates needs once a game minute when enabled. */
void on_npc_turn( npc &guy );
/** Advance the social need by `elapsed` and store all six needs on the NPC. */
void update_needs( npc &guy, const time_duration &elapsed );
/** Current needs, computed live (social is read from the stored var). */
needs_t current_needs( const npc &guy );
/** 0-1: enemy strength vs. our side from the NPC's own combat assessment, and injury. */
float safety_need( const npc &guy );
/** 0-1: stored social need (rises alone, falls with company). */
float social_need( const npc &guy );
/** A friendly, awake person (the player or an NPC) within 3 tiles. */
bool has_company( const npc &guy );
/** Colour-tagged "Needs: ..." text for the look-around panel. */
std::string needs_text( const npc &guy );

// ---- Quests (step 3.1) ----

/**
 * Called when a follower leaves the player (dialogue or the EOC "leave" effect).
 * With autonomy on, quests the player got from them are cancelled (failed),
 * since nobody is left to hand them in to.
 */
void on_follower_leaves( npc &guy );

} // namespace npc_autonomy

#endif // CATA_SRC_NPC_AUTONOMY_H
