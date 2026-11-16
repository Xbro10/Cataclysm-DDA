// Xbro10 fork: tests for "NPC autonomy (experimental)" (src/npc_autonomy.*).

#include <algorithm>
#include <sstream>
#include <string>

#include "activity_type.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "math_parser_diag_value.h"
#include "monster.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"
#include "weather.h"

static const efftype_id effect_hunger_hungry( "hunger_hungry" );
static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );

static const activity_id ACT_CONSUME( "ACT_CONSUME" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bottle_plastic( "bottle_plastic" );
static const itype_id itype_butane( "butane" );
static const itype_id itype_hoodie( "hoodie" );
static const itype_id itype_jeans( "jeans" );
static const itype_id itype_lighter( "lighter" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_sandwich_cheese_grilled( "sandwich_cheese_grilled" );
static const itype_id itype_splinter( "splinter" );
static const itype_id itype_tshirt( "tshirt" );
static const itype_id itype_water_clean( "water_clean" );

static const effect_on_condition_id effect_on_condition_EOC_AUTONOMY_DEBUG_LONELY( "EOC_AUTONOMY_DEBUG_LONELY" );
static const effect_on_condition_id
effect_on_condition_EOC_AUTONOMY_DEBUG_NOT_LONELY( "EOC_AUTONOMY_DEBUG_NOT_LONELY" );

static const furn_str_id furn_f_cupboard( "f_cupboard" );
static const furn_str_id furn_f_fireplace( "f_fireplace" );

static const field_type_str_id field_fd_fire( "fd_fire" );
static const furn_str_id furn_f_water_heater( "f_water_heater" );

static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const trap_str_id tr_spike_pit( "tr_spike_pit" );

static const faction_id faction_no_faction( "no_faction" );
static const faction_id faction_your_followers( "your_followers" );

static npc &spawn_follower( const point_bub_ms &where )
{
    npc &guy = spawn_npc( where, "test_talker" );
    clear_character( guy );
    // clear_character() parks everyone at (60, 60), on top of the avatar.
    guy.setpos( get_map(), tripoint_bub_ms( where, 0 ) );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_fac( faction_your_followers );
    return guy;
}

static double stored( const npc &guy, const std::string &key )
{
    const diag_value *val = guy.maybe_get_value( key );
    REQUIRE( val != nullptr );
    return val->dbl();
}

TEST_CASE( "npc_autonomy_needs_off_by_default", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option autonomy( "NPC_AUTONOMY", "false" );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 10, 0 ) );

    npc_autonomy::on_npc_turn( guy );
    CHECK( guy.maybe_get_value( "autonomy_need_social" ) == nullptr );
    CHECK( guy.maybe_get_value( "autonomy_need_food" ) == nullptr );
}

TEST_CASE( "npc_autonomy_social_need", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();

    SECTION( "alone, it rises: a quarter after 6 hours" ) {
        npc &guy = spawn_follower( player_pos + point( 10, 0 ) );
        REQUIRE_FALSE( npc_autonomy::has_company( guy ) );
        npc_autonomy::update_needs( guy, 6_hours );
        CHECK( stored( guy, "autonomy_need_social" ) == Approx( 25.0 ) );
        CHECK( npc_autonomy::current_needs( guy ).social == 25 );
        // All six needs are stored as plain vars.
        for( const char *need : {
                 "food", "water", "sleep", "warmth", "safety", "social"
             } ) {
            CHECK( guy.maybe_get_value( std::string( "autonomy_need_" ) + need ) != nullptr );
        }
    }

    SECTION( "next to the player, it falls" ) {
        npc &guy = spawn_follower( player_pos + point::east );
        guy.set_value( "autonomy_need_social", 80.0 );
        REQUIRE( npc_autonomy::has_company( guy ) );
        npc_autonomy::update_needs( guy, 1_hours );
        CHECK( stored( guy, "autonomy_need_social" ) == Approx( 30.0 ) );
        npc_autonomy::update_needs( guy, 2_hours );
        CHECK( stored( guy, "autonomy_need_social" ) == Approx( 0.0 ) );
    }

    SECTION( "another follower nearby counts as company" ) {
        npc &guy = spawn_follower( player_pos + point( 10, 0 ) );
        spawn_follower( player_pos + point( 12, 0 ) );
        CHECK( npc_autonomy::has_company( guy ) );
    }
}

