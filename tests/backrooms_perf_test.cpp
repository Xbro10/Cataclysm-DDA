// Xbro10 fork: timing for Backrooms Level 0, whose ceiling lights make it slow.
// Hidden benchmark; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_perf]"
// It fills the whole loaded area with Level 0 carpet and a grid of ceiling lights (open
// floor, so a worst case: real Level 0 rooms have walls) and times the game's per-turn map
// cache rebuild, with and without the lighting part, for several light layouts.

#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"

static const faction_id faction_your_followers_perf( "your_followers" );
static const itype_id itype_sandwich_cheese_grilled_perf( "sandwich_cheese_grilled" );

static const ter_str_id ter_t_carpet_backrooms( "t_carpet_backrooms" );
static const ter_str_id ter_t_carpet_backrooms_light( "t_carpet_backrooms_light" );

// A light at (2, 2) and every `spacing` tiles from there, like Level 0's grid; 0 = no lights.
static void lay_level0_floor( const int spacing )
{
    map &here = get_map();
    for( int x = 0; x < MAPSIZE_X; ++x ) {
        for( int y = 0; y < MAPSIZE_Y; ++y ) {
            const bool light = spacing > 0 && x % spacing == 2 % spacing && y % spacing == 2 % spacing;
            here.ter_set( tripoint_bub_ms( x, y, 0 ), light ? ter_t_carpet_backrooms_light :
                          ter_t_carpet_backrooms );
        }
    }
}

// Milliseconds per map cache rebuild (what the game does when the map changes or time passes).
static double ms_per_rebuild( const bool skip_lightmap )
{
    map &here = get_map();
    constexpr int reps = 10;
    // Warm up once.
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, skip_lightmap );
    const auto start = std::chrono::steady_clock::now();
    for( int i = 0; i < reps; ++i ) {
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, skip_lightmap );
    }
    const std::chrono::duration<double, std::milli> took = std::chrono::steady_clock::now() - start;
    return took.count() / reps;
}

// How long one NPC turn (npc::move) takes, NPC autonomy off vs on. Runs without mods.
TEST_CASE( "npc_turn_benchmark", "[.][benchmark][backrooms_perf][npc_perf]" )
{
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option food_on( "NO_NPC_FOOD", "false" );
    const point_bub_ms player_pos = get_avatar().pos_bub().xy();
    std::vector<npc *> crowd;
    for( int i = 0; i < 4; ++i ) {
        npc &guy = spawn_npc( player_pos + point( 2 + i, 2 ), "test_talker" );
        guy.set_attitude( NPCATT_FOLLOW );
        guy.set_fac( faction_your_followers_perf );
        guy.i_add( item( itype_sandwich_cheese_grilled_perf ) );
        crowd.push_back( &guy );
    }
    const auto ms_per_npc_turn = [&crowd]() {
        constexpr int turns = 200;
        const auto start = std::chrono::steady_clock::now();
        for( int t = 0; t < turns; ++t ) {
            for( npc *guy : crowd ) {
                guy->set_moves( guy->get_speed() );
                guy->move();
            }
        }
        const std::chrono::duration<double, std::milli> took = std::chrono::steady_clock::now() - start;
        return took.count() / ( turns * crowd.size() );
    };
    std::string report = "\nNPC turn (npc::move), 4 followers next to the player:\n";
    {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        ms_per_npc_turn();
        report += string_format( "  autonomy off  %7.3f ms per NPC turn\n", ms_per_npc_turn() );
    }
    {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        ms_per_npc_turn();
        report += string_format( "  autonomy on   %7.3f ms per NPC turn\n", ms_per_npc_turn() );
        // Each autonomy check on its own.
        const auto ms_per_call = [&crowd]( const std::function<void( const npc & )> &check ) {
            constexpr int calls = 500;
            const auto start = std::chrono::steady_clock::now();
            for( int c = 0; c < calls; ++c ) {
                for( const npc *guy : crowd ) {
                    check( *guy );
                }
            }
            const std::chrono::duration<double, std::milli> took = std::chrono::steady_clock::now() - start;
            return took.count() / ( calls * crowd.size() );
        };
        report += string_format( "    wants_meal      %7.3f ms\n", ms_per_call( []( const npc & g ) {
            npc_autonomy::wants_meal( g );
        } ) );
        report += string_format( "    wants_company   %7.3f ms\n", ms_per_call( []( const npc & g ) {
            npc_autonomy::wants_company( g );
        } ) );
        report += string_format( "    wants_water     %7.3f ms\n", ms_per_call( []( const npc & g ) {
            npc_autonomy::wants_water( g );
        } ) );
        report += string_format( "    current_needs   %7.3f ms\n", ms_per_call( []( const npc & g ) {
            npc_autonomy::current_needs( g );
        } ) );
        report += string_format( "    safety_need     %7.3f ms\n", ms_per_call( []( const npc & g ) {
            npc_autonomy::safety_need( g );
        } ) );
        report += string_format( "    has_company     %7.3f ms\n", ms_per_call( []( const npc & g ) {
            npc_autonomy::has_company( g );
        } ) );
        report += string_format( "    drinks nearby   %7.3f ms\n", ms_per_call( []( const npc & g ) {
            const_cast<npc &>( g ).find_nearby_food( npc::consume_filter::drink_only );
        } ) );
        report += string_format( "    water sources   %7.3f ms\n", ms_per_call( []( const npc & g ) {
            g.find_nearby_water_sources();
        } ) );
        report += string_format( "    food nearby     %7.3f ms\n", ms_per_call( []( const npc & g ) {
            const_cast<npc &>( g ).find_nearby_food( npc::consume_filter::food_only );
        } ) );
        for( const npc *guy : crowd ) {
            report += string_format( "    %s: thirst %d hunger %d goal \"%s\"\n", guy->get_name(),
                                     guy->get_thirst(), guy->get_hunger(), guy->get_committed_goal() );
        }
    }
    WARN( report );
    CHECK( true );
}

