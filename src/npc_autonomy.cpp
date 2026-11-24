#include "npc_autonomy.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "behavior.h"
#include "calendar.h"
#include "character_oracle.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "creature.h"
#include "character.h"
#include "character_id.h"
#include "dialogue_chatbin.h"
#include "enums.h"
#include "field_type.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "iexamine.h"
#include "item.h"
#include "item_location.h"
#include "item_category.h"
#include "itype.h"
#include "lightmap.h"
#include "map.h"
#include "mapdata.h"
#include "math_parser_diag_value.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "mtype.h"
#include "gun_mode.h"
#include "npc.h"
#include "npc_arbitration.h"
#include "npc_memory.h"
#include "npc_trip.h"
#include "options.h"
#include "pocket_type.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "viewer.h"
#include "weather.h"

static const efftype_id effect_hunger_famished( "hunger_famished" );
static const efftype_id effect_hunger_hungry( "hunger_hungry" );
static const efftype_id effect_hunger_near_starving( "hunger_near_starving" );
static const efftype_id effect_hunger_starving( "hunger_starving" );
static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );
static const efftype_id effect_npc_run_away( "npc_run_away" );

namespace npc_autonomy
{

namespace
{

// ---- Tuning ----

// Social: 0 -> 100 over a day alone; 100 -> 0 over two hours in company.
constexpr double social_rise_per_minute = 100.0 / ( 24.0 * 60.0 );
constexpr double social_fall_per_minute = 100.0 / ( 2.0 * 60.0 );
constexpr int company_range = 3;
// Any hostile the NPC can see this close makes Safety at least 50.
constexpr int close_hostile_range = 3;
// Further out, up to this range, hostiles they can see make Safety at least 50 when together
// they are a match for the NPC's side (step 5.2: a follower watching a Smiler 8 tiles off
// felt perfectly safe; a well-armed one shouldn't fear a lone zombie at 8 tiles).
constexpr int watch_hostile_range = 10;
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
// Where a shopkeeper (or a sheltering stranger) belongs, "x,y,z" (absolute): the game keeps
// them standing still and never remembers where, so once something moved them they stayed
// wherever they ended up (reported 2026-11-18: Exchange traders left in the aisle).
constexpr const char *var_home_spot = "autonomy_home_spot";
// They look for safety within this many tiles of it, and walk back once calm.
constexpr int home_spot_reach = 6;

double get_num( const Creature &who, const char *key, double fallback )
{
    const diag_value *val = who.maybe_get_value( key );
    return val != nullptr && val->is_dbl() ? val->dbl() : fallback;
}

std::optional<tripoint_abs_ms> home_spot( const Creature &who )
{
    const diag_value *val = who.maybe_get_value( var_home_spot );
    tripoint_abs_ms p;
    if( val == nullptr || !val->is_str() ||
        std::sscanf( val->str().c_str(), "%d,%d,%d", &p.x(), &p.y(), &p.z() ) != 3 ) {
        return std::nullopt;
    }
    return p;
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

// Body fat below the game's "normal" weight (fat BMI 3; 2 is underweight's floor,
// 1 emaciated's): just under normal is near 0, emaciated 50-75, no fat left 100.
constexpr std::array<curve_point, 4> fat_deficit_curve{ {
        { 0, 0 }, { 1, 50 }, { 2, 75 }, { 3, 100 }
    }
};

// Felt hunger: the game's hunger state (what the sidebar shows), or being
// underweight, whichever is worse. A normal or heavier body adds nothing, so a
// meal shows up right away.
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
    const double fat_deficit = character_weight_category::normal - guy.get_bmi_fat();
    return std::max( felt, on_curve( fat_deficit, fat_deficit_curve ) );
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

// Xbro10 fork (2026-11-23, speed): one NPC action asked safety_need() and is_safe() several
// times, each a scan of every creature with line of sight, plus a weapon rating. Inside an
// action_cache_scope the answers are kept until the NPC steps, changes weapon or the turn ends.
struct action_cache {
    const npc *who = nullptr;
    tripoint_abs_ms pos;
    time_point turn;
    const item *weapon = nullptr;
    std::map<std::pair<int, int>, std::pair<float, float>> threats;
    std::map<int, bool> hostile_within;
    std::optional<float> strength;
};
static action_cache the_cache;

// The cache for this NPC, emptied if it went stale; nullptr outside a scope.
static action_cache *cache_for( const npc &guy )
{
    if( the_cache.who != &guy ) {
        return nullptr;
    }
    const item_location weapon = guy.get_wielded_item();
    const item *held = weapon ? weapon.get_item() : nullptr;
    if( the_cache.pos != guy.pos_abs() || the_cache.turn != calendar::turn || the_cache.weapon != held ) {
        the_cache.pos = guy.pos_abs();
        the_cache.turn = calendar::turn;
        the_cache.weapon = held;
        the_cache.threats.clear();
        the_cache.hostile_within.clear();
        the_cache.strength.reset();
    }
    return &the_cache;
}

action_cache_scope::action_cache_scope( const npc &guy ) : previous( the_cache.who )
{
    the_cache = action_cache();
    the_cache.who = &guy;
    the_cache.turn = calendar::turn_zero;
}

action_cache_scope::~action_cache_scope()
{
    the_cache = action_cache();
    the_cache.who = previous;
    the_cache.turn = calendar::turn_zero;
}

static std::pair<float, float> scan_threats( const npc &guy, int near_range, int far_range );

static std::pair<float, float> watched_threats( const npc &guy, const int near_range, const int far_range )
{
    action_cache *cache = cache_for( guy );
    if( cache == nullptr ) {
        return scan_threats( guy, near_range, far_range );
    }
    const auto found = cache->threats.find( { near_range, far_range } );
    if( found != cache->threats.end() ) {
        return found->second;
    }
    return cache->threats[{ near_range, far_range }] = scan_threats( guy, near_range, far_range );
}

// The threat rating of the hostiles the NPC can see within near_range tiles and within far_range
// tiles (near_range <= far_range), from one scan. This runs several times per NPC turn and line of sight
// is the costly part, so it's only checked for hostiles in range: friends are never looked at.
static std::pair<float, float> scan_threats( const npc &guy, const int near_range, const int far_range )
{
    const map &here = get_map();
    const tripoint_abs_ms pos = guy.pos_abs();
    float near_total = 0.0f;
    float far_total = 0.0f;
    bool person_near = false;
    bool person_far = false;
    for( const Creature *critter : g->get_creatures_if( [&]( const Creature & other ) {
    return &other != &guy && other.pos_abs() != pos && rl_dist( pos, other.pos_abs() ) <= far_range &&
           guy.attitude_to( other ) == Creature::Attitude::HOSTILE && guy.sees( here, other );
    } ) ) {
        const bool is_near = rl_dist( pos, critter->pos_abs() ) <= near_range;
        const monster *mon = critter->as_monster();
        if( mon == nullptr ) {
            // No cheap way to rate an armed person: treat them as a real threat.
            person_far = true;
            person_near = person_near || is_near;
            continue;
        }
        const float hp = static_cast<float>( mon->get_hp() ) / std::max( mon->get_hp_max(), 1 );
        const float rating = std::max( static_cast<float>( mon->type->get_total_difficulty() ),
                                       NPC_DANGER_VERY_LOW ) * ( hp * 0.5f + 0.5f );
        far_total += rating;
        if( is_near ) {
            near_total += rating;
        }
    }
    constexpr float overwhelming = std::numeric_limits<float>::max();
    return { person_near ? overwhelming : near_total, person_far ? overwhelming : far_total };
}

static float watched_threat( const npc &guy, const int range )
{
    return watched_threats( guy, range, range ).second;
}

// npc::sees_hostile_within(), but line of sight is only checked for hostiles: it's the costly
// part and runs for every check of is_safe(), several per NPC turn.
static bool scan_hostile_within( const npc &guy, int radius );

static bool sees_hostile_within( const npc &guy, const int radius )
{
    action_cache *cache = cache_for( guy );
    if( cache == nullptr ) {
        return scan_hostile_within( guy, radius );
    }
    const auto found = cache->hostile_within.find( radius );
    if( found != cache->hostile_within.end() ) {
        return found->second;
    }
    return cache->hostile_within[radius] = scan_hostile_within( guy, radius );
}

static bool scan_hostile_within( const npc &guy, const int radius )
{
    const map &here = get_map();
    const tripoint_abs_ms pos = guy.pos_abs();
    return !g->get_creatures_if( [&]( const Creature & other ) {
        return &other != &guy && other.pos_abs() != pos && rl_dist( pos, other.pos_abs() ) <= radius &&
               guy.attitude_to( other ) == Creature::Attitude::HOSTILE && guy.sees( here, other );
    } ).empty();
}

// How well the NPC could fight on their own, on the same scale as a monster's threat
// rating: defence (dodge and armour) plus the weapon in hand, both as the game itself
// rates them, scaled by health. An unarmed survivor is about 4 (a zombie is 5); a katana
// adds about 10, or nearly 30 with good skills (a zombie brute is 27, a hulk 173). The game
// rates a loaded gun on a much bigger scale (a rifle about 680), so a gun counts a tenth:
// a rifle then beats zombies and brutes, but not a hulk.
// A shot's damage: the round or arrow in it, else the one they'd load.
static float shot_damage( const item &gun )
{
    float damage = gun.gun_damage().total_damage();
    if( gun.ammo_data() == nullptr ) {
        const itype_id ammo = gun.ammo_default();
        if( !ammo.is_null() && ammo->ammo ) {
            damage += ammo->ammo->damage.total_damage();
        }
    }
    return damage;
}

static float rate_own_strength( const npc &guy );

static float own_strength( const npc &guy )
{
    action_cache *cache = cache_for( guy );
    if( cache == nullptr ) {
        return rate_own_strength( guy );
    }
    if( !cache->strength ) {
        cache->strength = rate_own_strength( guy );
    }
    return *cache->strength;
}

static float rate_own_strength( const npc &guy )
{
    const float health = std::clamp( guy.hp_percentage() / 100.0f, 0.0f, 1.0f );
    const float defence = guy.mem_combat.my_defence_assess > 0.0f ? guy.mem_combat.my_defence_assess :
                          guy.get_dodge() * health;
    const item_location weapon = guy.get_wielded_item();
    float attack = weapon ? static_cast<float>( guy.evaluate_weapon( *weapon ) ) : 0.0f;
    if( weapon && weapon->is_gun() ) {
        attack /= 10.0f;
        // N18 (2026-11-20): the game rates a one-shot weapon by the one shot in it, so a short
        // bow was about 0.7 (0.07 here) and a skilled archer counted as weaker than one zombie:
        // every gunner rule was off. A gun or bow they can fire counts at least half a shot's damage.
        if( weapon->shots_remaining( get_map(), &guy ) > 0 ||
            !guy.find_ammo( *weapon, false, -1 ).empty() ) {
            attack = std::max( attack, shot_damage( *weapon ) / 2.0f );
        }
    }
    return defence + attack * health;
}

float safety_need( const npc &guy )
{
    // assess_danger() refreshes these each NPC turn. Below ~2 the game itself
    // treats the area as quiet.
    const float enemy = guy.mem_combat.assess_enemy;
    float threat = 0.0f;
    // Only hostiles that together match or beat what the NPC could take on themselves scare
    // them, at any range: well armed, a lone zombie or Hound isn't scary even close by (it's
    // fought; a katana fighter read Safety 50 from one Hound, 2026-11-18). A hostile person
    // always counts. Daily life (meals, chats, trips) still waits while anything hostile is
    // within 3 tiles: see is_safe().
    const auto [watched, seen] = watched_threats( guy, watch_hostile_range, 2 * watch_hostile_range );
    // Rating the weapon isn't free, and with no hostile in sight it decides nothing below.
    const float strength = seen > 0.0f ? own_strength( guy ) : 0.0f;
    if( watched > 0.0f && watched >= strength ) {
        threat = 0.5f;
    }
    // The game's own odds (it also counts hostiles further off), unless the NPC sees hostiles
    // and clearly outclasses all of them.
    if( enemy > 2.0f && ( seen <= 0.0f || seen >= strength ) ) {
        // Evenly matched reads as about 67; outmatched 1.5 to 1 or worse is 100.
        threat = std::max( threat, enemy / std::max( guy.mem_combat.assess_ally, 1.0f ) / 1.5f );
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
    // Also with the option just turned off: nothing else would give them their weapon back.
    set_down_wood_after_trip( guy );
    remember_home_spot( guy );
    track_fight_items( guy );
    // Their own weapon at their feet first, then something else from the pack.
    grab_dropped_weapon( guy );
    arm_up( guy );
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
    // Step 5.6: scared (Safety 50+) and badly hurt here: a place to stay away from.
    if( safety_need( guy ) >= 0.5f && guy.hp_percentage() < 50 ) {
        npc_memory::remember( guy, npc_memory::place_kind::danger, guy.pos_abs() );
    }
    drop_stale_plans( guy );
    dress_for_the_cold( guy );
}

void dress_for_the_cold( npc &guy )
{
    // Cold (Warmth 25+): put on warmer clothes they carry. The game itself waits for Very cold,
    // so a cold NPC walked around with a hoodie in their pack.
    if( !enabled() || guy.in_sleep_state() || guy.activity || current_needs( guy ).warmth < 25 ||
        safety_need( guy ) >= 0.5f ) {
        return;
    }
    guy.wear_warmest_item();
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

std::string memory_text( const npc &guy )
{
    // Step 5.6. On its own line after the plan (2026-11-20): added to the needs line it made that
    // wrap, and the look-around panel had no room left for the plan.
    const int places = npc_memory::count( guy );
    if( places <= 0 ) {
        return std::string();
    }
    return string_format( n_gettext( "Remembers %d place", "Remembers %d places", places ), places );
}

std::string plan_line( const npc &guy )
{
    const std::string plan = plan_text( guy );
    return plan.empty() ? warm_blocker( guy ) : plan;
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

// Follower rule "no_self_care" (Follower rules screen): followers who leave food and water
// to the player skip mealtimes and water trips. NPCs who aren't your allies always look after themselves.
bool looks_after_self( const npc &guy )
{
    return !guy.is_player_ally() || !guy.rules.has_flag( ally_rule::no_self_care );
}

// Calm enough for daily life (meals, chats, trips, fires): not scared, and nothing hostile
// right there, however weak (a starving follower kept eating with a zombie at arm's length).
bool is_safe( const npc &guy )
{
    return safety_need( guy ) < unsafe && !sees_hostile_within( guy, close_hostile_range );
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

// ---- Getting water and food (step 3.4) ----

constexpr const char *goal_get_water = "autonomy_get_water";
constexpr const char *goal_get_food = "autonomy_get_food";
// Very thirsty / Very hungry. The game's own planners wait for Parched / starving.
constexpr int need_to_act = 50;
constexpr int trip_radius = 20;
// Targets in a row that came to nothing (unreachable, no progress) before giving up.
constexpr int max_failed_targets = 3;
constexpr time_duration give_up_for = 1_hours;
// How long the "is there any in reach?" answer is reused, in turns.
constexpr int recheck_turns = 5;

// The behavior tree asks the "anything in reach?" questions every turn, and each answer
// comes from a 10-20 tile scan, so an answer is reused for recheck_turns turns. Answers older
// than that are never read again, so they're dropped once the store grows (it used to keep
// one entry for every NPC ever met).
template<typename Key, typename Answer>
class recent_answers
{
    public:
        template<typename Ask>
        Answer get( const Key &key, Ask &&ask ) {
            const int now = turns_since_start();
            const auto known = answers.find( key );
            if( known != answers.end() && fresh( known->second.turn, now ) ) {
                return known->second.answer;
            }
            if( answers.size() >= prune_at ) {
                for( auto it = answers.begin(); it != answers.end(); ) {
                    it = fresh( it->second.turn, now ) ? std::next( it ) : answers.erase( it );
                }
            }
            Answer answer = ask();
            answers[key] = { now, answer };
            return answer;
        }
    private:
        struct entry {
            int turn = 0;
            Answer answer;
        };
        // The clock can go backwards (a test, a loaded save), which makes an answer stale too.
        static bool fresh( int turn, int now ) {
            return now >= turn && now - turn < recheck_turns;
        }
        static constexpr size_t prune_at = 64;
        std::map<Key, entry> answers;
};

// One kind of trip: which of the game's planners runs it, where its state is kept, what's said.
struct trip_kind {
    npc::need_goal_id need;
    // The game's own goal for this need, and ours.
    const char *upstream_goal;
    const char *our_goal;
    // Targets in a row that came to nothing.
    const char *fails_var;
    // Turns since the start of the game until which the NPC has given up.
    const char *gave_up_var;
    // The target last announced, so "goes to get ..." shows once per trip.
    const char *target_var;
    const char *going_msg;
    const char *give_up_msg;
    const char *plan_here;
    const char *plan_away;
};

const trip_kind water_trip{
    npc::need_goal_id::drink_water, "drink_water", goal_get_water,
    "autonomy_water_fails", "autonomy_water_gave_up_until", "autonomy_water_target",
    translate_marker( "%s goes to get water." ),
    translate_marker( "%s gives up looking for water." ),
    translate_marker( "Plan: get water (drinking)" ),
    translate_marker( "Plan: get water (heading to water %d tiles away)" )
};

const trip_kind food_trip{
    npc::need_goal_id::eat_food, "eat_food", goal_get_food,
    "autonomy_food_fails", "autonomy_food_gave_up_until", "autonomy_food_target",
    translate_marker( "%s goes to get food." ),
    translate_marker( "%s gives up looking for food." ),
    translate_marker( "Plan: get food (eating)" ),
    translate_marker( "Plan: get food (heading to food %d tiles away)" )
};

// Something in the pack that meets this need, and room to take it.
bool carries_for( const npc &guy, const npc::need_goal_id need )
{
    if( !guy.intake_fits( need ) ) {
        return false;
    }
    for( const item *it : guy.items_with( []( const item & i ) {
    return i.is_food();
    } ) ) {
        if( guy.can_satisfy( *it, need ) ) {
            return true;
        }
    }
    return false;
}

std::string target_key( const tripoint_abs_ms &p )
{
    return string_format( "%d,%d,%d", p.x(), p.y(), p.z() );
}

// The liquid an adjacent water terrain gives, if any (what the game's planner drinks from).
itype_id adjacent_water( const npc &guy )
{
    const map &here = get_map();
    for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
        const ter_t &ter = here.ter( p ).obj();
        if( !ter.liquid_source_item_id.is_null() && ter.liquid_source_count == std::make_pair( 0, 0 ) ) {
            return ter.liquid_source_item_id;
        }
    }
    return itype_id::NULL_ID();
}

bool gave_up( const npc &guy, const trip_kind &kind )
{
    return turns_since_start() < get_num( guy, kind.gave_up_var, -1.0 );
}

void count_failed_target( npc &guy, const trip_kind &kind )
{
    const int fails = static_cast<int>( get_num( guy, kind.fails_var, 0.0 ) ) + 1;
    if( fails < max_failed_targets ) {
        guy.set_value( kind.fails_var, static_cast<double>( fails ) );
        return;
    }
    guy.set_value( kind.fails_var, 0.0 );
    guy.set_value( kind.gave_up_var,
                   static_cast<double>( turns_since_start() + to_turns<int>( give_up_for ) ) );
    guy.remove_value( kind.target_var );
    if( player_sees( guy ) ) {
        add_msg( m_neutral, _( kind.give_up_msg ), guy.get_name() );
    }
}

npc::need_result run_planner( npc &guy, const trip_kind &kind )
{
    return kind.need == npc::need_goal_id::drink_water ? guy.execute_drink_water() :
           guy.execute_eat_food();
}

// Step 5.6: how far a thirsty NPC with no follow or post limits walks to water they remember.
constexpr int remembered_water_range = 60;
// NPC var: the remembered water they last said they were heading for.
constexpr const char *var_water_memory_said = "autonomy_water_memory_said";

// Step 5.6: the nearest water they remember, if it's in the loaded map on their level and they
// may go there (inside their follow or post reach; anyone else within 60 tiles).
std::optional<tripoint_bub_ms> remembered_water( const npc &guy )
{
    const map &here = get_map();
    const std::optional<npc_reach> reach = guy.obligation_reach();
    // The nearest one they may go to: a nearer one out of reach (or on another level) doesn't
    // hide it.
    const std::optional<tripoint_abs_ms> spot = npc_memory::nearest( guy,
    npc_memory::place_kind::water, [&]( const tripoint_abs_ms & p ) {
        if( p.z() != guy.posz() || !here.inbounds( p ) ) {
            return false;
        }
        return reach ? reach->contains( p ) : rl_dist( guy.pos_abs(), p ) <= remembered_water_range;
    } );
    if( !spot ) {
        return std::nullopt;
    }
    return here.get_bub( *spot );
}

// Step 5.6: nothing to drink in reach, so head for water they remember. Once there (within 2
// tiles) and still nothing, or if they can't get there, it's forgotten.
goal_result walk_to_remembered_water( npc &guy )
{
    const std::optional<tripoint_bub_ms> spot = remembered_water( guy );
    if( !spot ) {
        return goal_result::none;
    }
    map &here = get_map();
    if( rl_dist( guy.pos_bub(), *spot ) <= 2 ||
        !guy.move_to_and_verify( npc_trip_spec::within_range( *spot, 1, true ) ) ) {
        npc_memory::forget( guy, npc_memory::place_kind::water, here.get_abs( *spot ) );
        guy.remove_value( var_water_memory_said );
        return goal_result::none;
    }
    const std::string key = target_key( here.get_abs( *spot ) );
    const diag_value *said = guy.maybe_get_value( var_water_memory_said );
    if( said == nullptr || !said->is_str() || said->str() != key ) {
        guy.set_value( var_water_memory_said, key );
        if( player_sees( guy ) ) {
            add_msg( m_neutral, _( "%s heads for water they remember." ), guy.get_name() );
        }
    }
    return goal_result::acted;
}

// Drives the game's own planner for this need (pick a target, walk, eat or drink there,
// cross off targets that fail) and adds what it lacks: saying where they're off to,
// giving up after a few failed targets, and for water, refilling a bottle.
goal_result fetch( npc &guy, const trip_kind &kind )
{
    if( gave_up( guy, kind ) ) {
        return goal_result::none;
    }
    // Only a target that turned out unreachable counts toward giving up, not "found nothing".
    const bool had_target = guy.plan_for( kind.need ).active();
    const npc::need_result result = run_planner( guy, kind );
    const npc::need_plan &plan = guy.plan_for( kind.need );

    // Off to a new target: say so once.
    if( plan.active() && rl_dist( guy.pos_abs(), plan.target ) > 1 ) {
        const std::string key = target_key( plan.target );
        const diag_value *announced = guy.maybe_get_value( kind.target_var );
        if( announced == nullptr || !announced->is_str() || announced->str() != key ) {
            guy.set_value( kind.target_var, key );
            if( player_sees( guy ) ) {
                add_msg( m_neutral, _( kind.going_msg ), guy.get_name() );
            }
        }
    }

    switch( result ) {
        case npc::need_result::satisfied: {
            guy.set_value( kind.fails_var, 0.0 );
            guy.remove_value( kind.target_var );
            if( kind.need != npc::need_goal_id::drink_water ) {
                return goal_result::acted;
            }
            // Last step for water: top up a bottle for later, at clean water only.
            // (Step 5.6's water memory is noted where they drink: drank_at_water, started_drinking.)
            const itype_id liquid = adjacent_water( guy );
            if( liquid.is_valid() && !liquid.is_null() && liquid->comestible &&
                liquid->comestible->parasites == 0 && liquid->comestible->contamination.empty() &&
                refill_bottle( guy, liquid ) && player_sees( guy ) ) {
                add_msg( m_neutral, _( "%s fills a bottle with water." ), guy.get_name() );
            }
            return goal_result::acted;
        }
        case npc::need_result::progressed:
            return guy.activity ? goal_result::activity : goal_result::acted;
        case npc::need_result::impossible:
            guy.plan_for( kind.need ).clear();
            if( had_target ) {
                count_failed_target( guy, kind );
            } else if( kind.need == npc::need_goal_id::drink_water ) {
                // Step 5.6: nothing in reach; water they remember.
                return walk_to_remembered_water( guy );
            }
            return goal_result::none;
        case npc::need_result::holding:
        case npc::need_result::blocked:
        case npc::need_result::deferred:
            // Wait a turn rather than wander off and back.
            guy.mod_moves( -guy.get_speed() );
            return goal_result::acted;
        case npc::need_result::idle:
            if( kind.need == npc::need_goal_id::drink_water && !guy.plan_for( kind.need ).active() ) {
                // Step 5.6: nothing in reach; water they remember.
                return walk_to_remembered_water( guy );
            }
            break;
    }
    return goal_result::none;
}

// Need at 50+, safe, awake, allowed by the follower rules, not given up, the game agrees
// it's worth it, and something in the pack or in reach.
bool may_make_trip( const npc &guy, const trip_kind &kind, const int need )
{
    return enabled() && guy.needs_food() && !guy.in_sleep_state() && is_safe( guy ) &&
           need >= need_to_act && looks_after_self( guy ) && !gave_up( guy, kind ) &&
           guy.intake_useful( kind.need );
}

bool wants_trip( const npc &guy, const trip_kind &kind, const int need )
{
    if( !may_make_trip( guy, kind, need ) ) {
        return false;
    }
    if( carries_for( guy, kind.need ) ) {
        return true;
    }
    static recent_answers<std::pair<character_id, int>, bool> in_reach;
    return in_reach.get( { guy.getID(), static_cast<int>( kind.need ) }, [&guy, &kind]() {
        const behavior::character_oracle_t oracle( &guy );
        return ( kind.need == npc::need_goal_id::drink_water ? oracle.can_obtain_water( "" ) :
                 oracle.can_obtain_food( "" ) ) == behavior::status_t::running;
    } );
}

// The NPC is on this trip right now (the game's own goal for the need, or ours).
bool pursuing( const npc &guy, const trip_kind &kind )
{
    const std::string &goal = guy.get_committed_goal();
    return goal == kind.upstream_goal || goal == kind.our_goal;
}

std::string trip_plan_text( const npc &guy, const trip_kind &kind )
{
    const npc::need_plan &plan = guy.plan_for( kind.need );
    // A plan left over after they moved on (following you, say) isn't shown.
    if( !plan.active() || !pursuing( guy, kind ) ) {
        return std::string();
    }
    const int dist = rl_dist( guy.pos_abs(), plan.target );
    if( dist <= 1 ) {
        return _( kind.plan_here );
    }
    return string_format( _( kind.plan_away ), dist );
}

// ---- Getting warm (step 3.4c) ----

constexpr const char *goal_get_warm = "autonomy_get_warm";
// By a fire, they stay until Warmth is below this (no longer cold).
constexpr int warm_enough = 25;
constexpr const char *var_warm_fails = "autonomy_warm_fails";
constexpr const char *var_warm_gave_up = "autonomy_warm_gave_up_until";
// What they're doing and where, "step:x,y,z": shown in the look-around panel and said once.
constexpr const char *var_warm_plan = "autonomy_warm_plan";
// The fire they last said they were warming up by.
constexpr const char *var_warm_fire = "autonomy_warm_fire";
// 1 while they hold a piece of firewood in their hands for the fire (it didn't fit the pack),
// so a 2x4 they fight with is never mistaken for fuel.
constexpr const char *var_wood_in_hands = "autonomy_wood_in_hands";

enum class warm_step { by_fire, to_fire, to_wood, to_hearth, feed_fire };

struct warm_plan {
    warm_step step;
    tripoint_bub_ms where;
};

// A fireplace, wood stove, fire pit or the like (what the game lets NPCs light).
bool is_hearth( const map &here, const tripoint_bub_ms &p )
{
    if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FIRE_CONTAINER, p ) ) {
        return false;
    }
    const map_data_common_t &top = here.has_furn( p ) ?
                                   static_cast<const map_data_common_t &>( here.furn( p ).obj() ) :
                                   static_cast<const map_data_common_t &>( here.ter( p ).obj() );
    return top.has_examine( iexamine::fireplace );
}

bool burning( const map &here, const tripoint_bub_ms &p )
{
    return here.get_field( p, fd_fire ) != nullptr;
}

// Firewood still lying on a hearth, waiting to burn.
int fuel_on( const tripoint_bub_ms &p )
{
    int pieces = 0;
    for( const item &it : get_map().i_at( p ) ) {
        if( it.has_flag( flag_FIREWOOD ) ) {
            pieces += it.count();
        }
    }
    return pieces;
}

// Pieces of firewood to keep on the fire: the colder it is, the bigger the fire. Mostly the
// air around them (the area's natural temperature, without the fire's own heat, so the fire
// doesn't shrink itself): 10 C or warmer 1, 0-10 C 2, freezing 3. If their body is colder
// than that suggests (Warmth 34+ / 67+), that wins.
int fuel_wanted( const npc &guy )
{
    const double air_c = units::to_celsius( get_weather().get_area_temperature( guy.pos_abs_omt() ) );
    const int for_air = air_c < 0.0 ? 3 : air_c < 10.0 ? 2 : 1;
    const int warmth = current_needs( guy ).warmth;
    const int for_body = warmth >= 67 ? 3 : warmth >= 34 ? 2 : 1;
    return std::max( for_air, for_body );
}

// Holding firewood they picked up for the fire (not their weapon).
bool holds_wood_for_fire( const npc &guy )
{
    const item *held = guy.get_wielded_item().get_item();
    return held != nullptr && held->has_flag( flag_FIREWOOD ) &&
           get_num( guy, var_wood_in_hands, 0.0 ) > 0.0;
}

// Firewood in the pack, or in their hands for the fire. A wielded 2x4 they fight with doesn't count.
bool carries_firewood( const npc &guy )
{
    if( holds_wood_for_fire( guy ) ) {
        return true;
    }
    const item *held = guy.get_wielded_item().get_item();
    return guy.has_item_with( [held]( const item & it ) {
        return &it != held && it.has_flag( flag_FIREWOOD );
    } );
}

// Wood too big for the pack (a plank, a log) can still go in their hands, one piece a trip:
// hands free, or a weapon that fits the pack while they carry it; light enough to lift.
bool can_carry_in_hands( const npc &guy, const item &wood )
{
    if( guy.is_using_bionic_weapon() ) {
        return false;
    }
    if( const item_location held = guy.get_wielded_item() ) {
        if( holds_wood_for_fire( guy ) || !guy.can_unwield( *held ).success() ||
            !const_cast<npc &>( guy ).can_stash( *held ) ) {
            return false;
        }
    }
    return guy.can_wield( wood ).success() && guy.can_pickWeight( wood, true );
}

// Put the wood in their hands on the hearth, then take their weapon back out.
void put_held_wood_on( npc &guy, const tripoint_bub_ms &p )
{
    item_location held = guy.get_wielded_item();
    guy.remove_value( var_wood_in_hands );
    if( !held || !held->has_flag( flag_FIREWOOD ) ) {
        return;
    }
    item wood = *held;
    held.remove_item();
    get_map().add_item_or_charges( p, wood, false );
    guy.mod_moves( -to_moves<int>( 2_seconds ) );
    guy.wield_better_weapon();
}

// How far this NPC may go for warmth: followers stay near the player, guards near their
// post (as the game's own reach rules), anyone else 20 tiles.
bool warm_spot_in_reach( const npc &guy, const tripoint_bub_ms &p )
{
    const map &here = get_map();
    if( guy.is_walking_with() ) {
        int spacing = 6;
        if( guy.rules.has_flag( ally_rule::follow_close ) ) {
            spacing = guy.rules.has_flag( ally_rule::follow_distance_2 ) ? 2 : 4;
        }
        return rl_dist( here.get_abs( p ), get_avatar().pos_abs() ) <= spacing + 2;
    }
    if( const std::optional<tripoint_abs_ms> post = guy.get_guard_post(); post && guy.is_on_shift() ) {
        return rl_dist( here.get_abs( p ), *post ) <= 6;
    }
    return rl_dist( guy.pos_bub(), p ) <= trip_radius;
}

// Nearest tile within 20 that the NPC can see, may go to, and that passes `wanted`.
// The cheap test runs first; line of sight only for tiles that pass it.
template<typename Wanted>
std::optional<tripoint_bub_ms> nearest_seen( const npc &guy, Wanted wanted )
{
    const map &here = get_map();
    for( const tripoint_bub_ms &p : closest_points_first( guy.pos_bub(), trip_radius ) ) {
        if( wanted( here, p ) && warm_spot_in_reach( guy, p ) && guy.sees( here, p ) ) {
            return p;
        }
    }
    return std::nullopt;
}

std::optional<tripoint_bub_ms> nearest_fire( const npc &guy )
{
    return nearest_seen( guy, []( const map & here, const tripoint_bub_ms & p ) {
        return is_hearth( here, p ) && burning( here, p );
    } );
}

std::optional<tripoint_bub_ms> nearest_hearth( const npc &guy )
{
    return nearest_seen( guy, [&guy]( const map & here, const tripoint_bub_ms & p ) {
        return is_hearth( here, p ) && !burning( here, p ) && !guy.is_no_go_position( here.get_abs( p ) );
    } );
}

// Firewood (sticks, planks, logs...) lying where the NPC may take it.
// need_room = false finds wood they may take but can't carry, in the pack or hands (to say why).
std::optional<tripoint_bub_ms> nearest_firewood( const npc &guy, const bool need_room = true )
{
    return nearest_seen( guy, [&guy, need_room]( const map & here, const tripoint_bub_ms & p ) {
        if( !here.has_items( p ) || is_hearth( here, p ) ) {
            return false;
        }
        for( item &it : get_map().i_at( p ) ) {
            if( it.has_flag( flag_FIREWOOD ) && guy.may_take( item_location( map_cursor( p ), &it ) ) &&
                ( !need_room || const_cast<npc &>( guy ).can_stash( it ) ||
                  can_carry_in_hands( guy, it ) ) ) {
                return true;
            }
        }
        return false;
    } );
}

// The next step of the plan: by a fire, to a fire, to the hearth with wood, or for wood.
std::optional<warm_plan> next_warm_step( const npc &guy )
{
    const map &here = get_map();
    std::optional<tripoint_bub_ms> fire;
    for( const tripoint_bub_ms &adj : here.points_in_radius( guy.pos_bub(), 1 ) ) {
        if( adj != guy.pos_bub() && burning( here, adj ) && is_hearth( here, adj ) ) {
            fire = adj;
            break;
        }
    }
    const bool beside_fire = fire.has_value();
    if( !fire ) {
        fire = nearest_fire( guy );
    }
    if( fire ) {
        // Keep it going, bigger the colder it is: put wood on, or fetch some first. Checked
        // away from the fire too, so a trip for wood isn't undone by "go back to the fire".
        if( fuel_on( *fire ) < fuel_wanted( guy ) ) {
            if( carries_firewood( guy ) ) {
                return warm_plan{ beside_fire ? warm_step::feed_fire : warm_step::to_fire, *fire };
            }
            if( const std::optional<tripoint_bub_ms> wood = nearest_firewood( guy ) ) {
                return warm_plan{ warm_step::to_wood, *wood };
            }
        }
        return warm_plan{ beside_fire ? warm_step::by_fire : warm_step::to_fire, *fire };
    }
    if( !guy.autonomy_has_firestarter() ) {
        return std::nullopt;
    }
    const std::optional<tripoint_bub_ms> hearth = nearest_hearth( guy );
    if( !hearth ) {
        return std::nullopt;
    }
    if( carries_firewood( guy ) ) {
        return warm_plan{ warm_step::to_hearth, *hearth };
    }
    if( const std::optional<tripoint_bub_ms> wood = nearest_firewood( guy ) ) {
        return warm_plan{ warm_step::to_wood, *wood };
    }
    return std::nullopt;
}

std::string warm_plan_key( const warm_plan &plan )
{
    const tripoint_abs_ms p = get_map().get_abs( plan.where );
    return string_format( "%d:%d,%d,%d", static_cast<int>( plan.step ), p.x(), p.y(), p.z() );
}

bool gave_up_on_warmth( const npc &guy )
{
    return turns_since_start() < get_num( guy, var_warm_gave_up, -1.0 );
}

void count_failed_warm_step( npc &guy )
{
    const int fails = static_cast<int>( get_num( guy, var_warm_fails, 0.0 ) ) + 1;
    guy.remove_value( var_warm_plan );
    if( fails < max_failed_targets ) {
        guy.set_value( var_warm_fails, static_cast<double>( fails ) );
        return;
    }
    guy.set_value( var_warm_fails, 0.0 );
    guy.set_value( var_warm_gave_up,
                   static_cast<double>( turns_since_start() + to_turns<int>( give_up_for ) ) );
    if( player_sees( guy ) ) {
        add_msg( m_neutral, _( "%s gives up trying to get warm." ), guy.get_name() );
    }
}

// Say what they're off to do, once per step and place.
void announce_warm_step( npc &guy, const warm_plan &plan )
{
    const std::string key = warm_plan_key( plan );
    const diag_value *said = guy.maybe_get_value( var_warm_plan );
    if( said != nullptr && said->is_str() && said->str() == key ) {
        return;
    }
    guy.set_value( var_warm_plan, key );
    if( !player_sees( guy ) ) {
        return;
    }
    switch( plan.step ) {
        case warm_step::to_fire:
            add_msg( m_neutral, _( "%s goes to warm up by the fire." ), guy.get_name() );
            break;
        case warm_step::to_wood:
            add_msg( m_neutral, _( "%s goes to gather wood." ), guy.get_name() );
            break;
        case warm_step::to_hearth:
            add_msg( m_neutral, _( "%s carries the wood to the fireplace." ), guy.get_name() );
            break;
        case warm_step::by_fire:
        case warm_step::feed_fire: {
            // Once per fire, not each time they add wood.
            const std::string fire = target_key( get_map().get_abs( plan.where ) );
            const diag_value *said_fire = guy.maybe_get_value( var_warm_fire );
            if( said_fire == nullptr || !said_fire->is_str() || said_fire->str() != fire ) {
                guy.set_value( var_warm_fire, fire );
                add_msg( m_neutral, _( "%s warms up by the fire." ), guy.get_name() );
            }
            break;
        }
    }
}

// Pick up the firewood on an adjacent tile.
bool take_firewood( npc &guy, const tripoint_bub_ms &p )
{
    map &here = get_map();
    for( item &it : here.i_at( p ) ) {
        item_location loc( map_cursor( p ), &it );
        // Only wood they have room for: anything else would just be dropped again.
        if( it.has_flag( flag_FIREWOOD ) && guy.may_take( loc ) && guy.can_stash( it ) ) {
            guy.i_add( it );
            loc.remove_item();
            guy.mod_moves( -to_moves<int>( 2_seconds ) );
            return true;
        }
    }
    // Too big for the pack: one piece in their hands, their weapon put away meanwhile.
    for( item &it : here.i_at( p ) ) {
        item_location loc( map_cursor( p ), &it );
        if( it.has_flag( flag_FIREWOOD ) && guy.may_take( loc ) && can_carry_in_hands( guy, it ) ) {
            if( item_location held = guy.get_wielded_item() ) {
                guy.stow_item( *held );
            }
            if( guy.get_wielded_item() || !guy.wield( loc ) ) {
                return false;
            }
            guy.set_value( var_wood_in_hands, 1.0 );
            return true;
        }
    }
    return false;
}

goal_result get_warm( npc &guy )
{
    if( gave_up_on_warmth( guy ) ) {
        return goal_result::none;
    }
    // Warmer clothes from the pack, first and anywhere.
    if( guy.wear_warmest_item() ) {
        return goal_result::acted;
    }
    const std::optional<warm_plan> plan = next_warm_step( guy );
    if( !plan ) {
        // Nothing of ours to do: the game's own warmth planner (shelter, clothes nearby).
        guy.remove_value( var_warm_plan );
        switch( guy.execute_regulate_temperature() ) {
            case npc::need_result::progressed:
                return guy.activity ? goal_result::activity : goal_result::acted;
            case npc::need_result::satisfied:
            case npc::need_result::holding:
            case npc::need_result::blocked:
            case npc::need_result::deferred:
                guy.mod_moves( -guy.get_speed() );
                return goal_result::acted;
            case npc::need_result::impossible:
            case npc::need_result::idle:
                break;
        }
        return goal_result::none;
    }
    announce_warm_step( guy, *plan );
    const bool adjacent = square_dist( guy.pos_bub(), plan->where ) <= 1;
    switch( plan->step ) {
        case warm_step::by_fire:
            if( current_needs( guy ).warmth < warm_enough ) {
                guy.set_value( var_warm_fails, 0.0 );
                guy.remove_value( var_warm_plan );
                return goal_result::none;
            }
            // Stay and warm up.
            guy.mod_moves( -guy.get_speed() );
            return goal_result::acted;
        case warm_step::to_hearth:
            if( adjacent ) {
                if( holds_wood_for_fire( guy ) ) {
                    put_held_wood_on( guy, plan->where );
                }
                if( guy.autonomy_light_hearth( plan->where ) ) {
                    guy.set_value( var_warm_fails, 0.0 );
                    if( player_sees( guy ) ) {
                        add_msg( m_neutral, _( "%s lights a fire." ), guy.get_name() );
                    }
                    return guy.activity ? goal_result::activity : goal_result::acted;
                }
                count_failed_warm_step( guy );
                return goal_result::none;
            }
            break;
        case warm_step::to_wood:
            if( adjacent ) {
                if( take_firewood( guy, plan->where ) ) {
                    return goal_result::acted;
                }
                count_failed_warm_step( guy );
                return goal_result::none;
            }
            break;
        case warm_step::feed_fire: {
            item_location wood;
            const item *wielded = guy.get_wielded_item().get_item();
            guy.visit_items( [&wood, wielded]( const item_location & it ) -> VisitResponse {
                if( it.get_item() != wielded && it->has_flag( flag_FIREWOOD ) )
                {
                    wood = it;
                    return VisitResponse::ABORT;
                }
                return VisitResponse::NEXT;
            } );
            if( wood ) {
                get_map().add_item_or_charges( plan->where, guy.i_rem( wood.get_item() ), false );
                guy.mod_moves( -to_moves<int>( 2_seconds ) );
            } else if( holds_wood_for_fire( guy ) ) {
                put_held_wood_on( guy, plan->where );
            } else {
                return goal_result::none;
            }
            if( player_sees( guy ) ) {
                add_msg( m_neutral, _( "%s adds wood to the fire." ), guy.get_name() );
            }
            return goal_result::acted;
        }
        case warm_step::to_fire:
            break;
    }
    // Walk there (next to it: a hearth or a fire isn't a tile to stand on).
    if( guy.move_to_and_verify( npc_trip_spec::within_range( plan->where, 1, false ) ) ) {
        return goal_result::acted;
    }
    count_failed_warm_step( guy );
    return goal_result::none;
}

// ---- Seeking safety (step 5.2) ----

constexpr const char *goal_seek_safety = "autonomy_seek_safety";
// The spot they're heading for or holding, "x,y,z" (absolute). Said once per spot.
constexpr const char *var_safe_spot = "autonomy_safe_spot";
// The last turn they were scared (Safety 50+) while seeking safety.
constexpr const char *var_safe_scared_at = "autonomy_safe_scared_at";
// A spot they couldn't walk to, skipped for a while.
constexpr const char *var_safe_bad = "autonomy_safe_bad";
constexpr const char *var_safe_bad_until = "autonomy_safe_bad_until";
constexpr int safety_radius = 20;
// They carry on until Safety is under 25, and this long after they were last scared.
constexpr int calm_enough = 25;
constexpr time_duration min_hold = 2_minutes;
// A new spot has to beat the current one by this much (so they don't hop between spots
// as hostiles move about).
constexpr float better_by = 2.0f;
constexpr float lit_bonus = 4.0f;
constexpr float friend_bonus = 1.5f;
constexpr float per_tile_from_hostile = 0.5f;
constexpr int hostile_far_enough = 12;
constexpr float walk_cost_per_tile = 0.15f;

struct safety_view {
    std::vector<tripoint_abs_ms> hostiles;
    std::vector<tripoint_abs_ms> friends;
};

// Who they can see that matters: hostiles, and friendly people who are awake.
safety_view look_around( const npc &guy )
{
    safety_view view;
    for( const Creature *critter : guy.get_visible_creatures( safety_radius ) ) {
        if( guy.attitude_to( *critter ) == Creature::Attitude::HOSTILE ) {
            view.hostiles.push_back( critter->pos_abs() );
        }
    }
    const auto consider = [&]( const Character & other ) {
        if( &other != &guy && !other.is_dead_state() && !other.in_sleep_state() &&
            rl_dist( guy.pos_abs(), other.pos_abs() ) <= safety_radius + company_range &&
            guy.attitude_to( other ) == Creature::Attitude::FRIENDLY ) {
            view.friends.push_back( other.pos_abs() );
        }
    };
    consider( get_avatar() );
    for( const npc &other : g->all_npcs() ) {
        consider( other );
    }
    return view;
}

// Followers stay within the game's own follow reach, guards near their post (the same
// rule the game uses for its own trips); anyone else 20 tiles.
bool safe_spot_in_reach( const npc &guy, const tripoint_abs_ms &p )
{
    if( const std::optional<npc_reach> reach = guy.obligation_reach() ) {
        return reach->contains( p );
    }
    // A shopkeeper stays close to their booth.
    if( stays_put( guy ) ) {
        if( const std::optional<tripoint_abs_ms> home = home_spot( guy ) ) {
            return rl_dist( *home, p ) <= home_spot_reach;
        }
    }
    return rl_dist( guy.pos_abs(), p ) <= safety_radius;
}

bool lit( const map &here, const tripoint_bub_ms &p )
{
    // The same test as the AVOIDS_LIGHT monsters (Smilers) use: they won't step into it.
    return here.ambient_light_at( p ) > LIGHT_AMBIENT_LIT;
}

// How safe a tile is, before the walk there: light, friends close by, distance from the
// hostiles they can see. Nullopt for a tile next to a hostile or one they can't stand on.
std::optional<float> spot_score( const npc &guy, const map &here, const safety_view &view,
                                 const tripoint_bub_ms &p )
{
    const bool own_tile = p == guy.pos_bub();
    if( !own_tile && ( !here.passable( p ) || get_creature_tracker().creature_at( p ) != nullptr ||
                       avoids_trap( guy, p ) || guy.is_dangerous_fields( here.field_at( p ) ) ) ) {
        return std::nullopt;
    }
    const tripoint_abs_ms abs = here.get_abs( p );
    int nearest_hostile = hostile_far_enough;
    for( const tripoint_abs_ms &h : view.hostiles ) {
        nearest_hostile = std::min( nearest_hostile, rl_dist( abs, h ) );
    }
    if( nearest_hostile <= 1 && !own_tile ) {
        return std::nullopt;
    }
    int friends_near = 0;
    for( const tripoint_abs_ms &f : view.friends ) {
        if( rl_dist( abs, f ) <= company_range ) {
            friends_near++;
        }
    }
    return ( lit( here, p ) ? lit_bonus : 0.0f ) + friend_bonus * std::min( friends_near, 2 ) +
           per_tile_from_hostile * nearest_hostile;
}

std::optional<tripoint_abs_ms> parse_spot( const diag_value *value )
{
    tripoint_abs_ms p;
    if( value == nullptr || !value->is_str() ||
        std::sscanf( value->str().c_str(), "%d,%d,%d", &p.x(), &p.y(), &p.z() ) != 3 ) {
        return std::nullopt;
    }
    return p;
}

// The safest spot they may reach, or where they stand when nowhere is clearly better.
// Nullopt when they may not stay where they are (a follower left behind) and no spot
// in reach will do.
std::optional<tripoint_abs_ms> find_safe_spot( const npc &guy )
{
    const map &here = get_map();
    const safety_view view = look_around( guy );
    const tripoint_bub_ms pos = guy.pos_bub();
    const std::optional<tripoint_abs_ms> bad = parse_spot( guy.maybe_get_value( var_safe_bad ) );
    const bool bad_active = bad && turns_since_start() < get_num( guy, var_safe_bad_until, -1.0 );

    // What to beat: staying put, or the spot they already picked.
    std::optional<tripoint_abs_ms> keep;
    float keep_total = -1.0e9f;
    if( safe_spot_in_reach( guy, guy.pos_abs() ) ) {
        if( const std::optional<float> s = spot_score( guy, here, view, pos ) ) {
            keep = guy.pos_abs();
            keep_total = *s;
        }
    }
    if( const std::optional<tripoint_abs_ms> picked = parse_spot( guy.maybe_get_value(
                var_safe_spot ) ); picked && *picked != guy.pos_abs() && here.inbounds( *picked ) &&
        safe_spot_in_reach( guy, *picked ) && !( bad_active && *picked == *bad ) ) {
        const tripoint_bub_ms p = here.get_bub( *picked );
        if( const std::optional<float> s = spot_score( guy, here, view, p ) ) {
            const float total = *s - walk_cost_per_tile * rl_dist( pos, p );
            if( total > keep_total ) {
                keep = *picked;
                keep_total = total;
            }
        }
    }

    // Everything in reach, best first; line of sight (the costly part) only for the best few.
    std::vector<std::pair<float, tripoint_bub_ms>> candidates;
    for( const tripoint_bub_ms &p : closest_points_first( pos, safety_radius ) ) {
        if( p == pos || p.z() != pos.z() || !here.inbounds( p ) ) {
            continue;
        }
        const tripoint_abs_ms abs = here.get_abs( p );
        if( ( bad_active && abs == *bad ) || !safe_spot_in_reach( guy, abs ) ) {
            continue;
        }
        if( const std::optional<float> s = spot_score( guy, here, view, p ) ) {
            const float total = *s - walk_cost_per_tile * rl_dist( pos, p );
            if( total > keep_total + better_by ) {
                candidates.emplace_back( total, p );
            }
        }
    }
    std::stable_sort( candidates.begin(), candidates.end(), []( const auto & a, const auto & b ) {
        return a.first > b.first;
    } );
    constexpr size_t max_sight_checks = 30;
    for( size_t i = 0; i < candidates.size() && i < max_sight_checks; ++i ) {
        if( guy.sees( here, candidates[i].second ) ) {
            return here.get_abs( candidates[i].second );
        }
    }
    return keep;
}

// The search scans 20 tiles and the behavior tree asks every turn, so reuse the answer.
std::optional<tripoint_abs_ms> safe_spot( const npc &guy )
{
    static recent_answers<character_id, std::optional<tripoint_abs_ms>> spots;
    return spots.get( guy.getID(), [&guy]() {
        return find_safe_spot( guy );
    } );
}

void forget_safe_spot( npc &guy )
{
    guy.remove_value( var_safe_spot );
    guy.remove_value( var_safe_scared_at );
}

goal_result seek_safety( npc &guy )
{
    if( safety_need( guy ) >= unsafe ) {
        guy.set_value( var_safe_scared_at, static_cast<double>( turns_since_start() ) );
    }
    const std::optional<tripoint_abs_ms> spot = safe_spot( guy );
    if( !spot ) {
        forget_safe_spot( guy );
        return goal_result::none;
    }
    map &here = get_map();
    const std::string key = target_key( *spot );
    const diag_value *said = guy.maybe_get_value( var_safe_spot );
    if( said == nullptr || !said->is_str() || said->str() != key ) {
        // Said once per scare, not each time a better spot turns up.
        if( said == nullptr && *spot != guy.pos_abs() && player_sees( guy ) ) {
            add_msg( m_neutral, lit( here, here.get_bub( *spot ) ) ?
                     _( "%s backs into the light." ) : _( "%s backs off to a safer spot." ),
                     guy.get_name() );
        }
        guy.set_value( var_safe_spot, key );
    }
    if( *spot == guy.pos_abs() ) {
        // Hold here, eyes open, until calmer. Step 5.6: a place to come back to.
        npc_memory::remember( guy, npc_memory::place_kind::safe, *spot );
        guy.mod_moves( -guy.get_speed() );
        return goal_result::acted;
    }
    if( guy.move_to_and_verify( npc_trip_spec::stand_on( here.get_bub( *spot ), true ) ) ) {
        return goal_result::acted;
    }
    // Can't get there: skip that spot for a while and look again.
    guy.set_value( var_safe_bad, key );
    guy.set_value( var_safe_bad_until,
                   static_cast<double>( turns_since_start() + to_turns<int>( 10_minutes ) ) );
    forget_safe_spot( guy );
    return goal_result::none;
}

std::string safe_plan_text( const npc &guy )
{
    if( guy.get_committed_goal() != goal_seek_safety ) {
        return std::string();
    }
    const std::optional<tripoint_abs_ms> spot = parse_spot( guy.maybe_get_value( var_safe_spot ) );
    if( !spot ) {
        return std::string();
    }
    const map &here = get_map();
    const bool in_light = here.inbounds( *spot ) && lit( here, here.get_bub( *spot ) );
    const int dist = rl_dist( guy.pos_abs(), *spot );
    if( dist == 0 ) {
        return in_light ? _( "Plan: stay safe (holding in the light)" ) :
               _( "Plan: stay safe (holding here)" );
    }
    return in_light ?
           string_format( _( "Plan: stay safe (heading to a lit spot %d tiles away)" ), dist ) :
           string_format( _( "Plan: stay safe (heading to a safer spot %d tiles away)" ), dist );
}

// ---- Back to their spot ----

constexpr const char *goal_return_home = "autonomy_return_home";
// A spot they couldn't get back to: they stop trying until this turn.
constexpr const char *var_home_gave_up = "autonomy_home_gave_up_until";

goal_result return_home( npc &guy )
{
    const std::optional<tripoint_abs_ms> home = home_spot( guy );
    map &here = get_map();
    if( !home || *home == guy.pos_abs() || !here.inbounds( *home ) ) {
        return goal_result::none;
    }
    const tripoint_bub_ms spot = here.get_bub( *home );
    // Someone in their spot: stand next to it.
    const Creature *there = get_creature_tracker().creature_at( spot );
    const npc_trip_spec trip = there != nullptr ? npc_trip_spec::within_range( spot, 1, true ) :
                               npc_trip_spec::stand_on( spot, true );
    if( there != nullptr && rl_dist( guy.pos_bub(), spot ) <= 1 ) {
        return goal_result::none;
    }
    if( guy.move_to_and_verify( trip ) ) {
        return goal_result::acted;
    }
    guy.set_value( var_home_gave_up,
                   static_cast<double>( turns_since_start() + to_turns<int>( 30_minutes ) ) );
    return goal_result::none;
}

std::string home_plan_text( const npc &guy )
{
    if( guy.get_committed_goal() != goal_return_home ) {
        return std::string();
    }
    const std::optional<tripoint_abs_ms> home = home_spot( guy );
    if( !home ) {
        return std::string();
    }
    return string_format( _( "Plan: back to their spot (%d tiles away)" ),
                          rl_dist( guy.pos_abs(), *home ) );
}

// ---- Picking up what they dropped in a fight (reported 2026-11-18) ----
// Dropping copies an item into the map with a new id, so a fight's start tags what the NPC
// carries with an item var (copied along with the item); the fight's end untags what they
// still carry, and the tagged things lying about are theirs to pick up.

constexpr const char *goal_recover_items = "autonomy_recover_items";
// Item var: the id of the NPC who carried it when a fight started.
constexpr const char *ivar_carried_by = "autonomy_carried_by";
// Item var: it was the weapon in their hands.
constexpr const char *ivar_was_wielded = "autonomy_was_wielded";
// NPC var: 1 while a fight is on (tags placed).
constexpr const char *var_in_fight = "autonomy_in_fight";
// NPC var: they look for their things until this turn.
constexpr const char *var_recover_until = "autonomy_recover_until";
// 20 tiles (2026-11-20, was 10): arrows and bolts land well past what they were shot at.
constexpr int recover_radius = 20;
// A follower may walk this far from the player for their own things (their follow distance
// is only 2-6 tiles); if the player walks off, the item drops out of reach and they follow.
constexpr int recover_follower_reach = 15;
constexpr time_duration recover_for = 30_minutes;
// NPC var: "place:misses" for the spot they couldn't walk to last.
constexpr const char *var_recover_failed = "autonomy_recover_failed";

std::string carrier_key( const npc &guy )
{
    return std::to_string( guy.getID().get_value() );
}

// Cheap checks only: this runs every NPC turn.
bool in_a_fight( const npc &guy )
{
    return guy.get_ai_danger() > 0.0f || guy.has_effect( effect_npc_run_away ) ||
           sees_hostile_within( guy, watch_hostile_range );
}

// The weapon in hand, what they wear, and what's in what they wear (not deeper: a whole bag
// dropped comes back with its contents).
void for_each_carried( npc &guy, const std::function<void( item & )> &fn )
{
    item_location weapon = guy.get_wielded_item();
    if( weapon ) {
        fn( *weapon.get_item() );
    }
    for( item_location &worn : guy.top_items_loc() ) {
        fn( *worn );
        for( item *inside : worn->all_items_top( pocket_type::CONTAINER ) ) {
            fn( *inside );
        }
    }
}

bool is_mine_to_recover( const npc &guy, const item &it )
{
    return it.get_var( ivar_carried_by ) == carrier_key( guy );
}

// Their things on this tile: lying loose, or in a corpse (N26, 2026-11-20: an arrow that sticks
// in a monster is in its inventory, and in its corpse once it dies).
bool has_lost_item_at( const npc &guy, map &here, const tripoint_bub_ms &p )
{
    for( const item &it : here.i_at( p ) ) {
        if( is_mine_to_recover( guy, it ) ) {
            return true;
        }
        if( it.is_corpse() ) {
            for( const item *inside : it.all_items_top( pocket_type::CONTAINER ) ) {
                if( is_mine_to_recover( guy, *inside ) ) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::vector<item_location> lost_items_at( const npc &guy, const tripoint_bub_ms &p )
{
    std::vector<item_location> found;
    for( item &it : get_map().i_at( p ) ) {
        item_location loc( map_cursor( p ), &it );
        if( is_mine_to_recover( guy, it ) ) {
            found.push_back( loc );
        } else if( it.is_corpse() ) {
            for( item *inside : it.all_items_top( pocket_type::CONTAINER ) ) {
                if( is_mine_to_recover( guy, *inside ) ) {
                    found.emplace_back( loc, inside );
                }
            }
        }
    }
    return found;
}

// The nearest tagged item of theirs lying where they may go. Not only tiles they can see
// (N26: an arrow that flew on into the dark was left); they know where they shot.
std::optional<tripoint_bub_ms> nearest_lost_item( const npc &guy )
{
    map &here = get_map();
    for( const tripoint_bub_ms &p : closest_points_first( guy.pos_bub(), recover_radius ) ) {
        if( !here.inbounds( p ) || !here.has_items( p ) || !has_lost_item_at( guy, here, p ) ) {
            continue;
        }
        const tripoint_abs_ms where = here.get_abs( p );
        const bool in_reach = guy.is_walking_with() ?
                              rl_dist( get_player_character().pos_abs(), where ) <= recover_follower_reach :
                              safe_spot_in_reach( guy, where );
        if( in_reach ) {
            return p;
        }
    }
    return std::nullopt;
}

// Pick up their things on an adjacent tile: into the pack, or in hand if it was their weapon
// or won't fit and their hands are free. Untags what they take.
bool take_lost_items( npc &guy, const tripoint_bub_ms &p )
{
    bool took = false;
    bool stuck = false;
    for( item_location &loc : lost_items_at( guy, p ) ) {
        const item &it = *loc;
        const bool was_weapon = it.get_var( ivar_was_wielded, 0.0 ) > 0.0;
        item copy = it;
        copy.erase_var( ivar_carried_by );
        copy.erase_var( ivar_was_wielded );
        // N26 (2026-11-20): out of arrows in a fight, an NPC may take the quiver off to hold it
        // (the game rated it above the empty bow); then nothing fits and they stopped looking.
        const item_location held = guy.get_wielded_item();
        if( held && !guy.can_stash( copy ) && held->is_armor() && !held->is_gun() &&
            held->can_contain( copy ).success() && guy.can_wear( *held ).success() ) {
            item to_wear = guy.remove_weapon();
            guy.wear_item( to_wear, false );
        }
        if( was_weapon && !guy.get_wielded_item() && guy.can_wield( copy ).success() ) {
            guy.wield( copy );
        } else if( guy.can_stash( copy ) ) {
            guy.i_add( copy );
        } else if( !guy.get_wielded_item() && guy.can_wield( copy ).success() ) {
            guy.wield( copy );
        } else {
            stuck = true;
            continue;
        }
        loc.remove_item();
        guy.mod_moves( -to_moves<int>( 1_seconds ) );
        took = true;
        // One per turn: the stack changed.
        break;
    }
    if( took ) {
        guy.wield_better_weapon();
        if( player_sees( guy ) ) {
            add_msg( m_neutral, _( "%s picks their things back up." ), guy.get_name() );
        }
    } else if( stuck ) {
        // Can't carry what's left: stop looking.
        guy.remove_value( var_recover_until );
    }
    return took;
}

goal_result recover_items( npc &guy )
{
    const std::optional<tripoint_bub_ms> spot = nearest_lost_item( guy );
    if( !spot ) {
        guy.remove_value( var_recover_until );
        return goal_result::none;
    }
    if( square_dist( guy.pos_bub(), *spot ) <= 1 ) {
        return take_lost_items( guy, *spot ) ? goal_result::acted : goal_result::none;
    }
    const std::string key = target_key( get_map().get_abs( *spot ) );
    if( guy.move_to_and_verify( npc_trip_spec::within_range( *spot, 1, true ) ) ) {
        guy.remove_value( var_recover_failed );
        return goal_result::acted;
    }
    // A walk can fail for a moment (the way changed): only the third miss in a row counts.
    const std::string failed = guy.get_value( var_recover_failed ).to_string();
    const int misses = failed.rfind( key + ":", 0 ) == 0 ?
                       std::atoi( failed.c_str() + key.size() + 1 ) + 1 : 1;
    if( misses < 3 ) {
        guy.set_value( var_recover_failed, key + ":" + std::to_string( misses ) );
        return goal_result::none;
    }
    guy.remove_value( var_recover_failed );
    // Can't get there: give up on what lies there, not on the rest (N26: one arrow over a
    // fence made them stop looking for all of them).
    for( item_location &loc : lost_items_at( guy, *spot ) ) {
        loc->erase_var( ivar_carried_by );
        loc->erase_var( ivar_was_wielded );
    }
    return goal_result::none;
}

std::string recover_plan_text( const npc &guy )
{
    if( guy.get_committed_goal() != goal_recover_items ) {
        return std::string();
    }
    return _( "Plan: pick their things back up" );
}

} // namespace

void drank_at_water( npc &guy, const tripoint_bub_ms &pos )
{
    // The game only lets NPCs sip from water terrain that never runs dry, so it's lasting.
    npc_memory::remember( guy, npc_memory::place_kind::water, get_map().get_abs( pos ) );
}

void started_drinking( npc &guy, const item_location &loc )
{
    if( !enabled() || !loc || loc.where() != item_location::type::map ) {
        // In a bottle, a pack or a vehicle: not a place to come back to.
        return;
    }
    const item &it = *loc;
    if( !it.made_of( phase_id::LIQUID ) || !it.is_comestible() ||
        it.get_comestible()->quench <= 0 ) {
        return;
    }
    const map &here = get_map();
    const tripoint_bub_ms pos = loc.pos_bub( here );
    if( !here.has_flag_furn( ter_furn_flag::TFLAG_LIQUIDCONT, pos ) ) {
        return;
    }
    npc_memory::remember( guy, npc_memory::place_kind::water, here.get_abs( pos ) );
}

void grab_dropped_weapon( npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || guy.is_using_bionic_weapon() ) {
        return;
    }
    item_location held = guy.get_wielded_item();
    if( held && ( held->is_gun() || held->is_melee() ) && !holds_wood_for_fire( guy ) ) {
        return;
    }
    map &here = get_map();
    const std::string key = carrier_key( guy );
    for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
        if( !here.has_items( p ) ) {
            continue;
        }
        for( item &it : here.i_at( p ) ) {
            if( it.get_var( ivar_carried_by ) != key || it.get_var( ivar_was_wielded, 0.0 ) <= 0.0 ) {
                continue;
            }
            item copy = it;
            copy.erase_var( ivar_carried_by );
            copy.erase_var( ivar_was_wielded );
            if( !guy.can_wield( copy ).success() ) {
                return;
            }
            if( held ) {
                // Wood or a tool in hand: into the pack, or on the ground.
                guy.stow_item( *held );
                if( guy.get_wielded_item() ) {
                    return;
                }
            }
            item_location loc( map_cursor( p ), &it );
            guy.wield( copy );
            loc.remove_item();
            if( player_sees( guy ) ) {
                add_msg( m_neutral, _( "%s grabs their weapon back." ), guy.get_name() );
            }
            return;
        }
    }
}

void track_fight_items( npc &guy )
{
    if( !enabled() ) {
        return;
    }
    const bool fighting = in_a_fight( guy );
    const bool was_fighting = get_num( guy, var_in_fight, 0.0 ) > 0.0;
    if( fighting == was_fighting ) {
        return;
    }
    const std::string key = carrier_key( guy );
    if( fighting ) {
        guy.set_value( var_in_fight, 1.0 );
        const item *weapon = guy.get_wielded_item().get_item();
        // Wood carried for a fire is set down on purpose; never fetch it back.
        const bool wood_for_fire = holds_wood_for_fire( guy );
        for_each_carried( guy, [&]( item & it ) {
            if( &it == weapon && wood_for_fire ) {
                return;
            }
            it.set_var( ivar_carried_by, key );
            if( &it == weapon ) {
                it.set_var( ivar_was_wielded, 1.0 );
            }
        } );
        return;
    }
    // Fight over: what they still carry isn't lost.
    guy.remove_value( var_in_fight );
    for( item *it : guy.items_with( []( const item & i ) {
    return i.has_var( ivar_carried_by );
    } ) ) {
        it->erase_var( ivar_carried_by );
        it->erase_var( ivar_was_wielded );
    }
    guy.set_value( var_recover_until,
                   static_cast<double>( turns_since_start() + to_turns<int>( recover_for ) ) );
}

bool wants_to_recover_items( const npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || !is_safe( guy ) ||
        turns_since_start() >= get_num( guy, var_recover_until, -1.0 ) ) {
        return false;
    }
    // The tile scan is cheap enough only every few turns.
    static recent_answers<character_id, bool> lost_items;
    return lost_items.get( guy.getID(), [&guy]() {
        return nearest_lost_item( guy ).has_value();
    } );
}

bool worth_carrying( const npc &guy, const item &it )
{
    if( !enabled() || !guy.is_player_ally() ) {
        return true;
    }
    if( is_mine_to_recover( guy, it ) ) {
        return true;
    }
    if( ( food_need( guy ) >= need_to_act || water_need( guy ) >= need_to_act ) &&
        guy.will_eat( it ).success() ) {
        return true;
    }
    const item_location weapon = guy.get_wielded_item();
    if( weapon && weapon->is_gun() &&
        ( ( it.is_magazine() && weapon->magazine_compatible().count( it.typeId() ) > 0 ) ||
          ( it.is_ammo() && weapon->ammo_types().count( it.ammo_type() ) > 0 ) ) ) {
        return true;
    }
    // Junk categories: building scraps (scrap metal, pipes, planks, plastic chunks, rags, nails,
    // rocks), empty containers, odds and ends, vehicle parts and corpses. A filled container counts
    // as what's in it. The player's pickup rules (c:spare_parts) can still ask for them.
    static const std::array<std::string, 5> junk_categories{ {
            "spare_parts", "container", "other", "veh_parts", "corpses"
        }
    };
    const std::string category = it.get_category_of_contents().get_id().str();
    if( std::find( junk_categories.begin(), junk_categories.end(), category ) !=
        junk_categories.end() ) {
        return false;
    }
    // Heavy and nearly worthless: a kilogram or more at under 25 cents a kilogram (npc::value is in
    // cents, the post-apocalypse price scaled up for what they need).
    const double kg = units::to_gram( it.weight() ) / 1000.0;
    return kg < 1.0 || guy.value( it ) / kg >= 25.0;
}

bool fetches_own_things( const npc &guy )
{
    return enabled() && guy.is_walking_with() &&
           rl_dist( get_player_character().pos_abs(), guy.pos_abs() ) <= recover_follower_reach &&
           wants_to_recover_items( guy );
}

bool is_goal( const std::string &goal )
{
    return goal == goal_meal || goal == goal_socialize || goal == goal_get_water ||
           goal == goal_get_food || goal == goal_get_warm || goal == goal_seek_safety ||
           goal == goal_return_home || goal == goal_recover_items;
}

bool stays_put( const npc &guy )
{
    return !guy.is_player_ally() && !guy.is_walking_with() &&
           ( guy.mission == NPC_MISSION_SHOPKEEP || guy.mission == NPC_MISSION_SHELTER );
}

void remember_home_spot( npc &guy )
{
    if( !enabled() || !stays_put( guy ) || guy.maybe_get_value( var_home_spot ) != nullptr ||
        guy.get_committed_goal() == goal_seek_safety || guy.has_effect( effect_npc_run_away ) ||
        !is_safe( guy ) ) {
        return;
    }
    guy.set_value( var_home_spot, target_key( guy.pos_abs() ) );
}

void arm_up( npc &guy )
{
    if( !enabled() || !calendar::once_every( 3_turns ) || guy.in_sleep_state() || guy.activity ||
        guy.is_using_bionic_weapon() || !sees_hostile_within( guy, watch_hostile_range ) ) {
        return;
    }
    // Only for empty hands, or hands holding something that isn't a weapon (wood for a fire,
    // a tool). Swapping one weapon for another is left to the game's own attack choice: asking
    // for "the best weapon" every few turns rated a pistol over the machete it had just drawn
    // for a zombie next to it, so a runner swapped back and forth and never attacked
    // (reported 2026-11-18).
    const item_location held = guy.get_wielded_item();
    if( held && ( held->is_gun() || held->is_melee() ) && !holds_wood_for_fire( guy ) ) {
        return;
    }
    // The game's own choice of weapon (it follows the follower rules on guns).
    guy.wield_better_weapon();
}

bool safe_for_daily_life( const npc &guy )
{
    return is_safe( guy );
}

bool wants_return_home( const npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || !stays_put( guy ) || !is_safe( guy ) ||
        turns_since_start() < get_num( guy, var_home_gave_up, -1.0 ) ) {
        return false;
    }
    const std::optional<tripoint_abs_ms> home = home_spot( guy );
    return home && *home != guy.pos_abs() && get_map().inbounds( *home ) &&
           !( get_creature_tracker().creature_at( get_map().get_bub( *home ) ) != nullptr &&
              rl_dist( guy.pos_abs(), *home ) <= 1 );
}

namespace
{

// A gun in hand they're allowed to fire (the same rules the game's own weapon choice uses)
// with a shot in it, or ammo to reload.
bool ready_gunner( npc &guy )
{
    const item_location weapon = guy.get_wielded_item();
    if( !weapon || !weapon->is_gun() || !guy.rules.has_flag( ally_rule::use_guns ) ) {
        return false;
    }
    bool may_fire = false;
    for( const std::pair<const gun_mode_id, gun_mode> &mode : weapon->gun_all_modes() ) {
        if( !( mode.second.melee() || mode.second.flags.count( "NPC_AVOID" ) ||
               !guy.can_use( *mode.second.target ) ||
               ( guy.rules.has_flag( ally_rule::use_silent ) && guy.is_player_ally() &&
                 !quiet_enough_for_follower_rules( *mode.second.target ) ) ) ) {
            may_fire = true;
            break;
        }
    }
    return may_fire && ( weapon->shots_remaining( get_map(), &guy ) > 0 || guy.can_reload_current() );
}

} // namespace

gunner_stance gunner_stance_for( npc &guy )
{
    if( !enabled() || !ready_gunner( guy ) ) {
        return gunner_stance::none;
    }
    const float watched = watched_threat( guy, watch_hostile_range );
    if( watched >= own_strength( guy ) ) {
        return gunner_stance::none;
    }
    bool close = false;
    for( const Creature *critter : guy.get_visible_creatures( 2 ) ) {
        if( guy.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
            continue;
        }
        if( critter->get_speed() >= guy.get_speed() ) {
            // Can't outpace it: fight as the game would.
            return gunner_stance::none;
        }
        close = true;
    }
    return close ? gunner_stance::step_back : gunner_stance::hold;
}

// A gunner backs off from something slower this close (see kite_step). With fire_now() they loose
// from 2-3 tiles without a long aim, so stepping out of reach is enough.
constexpr int kite_gap = 1;
// A follower backing off to shoot may go this far from the player (their follow distance is only
// 2-6 tiles, gone in a few steps); guards keep to their post. A first piece of Phase 9 item 6.
constexpr int kite_follower_reach = 10;

// A follower may back off to within kite_follower_reach of the player; anyone else with a reach
// (a guard) keeps to it.
static bool may_kite_to( const npc &guy, const std::optional<npc_reach> &reach,
                         const tripoint_abs_ms &p )
{
    if( guy.is_walking_with() ) {
        return rl_dist( get_player_character().pos_abs(), p ) <= kite_follower_reach;
    }
    return !reach || reach->contains( p );
}

bool kite_step( npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || !ready_gunner( guy ) ) {
        return false;
    }
    std::vector<tripoint_bub_ms> foes;
    bool beside = false;
    for( const Creature *critter : guy.get_visible_creatures( watch_hostile_range ) ) {
        if( guy.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
            continue;
        }
        if( rl_dist( guy.pos_bub(), critter->pos_bub() ) <= kite_gap ) {
            if( critter->get_speed() >= guy.get_speed() ) {
                // Can't get away from it: fight as the game would.
                return false;
            }
            beside = true;
        }
        foes.push_back( critter->pos_bub() );
    }
    if( !beside ) {
        return false;
    }
    const auto nearest_foe = [&foes]( const tripoint_bub_ms & p ) {
        int best = std::numeric_limits<int>::max();
        int total = 0;
        for( const tripoint_bub_ms &foe : foes ) {
            const int dist = rl_dist( p, foe );
            best = std::min( best, dist );
            total += dist;
        }
        return std::make_pair( best, total );
    };
    map &here = get_map();
    const std::optional<npc_reach> reach = guy.obligation_reach();
    const std::pair<int, int> now = nearest_foe( guy.pos_bub() );
    std::optional<tripoint_bub_ms> best_tile;
    std::pair<int, int> best = now;
    for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
        if( p == guy.pos_bub() || !guy.can_move_to( p, true ) || avoids_trap( guy, p ) ||
            !may_kite_to( guy, reach, here.get_abs( p ) ) ) {
            continue;
        }
        // Only a tile further from the nearest of them, then from all of them together.
        const std::pair<int, int> there = nearest_foe( p );
        if( there.first > best.first || ( there.first == best.first && best_tile &&
                                          there.second > best.second ) ) {
            best = there;
            best_tile = p;
        }
    }
    if( !best_tile || best.first <= now.first ) {
        return false;
    }
    guy.move_to( *best_tile, true );
    return true;
}

bool ready_the_shot( npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || !ready_gunner( guy ) ) {
        return false;
    }
    item_location weapon = guy.get_wielded_item();
    if( weapon->ammo_remaining() > 0 || !guy.can_reload_current() ) {
        return false;
    }
    bool in_view = false;
    for( const Creature *critter : guy.get_visible_creatures( watch_hostile_range ) ) {
        if( guy.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
            continue;
        }
        if( rl_dist( guy.pos_bub(), critter->pos_bub() ) <= kite_gap ) {
            // Right beside them: stepping away (kite_step) or fighting comes first.
            return false;
        }
        in_view = true;
    }
    if( !in_view ) {
        return false;
    }
    guy.do_reload( weapon );
    return true;
}