TEST_CASE( "npc_autonomy_felt_needs", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option autonomy( "NPC_AUTONOMY", "true" );
    override_option food_on( "NO_NPC_FOOD", "false" );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 10, 0 ) );
    // Water still in the stomach lowers felt thirst, so empty it first.
    guy.stomach.empty();
    const auto set_felt_thirst = [&]( int felt ) {
        guy.set_thirst( felt );
        REQUIRE( guy.get_instant_thirst() == felt );
    };

    SECTION( "fed, watered and rested: 0" ) {
        set_felt_thirst( 0 );
        guy.set_sleepiness( 0 );
        const npc_autonomy::needs_t needs = npc_autonomy::current_needs( guy );
        CHECK( needs.food == 0 );
        CHECK( needs.water == 0 );
        CHECK( needs.sleep == 0 );
    }
    SECTION( "the game's levels map onto the scale" ) {
        guy.add_effect( effect_hunger_very_hungry, 1_hours );
        set_felt_thirst( 80 );     // Very thirsty
        guy.set_sleepiness( 383 ); // Dead tired
        const npc_autonomy::needs_t needs = npc_autonomy::current_needs( guy );
        CHECK( needs.food == 50 );
        CHECK( needs.water == 50 );
        CHECK( needs.sleep == 67 );
    }
    SECTION( "in between levels it is a straight line" ) {
        set_felt_thirst( 160 );    // halfway from Very thirsty to Dehydrated
        guy.set_sleepiness( 575 ); // Exhausted
        const npc_autonomy::needs_t needs = npc_autonomy::current_needs( guy );
        CHECK( needs.water == 63 );
        CHECK( needs.sleep == 100 );
    }
    // get_healthy_kcal() is a fat BMI of 5; the game calls 3 normal and 2 underweight.
    SECTION( "a normal weight adds nothing" ) {
        guy.set_stored_kcal( guy.get_healthy_kcal() * 7 / 10 ); // fat BMI 3.5
        CHECK( npc_autonomy::current_needs( guy ).food == 0 );
    }
    SECTION( "being underweight counts as hunger" ) {
        guy.set_stored_kcal( guy.get_healthy_kcal() * 2 / 5 ); // fat BMI 2
        CHECK( npc_autonomy::current_needs( guy ).food == 50 );
        guy.set_stored_kcal( guy.get_healthy_kcal() / 5 );     // fat BMI 1, emaciated
        CHECK( npc_autonomy::current_needs( guy ).food == 75 );
    }
    SECTION( "with No NPC Food, food and water are 0" ) {
        override_option no_food( "NO_NPC_FOOD", "true" );
        guy.add_effect( effect_hunger_very_hungry, 1_hours );
        set_felt_thirst( 240 );
        const npc_autonomy::needs_t needs = npc_autonomy::current_needs( guy );
        CHECK_FALSE( needs.eats );
        CHECK( needs.food == 0 );
        CHECK( needs.water == 0 );
    }
}

TEST_CASE( "npc_autonomy_mealtimes", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option food_on( "NO_NPC_FOOD", "false" );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 10, 0 ) );
    // Peckish: below the game's own "eat when Hungry" line (hunger 80).
    guy.stomach.empty();
    guy.set_hunger( 50 );
    guy.add_effect( effect_hunger_hungry, 1_days );
    guy.i_add( item( itype_sandwich_cheese_grilled ) );

    SECTION( "option off: never" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        set_time( calendar::turn_zero + 7_hours );
        CHECK_FALSE( npc_autonomy::wants_meal( guy ) );
    }
    SECTION( "at breakfast, a Peckish NPC with food eats, once" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        set_time( calendar::turn_zero + 7_hours );
        REQUIRE( npc_autonomy::wants_meal( guy ) );
        CHECK( npc_autonomy::run_goal( guy, "autonomy_meal" ) != npc_autonomy::goal_result::none );
        CHECK_FALSE( npc_autonomy::wants_meal( guy ) );
    }
    SECTION( "at 3 PM, not a mealtime" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        set_time( calendar::turn_zero + 15_hours );
        CHECK_FALSE( npc_autonomy::wants_meal( guy ) );
    }
    SECTION( "no food in the pack: no meal" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        set_time( calendar::turn_zero + 12_hours );
        guy.remove_items_with( []( const item & it ) {
            return it.is_food();
        } );
        CHECK_FALSE( npc_autonomy::wants_meal( guy ) );
    }
}

// Xbro10 fork fixes: an NPC saved mid-meal used to lose its food on load. It isn't
// registered with the game while it loads ("Failed to find item_location owner"), and
// every item in a pocket got a new uid on load ("UID not found in container contents").
TEST_CASE( "npc_autonomy_meals_on_post_and_when_left_behind", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option food_on( "NO_NPC_FOOD", "false" );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    set_time( calendar::turn_zero + 7_hours );
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_follower( player_pos + point( 3, 0 ) );
    guy.stomach.empty();
    guy.set_hunger( 50 );
    guy.add_effect( effect_hunger_hungry, 1_days );
    guy.i_add( item( itype_sandwich_cheese_grilled ) );

    SECTION( "a guard on their post eats from their pack there" ) {
        guy.set_attitude( NPCATT_NULL );
        guy.set_guard_pos( guy.pos_abs() );
        REQUIRE( guy.is_on_shift() );
        CHECK( npc_autonomy::wants_meal_at_post( guy ) );
        // Off the post: not this rule (the plain mealtime rule still applies).
        guy.setpos( get_map(), guy.pos_bub() + point( 1, 0 ) );
        CHECK_FALSE( npc_autonomy::wants_meal_at_post( guy ) );
    }
    SECTION( "a follower near the player keeps eating" ) {
        CHECK_FALSE( npc_autonomy::should_leave_meal( guy ) );
    }
    SECTION( "a follower left behind drops the meal" ) {
        guy.setpos( get_map(), tripoint_bub_ms( player_pos + point( 12, 0 ), 0 ) );
        CHECK( npc_autonomy::should_leave_meal( guy ) );
        override_option off( "NPC_AUTONOMY", "false" );
        CHECK_FALSE( npc_autonomy::should_leave_meal( guy ) );
    }
}

