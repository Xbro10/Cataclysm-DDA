#pragma once
#ifndef CATA_SRC_NPC_AUTONOMY_H
#define CATA_SRC_NPC_AUTONOMY_H

// Xbro10 fork: "NPC autonomy (experimental)", playbook Phase 3.
// Everything here does nothing unless the NPC_AUTONOMY game option is on,
// so with the option off NPCs behave exactly like upstream.

#include <string>

#include "calendar.h"
#include "type_id.h"

class Character;
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

// ---- Daily life (step 3.3): behavior-tree goals "autonomy_meal", "autonomy_socialize" ----

/** True for the goal ids above (npc::move() dispatches them to run_goal()). */
bool is_goal( const std::string &goal );
/** Mealtime (breakfast, lunch, dinner), Peckish or worse, food in the pack, not yet eaten this meal, safe. */
bool wants_meal( const npc &guy );
/** Social 50+, safe, awake, not chatted in the last 30 minutes, someone to talk to nearby. */
bool wants_company( const npc &guy );
/** Nearest friendly, awake person within 15 tiles the NPC can see (nullptr if none). */
Character *chat_partner( const npc &guy );

enum class goal_result {
    activity, // started an activity (eating): a self-initiated one, not a player order
    acted,    // spent its moves (walked, chatted, ate something instant)
    none      // nothing to do: fall back to the game's own needs handling
};
goal_result run_goal( npc &guy, const std::string &goal );

// ---- Getting things (step 3.4): behavior-tree goal "autonomy_get_water" ----
// Drives the game's own drink-water planner (npc::execute_drink_water: pick a target,
// walk, drink, cross off failed targets) from Water 50 instead of Parched, over a
// longer range, then refills a bottle at clean water. 3 failed targets in a row and
// the NPC gives up for an hour.

/** Thirst (Very thirsty) at which a drink is worth a trip with autonomy on; upstream waits for Parched (520). */
constexpr int thirst_worth_a_trip = 80;
/** Water 50+, safe, awake, not given up, and a drink in the pack or a source in reach. */
bool wants_water( const npc &guy );
/** How far NPCs look for water terrain: 6 tiles (upstream) or, with autonomy on, 20. */
int water_search_radius();
/** Fill one empty watertight container in the NPC's pack with `liquid`. True if one was filled. */
bool refill_bottle( npc &guy, const itype_id &liquid );
/** "Plan: ..." line for the look-around panel, or "" when there's no plan. */
std::string plan_text( const npc &guy );

// ---- Quests (step 3.1) ----

/**
 * Called when a follower leaves the player (dialogue or the EOC "leave" effect).
 * With autonomy on, quests the player got from them are cancelled (failed),
 * since nobody is left to hand them in to.
 */
void on_follower_leaves( npc &guy );

} // namespace npc_autonomy

#endif // CATA_SRC_NPC_AUTONOMY_H