bool fire_now( const npc &shooter )
{
    if( !enabled() ) {
        return false;
    }
    for( const Creature *critter : shooter.get_visible_creatures( watch_hostile_range ) ) {
        if( shooter.attitude_to( *critter ) != Creature::Attitude::HOSTILE ) {
            continue;
        }
        // Tiles it covers in two turns (a zombie, speed 70: about 1.4), plus the tile beside them.
        const int reach = 1 + ( 2 * std::max( critter->get_speed(), 0 ) + 99 ) / 100;
        if( rl_dist( shooter.pos_bub(), critter->pos_bub() ) <= reach ) {
            return true;
        }
    }
    return false;
}

bool shoots_from_here( npc &guy )
{
    if( !enabled() || !ready_gunner( guy ) ) {
        return false;
    }
    // Outgunned or not: walking up to club something with a bow is never better. Only
    // something close and as fast as them (no getting away from it) is fought as the game would.
    for( const Creature *critter : guy.get_visible_creatures( 2 ) ) {
        if( guy.attitude_to( *critter ) == Creature::Attitude::HOSTILE &&
            critter->get_speed() >= guy.get_speed() ) {
            return false;
        }
    }
    return true;
}

bool shoots_from_kite_range( npc &guy )
{
    return enabled() && guy.is_walking_with() &&
           rl_dist( get_player_character().pos_abs(), guy.pos_abs() ) <= kite_follower_reach &&
           gunner_stance_for( guy ) != gunner_stance::none;
}