TEST_CASE( "npc_autonomy_rule_leave_food_and_water_to_me", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option food_on( "NO_NPC_FOOD", "false" );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    set_time( calendar::turn_zero + 7_hours );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 3, 0 ) );
    guy.stomach.empty();
    guy.set_hunger( 50 );
    guy.add_effect( effect_hunger_hungry, 1_days );
    guy.i_add( item( itype_sandwich_cheese_grilled ) );
    // Default (and old saves): they look after themselves.
    REQUIRE_FALSE( guy.rules.has_flag( ally_rule::no_self_care ) );
    CHECK( npc_autonomy::wants_meal( guy ) );
    guy.rules.set_flag( ally_rule::no_self_care );
    CHECK_FALSE( npc_autonomy::wants_meal( guy ) );
    guy.set_thirst( 100 );
    CHECK_FALSE( npc_autonomy::wants_water( guy ) );
}

// Reported 2026-11-15: a starving follower with food in their pack, left behind by the
// player, stood eating, never followed, and didn't fight a zombie next to them.
TEST_CASE( "npc_autonomy_starving_follower_left_behind", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    override_option food_on( "NO_NPC_FOOD", "false" );
    set_time( calendar::turn_zero + 12_hours );
    avatar &you = get_avatar();
    const tripoint_bub_ms start = you.pos_bub();
    npc &guy = spawn_follower( start.xy() + point( 2, 0 ) );
    guy.set_stored_kcal( guy.get_healthy_kcal() / 5 ); // badly underweight: Food 75+
    for( int i = 0; i < 3; ++i ) {
        guy.i_add( item( itype_sandwich_cheese_grilled ) );
    }
    const auto run = [&guy]( int turns ) {
        std::string trace;
        for( int t = 0; t < turns; ++t ) {
            guy.set_moves( guy.get_speed() );
            guy.move();
            if( t % 5 == 0 ) {
                trace += string_format( " t%d:d%d act=%s goal=%s", t,
                                        rl_dist( guy.pos_abs(), get_avatar().pos_abs() ),
                                        guy.activity.id().str(), guy.get_committed_goal() );
            }
        }
        return trace;
    };

    SECTION( "autonomy on, the player walks 12 tiles away: they come along" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        run( 5 );
        you.setpos( get_map(), start + tripoint( -12, 0, 0 ) );
        const std::string trace = run( 40 );
        INFO( trace );
        CHECK( rl_dist( guy.pos_abs(), you.pos_abs() ) <= 8 );
    }
    SECTION( "autonomy on, a zombie next to them: they stop eating and fight" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        run( 3 );
        monster &zed = spawn_test_monster( "mon_zombie", guy.pos_bub() + point( 1, 0 ) );
        const std::string trace = run( 3 );
        INFO( trace );
        // As upstream's own NPC combat tests check: the target, and no meal in the way.
        CHECK( guy.current_target() == &zed );
        CHECK( guy.get_committed_goal() == "fight" );
        CHECK_FALSE( guy.is_eating() );
    }
    SECTION( "autonomy off: the game's own behaviour, they keep eating where they are" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        run( 5 );
        you.setpos( get_map(), start + tripoint( -12, 0, 0 ) );
        run( 20 );
        CHECK( rl_dist( guy.pos_abs(), you.pos_abs() ) > 8 );
    }
}

