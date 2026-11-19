// Xbro10 fork: Backrooms Expanded's Breakroom jobs. Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_jobs]"

#include <string>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_id.h"
#include "debug.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "game.h"
#include "global_vars.h"
#include "input_context.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "mapgen_helpers.h"
#include "math_parser_diag_value.h"
#include "mission.h"
#include "npc.h"
#include "omdata.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "monster.h"
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

// Dot's jobs finish when you report to her, not out in the lanes. The Fuel run stays open with the gas in
// your pack (it used to finish at once and take the gas), and finishes when you hand it in (Dot's
// "Here's your five litres of gas." takes the gas and finishes the job).
TEST_CASE( "fuel_run_waits_until_handed_in", "[.][backrooms_jobs]" )
{
    if( !mission_MISSION_BR1_FUEL_RUN.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    avatar &u = get_avatar();

    item can( itype_id( "jerrycan" ) );
    item gas( itype_id( "gasoline" ) );
    gas.charges = 6000;
    REQUIRE( can.put_in( gas, pocket_type::CONTAINER ).success() );
    u.i_add( can );

    // As Dot's "I'll find you some." does it (the assign_mission effect: no giver).
    mission *m = mission::reserve_new( mission_MISSION_BR1_FUEL_RUN, character_id() );
    m->assign( u );
    m->process();
    CHECK( u.get_active_missions().size() == 1 );
    CHECK( u.charges_of( itype_id( "gasoline" ) ) == 6000 );

    // A job with no giver never finishes by itself: it waits for the hand-in line at Dot's.
    m->process();
    CHECK( u.get_active_missions().size() == 1 );

    npc &dot = spawn_npc( u.pos_bub().xy() + point( 2, 0 ), "thug" );
    dialogue d( get_talker_for( u ), get_talker_for( dot ) );
    d.add_topic( "TALK_BR1_HANDIN_DOT" );
    d.gen_responses( d.topic_stack.back() );
    talk_response *hand_in = nullptr;
    for( talk_response &r : d.responses ) {
        r.create_option_line( d, input_event() );
        if( r.text == "Here's your five litres of gas." ) {
            hand_in = &r;
        }
    }
    REQUIRE( hand_in != nullptr );
    hand_in->success.apply( d );
    CHECK( u.get_active_missions().empty() );
    CHECK( u.get_completed_missions().size() == 1 );
    CHECK( u.charges_of( itype_id( "gasoline" ) ) == 1000 );
    clear_npcs();
}

// Kills for "Thin the Hounds" count only while you have the job.
TEST_CASE( "hound_kills_count_for_the_job", "[.][backrooms_jobs]" )
{
    const mission_type_id pest( "MISSION_BR1_PEST_CONTROL" );
    if( !pest.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    avatar &u = get_avatar();
    map &here = get_map();
    const auto kills = [&u]() {
        const diag_value *v = u.maybe_get_value( "br1_pest_kills" );
        return v ? v->dbl() : 0.0;
    };
    const auto kill_a_hound = [&]() {
        monster &hound = spawn_test_monster( "mon_br_hound", u.pos_bub() + point( 3, 0 ) );
        hound.die( &here, &u );
    };

    kill_a_hound();
    CHECK( kills() == 0 );

    mission *m = mission::reserve_new( pest, character_id() );
    m->assign( u );
    kill_a_hound();
    kill_a_hound();
    CHECK( kills() == 2 );
    m->process();
    CHECK( u.get_active_missions().size() == 1 );
}

// On a revealed map (nothing unseen) the Hound den job still finds a spot: a garage square whose map isn't made yet,
// turned into a den. It used to fail with "Unable to find and assign mission target br1_hound_den".
TEST_CASE( "hound_den_job_on_a_revealed_map", "[.][backrooms_jobs]" )
{
    const mission_type_id den( "MISSION_BR1_HOUND_DEN" );
    if( !den.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    avatar &u = get_avatar();
    const tripoint_abs_omt home = u.pos_abs_omt();
    for( const tripoint_abs_omt &p : points_in_radius( home, 30 ) ) {
        overmap_buffer.set_seen( p, om_vision_level::full );
        if( overmap_buffer.ter( p ).id().str().rfind( "br1_hound_den", 0 ) == 0 ) {
            overmap_buffer.ter_set( p, oter_id( "field" ) );
        }
    }
    for( int x = 10; x < 14; x++ ) {
        overmap_buffer.ter_set( home + point( x, 0 ), oter_id( "br1_garage" ) );
    }

    mission *m = mission::reserve_new( den, character_id() );
    m->assign( u );
    REQUIRE( u.get_active_missions().size() == 1 );
    const tripoint_abs_omt target = m->get_target();
    REQUIRE_FALSE( target.is_invalid() );
    CHECK( overmap_buffer.ter( target ).id().str().rfind( "br1_hound_den", 0 ) == 0 );

    // Leave the overmap as other tests expect it.
    for( const tripoint_abs_omt &p : points_in_radius( home, 30 ) ) {
        const std::string id = overmap_buffer.ter( p ).id().str();
        if( id.rfind( "br1_hound_den", 0 ) == 0 || id.rfind( "br1_garage", 0 ) == 0 ) {
            overmap_buffer.ter_set( p, oter_id( "field" ) );
        }
    }
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

// Ruth, the Breakroom's medic: her round treats her own people (trait BR1_BREAKROOM_MEMBER) for free, not strangers; you pay in
// tally tokens, and while one of hers is still badly hurt she won't do a full treatment for you.
TEST_CASE( "breakroom_medic", "[.][backrooms_jobs]" )
{
    const effect_on_condition_id round( "EOC_BR1_MEDIC_ROUND" );
    if( !round.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return;
    }
    clear_map_without_vision();
    clear_avatar();
    clear_npcs();
    avatar &u = get_avatar();
    const bodypart_id torso( "torso" );

    npc &ruth = spawn_npc( u.pos_bub().xy() + point( 2, 0 ), "BR1_medic" );
    ruth.set_unique_id( "BR1_medic" );
    // One of hers (a stand-in with the member trait, so no runner dies at the end of the test).
    npc &jonah = spawn_npc( u.pos_bub().xy() + point( 4, 0 ), "thug" );
    jonah.set_mutation( trait_id( "BR1_BREAKROOM_MEMBER" ) );
    npc &stranger = spawn_npc( u.pos_bub().xy() + point( 5, 0 ), "thug" );

    SECTION( "her round treats one of hers, not a stranger" ) {
        jonah.set_part_hp_cur( torso, 10 );
        jonah.add_effect( efftype_id( "bleed" ), 10_minutes, torso );
        stranger.set_part_hp_cur( torso, 10 );
        dialogue d( get_talker_for( ruth ), nullptr );
        round->activate( d );
        CHECK( jonah.get_part_hp_cur( torso ) == 20 );
        CHECK_FALSE( jonah.has_effect( efftype_id( "bleed" ), torso ) );
        CHECK( stranger.get_part_hp_cur( torso ) == 10 );
        // Jonah is still badly hurt, so she's busy with him.
        CHECK( get_globals().get_global_value( "br1_medic_busy" ).dbl() == 1 );
    }

    SECTION( "you pay for treatment" ) {
        u.wear_item( item( itype_id( "backpack" ) ), false );
        u.set_part_hp_cur( torso, 20 );
        u.add_effect( efftype_id( "bleed" ), 10_minutes, torso );
        for( int i = 0; i < 30; i++ ) {
            u.i_add( item( itype_id( "br_tally_token" ) ) );
        }
        const auto tokens = [&u]() {
            return u.amount_of( itype_id( "br_tally_token" ) ) + [&u]() {
                int n = 0;
                for( const item &it : get_map().i_at( u.pos_bub() ) ) {
                    n += it.typeId() == itype_id( "br_tally_token" ) ? it.count() : 0;
                }
                return n;
            }();
        };
        const int had = tokens();
        REQUIRE( had > 0 );
        get_globals().set_global_value( "br1_medic_busy", 0.0 );

        dialogue d( get_talker_for( u ), get_talker_for( ruth ) );
        d.add_topic( "TALK_BR1_MEDIC" );
        d.gen_responses( d.topic_stack.back() );
        talk_response *full = nullptr;
        for( talk_response &r : d.responses ) {
            r.create_option_line( d, input_event() );
            if( r.text.rfind( "Fix me up properly.  [", 0 ) == 0 ) {
                full = &r;
            }
        }
        REQUIRE( full != nullptr );
        full->success.apply( d );
        CHECK( u.get_part_hp_cur( torso ) == u.get_part_hp_max( torso ) );
        CHECK_FALSE( u.has_effect( efftype_id( "bleed" ), torso ) );
        CHECK( tokens() == had - 20 );
    }
    clear_npcs();
}