void tag_shot_ammo( Character &shooter, item &drop, const bool fired )
{
    npc *guy = dynamic_cast<npc *>( &shooter );
    if( guy == nullptr || !enabled() || drop.is_null() ) {
        return;
    }
    const bool in_fight = get_num( *guy, var_in_fight, 0.0 ) > 0.0;
    // A throw only in a fight: not something an NPC tosses to the player.
    if( !fired && !in_fight ) {
        return;
    }
    if( !in_fight ) {
        // N26: a shot at something they don't count as a fight yet (further off than they
        // watch). The fight's end starts the time to look for it.
        guy->set_value( var_in_fight, 1.0 );
    }
    drop.set_var( ivar_carried_by, carrier_key( *guy ) );
}

bool stands_with_player( const npc &guy )
{
    return enabled() && guy.is_player_ally() && !guy.rules.has_flag( ally_rule::allow_fall_back );
}

bool falls_back_to_player( const npc &guy )
{
    return enabled() && guy.is_player_ally() && guy.rules.has_flag( ally_rule::allow_fall_back );
}

bool may_step_back( const npc &guy )
{
    const std::optional<npc_reach> reach = guy.obligation_reach();
    return !reach || reach->contains( guy.pos_abs() );
}

bool wants_safety( const npc &guy )
{
    // A follower or guard only goes looking for a safe spot once the player allows it.
    if( !enabled() || guy.in_sleep_state() || stands_with_player( guy ) ) {
        return false;
    }
    const float safety = safety_need( guy );
    if( safety < unsafe ) {
        // Once on their way or at their spot, they carry on until calmer (Safety under 25)
        // and two minutes after they were last scared. Dropping it the moment a hostile
        // went out of sight sent them back and forth between two zombies (2026-11-17).
        const double scared_at = get_num( guy, var_safe_scared_at, -1.0 );
        if( guy.get_committed_goal() != goal_seek_safety || scared_at < 0.0 ) {
            return false;
        }
        const bool calm_long_enough = turns_since_start() - scared_at >= to_turns<double>( min_hold );
        if( to_need( safety ) < calm_enough && calm_long_enough ) {
            return false;
        }
    }
    return safe_spot( guy ).has_value();
}

