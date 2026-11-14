// Xbro10 fork: tests for "NPC autonomy (experimental)" (src/npc_autonomy.*).

#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "item.h"
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

static const efftype_id effect_hunger_hungry( "hunger_hungry" );
static const efftype_id effect_hunger_very_hungry( "hunger_very_hungry" );

static const itype_id itype_sandwich_cheese_grilled( "sandwich_cheese_grilled" );

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
