// Xbro10 fork: Backrooms Expanded keeps Level 0 and Level 1 (weather generator default_backrooms)
// comfortably warm: a steady 21 C all year with only calm weather. Level 2 keeps its own swinging heat.
// Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_climate]"

#include <algorithm>
#include <set>
#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "type_id.h"
#include "units.h"
#include "weather_gen.h"

TEST_CASE( "backrooms_levels_0_and_1_stay_comfortable", "[.][backrooms_climate]" )
{
    const weather_generator_id office( "default_backrooms" );
    const weather_generator_id level2( "backrooms_level_2" );
    if( !level2.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    const weather_generator &wg = office.obj();
    REQUIRE( wg.steady_temperature() );
    CHECK_FALSE( level2.obj().steady_temperature() );

    const std::set<std::string> calm = { "clear", "sunny", "cloudy" };
    double l2_low = 1000;
    double l2_high = -1000;
    // Every 3 hours through a year, at a few places: always 21 C, never anything but calm weather.
    for( time_point t = calendar::turn_zero; t < calendar::turn_zero + calendar::year_length();
         t += 3_hours ) {
        for( const tripoint_abs_ms &p : {
                 tripoint_abs_ms::zero, tripoint_abs_ms( 1000, -500, 0 ), tripoint_abs_ms( -7000, 3000, 0 )
             } ) {
            const w_point w = wg.get_weather( p, t, 1234 );
            CAPTURE( to_string( t ) );
            CHECK( units::to_celsius( w.temperature ) == Approx( 21.0 ) );
            CHECK( calm.count( wg.get_weather_conditions( w ).str() ) == 1 );
            const double l2 = units::to_celsius( level2.obj().get_weather( p, t, 1234 ).temperature );
            l2_low = std::min( l2_low, l2 );
            l2_high = std::max( l2_high, l2 );
        }
    }
    // Level 2 keeps its own weather, which swings (its coldest winter nights reach about 9 C).
    CHECK( l2_high - l2_low > 10.0 );
}