std::string warm_blocker( const npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || current_needs( guy ).warmth < need_to_act ) {
        return std::string();
    }
    if( !is_safe( guy ) ) {
        return _( "Cold, but not safe to deal with it" );
    }
    if( gave_up_on_warmth( guy ) ) {
        return _( "Cold, gave up for now" );
    }
    if( next_warm_step( guy ) ) {
        // There is a way: the plan line shows it once they're on it.
        return std::string();
    }
    if( !guy.autonomy_has_firestarter() ) {
        return _( "Cold: no fire in reach and nothing that works to light one" );
    }
    if( !nearest_hearth( guy ) ) {
        return guy.is_walking_with() ?
               _( "Cold: no fireplace, stove or fire pit close to you" ) :
               _( "Cold: no fireplace, stove or fire pit in reach" );
    }
    if( nearest_firewood( guy, false ) ) {
        return _( "Cold: can't carry the firewood (no room in the pack, hands busy, or too heavy)" );
    }
    return _( "Cold: no firewood in sight" );
}

bool wants_warmth( const npc &guy )
{
    if( !enabled() || guy.in_sleep_state() || !is_safe( guy ) || gave_up_on_warmth( guy ) ) {
        return false;
    }
    const int warmth = current_needs( guy ).warmth;
    if( warmth < warm_enough ) {
        return false;
    }
    // Already beside a fire: stay until no longer cold, not just until 49.
    const map &here = get_map();
    for( const tripoint_bub_ms &adj : here.points_in_radius( guy.pos_bub(), 1 ) ) {
        if( adj != guy.pos_bub() && burning( here, adj ) && is_hearth( here, adj ) ) {
            return true;
        }
    }
    if( warmth < need_to_act ) {
        return false;
    }
    static recent_answers<character_id, bool> warm_steps;
    return warm_steps.get( guy.getID(), [&guy]() {
        return next_warm_step( guy ).has_value() ||
               behavior::character_oracle_t( &guy ).can_regulate_temperature( "" ) ==
               behavior::status_t::running;
    } );
}