TEST_CASE( "npc_meal_survives_save_and_load", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option food_on( "NO_NPC_FOOD", "false" );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    set_time( calendar::turn_zero + 7_hours );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 10, 0 ) );
    guy.stomach.empty();
    guy.set_hunger( 50 );
    guy.add_effect( effect_hunger_hungry, 1_days );
    // In a backpack, so the meal points into a container on the NPC.
    guy.wear_item( item( itype_backpack ), false );
    guy.i_add( item( itype_sandwich_cheese_grilled ) );
    REQUIRE( npc_autonomy::run_goal( guy, "autonomy_meal" ) == npc_autonomy::goal_result::activity );
    REQUIRE( guy.activity.id() == ACT_CONSUME );
    const auto sandwich_uid = []( const npc & who ) {
        const std::vector<const item *> found = who.items_with( []( const item & it ) {
            return it.typeId() == itype_sandwich_cheese_grilled;
        } );
        return found.empty() ? int64_t( 0 ) : found.front()->uid().get_value();
    };
    const int64_t uid_before = sandwich_uid( guy );
    REQUIRE( uid_before != 0 );

    std::ostringstream saved;
    JsonOut jsout( saved );
    guy.serialize( jsout );
    // Gone from the game, like every NPC before a save is loaded.
    clear_npcs();

    // Fails the test with a debug message if the meal can't find its food.
    npc loaded;
    JsonValue jv = json_loader::from_string( saved.str() );
    loaded.deserialize( jv.get_object() );
    CHECK( sandwich_uid( loaded ) == uid_before );
    CHECK( loaded.activity.id() == ACT_CONSUME );
}

TEST_CASE( "npc_autonomy_socializing", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_follower( player_pos + point( 10, 0 ) );
    npc &friend_npc = spawn_follower( player_pos + point( 11, 0 ) );
    guy.set_value( "autonomy_need_social", 80.0 );
    friend_npc.set_value( "autonomy_need_social", 60.0 );

    SECTION( "option off: never" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        CHECK_FALSE( npc_autonomy::wants_company( guy ) );
    }
    SECTION( "a lonely NPC chats with a friend next to them" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        REQUIRE( npc_autonomy::chat_partner( guy ) == &friend_npc );
        REQUIRE( npc_autonomy::wants_company( guy ) );
        CHECK( npc_autonomy::run_goal( guy, "autonomy_socialize" ) == npc_autonomy::goal_result::acted );
        CHECK( stored( guy, "autonomy_need_social" ) == Approx( 40.0 ) );
        CHECK( stored( friend_npc, "autonomy_need_social" ) == Approx( 20.0 ) );
        // Then a 30-minute break before the next chat.
        guy.set_value( "autonomy_need_social", 80.0 );
        CHECK_FALSE( npc_autonomy::wants_company( guy ) );
    }
    SECTION( "not lonely enough: no chat" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.set_value( "autonomy_need_social", 30.0 );
        CHECK_FALSE( npc_autonomy::wants_company( guy ) );
    }
}

TEST_CASE( "npc_autonomy_get_water", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option food_on( "NO_NPC_FOOD", "false" );
    map &here = get_map();
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_follower( player_pos + point( 2, 0 ) );
    // Past the game's own 6-tile search, inside ours.
    const tripoint_bub_ms pool( player_pos + point( 12, 0 ), 0 );
    here.ter_set( pool, ter_t_water_sh );
    guy.stomach.empty();
    guy.set_thirst( 100 ); // Very thirsty: Water about 53

    SECTION( "option off: never, and the search stays at 6 tiles" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        CHECK( npc_autonomy::water_search_radius() == 6 );
        CHECK_FALSE( npc_autonomy::wants_water( guy ) );
    }
    SECTION( "not thirsty enough: no trip" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.set_thirst( 40 ); // Thirsty: Water 25
        CHECK_FALSE( npc_autonomy::wants_water( guy ) );
    }
    SECTION( "following a player on the move: the game's leash keeps them from far water" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        CHECK_FALSE( npc_autonomy::wants_water( guy ) );
    }
    SECTION( "very thirsty, on their own: walks to water 10 tiles off and drinks" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.set_attitude( NPCATT_NULL );
        guy.set_fac( faction_no_faction );
        REQUIRE( npc_autonomy::current_needs( guy ).water >= 50 );
        REQUIRE( npc_autonomy::wants_water( guy ) );
        for( int turn = 0; turn < 60 && guy.get_thirst() >= 100; ++turn ) {
            guy.set_moves( guy.get_speed() );
            npc_autonomy::run_goal( guy, "autonomy_get_water" );
        }
        CHECK( rl_dist( guy.pos_bub(), pool ) <= 1 );
        CHECK( guy.get_thirst() < 100 );
    }
}

// A follower next to the player, water a few tiles off: a bottle on the floor, or a
// water cooler (keg furniture, water held on its tile as in the Backrooms' f_watercooler).
TEST_CASE( "npc_autonomy_get_water_close_by", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option food_on( "NO_NPC_FOOD", "false" );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    map &here = get_map();
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_follower( player_pos + point( 1, 0 ) );
    const tripoint_bub_ms spot( player_pos + point( 4, 0 ), 0 );
    guy.stomach.empty();
    guy.set_thirst( 100 );

    SECTION( "a bottle of water on the floor" ) {
        item bottle( itype_bottle_plastic );
        bottle.fill_with( item( itype_water_clean, calendar::turn ) );
        here.add_item_or_charges( spot, bottle );
    }
    SECTION( "a water cooler" ) {
        here.furn_set( spot, furn_f_water_heater );
        here.add_item( spot, item( itype_water_clean, calendar::turn, 30 ) );
    }
    REQUIRE( npc_autonomy::wants_water( guy ) );
    for( int turn = 0; turn < 40 && guy.get_thirst() >= 100; ++turn ) {
        guy.set_moves( guy.get_speed() );
        if( guy.activity ) {
            // Drinking from an item is a short activity.
            guy.activity.do_turn( guy );
        } else {
            npc_autonomy::run_goal( guy, "autonomy_get_water" );
        }
    }
    CHECK( guy.get_thirst() < 100 );
}

