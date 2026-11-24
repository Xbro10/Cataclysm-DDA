// Xbro10 fork: NPC autonomy step 5.6 A, place memory (src/npc_memory.{h,cpp}).

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_scope_helpers.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "npc_memory.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const faction_id faction_no_faction( "no_faction" );

static const ter_str_id ter_t_water_sh( "t_water_sh" );

using npc_memory::place_kind;

static npc &spawn_stranger( const point_bub_ms &where )
{
    npc &guy = spawn_npc( where, "test_talker" );
    clear_character( guy );
    guy.setpos( get_map(), tripoint_bub_ms( where, 0 ) );
    guy.set_attitude( NPCATT_NULL );
    guy.set_fac( faction_no_faction );
    return guy;
}

static tripoint_abs_ms spot( const npc &guy, int dx, int dy )
{
    return guy.pos_abs() + tripoint( dx, dy, 0 );
}

TEST_CASE( "npc_place_memory_basics", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 2_days );
    npc &guy = spawn_stranger( get_avatar().pos_bub().xy() + point( 3, 0 ) );

    SECTION( "an old save: nothing remembered" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        CHECK( npc_memory::count( guy ) == 0 );
        CHECK_FALSE( npc_memory::nearest( guy, place_kind::water ) );
    }
    SECTION( "option off: nothing is noted" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        npc_memory::remember( guy, place_kind::water, spot( guy, 5, 0 ) );
        CHECK( npc_memory::count( guy ) == 0 );
    }
    SECTION( "the same place seen again is one memory, refreshed" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        npc_memory::remember( guy, place_kind::water, spot( guy, 5, 0 ) );
        calendar::turn += 1_hours;
        npc_memory::remember( guy, place_kind::water, spot( guy, 6, 0 ) );
        REQUIRE( npc_memory::count( guy ) == 1 );
        CHECK( *npc_memory::nearest( guy, place_kind::water ) == spot( guy, 6, 0 ) );
        // Another kind at the same spot is another memory.
        npc_memory::remember( guy, place_kind::safe, spot( guy, 6, 0 ) );
        CHECK( npc_memory::count( guy ) == 2 );
    }
    SECTION( "forgetting a place" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        npc_memory::remember( guy, place_kind::water, spot( guy, 5, 0 ) );
        npc_memory::forget( guy, place_kind::water, spot( guy, 5, 1 ) );
        CHECK( npc_memory::count( guy ) == 0 );
    }
    SECTION( "the nearest of a kind" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        npc_memory::remember( guy, place_kind::water, spot( guy, 30, 0 ) );
        npc_memory::remember( guy, place_kind::water, spot( guy, -10, 0 ) );
        npc_memory::remember( guy, place_kind::danger, spot( guy, 2, 0 ) );
        CHECK( *npc_memory::nearest( guy, place_kind::water ) == spot( guy, -10, 0 ) );
    }
}

TEST_CASE( "npc_place_memory_cap_and_fading", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 2_days );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    npc &guy = spawn_stranger( get_avatar().pos_bub().xy() + point( 3, 0 ) );

    SECTION( "never more than 30: the least important and oldest go first" ) {
        npc_memory::remember( guy, place_kind::danger, spot( guy, 0, 40 ) );
        calendar::turn += 1_hours;
        for( int i = 0; i < 35; ++i ) {
            npc_memory::remember( guy, place_kind::looted, spot( guy, i * 10, 0 ) );
            calendar::turn += 1_minutes;
        }
        CHECK( npc_memory::count( guy ) == npc_memory::max_places );
        // The danger spot outranks the looted rooms; the five oldest looted rooms went.
        CHECK( npc_memory::nearest( guy, place_kind::danger ) );
        bool oldest_kept = false;
        for( const npc_memory::place &p : npc_memory::places( guy ) ) {
            oldest_kept = oldest_kept || p.pos == spot( guy, 0, 0 );
        }
        CHECK_FALSE( oldest_kept );
    }
    SECTION( "memories fade: a looted mark after a week, water lasts" ) {
        npc_memory::remember( guy, place_kind::looted, spot( guy, 10, 0 ) );
        npc_memory::remember( guy, place_kind::water, spot( guy, 20, 0 ) );
        calendar::turn += 8_days;
        CHECK_FALSE( npc_memory::nearest( guy, place_kind::looted ) );
        CHECK( npc_memory::nearest( guy, place_kind::water ) );
    }
}

