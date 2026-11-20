// Xbro10 fork: Backrooms Expanded's Breakroom runners (Jonah and Rosa) must really leave on a run,
// at the hour the runner board in the Breakroom shows.
// Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_runner]"
// It puts a Breakroom under the player with a squatter camp, a lobby and a generator room 4 squares
// away, gives a runner the guard post the Breakroom mapgen gives them, and runs their errand script
// (EOC_BR1_RUNNER_TICK) before and after their departure hour, then lets them take turns while the
// player watches.

#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "game.h"
#include "clzones.h"
#include "game_constants.h"
#include "global_vars.h"
#include "item.h"
#include "item_location.h"
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
#include "npctrade.h"
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

// How many different kinds of item are on a shopkeeper's own booth (2 tiles; the Exchange stalls sit 4 apart).
// Concentrated stock = few kinds, deep piles. The faction's currency (tally tokens) is cash to pay with, not stock.
static int stock_kinds_near( const npc &guy )
{
    map &here = get_map();
    const faction *fac = guy.get_faction();
    std::set<itype_id> kinds;
    for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 2 ) ) {
        for( const item &it : here.i_at( p ) ) {
            if( it.get_owner() == guy.get_fac_id() && !( fac && it.typeId() == fac->currency ) ) {
                kinds.insert( it.typeId() );
            }
        }
    }
    return static_cast<int>( kinds.size() );
}

// Every item in the trader's own loot zones (what the trade window lists) belongs to their trade, so
// neighbouring stalls don't mix; returns how many items that is.
static int trade_window_stock( npc &guy )
{
    map &here = get_map();
    int n = 0;
    for( const tripoint_bub_ms &p : zone_manager::get_manager().get_point_set_loot( guy.pos_abs(),
            PICKUP_RANGE, guy.get_fac_id() ) ) {
        for( const item &it : here.i_at( p ) ) {
            CAPTURE( it.typeId().str() );
            // TRADER_AVOID items (notes) are sold but never bought back, by any trader.
            if( !it.has_flag( flag_id( "TRADER_AVOID" ) ) ) {
                CHECK( guy.wants_to_buy( it ) );
            }
            n++;
        }
    }
    return n;
}

