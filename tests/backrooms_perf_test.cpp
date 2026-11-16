// Xbro10 fork: timing for Backrooms Level 0, whose ceiling lights make it slow.
// Hidden benchmark; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_perf]"
// It fills the whole loaded area with Level 0 carpet and a grid of ceiling lights (open
// floor, so a worst case: real Level 0 rooms have walls) and times the game's per-turn map
// cache rebuild, with and without the lighting part, for several light layouts.

#include <algorithm>
#include <chrono>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"

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