bool wants_water( const npc &guy )
{
    const int need = enabled() ? water_need( guy ) : 0;
    if( wants_trip( guy, water_trip, need ) ) {
        return true;
    }
    // Step 5.6: nothing in reach, but they remember water.
    return may_make_trip( guy, water_trip, need ) && remembered_water( guy ).has_value();
}

bool wants_food( const npc &guy )
{
    return wants_trip( guy, food_trip, enabled() ? food_need( guy ) : 0 );
}

bool hungry_enough_for_a_trip( const npc &guy )
{
    return enabled() && guy.needs_food() && food_need( guy ) >= need_to_act;
}

int water_search_radius()
{
    return enabled() ? trip_radius : 6;
}

int food_search_radius()
{
    return enabled() ? trip_radius : 6;
}

bool refill_bottle( npc &guy, const itype_id &liquid )
{
    const item water( liquid, calendar::turn );
    for( item *container : guy.items_with( []( const item & it ) {
    return it.is_watertight_container() && it.is_container_empty();
    } ) ) {
        const int room = container->get_remaining_capacity_for_liquid( water, guy );
        if( room > 0 && container->fill_with( water, room ) > 0 ) {
            guy.mod_moves( -to_moves<int>( 10_seconds ) );
            return true;
        }
    }
    return false;
}

