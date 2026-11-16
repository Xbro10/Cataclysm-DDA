#pragma once
#ifndef CATA_SRC_NPC_AUTONOMY_H
#define CATA_SRC_NPC_AUTONOMY_H

// Xbro10 fork: "NPC autonomy (experimental)", playbook Phase 3.
// Everything here does nothing unless the NPC_AUTONOMY game option is on,
// so with the option off NPCs behave exactly like upstream.

#include <string>

#include "calendar.h"
#include "coords_fwd.h"
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
/** Once a minute: a Cold NPC (Warmth 25+) puts on the warmest clothes it carries. */
void dress_for_the_cold( npc &guy );
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
/** The panel's second line: "Plan: ..." while on a trip, or why a cold NPC can't warm up; "" otherwise. */
std::string plan_line( const npc &guy );

// ---- Daily life (step 3.3): behavior-tree goals "autonomy_meal", "autonomy_socialize" ----

/** True for the goal ids above (npc::move() dispatches them to run_goal()). */
bool is_goal( const std::string &goal );
/** Mealtime (breakfast, lunch, dinner), Peckish or worse, food in the pack, not yet eaten this meal, safe. */
bool wants_meal( const npc &guy );
/** wants_meal() for a guard standing on their post during their shift: they eat from their pack there. */
bool wants_meal_at_post( const npc &guy );
/** A follower eating while the player has walked beyond their follow distance: drop the meal and follow. */
bool should_leave_meal( const npc &guy );
/**
 * The behavior tree picked an eat or drink goal (the game's own or ours) for a follower
 * the player has walked away from: follow instead, and eat once close again. Not when
 * Food or Water is 100 (starving, Parched).
 */
bool should_catch_up( const npc &guy, const std::string &goal );
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

// ---- Getting things (step 3.4): behavior-tree goals "autonomy_get_water", "autonomy_get_food" ----
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

// Step 3.4b, goal "autonomy_get_food": the same for food, driving npc::execute_eat_food from
// Food 50 (Very hungry) instead of starving: eat from the pack, else walk up to 20 tiles to
// food the NPC can see (cupboards and containers too, nothing owned) and eat it there.

/** Food 50+, safe, awake, not given up, and food in the pack or in reach. */
bool wants_food( const npc &guy );

// Step 3.4c, goal "autonomy_get_warm": Warmth 50+ (between the game's Cold and Very cold).
// Warmer clothes from the pack; else stand by a hearth fire they can see; else, with a
// firestarter, take firewood (carried, or picked up nearby) to a fireplace, stove or fire
// pit and light it; else the game's own planner (shelter, clothes nearby). Fires are only
// ever lit in a hearth. Beside a fire they stay until Warmth is under 25.

/** Warmth 50+ (or 25+ beside a fire), safe, awake, not given up, and a way to warm up. */
bool wants_warmth( const npc &guy );
/**
 * For the look-around panel: why a cold NPC (Warmth 50+) can't warm up, e.g. "Cold: no room
 * to carry the firewood"; "" when they aren't cold or have a way.
 */
std::string warm_blocker( const npc &guy );
/** Food need 50+ with autonomy on: the game's "is eating worth it" check also says yes. */
bool hungry_enough_for_a_trip( const npc &guy );
/** How far NPCs look for food: 6 tiles (upstream) or, with autonomy on, 20. */
int food_search_radius();
/** Fill one empty watertight container in the NPC's pack with `liquid`. True if one was filled. */
bool refill_bottle( npc &guy, const itype_id &liquid );
/**
 * With autonomy on, an NPC never steps onto a harmful trap it can see or knows about (a pit,
 * spikes, a bear trap): it's a wall for their pathing. The game only made such tiles costly,
 * so a hungry NPC walked through spiked pits to food on the far side.
 */
bool avoids_trap( const npc &guy, const tripoint_bub_ms &p );
/** "Plan: ..." line for the look-around panel, or "" when there's no plan being pursued. */
std::string plan_text( const npc &guy );
/** Once a minute: forget water and food plans the NPC isn't pursuing any more (they moved on). */
void drop_stale_plans( npc &guy );

// ---- Quests (step 3.1) ----

/**
 * Called when a follower leaves the player (dialogue or the EOC "leave" effect).
 * With autonomy on, quests the player got from them are cancelled (failed),
 * since nobody is left to hand them in to.
 */
void on_follower_leaves( npc &guy );

} // namespace npc_autonomy

#endif // CATA_SRC_NPC_AUTONOMY_H
