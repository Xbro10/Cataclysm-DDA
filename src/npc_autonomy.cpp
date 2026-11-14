#include "npc_autonomy.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "character_oracle.h"
#include "coordinates.h"
#include "creature.h"
#include "character.h"
#include "dialogue_chatbin.h"
#include "game.h"
#include "map.h"
#include "math_parser_diag_value.h"
#include "messages.h"
#include "mission.h"
#include "npc.h"
#include "npc_trip.h"
#include "options.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "viewer.h"

static const efftype_id effect_hunger_famished( "hunger_famished" );
static const efftype_id effect_hunger_hungry( "hunger_hungry" );
static const efftype_id effect_hunger_near_starving( "hunger_near_starving" );
static const efftype_id effect_hunger_starving( "hunger_starving" );
static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );

namespace npc_autonomy
{

namespace
{

// ---- Tuning ----

// Social: 0 -> 100 over a day alone; 100 -> 0 over two hours in company.
constexpr double social_rise_per_minute = 100.0 / ( 24.0 * 60.0 );
constexpr double social_fall_per_minute = 100.0 / ( 2.0 * 60.0 );
constexpr int company_range = 3;
// A longer gap since the last update means the NPC was unloaded: no catch-up.
constexpr time_duration max_catch_up = 5_minutes;

// ---- Stored NPC vars (plain vars: EOCs read them as u_autonomy_need_food etc.) ----

constexpr const char *var_food = "autonomy_need_food";
constexpr const char *var_water = "autonomy_need_water";
constexpr const char *var_sleep = "autonomy_need_sleep";
constexpr const char *var_warmth = "autonomy_need_warmth";
constexpr const char *var_safety = "autonomy_need_safety";
constexpr const char *var_social = "autonomy_need_social";
// Turns since the start of the game at the last update.
constexpr const char *var_updated = "autonomy_needs_updated";

double get_num( const Creature &who, const char *key, double fallback )
{
    const diag_value *val = who.maybe_get_value( key );
    return val != nullptr && val->is_dbl() ? val->dbl() : fallback;
}

int turns_since_start()
{
    return to_turns<int>( calendar::turn - calendar::turn_zero );
}

// ---- Scales ----

int to_need( float frac )
{
    return static_cast<int>( std::lround( std::clamp( frac, 0.0f, 1.0f ) * 100.0f ) );
}

// A need curve: straight lines between (game value, need) points, flat past the ends.
struct curve_point {
    double raw;
    double need;
};

template<size_t N>
int on_curve( double raw, const std::array<curve_point, N> &curve )
{
    if( raw <= curve.front().raw ) {
        return static_cast<int>( std::lround( curve.front().need ) );
    }
    for( size_t i = 1; i < N; ++i ) {
        const curve_point &lo = curve[i - 1];
        const curve_point &hi = curve[i];
        if( raw <= hi.raw ) {
            const double t = ( raw - lo.raw ) / ( hi.raw - lo.raw );
            return static_cast<int>( std::lround( lo.need + t * ( hi.need - lo.need ) ) );
        }
    }
    return static_cast<int>( std::lround( curve.back().need ) );
}

// Thirsty (40) = 25, Very thirsty (80) = 50, Dehydrated (240) = 75, Parched (520) = 100.
constexpr std::array<curve_point, 5> thirst_curve{ {
        { 0, 0 }, { 40, 25 }, { 80, 50 }, { 240, 75 }, { 520, 100 }
    }
};
// Tired (191) = 33, Dead tired (383) = 67, Exhausted (575) = 100.
constexpr std::array<curve_point, 4> sleepiness_curve{ {
        { 0, 0 }, { 191, 33 }, { 383, 67 }, { 575, 100 }
    }
};

// ---- Each need ----

// Felt hunger: the game's hunger state (what the sidebar shows), or lost body
// reserves, whichever is worse.
int food_need( const npc &guy )
{
    // Worst first.
    static const std::array<std::pair<const efftype_id *, int>, 5> hunger_states{ {
            { &effect_hunger_starving, 100 },
            { &effect_hunger_near_starving, 85 },
            { &effect_hunger_famished, 70 },
            { &effect_hunger_very_hungry, 50 },
            { &effect_hunger_hungry, 25 }
        }
    };
    int felt = 0;
    for( const auto &[effect, need] : hunger_states ) {
        if( guy.has_effect( *effect ) ) {
            felt = need;
            break;
        }
    }
    int reserves = 0;
    if( const int healthy = guy.get_healthy_kcal(); healthy > 0 ) {
        // Down a quarter of a healthy reserve = 50, down half = 100.
        const float lost = 1.0f - static_cast<float>( guy.get_stored_kcal() ) / healthy;
        reserves = to_need( 2.0f * lost );
    }
    return std::max( felt, reserves );
}

int water_need( const npc &guy )
{
    return on_curve( guy.get_instant_thirst(), thirst_curve );
}

int sleep_need( const npc &guy )
{
    return on_curve( guy.get_sleepiness(), sleepiness_curve );
}

// ---- Look-around panel ----

const char *need_color( int value )
{
    if( value >= 75 ) {
        return "red";
    } else if( value >= 50 ) {
        return "light_red";
    } else if( value >= 25 ) {
        return "yellow";
    }
    return "light_green";
}

std::string need_part( const std::string &label, int value )
{
    return string_format( "%s <color_%s>%d</color>", label, need_color( value ), value );
}

std::string untracked_part( const std::string &label )
{
    return string_format( "%s <color_dark_gray>-</color>", label );
}

} // namespace

bool enabled()
{
    return get_option<bool>( "NPC_AUTONOMY" );
}

// ---- Needs ----

bool has_company( const npc &guy )
{
    const tripoint_abs_ms here = guy.pos_abs();
    const auto is_company = [&]( const Character &other ) {
        return !other.is_dead_state() && !other.in_sleep_state() &&
               rl_dist( here, other.pos_abs() ) <= company_range &&
               guy.attitude_to( other ) == Creature::Attitude::FRIENDLY;
    };
    if( is_company( get_avatar() ) ) {
        return true;
    }
    for( const npc &other : g->all_npcs() ) {
        if( &other != &guy && is_company( other ) ) {
            return true;
        }
    }
    return false;
}

float safety_need( const npc &guy )
{
    // assess_danger() refreshes these each NPC turn. Below ~2 the game itself
    // treats the area as quiet.
    const float enemy = guy.mem_combat.assess_enemy;
    float threat = 0.0f;
    if( enemy > 2.0f ) {
        // Evenly matched reads as about 67; outmatched 1.5 to 1 or worse is 100.
        threat = enemy / std::max( guy.mem_combat.assess_ally, 1.0f ) / 1.5f;
    }
    const float injury = 1.0f - guy.hp_percentage() / 100.0f;
    return std::clamp( std::max( threat, 0.6f * injury ), 0.0f, 1.0f );
}

float social_need( const npc &guy )
{
    return std::clamp( static_cast<float>( get_num( guy, var_social, 0.0 ) ) / 100.0f, 0.0f, 1.0f );
}

needs_t current_needs( const npc &guy )
{
    needs_t needs;
    // "No NPC Food" mod / NO_NPC_FOOD: NPCs never get hungry or thirsty.
    needs.eats = guy.needs_food();
    needs.food = needs.eats ? food_need( guy ) : 0;
    needs.water = needs.eats ? water_need( guy ) : 0;
    needs.sleep = sleep_need( guy );
    needs.warmth = to_need( behavior::character_oracle_t( &guy ).temperature_urgency( "" ) );
    needs.safety = to_need( safety_need( guy ) );
    needs.social = to_need( social_need( guy ) );
    return needs;
}

void update_needs( npc &guy, const time_duration &elapsed )
{
    // Social changes over time; asleep, it is paused.
    double social = get_num( guy, var_social, 0.0 );
    if( !guy.in_sleep_state() ) {
        const double minutes = to_turns<double>( elapsed ) / to_turns<double>( 1_minutes );
        social += minutes * ( has_company( guy ) ? -social_fall_per_minute : social_rise_per_minute );
    }
    guy.set_value( var_social, std::clamp( social, 0.0, 100.0 ) );

    // The rest are read from the NPC's current state.
    const needs_t needs = current_needs( guy );
    const std::array<std::pair<const char *, int>, 5> stored{ {
            { var_food, needs.food }, { var_water, needs.water }, { var_sleep, needs.sleep },
            { var_warmth, needs.warmth }, { var_safety, needs.safety }
        }
    };
    for( const auto &[key, value] : stored ) {
        guy.set_value( key, static_cast<double>( value ) );
    }
    guy.set_value( var_updated, static_cast<double>( turns_since_start() ) );
}

void on_npc_turn( npc &guy )
{
    if( !enabled() || !calendar::once_every( 1_minutes ) ) {
        return;
    }
    const int now = turns_since_start();
    const int last = static_cast<int>( get_num( guy, var_updated, now - to_turns<int>( 1_minutes ) ) );
    time_duration elapsed = time_duration::from_turns( now - last );
    if( elapsed <= 0_turns || elapsed > max_catch_up ) {
        elapsed = 1_minutes;
    }
    update_needs( guy, elapsed );
}

std::string needs_text( const npc &guy )
{
    const needs_t needs = current_needs( guy );
    return string_format( "%s %s  %s  %s  %s  %s  %s", _( "Needs:" ),
                          needs.eats ? need_part( _( "Food" ), needs.food ) : untracked_part( _( "Food" ) ),
                          needs.eats ? need_part( _( "Water" ), needs.water ) : untracked_part( _( "Water" ) ),
                          need_part( _( "Sleep" ), needs.sleep ), need_part( _( "Warmth" ), needs.warmth ),
                          need_part( _( "Safety" ), needs.safety ), need_part( _( "Social" ), needs.social ) );
}

// ---- Daily life ----

namespace
{

constexpr const char *goal_meal = "autonomy_meal";
constexpr const char *goal_socialize = "autonomy_socialize";

// Too unsafe to stop for a meal or a chat (Safety 50+).
constexpr float unsafe = 0.5f;
// Peckish is enough at mealtimes; outside them the game's own "eat when Hungry" applies.
constexpr int meal_hunger = 25;
constexpr float lonely = 0.5f;
constexpr int chat_range = 15;
constexpr time_duration chat_cooldown = 30_minutes;
constexpr double chat_social_relief = 40.0;

// Day * 3 + meal index of the last meal (eaten or tried), so it happens once per meal.
constexpr const char *var_last_meal = "autonomy_last_meal";
// Turns since the start of the game at the last chat (or failed try).
constexpr const char *var_last_chat = "autonomy_last_chat";

struct mealtime {
    int start_hour; // inclusive
    int end_hour;   // exclusive
    const char *lines; // snippet category, %1$s = the NPC
};
// Breakfast 6-9 AM, lunch 11 AM-1 PM, dinner 5-8 PM.
constexpr std::array<mealtime, 3> mealtimes{ {
        { 6, 9, "npc_autonomy_breakfast" },
        { 11, 13, "npc_autonomy_lunch" },
        { 17, 20, "npc_autonomy_dinner" }
    }
};

// Index into mealtimes, or -1 outside them.
int current_meal()
{
    const int hour = to_hours<int>( time_past_midnight( calendar::turn ) );
    for( size_t i = 0; i < mealtimes.size(); ++i ) {
        if( hour >= mealtimes[i].start_hour && hour < mealtimes[i].end_hour ) {
            return static_cast<int>( i );
        }
    }
    return -1;
}

double meal_key( int meal )
{
    return to_days<int>( calendar::turn - calendar::turn_zero ) * 3.0 + meal;
}

bool is_safe( const npc &guy )
{
    return safety_need( guy ) < unsafe;
}

// Food in the pack they'd eat, and room in the stomach. (The behavior tree's own
// npc_has_food also demands hunger past "Hungry", too late for a Peckish meal.)
bool carries_meal( const npc &guy )
{
    if( !guy.intake_fits( npc::need_goal_id::eat_food ) ) {
        return false;
    }
    for( const item *food : guy.items_with( []( const item & it ) {
    return it.is_food();
    } ) ) {
        if( guy.can_satisfy( *food, npc::need_goal_id::eat_food ) ) {
            return true;
        }
    }
    return false;
}

std::string line_from( const char *category )
{
    const std::optional<translation> line = SNIPPET.random_from_category( category );
    return line ? line->translated() : std::string();
}

bool player_sees( const Creature &who )
{
    return get_player_view().sees( get_map(), who );
}

goal_result eat_meal( npc &guy )
{
    const int meal = current_meal();
    if( meal < 0 ) {
        return goal_result::none;
    }
    // Marked even if eating fails, so a failed try doesn't repeat every turn.
    guy.set_value( var_last_meal, meal_key( meal ) );
    if( !guy.consume_food( npc::consume_filter::food_only ) ) {
        return goal_result::none;
    }
    if( player_sees( guy ) ) {
        add_msg( m_neutral, "%s", string_format( line_from( mealtimes[meal].lines ), guy.get_name() ) );
    }
    return guy.activity ? goal_result::activity : goal_result::acted;
}

void chat( npc &guy, Character &partner )
{
    const double now = turns_since_start();
    for( Character *who : std::array<Character *, 2> { &guy, &partner } ) {
        if( who->is_npc() ) {
            const double social = get_num( *who, var_social, 0.0 ) - chat_social_relief;
            who->set_value( var_social, std::max( social, 0.0 ) );
            who->set_value( var_last_chat, now );
        }
    }
    if( player_sees( guy ) || player_sees( partner ) ) {
        if( partner.is_avatar() ) {
            add_msg( m_neutral, "%s", string_format( line_from( "npc_autonomy_chat_player" ),
                     guy.get_name() ) );
        } else {
            add_msg( m_neutral, "%s", string_format( line_from( "npc_autonomy_chat_npc" ),
                     guy.get_name(), partner.get_name() ) );
        }
    }
    // A turn spent talking.
    guy.mod_moves( -guy.get_speed() );
}

goal_result socialize( npc &guy )
{
    Character *partner = chat_partner( guy );
    if( partner != nullptr ) {
        if( rl_dist( guy.pos_abs(), partner->pos_abs() ) <= 1 ) {
            chat( guy, *partner );
            return goal_result::acted;
        }
        if( guy.move_to_and_verify( npc_trip_spec::within_range( partner->pos_bub(), 1, true ) ) ) {
            return goal_result::acted;
        }
    }
    // Nobody to talk to, or no way there: try again after the cooldown.
    guy.set_value( var_last_chat, static_cast<double>( turns_since_start() ) );
    return goal_result::none;
}

} // namespace

bool is_goal( const std::string &goal )
{
    return goal == goal_meal || goal == goal_socialize;
}

bool wants_meal( const npc &guy )
{
    if( !enabled() || !guy.needs_food() || guy.in_sleep_state() || !is_safe( guy ) ) {
        return false;
    }
    const int meal = current_meal();
    if( meal < 0 || get_num( guy, var_last_meal, -1.0 ) == meal_key( meal ) ||
        food_need( guy ) < meal_hunger ) {
        return false;
    }
    return carries_meal( guy );
}

Character *chat_partner( const npc &guy )
{
    const map &here = get_map();
    Character *best = nullptr;
    int best_dist = chat_range + 1;
    const auto consider = [&]( Character & other ) {
        if( other.is_dead_state() || other.in_sleep_state() ) {
            return;
        }
        const int dist = rl_dist( guy.pos_abs(), other.pos_abs() );
        if( dist < best_dist && guy.attitude_to( other ) == Creature::Attitude::FRIENDLY &&
            guy.sees( here, other ) ) {
            best = &other;
            best_dist = dist;
        }
    };
    consider( get_avatar() );
    for( npc &other : g->all_npcs() ) {
        if( &other != &guy ) {
            consider( other );
        }
    }
    return best;
}

bool wants_company( const npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || !is_safe( guy ) || social_need( guy ) < lonely ) {
        return false;
    }
    const double last = get_num( guy, var_last_chat, -1.0e9 );
    if( turns_since_start() - last < to_turns<int>( chat_cooldown ) ) {
        return false;
    }
    return chat_partner( guy ) != nullptr;
}

goal_result run_goal( npc &guy, const std::string &goal )
{
    if( goal == goal_meal ) {
        return eat_meal( guy );
    }
    if( goal == goal_socialize ) {
        return socialize( guy );
    }
    return goal_result::none;
}

// ---- Quests ----

void on_follower_leaves( npc &guy )
{
    if( !enabled() ) {
        return;
    }
    const avatar &player_character = get_avatar();
    // Copy: failing a mission can change the giver's list.
    const std::vector<mission *> assigned = guy.chatbin.missions_assigned;
    for( mission *miss : assigned ) {
        if( miss == nullptr || !miss->in_progress() ||
            miss->get_assigned_player_id() != player_character.getID() ) {
            continue;
        }
        miss->fail();
        add_msg( m_bad, _( "Quest cancelled: %1$s left you, so \"%2$s\" can't be handed in." ),
                 guy.get_name(), miss->name() );
    }
    guy.chatbin.missions_assigned.clear();
}

} // namespace npc_autonomy