bool avoids_trap( const npc &guy, const tripoint_bub_ms &p )
{
    if( !enabled() ) {
        return false;
    }
    const map &here = get_map();
    const trap &tr = here.tr_at( p );
    if( tr.is_null() || tr.is_benign() || tr.is_always_invisible() ) {
        return false;
    }
    // Known to them, or in plain sight (pits, spikes: nothing hides them) where they can see.
    // Hidden traps (a buried landmine) only count once known.
    return tr.can_see( p, guy ) || ( tr.is_trivial_to_spot() && guy.sees( here, p ) );
}

void drop_stale_plans( npc &guy )
{
    for( const trip_kind *kind : {
             &water_trip, &food_trip
         } ) {
        npc::need_plan &plan = guy.plan_for( kind->need );
        if( plan.active() && !pursuing( guy, *kind ) ) {
            plan.clear();
            guy.remove_value( kind->target_var );
        }
    }
    if( guy.get_committed_goal() != goal_get_warm ) {
        guy.remove_value( var_warm_plan );
        guy.remove_value( var_warm_fire );
        if( !holds_wood_for_fire( guy ) ) {
            guy.remove_value( var_wood_in_hands );
        }
    }
    if( guy.get_committed_goal() != goal_seek_safety ) {
        forget_safe_spot( guy );
    }
}

