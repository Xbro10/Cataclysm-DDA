// Xbro10 fork: Backrooms Expanded's Breakroom runners (Jonah and Rosa) must really leave on a run,
// at the hour the runner board in the Breakroom shows.
// Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_runner]"
// It puts a Breakroom under the player with a squatter camp, a lobby and a generator room 4 squares
// away, gives a runner the guard post the Breakroom mapgen gives them, and runs their errand script
// (EOC_BR1_RUNNER_TICK) before and after their departure hour, then lets them take turns while the
// player watches.

#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "game.h"
#include "global_vars.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "mapdata.h"
#include "mapgen_helpers.h"
#include "npc_class.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "omdata.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"

static const effect_on_condition_id effect_on_condition_EOC_BR1_RUNNER_BOARD_READ(
    "EOC_BR1_RUNNER_BOARD_READ" );
static const effect_on_condition_id effect_on_condition_EOC_BR1_RUNNER_SCHEDULE(
    "EOC_BR1_RUNNER_SCHEDULE" );
static const effect_on_condition_id effect_on_condition_EOC_BR1_RUNNER_TICK( "EOC_BR1_RUNNER_TICK" );

static double runner_var( const npc &guy, const std::string &key )
{
    const diag_value *v = guy.maybe_get_value( key );
    return v && v->is_dbl() ? v->dbl() : 0.0;
}

static std::string global_str( const std::string &key )
{
    const diag_value *v = get_globals().maybe_get_global_value( key );
    return v ? v->to_string() : std::string();
}

static void run_eoc( Creature &who, const effect_on_condition_id &eoc )
{
    dialogue d( get_talker_for( who ), nullptr );
    eoc->activate( d );
}

TEST_CASE( "breakroom_runner_leaves_on_a_run", "[.][backrooms_runner]" )
{
    if( !effect_on_condition_EOC_BR1_RUNNER_TICK.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 9_hours );
    const tripoint_abs_omt home_omt = get_avatar().pos_abs_omt();
    overmap_buffer.ter_set( home_omt, oter_id( "br1_breakroom" ) );
    overmap_buffer.ter_set( home_omt + point( 4, 0 ), oter_id( "br1_squatter_camp" ) );
    overmap_buffer.ter_set( home_omt + point( 0, 4 ), oter_id( "br1_elevator_lobby" ) );
    overmap_buffer.ter_set( home_omt + point( -4, 0 ), oter_id( "br1_generator_site" ) );

    npc &guy = spawn_npc( get_avatar().pos_bub().xy() + point( 3, 0 ), "test_talker" );
    guy.set_attitude( NPCATT_NULL );
    guy.set_mission( NPC_MISSION_GUARD );
    guy.guard_pos = guy.pos_abs();
    const tripoint_abs_ms start = guy.pos_abs();

    // Today's schedule, then Jonah's departure fixed at 10 AM.
    global_variables &globals = get_globals();
    run_eoc( get_avatar(), effect_on_condition_EOC_BR1_RUNNER_SCHEDULE );
    globals.set_global_value( "br1_depart_1", 10.0 );
    globals.set_global_value( "br1_depart_1_h", 10.0 );
    globals.set_global_value( "br1_depart_1_ap", std::string( "AM" ) );
    globals.set_global_value( "br1_tick_who", 1.0 );
    globals.set_global_value( "br1_tick_depart", 10.0 );
    globals.set_global_value( "br1_runner_out", 0.0 );

    // 9 AM: home, not yet time to go; the board says when.
    run_eoc( guy, effect_on_condition_EOC_BR1_RUNNER_TICK );
    CHECK( runner_var( guy, "br1_home_set" ) == 1 );
    CHECK( runner_var( guy, "br1_run_state" ) == 0 );
    run_eoc( get_avatar(), effect_on_condition_EOC_BR1_RUNNER_BOARD_READ );
    CHECK( global_str( "br1_board_1" ) == "JONAH: in.  Leaves at 10 AM." );

    // 10 AM: off on a run to one of the three places.
    set_time( calendar::turn_zero + 10_hours );
    run_eoc( guy, effect_on_condition_EOC_BR1_RUNNER_TICK );
    INFO( "run_dest " << runner_var( guy, "br1_run_dest" ) << ", mission " <<
          static_cast<int>( guy.mission ) << ", goal " << guy.goal.to_string() );
    REQUIRE( runner_var( guy, "br1_run_state" ) == 1 );
    CHECK( rl_dist( guy.goal, home_omt ) == 4 );
    run_eoc( get_avatar(), effect_on_condition_EOC_BR1_RUNNER_BOARD_READ );
    CHECK( global_str( "br1_board_1" ).find( "JONAH: OUT, to " ) == 0 );

    // They walk off while the player stands at the Breakroom.
    int max_dist = 0;
    for( int turn = 0; turn < 300; ++turn ) {
        guy.set_moves( 100 );
        guy.move();
        calendar::turn += 1_turns;
        max_dist = std::max( max_dist, rl_dist( start, guy.pos_abs() ) );
    }
    INFO( "walked at most " << max_dist << " tiles" );
    CHECK( max_dist >= 20 );
}

// Both Breakroom layouts build, with the runner board, supply locker and front defences in place.
TEST_CASE( "breakroom_layouts_generate", "[.][backrooms_runner]" )
{
    const furn_str_id board( "f_br1_runner_board" );
    if( !board.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    const furn_str_id locker( "f_br1_supply_locker" );
    map &here = get_map();
    struct layout {
        const char *id;
        point board_at;
        const char *front_ter;
        point front_at;
    };
    const layout layouts[] = {
        { "br1_breakroom_checkpoint", point( 4, 12 ), "t_chaingate_c", point( 15, 19 ) },
        { "br1_breakroom_bunker", point( 6, 16 ), "t_fence_barbed", point( 4, 20 ) },
    };
    for( const layout &l : layouts ) {
        CAPTURE( l.id );
        clear_map_without_vision();
        clear_avatar();
        const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
        manual_mapgen( omt, manual_nested_mapgen, nested_mapgen_id( l.id ) );
        const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );
        CHECK( here.furn( corner + l.board_at ) == board );
        CHECK( here.furn( corner + point( 17, 10 ) ) == locker );
        CHECK( here.ter( corner + l.front_at ) == ter_str_id( l.front_ter ) );
        int guards = 0;
        for( const npc &guy : g->all_npcs() ) {
            if( guy.myclass == npc_class_id( "NC_BR1_GATE_GUARD" ) ) {
                guards++;
            }
        }
        CHECK( guards >= 1 );
    }
}