// A trader sells only their stall stock (nothing they carry or wear) and buys only their own trade.
static void check_trader_rules( npc &guy, const char *buys, const char *refuses )
{
    CHECK( npc_trading::init_selling( guy ).empty() );
    CHECK( guy.wants_to_buy( item( itype_id( buys ) ) ) );
    CHECK_FALSE( guy.wants_to_buy( item( itype_id( refuses ) ) ) );
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

// Camps and lobbies are rare on Level 1. A runner whose nearest camp is 20 squares away still goes,
// and one with no camp, lobby or generator room in reach goes scavenging in the lanes (destination 4)
// instead of staying home ("Done for today", the bug seen in play).
TEST_CASE( "breakroom_runner_goes_far_or_scavenges", "[.][backrooms_runner]" )
{
    if( !effect_on_condition_EOC_BR1_RUNNER_TICK.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    const bool far_camp = GENERATE( true, false );
    CAPTURE( far_camp );
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 10_hours );
    const tripoint_abs_omt home_omt = get_avatar().pos_abs_omt();
    // Wipe any camps, lobbies and generator rooms the test overmap generated near home.
    for( const tripoint_abs_omt &p : points_in_radius( home_omt, 32 ) ) {
        const std::string id = overmap_buffer.ter( p ).id().str();
        if( id.rfind( "br1_squatter_camp", 0 ) == 0 || id.rfind( "br1_elevator_lobby", 0 ) == 0 ||
            id.rfind( "br1_generator_site", 0 ) == 0 || id.rfind( "br1_garage", 0 ) == 0 ) {
            overmap_buffer.ter_set( p, oter_id( "field" ) );
        }
    }
    overmap_buffer.ter_set( home_omt, oter_id( "br1_breakroom" ) );
    if( far_camp ) {
        overmap_buffer.ter_set( home_omt + point( 20, 0 ), oter_id( "br1_squatter_camp" ) );
    } else {
        overmap_buffer.ter_set( home_omt + point( 0, 6 ), oter_id( "br1_garage" ) );
    }

    npc &guy = spawn_npc( get_avatar().pos_bub().xy() + point( 3, 0 ), "test_talker" );
    guy.set_attitude( NPCATT_NULL );
    guy.set_mission( NPC_MISSION_GUARD );
    guy.guard_pos = guy.pos_abs();

    global_variables &globals = get_globals();
    run_eoc( get_avatar(), effect_on_condition_EOC_BR1_RUNNER_SCHEDULE );
    globals.set_global_value( "br1_tick_who", 1.0 );
    globals.set_global_value( "br1_tick_depart", 10.0 );
    globals.set_global_value( "br1_depart_1", 10.0 );
    globals.set_global_value( "br1_runner_out", 0.0 );

    run_eoc( guy, effect_on_condition_EOC_BR1_RUNNER_TICK );
    INFO( "run_dest " << runner_var( guy, "br1_run_dest" ) << ", goal " << guy.goal.to_string() );
    REQUIRE( runner_var( guy, "br1_run_state" ) == 1 );
    if( far_camp ) {
        CHECK( runner_var( guy, "br1_run_dest" ) == 1 );
        CHECK( rl_dist( guy.goal, home_omt ) == 20 );
    } else {
        CHECK( runner_var( guy, "br1_run_dest" ) == 4 );
        CHECK( rl_dist( guy.goal, home_omt ) == 6 );
        run_eoc( get_avatar(), effect_on_condition_EOC_BR1_RUNNER_BOARD_READ );
        CHECK( global_str( "br1_board_1" ) == "JONAH: OUT, scavenging the lanes.  Back in a day or two." );
    }
}

// A runner badly hurt on a run cuts it short and heads home at once; an unhurt one keeps going.
TEST_CASE( "breakroom_runner_comes_home_when_hurt", "[.][backrooms_runner]" )
{
    if( !effect_on_condition_EOC_BR1_RUNNER_TICK.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    const bool hurt = GENERATE( true, false );
    CAPTURE( hurt );
    clear_map_without_vision();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    npc &guy = spawn_npc( get_avatar().pos_bub().xy() + point( 3, 0 ), "test_talker" );
    guy.set_attitude( NPCATT_NULL );
    // Out on a run: home and destination both far away, not due back for a day.
    guy.set_value( "br1_run_state", 1.0 );
    guy.set_value( "br1_home_set", 1.0 );
    guy.set_value( "br1_home", guy.pos_abs() + tripoint( 200, 0, 0 ) );
    guy.set_value( "br1_dest", guy.pos_abs() + tripoint( 0, 200, 0 ) );
    guy.set_value( "br1_run_turn", to_turns<double>( calendar::turn + 1_days - calendar::turn_zero ) );
    get_globals().set_global_value( "br1_tick_who", 1.0 );
    if( hurt ) {
        guy.set_part_hp_cur( bodypart_id( "torso" ), guy.get_part_hp_max( bodypart_id( "torso" ) ) / 3 );
    }
    run_eoc( guy, effect_on_condition_EOC_BR1_RUNNER_TICK );
    CHECK( runner_var( guy, "br1_run_state" ) == ( hurt ? 2 : 1 ) );
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
        // The two runners are the Breakroom's best fighters: machete in hand, a Glock and meds, the skills to use them.
        for( npc &guy : g->all_npcs() ) {
            if( guy.myclass != npc_class_id( "NC_BR1_RUNNER" ) ) {
                continue;
            }
            REQUIRE( guy.get_wielded_item() );
            CHECK( guy.get_wielded_item()->typeId() == itype_id( "machete" ) );
            CHECK( guy.meets_requirements( *guy.get_wielded_item() ) );
            CHECK( guy.has_amount( itype_id( "glock_19" ), 1 ) );
            CHECK( guy.meets_requirements( item( itype_id( "glock_19" ) ) ) );
            CHECK( guy.has_amount( itype_id( "bandages" ), 4 ) );
            CHECK( static_cast<int>( guy.get_skill_level( skill_id( "melee" ) ) ) >= 5 );
            CHECK( static_cast<int>( guy.get_skill_level( skill_id( "dodge" ) ) ) >= 5 );
        }
        // Two archers with bows at the front, and a stocked supply depot.
        int archers = 0;
        for( npc &guy : g->all_npcs() ) {
            if( guy.myclass == npc_class_id( "NC_BR1_ARCHER" ) ) {
                archers++;
                REQUIRE( guy.get_wielded_item() );
                CHECK( guy.get_wielded_item()->is_gun() ); // bows are guns that fire arrows
                // Strong and skilled enough for the bow (Strength 9) and the knife (stabbing 2, melee 1).
                CHECK( guy.meets_requirements( *guy.get_wielded_item() ) );
                CHECK( guy.meets_requirements( item( itype_id( "knife_combat" ) ) ) );
                // Half sandbags in front of every archer (the Bunker's used to stand in the open).
                CHECK( here.furn( guy.pos_bub() + point( 0, 1 ) ) == furn_str_id( "f_sandbag_half" ) );
                // A knife for close fights; drawing it puts the bow in the bow sling, not on the floor.
                CHECK( guy.is_wearing( itype_id( "bow_sling" ) ) );
                REQUIRE( guy.has_amount( itype_id( "knife_combat" ), 1 ) );
                const itype_id bow = guy.get_wielded_item()->typeId();
                const std::vector<item *> knives = guy.items_with( []( const item & it ) {
                    return it.typeId() == itype_id( "knife_combat" );
                } );
                REQUIRE( !knives.empty() );
                item_location knife( guy, knives.front() );
                const int on_floor = here.i_at( guy.pos_bub() ).size();
                CHECK( guy.wield( knife ) );
                CHECK( guy.get_wielded_item()->typeId() == itype_id( "knife_combat" ) );
                CHECK( guy.has_amount( bow, 1 ) );
                CHECK( static_cast<int>( here.i_at( guy.pos_bub() ).size() ) == on_floor );
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

// Level 1 garage squares (puddles, junk, the booth and the other rooms mixed in) generate without
// mapgen errors. Reported 2026-11-19: the booth was set over a puddle's murky water.
TEST_CASE( "level1_garage_generates_cleanly", "[.][backrooms_runner]" )
{
    const oter_str_id garage( "br1_garage" );
    if( !garage.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    const tripoint_abs_omt pos( 50, 50, 0 );
    std::string errors;
    for( int i = 0; i < 400; ++i ) {
        overmap_buffer.ter_set( pos, garage.id() );
        MAPBUFFER.clear_outside_reality_bubble();
        errors += capture_debugmsg_during( [&pos]() {
            smallmap tm;
            tm.generate( pos, calendar::turn, false, true );
            tm.delete_unmerged_submaps();
        } );
    }
    INFO( errors.substr( 0, 1500 ) );
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
        if( guy.get_fac_id().str().rfind( "br1_exchange", 0 ) != 0 ) {
            continue;
        }
        if( guy.mission == NPC_MISSION_SHOPKEEP ) {
            traders++;
            CAPTURE( guy.myclass.str() );
            // The mapgen gave this stall its own loot zone, so restock won't add a fallback zone over its neighbours.
            CHECK( zone_manager::get_manager().has_near( zone_type_id( "LOOT_UNSORTED" ), guy.pos_abs(), PICKUP_RANGE,
                    guy.get_fac_id() ) );
            guy.shop_restock();
            CHECK( trade_window_stock( guy ) >= 8 );
            CHECK( owned_stock_near( guy ) > 5 );
            CHECK( owned_stock_near( guy ) >= 8 );
            CHECK( stock_kinds_near( guy ) <= 12 );
            if( guy.myclass == npc_class_id( "NC_BR1_TRADER_SUPPLIES" ) ) {
                check_trader_rules( guy, "can_beans", "hammer" );
            } else if( guy.myclass == npc_class_id( "NC_BR1_TRADER_GEAR" ) ) {
                check_trader_rules( guy, "hammer", "can_beans" );
            } else {
                check_trader_rules( guy, "survivormap", "hammer" );
            }
        } else {
            guards++;
            // Real protection: armour, a helmet, a weapon they can use and a loaded sidearm.
            CHECK( guy.myclass == npc_class_id( "NC_BR1_EXCHANGE_GUARD" ) );
            REQUIRE( guy.get_wielded_item() );
            CHECK( guy.meets_requirements( *guy.get_wielded_item() ) );
            CHECK( guy.is_wearing( itype_id( "kevlar" ) ) );
            CHECK( guy.is_wearing( itype_id( "helmet_army" ) ) );
            const std::vector<item *> guns = guy.items_with( []( const item &it ) {
                return it.typeId() == itype_id( "glock_19" );
            } );
            REQUIRE( !guns.empty() );
            CHECK( guns.front()->ammo_remaining() > 0 );
        }
    }
    CHECK( traders == 3 );
    CHECK( guards == 3 );
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
            CHECK( trade_window_stock( guy ) >= 8 );
            CHECK( owned_stock_near( guy ) > 5 );
            check_trader_rules( guy, "can_beans", "hammer" );
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