void set_down_wood_after_trip( npc &guy )
{
    // Checked every turn, not once a minute: warmed up the moment they picked the plank up,
    // they kept holding it instead of their weapon until the next minute (reported 2026-11-17).
    // Not mid-fight: their combat code picks the weapon then.
    if( guy.get_committed_goal() != goal_get_warm && holds_wood_for_fire( guy ) && is_safe( guy ) ) {
        put_held_wood_on( guy, guy.pos_bub() );
    }
}

namespace
{

std::string warm_plan_text( const npc &guy )
{
    const diag_value *plan = guy.maybe_get_value( var_warm_plan );
    if( plan == nullptr || !plan->is_str() || guy.get_committed_goal() != goal_get_warm ) {
        return std::string();
    }
    int step = 0;
    tripoint_abs_ms where;
    if( std::sscanf( plan->str().c_str(), "%d:%d,%d,%d", &step, &where.x(), &where.y(),
                     &where.z() ) != 4 ) {
        return std::string();
    }
    const int dist = rl_dist( guy.pos_abs(), where );
    switch( static_cast<warm_step>( step ) ) {
        case warm_step::by_fire:
            return _( "Plan: get warm (by the fire)" );
        case warm_step::feed_fire:
            return _( "Plan: get warm (adding wood to the fire)" );
        case warm_step::to_fire:
            return dist <= 1 ? _( "Plan: get warm (by the fire)" ) :
                   string_format( _( "Plan: get warm (heading to a fire %d tiles away)" ), dist );
        case warm_step::to_wood:
            return string_format( _( "Plan: get warm (gathering wood %d tiles away)" ), dist );
        case warm_step::to_hearth:
            return string_format( _( "Plan: get warm (taking wood to a fireplace %d tiles away)" ), dist );
    }
    return std::string();
}

} // namespace