TEST_CASE( "npc_autonomy_debug_lonely", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 3, 0 ) );
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    effect_on_condition_EOC_AUTONOMY_DEBUG_LONELY->activate( d );
    CHECK( stored( guy, "autonomy_need_social" ) == Approx( 100.0 ) );
    effect_on_condition_EOC_AUTONOMY_DEBUG_NOT_LONELY->activate( d );
    CHECK( stored( guy, "autonomy_need_social" ) == Approx( 0.0 ) );
}

TEST_CASE( "npc_autonomy_no_water_is_not_a_failed_try", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option food_on( "NO_NPC_FOOD", "false" );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 1, 0 ) );
    guy.stomach.empty();
    guy.set_thirst( 100 );
    for( int turn = 0; turn < 10; ++turn ) {
        guy.set_moves( guy.get_speed() );
        npc_autonomy::run_goal( guy, "autonomy_get_water" );
    }
    // Nothing to find isn't a target that failed: no giving up, no message.
    CHECK( guy.maybe_get_value( "autonomy_water_gave_up_until" ) == nullptr );
}

TEST_CASE( "npc_autonomy_get_food", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option food_on( "NO_NPC_FOOD", "false" );
    map &here = get_map();
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_follower( player_pos + point( 2, 0 ) );
    // Past the game's own 6-tile search, inside ours.
    const tripoint_bub_ms spot( player_pos + point( 12, 0 ), 0 );
    guy.stomach.empty();
    guy.add_effect( effect_hunger_very_hungry, 1_days ); // Food 50
    const auto eaten = [&guy, &here]() {
        const tripoint_bub_ms where( get_avatar().pos_bub().xy() + point( 12, 0 ), 0 );
        // A 10-tile walk, then a meal, which takes real time (an activity).
        for( int turn = 0; turn < 600 && !here.i_at( where ).empty(); ++turn ) {
            guy.set_moves( guy.get_speed() );
            if( guy.activity ) {
                guy.activity.do_turn( guy );
            } else {
                npc_autonomy::run_goal( guy, "autonomy_get_food" );
            }
        }
        CAPTURE( rl_dist( guy.pos_bub(), where ) );
        return here.i_at( where ).empty();
    };

    SECTION( "option off: never, and the search stays at 6 tiles" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        here.add_item_or_charges( spot, item( itype_sandwich_cheese_grilled ) );
        CHECK( npc_autonomy::food_search_radius() == 6 );
        CHECK_FALSE( npc_autonomy::wants_food( guy ) );
    }
    SECTION( "only Hungry: no trip" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        here.add_item_or_charges( spot, item( itype_sandwich_cheese_grilled ) );
        guy.remove_effect( effect_hunger_very_hungry );
        guy.add_effect( effect_hunger_hungry, 1_days ); // Food 25
        CHECK_FALSE( npc_autonomy::wants_food( guy ) );
    }
    SECTION( "following a player on the move: the game's leash keeps them from far food" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        here.add_item_or_charges( spot, item( itype_sandwich_cheese_grilled ) );
        CHECK_FALSE( npc_autonomy::wants_food( guy ) );
    }
    SECTION( "on their own: walks to food 10 tiles off and eats it" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        here.add_item_or_charges( spot, item( itype_sandwich_cheese_grilled ) );
        guy.set_attitude( NPCATT_NULL );
        guy.set_fac( faction_no_faction );
        REQUIRE( npc_autonomy::wants_food( guy ) );
        CHECK( eaten() );
        CHECK( rl_dist( guy.pos_bub(), spot ) <= 1 );
    }
    SECTION( "on their own: finds food in a cupboard" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        here.furn_set( spot, furn_f_cupboard );
        here.add_item_or_charges( spot, item( itype_sandwich_cheese_grilled ) );
        guy.set_attitude( NPCATT_NULL );
        guy.set_fac( faction_no_faction );
        REQUIRE( npc_autonomy::wants_food( guy ) );
        CHECK( eaten() );
    }
    SECTION( "the 'leave food and water to me' rule stops it" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        here.add_item_or_charges( spot, item( itype_sandwich_cheese_grilled ) );
        guy.rules.set_flag( ally_rule::no_self_care );
        CHECK_FALSE( npc_autonomy::wants_food( guy ) );
    }
}

