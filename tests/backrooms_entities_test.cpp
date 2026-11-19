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
#include "field_type.h"
#include "damage.h"
#include "weakpoint.h"
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

static const mtype_id mon_br_skin_stealer( "mon_br_skin_stealer" );
static const mtype_id mon_br_deathmoth( "mon_br_deathmoth" );

// A Skin-stealer's nerve breaks under bright light (a flashlight beam in a dark room), so it backs off;
// in the dark it keeps coming.
TEST_CASE( "skin_stealer_backs_off_from_bright_light", "[.][backrooms_entities]" )
{
    if( !mon_br_skin_stealer.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 0_hours );
    map &here = get_map();
    const tripoint_bub_ms pos = get_avatar().pos_bub() + point( 4, 0 );
    monster &stealer = spawn_test_monster( mon_br_skin_stealer.str(), pos );

    SECTION( "in the dark" ) {
        here.build_map_cache( pos.z() );
        stealer.process_turn();
        CHECK( stealer.morale >= 0 );
    }
    SECTION( "lit" ) {
        here.ter_set( pos, ter_str_id( "t_thconc_y_olight" ) );
        here.build_map_cache( pos.z() );
        stealer.process_turn();
        CHECK( stealer.morale < 0 );
    }
}

// Deathmoths aren't scared of fire and their powdery wings burn: in a strong fire they lose health every
// turn (a ground fire barely hurts most flyers), and burning weapons do triple damage to the wings.
// Flying to light (SEEKS_LIGHT) is checked in play.
TEST_CASE( "deathmoth_burns_in_fire", "[.][backrooms_entities]" )
{
    if( !mon_br_deathmoth.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    const mtype &moth_type = *mon_br_deathmoth;
    CHECK( !moth_type.has_fear_trigger( mon_trigger::FIRE ) );
    bool heat_weak_wings = false;
    for( const weakpoint &wp : moth_type.weakpoints.weakpoint_list ) {
        auto it = wp.damage_mult.find( damage_type_id( "heat" ) );
        if( it != wp.damage_mult.end() && it->second >= 3.0f ) {
            heat_weak_wings = true;
        }
    }
    CHECK( heat_weak_wings );

    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 0_hours );
    map &here = get_map();
    // Far from the test character, so the light is the only thing it cares about.
    const tripoint_bub_ms light = get_avatar().pos_bub() + point( 45, 0 );
    const tripoint_bub_ms start = light + point( -6, 4 );
    monster &moth = spawn_test_monster( mon_br_deathmoth.str(), start );
    moth.anger = 0;

    SECTION( "in a fire it burns" ) {
        const int full = moth.get_hp();
        here.add_field( moth.pos_bub(), field_type_id( "fd_fire" ), 3 );
        for( int turn = 0; turn < 3 && !moth.is_dead(); turn++ ) {
            here.creature_in_field( moth );
        }
        CHECK( ( moth.is_dead() || moth.get_hp() < full ) );
    }
}
