// Xbro10 fork (reported 2026-11-20): an NPC handed a bow shot arrows but didn't pick them back
// up, and a gunner beside a zombie clubbed it instead of stepping back to shoot.

#include <sstream>
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
#include "npc_attack.h"
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
        CHECK( rl_dist( guy.pos_bub(), you ) <= 10 );
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

static const itype_id itype_quiver( "quiver" );
static const itype_id itype_shortbow( "shortbow" );
static const skill_id skill_archery( "archery" );

// Played out with the follower's default rules (reported 2026-11-20, again): an archer
// follower, a zombie walking up. They shoot, step away when it reaches them instead of
// clubbing it, and pick their arrows back up once it's dead.
TEST_CASE( "an_archer_follower_kites_and_collects_arrows", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    map &here = get_map();
    const tripoint_bub_ms you = get_avatar().pos_bub();
    npc &guy = spawn_ranged_follower( you.xy() + point( 1, 0 ) );
    // A short bow needs Strength 8.
    guy.mod_str_bonus( 4 );
    guy.set_skill_level( skill_archery, 5 );
    item quiver( itype_quiver );
    quiver.put_in( item( itype_arrow_wood, calendar::turn, 12 ), pocket_type::CONTAINER );
    guy.wear_item( quiver, false );
    item bow( itype_shortbow );
    guy.wield( bow );
    CAPTURE( guy.get_wielded_item()->is_silent() );
    CAPTURE( guy.rules.has_flag( ally_rule::use_silent ) );
    CAPTURE( guy.get_wielded_item()->shots_remaining( here, &guy ) );
    CAPTURE( guy.can_reload_current() );
    CAPTURE( quiet_enough_for_follower_rules( *guy.get_wielded_item() ) );
    CAPTURE( npc_autonomy::gunner_stance_for( guy ) == npc_autonomy::gunner_stance::none );
    REQUIRE( guy.charges_of( itype_arrow_wood ) == 12 );

    monster &zed = spawn_test_monster( "mon_zombie", you + tripoint( 8, 0, 0 ) );
    zed.set_hp( 400 );
    here.build_map_cache( 0 );
    std::ostringstream trace;
    int adjacent_turns = 0;
    int stepped_away = 0;
    for( int t = 0; t < 60; ++t ) {
        calendar::turn += 1_turns;
        const bool was_adjacent = rl_dist( guy.pos_bub(), zed.pos_bub() ) <= 1;
        const tripoint_bub_ms guy_before = guy.pos_bub();
        guy.mod_moves( guy.get_speed() );
        guy.process_turn();
        guy.npc_update_body();
        // As the game does: act while there are moves left.
        for( int act = 0; act < 5 && guy.get_moves() > 0; ++act ) {
            guy.move();
        }
        trace << t << ":d" << rl_dist( guy_before, zed.pos_bub() ) << ( guy.pos_bub() != guy_before ? "mv" : "" ) <<
              "m" << guy.get_moves() << "a" << guy.charges_of( itype_arrow_wood ) << "n" <<
              guy.get_wielded_item()->ammo_remaining() << "r" << guy.can_reload_current() <<
              ( dynamic_cast<const npc_attack_gun *>( guy.get_current_attack().get() ) ? "G" :
                dynamic_cast<const npc_attack_melee *>( guy.get_current_attack().get() ) ? "M" : "-" ) << " ";
        if( was_adjacent ) {
            ++adjacent_turns;
            if( rl_dist( guy.pos_bub(), zed.pos_bub() ) > 1 ) {
                ++stepped_away;
            }
        }
        // The zombie heads straight for them.
        zed.anger = 100;
        zed.set_dest( here.get_abs( guy.pos_bub() ) );
        // Moves build up as in the game: a zombie (speed 70) steps most turns, not every turn.
        zed.mod_moves( zed.get_speed() );
        for( int step = 0; step < 3 && zed.get_moves() > 0; ++step ) {
            const int before = zed.get_moves();
            zed.move();
            if( zed.get_moves() == before ) {
                break;
            }
        }
        here.build_map_cache( 0 );
    }
    const int arrows_left = guy.charges_of( itype_arrow_wood );
    INFO( trace.str() );
    INFO( "adjacent turns " << adjacent_turns << ", stepped away " << stepped_away << ", arrows left " <<
          arrows_left );
    // They shoot as it comes and between steps: several arrows, not the one they got off before.
    CHECK( arrows_left <= 9 );
    CHECK( adjacent_turns > 0 );
    CHECK( stepped_away * 2 >= adjacent_turns );

    // The fight ends: they pick up the arrows they can find.
    g->remove_zombie( zed );
    here.build_map_cache( 0 );
    for( int t = 0; t < 150; ++t ) {
        calendar::turn += 1_turns;
        guy.set_moves( guy.get_speed() );
        guy.process_turn();
        guy.npc_update_body();
        guy.move();
    }
    int arrows_on_ground = 0;
    // Only where a follower may go: 15 tiles from the player.
    for( const tripoint_bub_ms &p : here.points_in_radius( you, 15 ) ) {
        for( const item &it : here.i_at( p ) ) {
            if( it.typeId() == itype_arrow_wood ) {
                arrows_on_ground += it.count();
            }
        }
    }
    int tagged = 0;
    for( const tripoint_bub_ms &p : here.points_in_radius( you, 15 ) ) {
        for( const item &it : here.i_at( p ) ) {
            if( it.typeId() == itype_arrow_wood && it.has_var( "autonomy_carried_by" ) ) {
                tagged += it.count();
            }
        }
    }
    INFO( "tagged " << tagged << ", wants " << npc_autonomy::wants_to_recover_items( guy ) << ", goal " <<
          guy.get_committed_goal() << ", in fight " << guy.get_value( "autonomy_in_fight" ).to_string() <<
          ", until " << guy.get_value( "autonomy_recover_until" ).to_string() );
    INFO( "arrows back " << guy.charges_of( itype_arrow_wood ) << ", on the ground " << arrows_on_ground );
    CHECK( arrows_on_ground == 0 );
}

