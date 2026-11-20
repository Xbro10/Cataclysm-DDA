// Xbro10 fork (user request 2026-11-19): with NPC autonomy on, followers and guards stand with the
// player and only run once the follower rule "allow_fall_back" is set.

#include <algorithm>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "monster.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const efftype_id effect_npc_run_away( "npc_run_away" );

static const faction_id faction_your_followers( "your_followers" );

static npc &spawn_fall_back_follower( const point_bub_ms &where )
{
    npc &guy = spawn_npc( where, "test_talker" );
    clear_character( guy );
    // clear_character() parks everyone on top of the avatar.
    guy.setpos( get_map(), tripoint_bub_ms( where, 0 ) );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_fac( faction_your_followers );
    guy.personality.bravery = 0;
    return guy;
}

// Badly outmatched (as the game rates it): the long retreat, unless they stand with the player.
static void outmatch( npc &guy )
{
    guy.mem_combat.assess_enemy = 20.0f;
    guy.mem_combat.assess_ally = 1.0f;
    guy.mem_combat.panic = 5;
    guy.act_on_danger_assessment();
}

TEST_CASE( "followers_stand_with_the_player_unless_allowed_to_fall_back", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_fall_back_follower( get_avatar().pos_bub().xy() + point( 2, 0 ) );
    REQUIRE( guy.is_player_ally() );
    REQUIRE_FALSE( guy.rules.has_flag( ally_rule::allow_fall_back ) );

    SECTION( "autonomy on, default rules: no retreat" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        CHECK( npc_autonomy::stands_with_player( guy ) );
        outmatch( guy );
        // At most the game's two-turn step back from a crowd, never the 20-turn retreat.
        CHECK( ( !guy.has_effect( effect_npc_run_away ) ||
                 guy.get_effect_dur( effect_npc_run_away ) <= 2_turns ) );
    }
    SECTION( "autonomy on, allowed to fall back: the game's retreat" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        guy.rules.set_flag( ally_rule::allow_fall_back );
        CHECK_FALSE( npc_autonomy::stands_with_player( guy ) );
        CHECK( npc_autonomy::falls_back_to_player( guy ) );
        outmatch( guy );
        REQUIRE( guy.has_effect( effect_npc_run_away ) );
        CHECK( guy.get_effect_dur( effect_npc_run_away ) >= 20_turns );
    }
    SECTION( "autonomy off: upstream's retreat" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        CHECK_FALSE( npc_autonomy::stands_with_player( guy ) );
        outmatch( guy );
        REQUIRE( guy.has_effect( effect_npc_run_away ) );
        CHECK( guy.get_effect_dur( effect_npc_run_away ) >= 20_turns );
    }
}

TEST_CASE( "a_follower_running_off_stops_once_beyond_follow_distance", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    override_option autonomy( "NPC_AUTONOMY", "true" );
    npc &guy = spawn_fall_back_follower( get_avatar().pos_bub().xy() + point( 12, 0 ) );
    REQUIRE_FALSE( npc_autonomy::may_step_back( guy ) );
    // Still running from before (an older save, or the rule just switched off).
    guy.add_effect( effect_npc_run_away, 1_hours );
    guy.mem_combat.assess_enemy = 20.0f;
    guy.mem_combat.assess_ally = 1.0f;
    guy.act_on_danger_assessment();
    CHECK_FALSE( guy.has_effect( effect_npc_run_away ) );

    // Allowed to fall back, they keep running (and regroup on the player).
    guy.rules.set_flag( ally_rule::allow_fall_back );
    guy.add_effect( effect_npc_run_away, 1_hours );
    guy.act_on_danger_assessment();
    CHECK( guy.has_effect( effect_npc_run_away ) );
}

// Played out: an unarmed, hurt follower beside the player, three zombies close. They fight or hold
// near the player instead of running off, and never go looking for a safe spot on their own.
TEST_CASE( "an_outmatched_follower_stays_near_the_player", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 12_hours );
    override_option autonomy( "NPC_AUTONOMY", "true" );
    map &here = get_map();
    avatar &you = get_avatar();
    npc &guy = spawn_fall_back_follower( you.pos_bub().xy() + point( 1, 0 ) );
    guy.remove_weapon();
    guy.mod_str_bonus( -4 );
    guy.set_all_parts_hp_cur( 20 );
    std::vector<monster *> zeds;
    for( int x = -1; x <= 1; ++x ) {
        monster &zed = spawn_test_monster( "mon_zombie", guy.pos_bub() + point( x, 3 ) );
        // Frozen in place, so the follower decides on their own what to do about them.
        zed.set_moves( -100000 );
        zeds.push_back( &zed );
    }
    here.build_map_cache( 0 );
    int furthest = 0;
    bool sought_safety = false;
    bool long_retreat = false;
    for( int t = 0; t < 20; ++t ) {
        calendar::turn += 1_turns;
        guy.set_moves( guy.get_speed() );
        guy.move();
        guy.process_effects();
        furthest = std::max( furthest, rl_dist( guy.pos_bub(), you.pos_bub() ) );
        sought_safety = sought_safety || guy.get_committed_goal() == "autonomy_seek_safety";
        long_retreat = long_retreat || ( guy.has_effect( effect_npc_run_away ) &&
                                         guy.get_effect_dur( effect_npc_run_away ) > 2_turns );
    }
    CAPTURE( guy.follow_distance() );
    CHECK_FALSE( sought_safety );
    CHECK_FALSE( long_retreat );
    CHECK( furthest <= guy.follow_distance() + 2 );
}