// Reported 2026-11-16: a hungry NPC walked into spiked pits to reach food in the middle.
TEST_CASE( "npc_autonomy_food_behind_traps", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option food_on( "NO_NPC_FOOD", "false" );
    map &here = get_map();
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_follower( player_pos + point( 2, 0 ) );
    guy.set_attitude( NPCATT_NULL );
    guy.set_fac( faction_no_faction );
    guy.stomach.empty();
    guy.add_effect( effect_hunger_very_hungry, 1_days );
    const tripoint_bub_ms food_spot( player_pos + point( 10, 0 ), 0 );
    here.add_item_or_charges( food_spot, item( itype_sandwich_cheese_grilled ) );
    std::vector<tripoint_bub_ms> pits;
    for( const tripoint_bub_ms &p : here.points_in_radius( food_spot, 1 ) ) {
        if( p != food_spot ) {
            here.trap_set( p, tr_spike_pit.id() );
            pits.push_back( p );
        }
    }
    here.build_map_cache( 0 );
    const int hp_before = guy.get_hp();

    SECTION( "autonomy on: they never step into the pits" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        for( int turn = 0; turn < 120; ++turn ) {
            guy.set_moves( guy.get_speed() );
            if( guy.activity ) {
                guy.activity.do_turn( guy );
            } else {
                npc_autonomy::run_goal( guy, "autonomy_get_food" );
            }
            CHECK( std::find( pits.begin(), pits.end(), guy.pos_bub() ) == pits.end() );
        }
        CHECK( guy.get_hp() == hp_before );
        CHECK_FALSE( here.i_at( food_spot ).empty() );
        CHECK( npc_autonomy::avoids_trap( guy, pits.front() ) );
    }
    SECTION( "autonomy off: pits are only costly (the game's own rule)" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        CHECK_FALSE( npc_autonomy::avoids_trap( guy, pits.front() ) );
    }
}

