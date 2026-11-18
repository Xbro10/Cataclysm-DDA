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
#include "debug.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "mapgen_helpers.h"
#include "npc_class.h"
#include "math_parser_diag_value.h"
#include "monfaction.h"
#include "mtype.h"
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

// Items owned by a shopkeeper's faction lying around them (where restocking puts the stock and
// where the trade window looks for it).
static int owned_stock_near( const npc &guy )
{
    map &here = get_map();
    int n = 0;
    for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 6 ) ) {
        for( const item &it : here.i_at( p ) ) {
            if( it.get_owner() == guy.get_fac_id() ) {
                n++;
            }
        }
    }
    return n;
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
        point depot_at;
    };
    const layout layouts[] = {
        { "br1_breakroom_checkpoint", point( 4, 12 ), "t_chaingate_c", point( 15, 19 ), point( 11, 16 ) },
        { "br1_breakroom_bunker", point( 6, 16 ), "t_fence_barbed", point( 4, 20 ), point( 4, 13 ) },
    };
    for( const layout &l : layouts ) {
        CAPTURE( l.id );
        clear_map_without_vision();
        clear_avatar();
        const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
        manual_mapgen( omt, manual_nested_mapgen, nested_mapgen_id( l.id ) );
        const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );
        CHECK( here.furn( corner + l.board_at ) == board );
        CHECK( here.furn( corner + point( 16, 10 ) ) == locker );
        CHECK( here.ter( corner + l.front_at ) == ter_str_id( l.front_ter ) );
        int guards = 0;
        for( const npc &guy : g->all_npcs() ) {
            if( guy.myclass == npc_class_id( "NC_BR1_GATE_GUARD" ) ) {
                guards++;
            }
        }
        CHECK( guards >= 1 );
        // The runners carry the unique ids the 30-minute tick looks them up by.
        CHECK( g->unique_npc_exists( "BR1_runner_jonah" ) );
        CHECK( g->unique_npc_exists( "BR1_runner_rosa" ) );
        // Two archers with bows at the front, and a stocked supply depot.
        int archers = 0;
        for( const npc &guy : g->all_npcs() ) {
            if( guy.myclass == npc_class_id( "NC_BR1_ARCHER" ) ) {
                archers++;
                REQUIRE( guy.get_wielded_item() );
                CHECK( guy.get_wielded_item()->is_gun() ); // bows are guns that fire arrows
            }
        }
        CHECK( archers == 2 );
        CHECK( here.furn( corner + l.depot_at ) == furn_str_id( "f_rack" ) );
        CHECK( !here.i_at( corner + l.depot_at ).empty() );
        // Larder: food on the kitchen counters, drinks in the fridge.
        CHECK( !here.i_at( corner + point( 3, 10 ) ).empty() );
        CHECK( !here.i_at( corner + point( 4, 10 ) ).empty() );
        CHECK( !here.i_at( corner + point( 6, 10 ) ).empty() );
    }
}

// A runner coming home restocks the larder, owned by the Breakroom.
TEST_CASE( "breakroom_larder_restock", "[.][backrooms_runner]" )
{
    const update_mapgen_id restock( "br1_breakroom_restock" );
    if( !restock.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
    manual_mapgen( omt, manual_update_mapgen, restock );
    const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );
    const faction_id breakroom( "br_breakroom" );
    for( const point &p : { point( 3, 10 ), point( 4, 10 ), point( 6, 10 ) } ) {
        CAPTURE( p );
        map_stack items = here.i_at( corner + p );
        REQUIRE( !items.empty() );
        for( const item &it : items ) {
            CHECK( it.get_owner() == breakroom );
        }
    }
}

// Level 0's Manila Room always has food in the cupboard and almond water on the table.
TEST_CASE( "manila_room_has_food_and_water", "[.][backrooms_runner]" )
{
    const nested_mapgen_id manila( "br0_manila_room" );
    if( !manila.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    const furn_str_id cupboard( "f_cupboard" );
    const furn_str_id table( "f_table" );
    const itype_id almond_water( "almond_water" );
    map &here = get_map();
    for( int i = 0; i < 4; ++i ) {
        clear_map_without_vision();
        clear_avatar();
        const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
        manual_mapgen( omt, manual_nested_mapgen, manila );
        const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );
        bool food = false;
        bool water = false;
        for( int x = 0; x < 6; ++x ) {
            for( int y = 0; y < 6; ++y ) {
                const tripoint_bub_ms p = corner + point( x, y );
                if( here.furn( p ) == cupboard ) {
                    for( const item &it : here.i_at( p ) ) {
                        food = food || it.is_food() || it.is_food_container();
                    }
                }
                if( here.furn( p ) == table ) {
                    for( const item &it : here.i_at( p ) ) {
                        water = water || it.typeId() == almond_water ||
                                ( !it.empty() && it.only_item().typeId() == almond_water );
                    }
                }
            }
        }
        CHECK( food );
        CHECK( water );
    }
}