std::string plan_text( const npc &guy )
{
    if( !enabled() ) {
        return std::string();
    }
    const std::string water = trip_plan_text( guy, water_trip );
    if( !water.empty() ) {
        return water;
    }
    const std::string food = trip_plan_text( guy, food_trip );
    if( !food.empty() ) {
        return food;
    }
    const std::string safe = safe_plan_text( guy );
    if( !safe.empty() ) {
        return safe;
    }
    const std::string home = home_plan_text( guy );
    if( !home.empty() ) {
        return home;
    }
    const std::string recover = recover_plan_text( guy );
    if( !recover.empty() ) {
        return recover;
    }
    // N27 (2026-11-20): mealtimes and chats had no plan line.
    if( guy.get_committed_goal() == goal_meal ) {
        return _( "Plan: have a meal" );
    }
    if( guy.get_committed_goal() == goal_socialize ) {
        return _( "Plan: have a chat" );
    }
    return warm_plan_text( guy );
}

bool wants_meal( const npc &guy )
{
    if( !enabled() || !guy.needs_food() || guy.in_sleep_state() || !is_safe( guy ) ||
        !looks_after_self( guy ) ) {
        return false;
    }
    const int meal = current_meal();
    if( meal < 0 || get_num( guy, var_last_meal, -1.0 ) == meal_key( meal ) ||
        food_need( guy ) < meal_hunger ) {
        return false;
    }
    return carries_meal( guy );
}

bool wants_meal_at_post( const npc &guy )
{
    const std::optional<tripoint_abs_ms> post = guy.get_guard_post();
    return post && guy.is_on_shift() && guy.pos_abs() == *post && wants_meal( guy );
}

bool should_leave_meal( const npc &guy )
{
    if( !enabled() || !guy.is_walking_with() ) {
        return false;
    }
    // The follow distance from the follower rules (as in npc_arbitration's follow_spacing),
    // plus a little slack so a step or two doesn't end the meal.
    int spacing = 6;
    if( guy.rules.has_flag( ally_rule::follow_close ) ) {
        spacing = guy.rules.has_flag( ally_rule::follow_distance_2 ) ? 2 : 4;
    }
    return rl_dist( guy.pos_abs(), get_avatar().pos_abs() ) > spacing + 2;
}

bool should_catch_up( const npc &guy, const std::string &goal )
{
    // Step 5.3: a chat gives way too.
    static const std::array<std::string, 7> eat_or_drink{ {
            "eat_food", "drink_water", goal_meal, goal_get_food, goal_get_water, goal_get_warm,
            goal_socialize
        }
    };
    if( std::find( eat_or_drink.begin(), eat_or_drink.end(), goal ) == eat_or_drink.end() ) {
        return false;
    }
    if( !should_leave_meal( guy ) ) {
        return false;
    }
    // A real emergency still comes first.
    const needs_t needs = current_needs( guy );
    return needs.food < 100 && needs.water < 100;
}

// Step 5.3: may they walk over to chat with someone there? The game's own rules for followers
// and guards decide (npc::judge_option, with the reach from npc::obligation_reach): a chat
// is never worth stretching them, so only "allowed" will do. A follower stays within follow
// distance, a guard near their post, an NPC sent somewhere goes there first; anyone else
// may walk the 15 tiles.
static bool may_walk_over_to_chat( const npc &guy, const tripoint_abs_ms &where )
{
    npc_option_cost cost = npc_option_cost::located( where, npc_option_basis::destination );
    cost.walks = rl_dist( guy.pos_abs(), where ) > 1;
    const std::optional<npc_reach> reach = guy.obligation_reach();
    cost.route_in_reach = !reach || reach->contains( where );
    return guy.judge_option( cost ).permission == npc_permission::allowed;
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
            may_walk_over_to_chat( guy, other.pos_abs() ) && guy.sees( here, other ) ) {
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

bool wants_company_at_post( const npc &guy )
{
    // Guard duty (0.45) always beat a plain chat (0.25), so guards never chatted, even with
    // another guard beside them (reported 2026-11-18). The partner is already limited to the
    // post area (may_walk_over_to_chat).
    const std::optional<tripoint_abs_ms> post = guy.get_guard_post();
    return post && guy.is_on_shift() && wants_company( guy );
}

goal_result run_goal( npc &guy, const std::string &goal )
{
    if( goal == goal_seek_safety ) {
        return seek_safety( guy );
    }
    if( goal == goal_return_home ) {
        return return_home( guy );
    }
    if( goal == goal_recover_items ) {
        return recover_items( guy );
    }
    if( goal == goal_meal ) {
        return eat_meal( guy );
    }
    if( goal == goal_socialize ) {
        return socialize( guy );
    }
    if( goal == goal_get_water ) {
        return fetch( guy, water_trip );
    }
    if( goal == goal_get_warm ) {
        return get_warm( guy );
    }
    if( goal == goal_get_food ) {
        return fetch( guy, food_trip );
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