TEST_CASE( "npc_autonomy_get_warm", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    map &here = get_map();
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_follower( player_pos + point( 2, 0 ) );
    guy.set_all_parts_temp_conv( BODYTEMP_VERY_COLD ); // Warmth about 67
    const tripoint_bub_ms hearth( player_pos + point( 10, 0 ), 0 );
    here.furn_set( hearth, furn_f_fireplace );
    const auto on_their_own = [&guy]() {
        guy.set_attitude( NPCATT_NULL );
        guy.set_fac( faction_no_faction );
    };
    const auto run = [&guy]( int turns ) {
        for( int turn = 0; turn < turns; ++turn ) {
            guy.set_moves( guy.get_speed() );
            if( guy.activity ) {
                guy.activity.do_turn( guy );
            } else {
                npc_autonomy::run_goal( guy, "autonomy_get_warm" );
            }
        }
    };

    SECTION( "option off: never" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        CHECK_FALSE( npc_autonomy::wants_warmth( guy ) );
    }
    SECTION( "warm enough: nothing to do" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        CHECK_FALSE( npc_autonomy::wants_warmth( guy ) );
    }
    SECTION( "a fire in the fireplace: they walk over and stay by it" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        here.add_field( hearth, field_fd_fire, 1 );
        REQUIRE( npc_autonomy::wants_warmth( guy ) );
        run( 40 );
        CHECK( square_dist( guy.pos_bub(), hearth ) <= 1 );
        CHECK( npc_autonomy::wants_warmth( guy ) );
    }
    SECTION( "an unlit fireplace, wood nearby and a lighter: they gather the wood and light it" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        guy.wear_item( item( itype_backpack ), false );
        item lighter( itype_lighter );
        lighter.ammo_set( itype_butane, 100 );
        guy.i_add( lighter );
        here.add_item_or_charges( tripoint_bub_ms( player_pos + point( 6, 3 ), 0 ), item( itype_2x4 ) );
        REQUIRE( npc_autonomy::wants_warmth( guy ) );
        REQUIRE( guy.autonomy_has_firestarter() );
        run( 300 );
        CHECK( here.get_field( hearth, field_fd_fire ) != nullptr );
        // The fire is only ever in the hearth.
        CHECK_FALSE( here.get_field( guy.pos_bub(), field_fd_fire ) );
    }
    SECTION( "very cold, several planks: they light the fire and keep bringing wood until it's big" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        guy.wear_item( item( itype_backpack ), false );
        item lighter( itype_lighter );
        lighter.ammo_set( itype_butane, 100 );
        guy.i_add( lighter );
        for( int i = 0; i < 4; ++i ) {
            here.add_item_or_charges( tripoint_bub_ms( player_pos + point( 6, 3 ), 0 ), item( itype_2x4 ) );
        }
        REQUIRE( npc_autonomy::current_needs( guy ).warmth >= 67 );
        run( 600 );
        CHECK( here.get_field( hearth, field_fd_fire ) != nullptr );
        int pieces = 0;
        for( const item &it : here.i_at( hearth ) ) {
            pieces += it.typeId() == itype_2x4 ? 1 : 0;
        }
        INFO( "Warmth " << npc_autonomy::current_needs( guy ).warmth << ", plan: " <<
              npc_autonomy::plan_line( guy ) );
        CHECK( pieces >= 3 );
    }
    SECTION( "no bag for the plank: they carry it over in their hands and take their weapon back" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        guy.wear_item( item( itype_jeans ), false ); // pockets, but no room for a plank
        item lighter( itype_lighter );
        lighter.ammo_set( itype_butane, 100 );
        guy.i_add( lighter );
        item rock( itype_rock );
        REQUIRE( guy.can_stash( rock ) );
        guy.wield( rock );
        here.add_item_or_charges( tripoint_bub_ms( player_pos + point( 6, 3 ), 0 ), item( itype_2x4 ) );
        REQUIRE_FALSE( guy.can_stash( item( itype_2x4 ) ) );
        REQUIRE( npc_autonomy::wants_warmth( guy ) );
        run( 300 );
        CHECK( here.get_field( hearth, field_fd_fire ) != nullptr );
        REQUIRE( guy.get_wielded_item() );
        CHECK( guy.get_wielded_item()->typeId() == itype_rock );
    }
    SECTION( "a 2x4 they fight with is not firewood" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        guy.wear_item( item( itype_jeans ), false );
        item lighter( itype_lighter );
        lighter.ammo_set( itype_butane, 100 );
        guy.i_add( lighter );
        item plank( itype_2x4 );
        guy.wield( plank );
        run( 50 );
        CHECK_FALSE( here.get_field( hearth, field_fd_fire ) );
        REQUIRE( guy.get_wielded_item() );
        CHECK( guy.get_wielded_item()->typeId() == itype_2x4 );
    }
    SECTION( "beside a fire with no wood left on it: they add wood from their pack" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        guy.setpos( here, hearth + tripoint( -1, 0, 0 ) );
        here.add_field( hearth, field_fd_fire, 1 );
        guy.wear_item( item( itype_backpack ), false );
        for( int i = 0; i < 4; ++i ) {
            guy.i_add( item( itype_splinter ) );
        }
        REQUIRE( npc_autonomy::wants_warmth( guy ) );
        run( 10 );
        int planks_on_fire = 0;
        for( const item &it : here.i_at( hearth ) ) {
            planks_on_fire += it.typeId() == itype_splinter ? 1 : 0;
        }
        // Very cold: a bigger fire, three pieces, and the fourth stays in the pack.
        CHECK( planks_on_fire == 3 );
    }
    SECTION( "a freezing day: a big fire even when they're only a bit cold" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        restore_on_out_of_scope restore_air( get_weather().temperature );
        get_weather().temperature = units::from_celsius( -10 );
        guy.set_all_parts_temp_conv( units::from_celsius( 34.5 ) ); // Warmth about 28
        guy.setpos( here, hearth + tripoint( -1, 0, 0 ) );
        here.add_field( hearth, field_fd_fire, 1 );
        guy.wear_item( item( itype_backpack ), false );
        for( int i = 0; i < 4; ++i ) {
            guy.i_add( item( itype_splinter ) );
        }
        REQUIRE( npc_autonomy::wants_warmth( guy ) );
        run( 10 );
        int pieces = 0;
        for( const item &it : here.i_at( hearth ) ) {
            pieces += it.typeId() == itype_splinter ? 1 : 0;
        }
        CHECK( pieces == 3 );
    }
    SECTION( "the look-around panel says why they can't warm up" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        on_their_own();
        guy.wear_item( item( itype_jeans ), false ); // pockets, but no room for a plank
        here.add_item_or_charges( tripoint_bub_ms( player_pos + point( 6, 3 ), 0 ), item( itype_2x4 ) );
        // No lighter.
        CHECK( npc_autonomy::warm_blocker( guy ).find( "light one" ) != std::string::npos );
        // A lighter, but no bag for the plank.
        item lighter( itype_lighter );
        lighter.ammo_set( itype_butane, 100 );
        guy.i_add( lighter );
        REQUIRE( guy.autonomy_has_firestarter() );
        // Hands free: they'll carry the plank in them, so there's a plan.
        CHECK( npc_autonomy::warm_blocker( guy ).empty() );
        // A plank in hand they fight with, no bag to put it in: no way to carry another.
        item plank( itype_2x4 );
        guy.wield( plank );
        CHECK( npc_autonomy::warm_blocker( guy ).find( "carry" ) != std::string::npos );
        // A backpack: room for the wood.
        guy.wear_item( item( itype_backpack ), false );
        CHECK( npc_autonomy::warm_blocker( guy ).empty() );
        // Warm: nothing to say.
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        CHECK( npc_autonomy::warm_blocker( guy ).empty() );
    }
    SECTION( "following the player: a fireplace far from them is out of reach" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        here.add_field( hearth, field_fd_fire, 1 );
        CHECK_FALSE( npc_autonomy::wants_warmth( guy ) );
    }
}

