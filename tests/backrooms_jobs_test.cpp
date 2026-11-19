// Xbro10 fork: Backrooms Expanded's Breakroom jobs. Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_jobs]"

#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_id.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "mapgen_helpers.h"
#include "math_parser_diag_value.h"
#include "mission.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"

static const mission_type_id mission_MISSION_BR1_FUEL_RUN( "MISSION_BR1_FUEL_RUN" );
static const effect_on_condition_id effect_on_condition_EOC_BR1_STANDING_UP( "EOC_BR1_STANDING_UP" );
static const effect_on_condition_id effect_on_condition_EOC_BR1_STANDING_REWARDS(
    "EOC_BR1_STANDING_REWARDS" );

static void run_player_eoc( const effect_on_condition_id &eoc )
{
    dialogue d( get_talker_for( get_avatar() ), nullptr );
    eoc->activate( d );
}

// Keys held or at your feet (a test character has no pockets, so spawned items drop).
static int keys_near_player( const itype_id &key )
{
    int n = get_avatar().amount_of( key );
    for( const item &it : get_map().i_at( get_avatar().pos_bub() ) ) {
        if( it.typeId() == key ) {
            n++;
        }
    }
    return n;
}

// Handing in the Fuel run gives Dot the five litres: the gas leaves your can.
TEST_CASE( "fuel_run_takes_the_gasoline", "[.][backrooms_jobs]" )
{
    if( !mission_MISSION_BR1_FUEL_RUN.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    avatar &u = get_avatar();
    const character_id dot = get_map().place_npc( u.pos_bub().xy() + point( 2, 0 ),
                             string_id<npc_template>( "test_talker" ) );
    g->load_npcs();

    item can( itype_id( "jerrycan" ) );
    item gas( itype_id( "gasoline" ) );
    gas.charges = 6000;
    REQUIRE( can.put_in( gas, pocket_type::CONTAINER ).success() );
    u.i_add( can );
    REQUIRE( u.charges_of( itype_id( "gasoline" ) ) == 6000 );

    mission *m = mission::reserve_new( mission_MISSION_BR1_FUEL_RUN, dot );
    m->assign( u );
    REQUIRE( m->is_complete( dot ) );
    m->wrap_up();
    CHECK( u.charges_of( itype_id( "gasoline" ) ) == 1000 );
}

// All four Hound den layouts build without errors.
TEST_CASE( "hound_den_layouts_generate", "[.][backrooms_jobs]" )
{
    const nested_mapgen_id core( "br1_hound_den_core" );
    if( !core.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    for( const char *id : {
             "br1_hound_den_core", "br1_hound_den_old", "br1_hound_den_last_stand", "br1_hound_den_feeding"
         } ) {
        CAPTURE( id );
        clear_map_without_vision();
        clear_avatar();
        const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
        const std::string msg = capture_debugmsg_during( [&]() {
            manual_mapgen( omt, manual_nested_mapgen, nested_mapgen_id( id ) );
        } );
        CHECK( msg.empty() );
    }
}

// The 3-job reward (the stores key) waits until you talk to Dot, even when the third job finishes out
// in the lanes (Thin the Hounds ends on the third kill).
TEST_CASE( "standing_reward_waits_for_dot", "[.][backrooms_jobs]" )
{
    if( !effect_on_condition_EOC_BR1_STANDING_UP.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_avatar();
    avatar &u = get_avatar();
    const itype_id key( "br1_breakroom_key" );
    u.set_value( "br1_standing", 2.0 );
    run_player_eoc( effect_on_condition_EOC_BR1_STANDING_UP );
    CHECK( keys_near_player( key ) == 0 );
    const diag_value *due = u.maybe_get_value( "br1_key_due" );
    REQUIRE( due );
    CHECK( due->dbl() == 1 );

    run_player_eoc( effect_on_condition_EOC_BR1_STANDING_REWARDS );
    CHECK( keys_near_player( key ) == 1 );
    // Handed over once.
    run_player_eoc( effect_on_condition_EOC_BR1_STANDING_REWARDS );
    CHECK( keys_near_player( key ) == 1 );
}
