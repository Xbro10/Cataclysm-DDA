// Xbro10 fork (reported 2026-11-19): a follower kept smashing a zombie corpse while another zombie
// chewed on her. Based on the Levels session's reproduction attempt.

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monster.h"
#include "npc.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const activity_id ACT_PULP( "ACT_PULP" );

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_machete( "machete" );

static const mtype_id mon_zombie( "mon_zombie" );

TEST_CASE( "npc_stops_pulping_to_fight_a_zombie_on_top_of_them", "[npc]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    const bool autonomy_on = GENERATE( false, true );
    CAPTURE( autonomy_on );
    override_option autonomy( "NPC_AUTONOMY", autonomy_on ? "true" : "false" );
    map &here = get_map();
    const point_bub_ms where = get_avatar().pos_bub().xy() + point( 2, 0 );
    npc &guy = spawn_npc( where, "test_talker" );
    clear_character( guy );
    guy.setpos( here, tripoint_bub_ms( where, 0 ) );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_fac( faction_your_followers );
    item machete( itype_machete );
    guy.wield( machete );
    // Smashing a zombie corpse beside them.
    const tripoint_bub_ms corpse_at = guy.pos_bub() + tripoint( 1, 0, 0 );
    here.add_item_or_charges( corpse_at, item::make_corpse( mon_zombie, calendar::turn ) );
    here.build_map_cache( 0 );
    const auto run = [&guy]( int turns ) {
        for( int t = 0; t < turns; ++t ) {
            calendar::turn += 1_turns;
            guy.set_moves( guy.get_speed() );
            guy.move();
        }
    };
    guy.pulp_location = here.get_abs( corpse_at );
    guy.assign_activity( pulp_activity_actor( here.get_abs( corpse_at ) ) );
    run( 2 );
    REQUIRE( guy.activity.id() == ACT_PULP );
    // Another zombie walks up and starts on them: they fight it and drop the corpse job.
    monster &zed = spawn_test_monster( "mon_zombie", guy.pos_bub() + point( 0, 1 ) );
    zed.set_hp( 10000 );
    const int zed_hp = zed.get_hp();
    here.build_map_cache( 0 );
    run( 10 );
    CHECK( zed.get_hp() < zed_hp );
    CHECK( guy.activity.id() != ACT_PULP );
}
