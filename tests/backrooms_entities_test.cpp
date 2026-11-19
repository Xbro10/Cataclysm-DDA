// Xbro10 fork: Backrooms entities' weaknesses. Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_entities]"

#include "calendar.h"
#include "cata_catch.h"
#include "avatar.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "mapdata.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const mtype_id mon_br_smiler( "mon_br_smiler" );

// A Smiler caught in bright light (here a floor light of brightness 120, like a flashlight beam) burns every turn and dies in a
// few turns; in the dark it doesn't burn at all.
TEST_CASE( "smiler_burns_in_bright_light", "[.][backrooms_entities]" )
{
    if( !mon_br_smiler.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 0_hours ); // midnight: no daylight
    map &here = get_map();
    const tripoint_bub_ms pos = get_avatar().pos_bub() + point( 6, 0 );
    monster &smiler = spawn_test_monster( mon_br_smiler.str(), pos );
    const int full = smiler.get_hp();

    SECTION( "in the dark it doesn't burn" ) {
        here.build_map_cache( pos.z() );
        REQUIRE( here.ambient_light_at( pos ) < 50.0f );
        for( int i = 0; i < 5; i++ ) {
            smiler.process_turn();
        }
        CHECK( smiler.get_hp() == full );
    }
    SECTION( "under a bright light it burns to death in a few turns" ) {
        // A floor light of brightness 120 under it, like standing in a flashlight beam.
        here.ter_set( pos, ter_str_id( "t_thconc_y_olight" ) );
        here.build_map_cache( pos.z() );
        CAPTURE( here.ambient_light_at( pos ) );
        REQUIRE( here.ambient_light_at( pos ) >= 50.0f );
        smiler.process_turn();
        CHECK( smiler.get_hp() < full );
        for( int i = 0; i < 5 && !smiler.is_dead(); i++ ) {
            smiler.process_turn();
        }
        CHECK( smiler.get_hp() <= 0 );
    }
}
