// Xbro10 fork: tests for "NPC autonomy (experimental)" (src/npc_autonomy.*).

#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );

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
    SECTION( "lost body reserves count as hunger" ) {
        guy.set_stored_kcal( guy.get_healthy_kcal() * 3 / 4 );
        CHECK( npc_autonomy::current_needs( guy ).food == 50 );
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