// Level 2 tunnels generate without mapgen errors (a light used to land on a junk pile's items).
// Any debug message fails the test.
TEST_CASE( "level2_tunnels_generate_cleanly", "[.][backrooms_runner]" )
{
    const oter_str_id tunnels( "br2_tunnels" );
    if( !tunnels.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    const tripoint_abs_omt pos( 50, 50, 0 );
    std::string errors;
    for( int i = 0; i < 400; ++i ) {
        overmap_buffer.ter_set( pos, tunnels.id() );
        MAPBUFFER.clear_outside_reality_bubble();
        errors += capture_debugmsg_during( [&pos]() {
            smallmap tm;
            tm.generate( pos, calendar::turn, false, true );
            tm.delete_unmerged_submaps();
        } );
    }
    CHECK( errors.empty() );
}

// Level 0 squares (with the Manila Room, archway rooms, blackout zones and pillar halls mixed in)
// generate without mapgen errors. Any debug message fails the test.
TEST_CASE( "level0_generates_cleanly", "[.][backrooms_runner]" )
{
    const oter_str_id lobby( "backfloorchamber" );
    const ter_str_id deep( "t_br0_carpet_deep" );
    if( !lobby.is_valid() || !deep.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    const tripoint_abs_omt pos( 50, 50, 0 );
    std::string errors;
    for( int i = 0; i < 400; ++i ) {
        overmap_buffer.ter_set( pos, lobby.id() );
        MAPBUFFER.clear_outside_reality_bubble();
        errors += capture_debugmsg_during( [&pos]() {
            smallmap tm;
            tm.generate( pos, calendar::turn, false, true );
            tm.delete_unmerged_submaps();
        } );
    }
    CHECK( errors.empty() );
}

// Level 1's Exchange builds with three traders and a guard, a locked storeroom, and traders who stock up.
TEST_CASE( "exchange_market_generates", "[.][backrooms_runner]" )
{
    const nested_mapgen_id exchange( "br1_exchange_core" );
    if( !exchange.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
    manual_mapgen( omt, manual_nested_mapgen, exchange );
    const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );
    CHECK( here.ter( corner + point( 19, 4 ) ) == ter_str_id( "t_door_metal_locked" ) );
    CHECK( here.ter( corner + point( 10, 10 ) ) == ter_str_id( "t_chaingate_c" ) );
    int traders = 0;
    int guards = 0;
    for( npc &guy : g->all_npcs() ) {
        if( guy.get_fac_id() != faction_id( "br1_exchange" ) ) {
            continue;
        }
        if( guy.mission == NPC_MISSION_SHOPKEEP ) {
            traders++;
            guy.shop_restock();
            CAPTURE( guy.myclass.str() );
            CHECK( owned_stock_near( guy ) > 5 );
        } else {
            guards++;
        }
    }
    CHECK( traders == 3 );
    CHECK( guards == 1 );
}

// Level 1's All-Nite diner builds with a cook who sells meals, a server, a stocked fridge and cupboards.
TEST_CASE( "diner_generates", "[.][backrooms_runner]" )
{
    const nested_mapgen_id diner( "br1_diner_core" );
    if( !diner.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
    manual_mapgen( omt, manual_nested_mapgen, diner );
    const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );
    CHECK( here.furn( corner + point( 9, 2 ) ) == furn_str_id( "f_fridge" ) );
    CHECK( !here.i_at( corner + point( 9, 2 ) ).empty() );
    CHECK( !here.i_at( corner + point( 3, 2 ) ).empty() );
    CHECK( here.ter( corner + point( 14, 2 ) ) == ter_str_id( "t_br1_tap" ) );
    int cooks = 0;
    int servers = 0;
    for( npc &guy : g->all_npcs() ) {
        if( guy.get_fac_id() != faction_id( "br1_diner" ) ) {
            continue;
        }
        if( guy.mission == NPC_MISSION_SHOPKEEP ) {
            cooks++;
            guy.shop_restock();
            CHECK( owned_stock_near( guy ) > 5 );
        } else {
            servers++;
        }
    }
    CHECK( cooks == 1 );
    CHECK( servers == 1 );
}

// A runner home restocks the supply depot of whichever layout the Breakroom has, owned by the Breakroom.
TEST_CASE( "breakroom_depot_restock_finds_the_layout", "[.][backrooms_runner]" )
{
    const effect_on_condition_id restock( "EOC_BR1_DEPOT_RESTOCK" );
    if( !restock.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    // The depot's row of racks and crates in each layout (x from, x to, y).
    struct layout {
        const char *id;
        int x0;
        int x1;
        int y;
    };
    const layout layouts[] = {
        { "br1_breakroom_checkpoint", 10, 17, 16 },
        { "br1_breakroom_bunker", 4, 10, 13 },
    };
    map &here = get_map();
    const faction_id breakroom( "br_breakroom" );
    for( const layout &l : layouts ) {
        CAPTURE( l.id );
        clear_map_without_vision();
        clear_avatar();
        const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
        overmap_buffer.ter_set( omt, oter_id( "br1_breakroom" ) );
        manual_mapgen( omt, manual_nested_mapgen, nested_mapgen_id( l.id ) );
        const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );
        // Empty the depot so only the restock's items count (the layout's own aren't owned here).
        for( int x = l.x0; x <= l.x1; ++x ) {
            here.i_clear( corner + point( x, l.y ) );
        }
        dialogue d( get_talker_for( get_avatar() ), nullptr );
        restock->activate( d );
        int owned = 0;
        for( int x = l.x0; x <= l.x1; ++x ) {
            for( const item &it : here.i_at( corner + point( x, l.y ) ) ) {
                owned += it.get_owner() == breakroom ? 1 : 0;
            }
        }
        CHECK( owned > 0 );
    }
}

// The Backrooms' entities are one faction: friends with each other, enemies of zombies.
TEST_CASE( "backrooms_entities_faction", "[.][backrooms_runner]" )
{
    const mfaction_str_id entity( "br_entity" );
    if( !entity.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    CHECK( mtype_id( "mon_br_hound" )->default_faction == entity );
    CHECK( mtype_id( "mon_br_smiler" )->default_faction == entity );
    CHECK( mtype_id( "mon_br_deathmoth" )->default_faction == entity );
    CHECK( entity->attitude( entity.id() ) == MFA_FRIENDLY );
    CHECK( entity->attitude( mfaction_str_id( "zombie" ).id() ) == MFA_HATE );
    CHECK( entity->attitude( mfaction_str_id( "br_faceling" ).id() ) == MFA_NEUTRAL );
}