TEST_CASE( "npc_autonomy_dress_for_the_cold", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 2, 0 ) );
    guy.wear_item( item( itype_tshirt ), false );
    guy.wear_item( item( itype_backpack ), false );
    guy.i_add( item( itype_hoodie ) );
    const auto wears_hoodie = [&guy]() {
        return guy.is_wearing( itype_hoodie );
    };

    SECTION( "cold: they put on the hoodie from their pack, even over the backpack" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.set_all_parts_temp_conv( BODYTEMP_COLD ); // Warmth about 33
        npc_autonomy::dress_for_the_cold( guy );
        CHECK( wears_hoodie() );
    }
    SECTION( "warm: they leave it in the pack" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        npc_autonomy::dress_for_the_cold( guy );
        CHECK_FALSE( wears_hoodie() );
    }
    SECTION( "option off: the game's own rule (only once Very cold)" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        guy.set_all_parts_temp_conv( BODYTEMP_COLD );
        npc_autonomy::dress_for_the_cold( guy );
        CHECK_FALSE( wears_hoodie() );
    }
}

TEST_CASE( "npc_autonomy_stale_plan_is_dropped", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option food_on( "NO_NPC_FOOD", "false" );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    map &here = get_map();
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 2, 0 ) );
    guy.set_attitude( NPCATT_NULL );
    guy.set_fac( faction_no_faction );
    here.add_item_or_charges( tripoint_bub_ms( get_avatar().pos_bub().xy() + point( 12, 0 ), 0 ),
                              item( itype_sandwich_cheese_grilled ) );
    guy.stomach.empty();
    guy.add_effect( effect_hunger_very_hungry, 1_days );
    guy.set_moves( guy.get_speed() );
    npc_autonomy::run_goal( guy, "autonomy_get_food" );
    REQUIRE( guy.get_food_plan().active() );
    // They've moved on to something else (no food goal committed): no plan line, and the
    // once-a-minute tidy forgets it.
    CHECK( npc_autonomy::plan_text( guy ).empty() );
    npc_autonomy::drop_stale_plans( guy );
    CHECK_FALSE( guy.get_food_plan().active() );
}

TEST_CASE( "npc_autonomy_refill_bottle", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 2, 0 ) );
    const auto water_carried = [&guy]() {
        int charges = 0;
        for( const item *it : guy.items_with( []( const item & i ) {
        return i.typeId() == itype_water_clean;
        } ) ) {
            charges += it->charges;
        }
        return charges;
    };

    SECTION( "no empty container: nothing to fill" ) {
        CHECK_FALSE( npc_autonomy::refill_bottle( guy, itype_water_clean ) );
    }
    SECTION( "an empty bottle gets filled" ) {
        guy.i_add( item( itype_bottle_plastic ) );
        REQUIRE( water_carried() == 0 );
        CHECK( npc_autonomy::refill_bottle( guy, itype_water_clean ) );
        CHECK( water_carried() > 0 );
    }
}

TEST_CASE( "npc_autonomy_safety_need", "[npc][autonomy]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option autonomy( "NPC_AUTONOMY", "true" );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 10, 0 ) );

    SECTION( "nothing around and unhurt: 0" ) {
        guy.mem_combat.assess_enemy = 0.0f;
        guy.mem_combat.assess_ally = 5.0f;
        CHECK( npc_autonomy::current_needs( guy ).safety == 0 );
    }
    SECTION( "evenly matched: about 67" ) {
        guy.mem_combat.assess_enemy = 6.0f;
        guy.mem_combat.assess_ally = 6.0f;
        CHECK( npc_autonomy::current_needs( guy ).safety == 67 );
    }
    SECTION( "outmatched: 100" ) {
        guy.mem_combat.assess_enemy = 10.0f;
        guy.mem_combat.assess_ally = 5.0f;
        CHECK( npc_autonomy::current_needs( guy ).safety == 100 );
    }
}

TEST_CASE( "npc_autonomy_safety_hostile_close_by", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    npc &guy = spawn_follower( get_avatar().pos_bub().xy() + point( 10, 0 ) );
    // The game rates a lone zombie as no threat at all.
    guy.mem_combat.assess_enemy = 0.0f;
    guy.mem_combat.assess_ally = 5.0f;
    REQUIRE( npc_autonomy::current_needs( guy ).safety == 0 );
    spawn_test_monster( "mon_zombie", guy.pos_bub() + point( 2, 0 ) );
    get_map().build_map_cache( 0 );
    CHECK( npc_autonomy::current_needs( guy ).safety >= 50 );
}