TEST_CASE( "backrooms_level0_lighting_benchmark", "[.][benchmark][backrooms_perf]" )
{
    if( !ter_t_carpet_backrooms_light.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    // Midnight: only the ceiling lights light the floor.
    set_time( calendar::turn_zero + 24_hours );

    std::string report = "\nLevel 0 map cache rebuild (whole loaded area, open floor):\n";
    const auto measure = [&report]( const std::string & label ) {
        const double without_light = ms_per_rebuild( true );
        const double with_light = ms_per_rebuild( false );
        // Darkest floor tile in the middle of the map (10 = "brightly lit", 5 = dim, 3.5 = dark).
        const map &here = get_map();
        float darkest = 1e9f;
        for( int x = MAPSIZE_X / 2 - 12; x < MAPSIZE_X / 2 + 12; ++x ) {
            for( int y = MAPSIZE_Y / 2 - 12; y < MAPSIZE_Y / 2 + 12; ++y ) {
                darkest = std::min( darkest, here.ambient_light_at( tripoint_bub_ms( x, y, 0 ) ) );
            }
        }
        report += string_format( "  %-36s %7.2f ms   (lighting part %7.2f ms)   darkest spot %6.1f\n",
                                 label, with_light, with_light - without_light, darkest );
    };

    ter_t &lamp = const_cast<ter_t &>( ter_t_carpet_backrooms_light.obj() );
    const int real_brightness = lamp.light_emitted;
    for( const int spacing : { 0, 8, 6, 4 } ) {
        lay_level0_floor( spacing );
        measure( spacing == 0 ? std::string( "no lights" ) :
                 string_format( "every %d tiles, brightness %d", spacing, real_brightness ) );
    }
    lay_level0_floor( 4 );
    for( const int brightness : { 80, 60, 40 } ) {
        lamp.light_emitted = brightness;
        measure( string_format( "every 4 tiles, brightness %d", brightness ) );
    }
    lay_level0_floor( 6 );
    for( const int brightness : { 80, 60 } ) {
        lamp.light_emitted = brightness;
        measure( string_format( "every 6 tiles, brightness %d", brightness ) );
    }
    lay_level0_floor( 8 );
    for( const int brightness : { 80, 60, 40 } ) {
        lamp.light_emitted = brightness;
        measure( string_format( "every 8 tiles, brightness %d", brightness ) );
    }
    lamp.light_emitted = real_brightness;
    WARN( report );
    CHECK( true );
}
