// Xbro10 fork (reported 2026-11-20): an NPC handed a bow shot arrows but didn't pick them back
// up, and a gunner beside a zombie clubbed it instead of stepping back to shoot.

#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monster.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "type_id.h"

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_9mm( "9mm" );
static const itype_id itype_arrow_wood( "arrow_wood" );
static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_glockmag( "glockmag" );

static npc &spawn_ranged_follower( const point_bub_ms &where )
{
    npc &guy = spawn_npc( where, "test_talker" );
    clear_character( guy );
    guy.setpos( get_map(), tripoint_bub_ms( where, 0 ) );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_fac( faction_your_followers );
    return guy;
}

static void arm_with_loaded_glock( npc &guy )
{
    item pistol( itype_glock_19 );
    item mag( itype_glockmag );
    mag.ammo_set( itype_9mm, 15 );
    pistol.put_in( mag, pocket_type::MAGAZINE_WELL );
    guy.wield( pistol );
    // Followers only fire silent guns by default; the Glock is loud.
    guy.rules.clear_flag( ally_rule::use_silent );
    REQUIRE( guy.get_wielded_item()->shots_remaining( get_map(), &guy ) > 0 );
}

TEST_CASE( "an_npc_tags_what_it_shoots_in_a_fight", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_ranged_follower( get_avatar().pos_bub().xy() + point( 2, 0 ) );
    item arrow( itype_arrow_wood, calendar::turn, 1 );
    const std::string mine = std::to_string( guy.getID().get_value() );

    SECTION( "autonomy on, in a fight: tagged as theirs" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.set_value( "autonomy_in_fight", 1.0 );
        npc_autonomy::tag_shot_ammo( guy, arrow );
        CHECK( arrow.get_var( "autonomy_carried_by" ) == mine );
    }
    SECTION( "not in a fight (tossing something over): not tagged" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        npc_autonomy::tag_shot_ammo( guy, arrow );
        CHECK_FALSE( arrow.has_var( "autonomy_carried_by" ) );
    }
    SECTION( "the player's shots are never tagged" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        npc_autonomy::tag_shot_ammo( get_avatar(), arrow );
        CHECK_FALSE( arrow.has_var( "autonomy_carried_by" ) );
    }
    SECTION( "autonomy off: not tagged" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        guy.set_value( "autonomy_in_fight", 1.0 );
        npc_autonomy::tag_shot_ammo( guy, arrow );
        CHECK_FALSE( arrow.has_var( "autonomy_carried_by" ) );
    }
}

TEST_CASE( "a_follower_fetches_arrows_beyond_follow_distance", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    map &here = get_map();
    const tripoint_bub_ms you = get_avatar().pos_bub();
    npc &guy = spawn_ranged_follower( you.xy() + point( 1, 0 ) );
    // An arrow of theirs 10 tiles from the player: past their follow distance (6), within 15.
    item arrow( itype_arrow_wood, calendar::turn, 1 );
    arrow.set_var( "autonomy_carried_by", std::to_string( guy.getID().get_value() ) );
    const tripoint_bub_ms where = you + tripoint( 10, 0, 0 );
    here.add_item_or_charges( where, arrow );
    // A fight just ended.
    guy.set_value( "autonomy_recover_until",
                   static_cast<double>( to_turns<int>( calendar::turn - calendar::turn_zero ) + 600 ) );
    here.build_map_cache( 0 );
    REQUIRE( npc_autonomy::wants_to_recover_items( guy ) );
    for( int t = 0; t < 20 && !here.i_at( where ).empty(); ++t ) {
        calendar::turn += 1_turns;
        guy.set_moves( guy.get_speed() );
        npc_autonomy::run_goal( guy, "autonomy_recover_items" );
    }
    CHECK( here.i_at( where ).empty() );
    CHECK( guy.has_amount( itype_arrow_wood, 1 ) );
}

TEST_CASE( "a_gunner_steps_away_from_a_zombie_beside_them", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    map &here = get_map();
    const tripoint_bub_ms you = get_avatar().pos_bub();
    npc &guy = spawn_ranged_follower( you.xy() + point( 2, 0 ) );
    arm_with_loaded_glock( guy );
    monster &zed = spawn_test_monster( "mon_zombie", guy.pos_bub() + point( 1, 0 ) );
    here.build_map_cache( 0 );
    REQUIRE( zed.get_speed() < guy.get_speed() );

    SECTION( "a slower zombie beside them: a step further away, still near the player" ) {
        REQUIRE( npc_autonomy::kite_step( guy ) );
        CHECK( rl_dist( guy.pos_bub(), zed.pos_bub() ) == 2 );
        CHECK( rl_dist( guy.pos_bub(), you ) <= 6 );
    }
    SECTION( "as fast as them: no point running, they fight" ) {
        zed.set_speed_base( guy.get_speed() + 20 );
        zed.set_moves( 0 );
        CHECK_FALSE( npc_autonomy::kite_step( guy ) );
    }
    SECTION( "nothing beside them: they stay and shoot" ) {
        g->remove_zombie( zed );
        spawn_test_monster( "mon_zombie", guy.pos_bub() + point( 4, 0 ) );
        here.build_map_cache( 0 );
        CHECK_FALSE( npc_autonomy::kite_step( guy ) );
    }
    SECTION( "autonomy off: the game's own fighting" ) {
        override_option off( "NPC_AUTONOMY", "false" );
        CHECK_FALSE( npc_autonomy::kite_step( guy ) );
    }
}