TEST_CASE( "npc_place_memory_survives_save_and_load", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 2_days );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    npc &guy = spawn_stranger( get_avatar().pos_bub().xy() + point( 3, 0 ) );
    npc_memory::remember( guy, place_kind::water, spot( guy, 12, 3 ) );
    npc_memory::remember( guy, place_kind::danger, spot( guy, -20, 0 ) );
    const std::vector<npc_memory::place> before = npc_memory::places( guy );
    REQUIRE( before.size() == 2 );

    std::ostringstream saved;
    JsonOut jsout( saved );
    guy.serialize( jsout );
    clear_npcs();

    npc loaded;
    JsonValue jv = json_loader::from_string( saved.str() );
    loaded.deserialize( jv.get_object() );
    const std::vector<npc_memory::place> after = npc_memory::places( loaded );
    REQUIRE( after.size() == before.size() );
    for( size_t i = 0; i < before.size(); ++i ) {
        CHECK( after[i].kind == before[i].kind );
        CHECK( after[i].pos == before[i].pos );
        CHECK( after[i].seen == before[i].seen );
        CHECK( after[i].dimension == before[i].dimension );
    }
}

TEST_CASE( "npc_place_memory_text_is_forgiving", "[npc][autonomy]" )
{
    CHECK( npc_memory::decode( "" ).empty() );
    // A damaged entry is dropped; the good one stays.
    const std::vector<npc_memory::place> list = npc_memory::decode( "0,1,2,0,50,;oops;9,1,1,1,1;2,5,6,0,70,backrooms_level_1" );
    REQUIRE( list.size() == 2 );
    CHECK( list[0].kind == place_kind::water );
    CHECK( list[0].dimension.empty() );
    CHECK( list[1].kind == place_kind::danger );
    CHECK( list[1].dimension == "backrooms_level_1" );
    CHECK( npc_memory::decode( npc_memory::encode( list ) ).size() == 2 );
}

// The playbook's test: a thirsty NPC with no water in reach walks to water it remembers.
TEST_CASE( "npc_walks_to_water_it_remembers", "[npc][autonomy]" )
{
    restore_on_out_of_scope<std::optional<units::temperature>> forced( get_weather().forced_temperature );
    restore_on_out_of_scope<units::temperature> air( get_weather().temperature );
    get_weather().forced_temperature.reset();
    get_weather().temperature = units::from_celsius( 20 );
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option food_on( "NO_NPC_FOOD", "false" );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    map &here = get_map();
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    npc &guy = spawn_stranger( player_pos + point( 2, 0 ) );
    // Past the 20-tile water search, inside the 60 tiles they'd walk for water they remember.
    const tripoint_bub_ms pool( player_pos + point( 35, 0 ), 0 );
    here.ter_set( pool, ter_t_water_sh );
    guy.stomach.empty();
    guy.set_thirst( 100 ); // Very thirsty: Water about 53
    REQUIRE( npc_autonomy::current_needs( guy ).water >= 50 );

    SECTION( "nothing remembered: no trip" ) {
        CHECK_FALSE( npc_autonomy::wants_water( guy ) );
    }
    SECTION( "remembered water: they walk there and drink" ) {
        npc_memory::remember( guy, place_kind::water, here.get_abs( pool ) );
        // Nearer, but a level up: it doesn't hide the pool.
        npc_memory::remember( guy, place_kind::water, guy.pos_abs() + tripoint( 3, 0, 1 ) );
        REQUIRE( npc_memory::count( guy ) == 2 );
        REQUIRE( npc_autonomy::wants_water( guy ) );
        for( int turn = 0; turn < 120 && guy.get_thirst() >= 100; ++turn ) {
            guy.set_moves( guy.get_speed() );
            npc_autonomy::run_goal( guy, "autonomy_get_water" );
        }
        CHECK( rl_dist( guy.pos_bub(), pool ) <= 1 );
        CHECK( guy.get_thirst() < 100 );
    }
    SECTION( "remembered water that's gone: forgotten once they get there" ) {
        const tripoint_bub_ms dry( player_pos + point( -30, 0 ), 0 );
        here.ter_set( pool, here.ter( dry ) );
        npc_memory::remember( guy, place_kind::water, here.get_abs( pool ) );
        for( int turn = 0; turn < 120 && npc_memory::count( guy ) > 0; ++turn ) {
            guy.set_moves( guy.get_speed() );
            npc_autonomy::run_goal( guy, "autonomy_get_water" );
        }
        CHECK( npc_memory::count( guy ) == 0 );
    }
}
