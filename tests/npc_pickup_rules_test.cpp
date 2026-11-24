// Xbro10 fork (user requests 2026-11-24): a follower's favourites show apart from the player's,
// and follower pickup: category rules, Exclude rules that win, and no junk by default.

#include <string>

#include "auto_pickup.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_bandages( "bandages" );
static const itype_id itype_can_beans( "can_beans" );
static const itype_id itype_can_drink( "can_drink" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_pipe( "pipe" );
static const itype_id itype_scrap( "scrap" );

static npc &spawn_rules_follower()
{
    const point_bub_ms where = get_avatar().pos_bub().xy() + point( 2, 0 );
    npc &guy = spawn_npc( where, "test_talker" );
    clear_character( guy );
    guy.setpos( get_map(), tripoint_bub_ms( where, 0 ) );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_fac( faction_your_followers );
    guy.rules.set_flag( ally_rule::allow_pick_up );
    return guy;
}

// Rules as the pickup rules editor saves them: {"rule", "active", "exclude"}.
static void set_pickup_rules( npc &guy, const std::string &json )
{
    const JsonValue jv = json_loader::from_string( json );
    guy.rules.pickup_whitelist->deserialize( jv.get_array() );
}

TEST_CASE( "a_followers_favourite_shows_apart_from_yours", "[npc]" )
{
    clear_avatar();
    override_option asterisk( "ASTERISK_POSITION", "prefix" );
    avatar &you = get_avatar();

    SECTION( "handed to a follower: their favourite, a light red asterisk" ) {
        item hammer( itype_hammer );
        mark_npc_favorite( hammer );
        CHECK( hammer.is_favorite );
        CHECK( hammer.has_var( "npc_favorite" ) );
        CHECK( hammer.tname().find( "<color_" ) != std::string::npos );
    }
    SECTION( "your own favourite: a plain asterisk" ) {
        item hammer( itype_hammer );
        hammer.set_favorite( true );
        CHECK( hammer.tname().rfind( "* ", 0 ) == 0 );
        CHECK( hammer.tname().find( "<color_" ) == std::string::npos );
    }
    SECTION( "back with you: not a favourite any more" ) {
        item hammer( itype_hammer );
        mark_npc_favorite( hammer );
        item_location back = you.i_add( hammer );
        REQUIRE( back );
        CHECK_FALSE( back->is_favorite );
        CHECK_FALSE( back->has_var( "npc_favorite" ) );
    }
    SECTION( "your favourite before you gave it stays yours" ) {
        item hammer( itype_hammer );
        hammer.set_favorite( true );
        mark_npc_favorite( hammer );
        CHECK_FALSE( hammer.has_var( "npc_favorite" ) );
        item_location back = you.i_add( hammer );
        REQUIRE( back );
        CHECK( back->is_favorite );
    }
}

TEST_CASE( "follower_pickup_rules_by_category", "[npc]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_rules_follower();

    SECTION( "c:food takes food and nothing else" ) {
        set_pickup_rules( guy, R"([{"rule":"c:food","active":true,"exclude":false}])" );
        CHECK( guy.item_whitelisted( item( itype_can_beans ) ) );
        CHECK_FALSE( guy.item_whitelisted( item( itype_hammer ) ) );
    }
    SECTION( "an Exclude rule wins, even over a better weapon than bare fists" ) {
        set_pickup_rules( guy, R"([{"rule":"*","active":true,"exclude":false},)"
                          R"({"rule":"c:spare_parts","active":true,"exclude":true}])" );
        const item pipe( itype_pipe );
        REQUIRE( guy.evaluate_weapon( pipe ) > guy.evaluate_weapon( null_item_reference() ) );
        CHECK( guy.item_blacklisted( pipe ) );
        CHECK_FALSE( guy.wants_take_that( pipe ) );
        CHECK( guy.wants_take_that( item( itype_hammer ) ) );
    }
    SECTION( "material rules count for followers too" ) {
        set_pickup_rules( guy, R"([{"rule":"m:wood","active":true,"exclude":false}])" );
        CHECK( guy.item_whitelisted( item( itype_2x4 ) ) );
        CHECK_FALSE( guy.item_whitelisted( item( itype_bandages ) ) );
    }
}

TEST_CASE( "a_follower_without_rules_leaves_junk", "[npc][autonomy]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_rules_follower();
    REQUIRE_FALSE( guy.has_item_whitelist() );

    SECTION( "NPC autonomy on: scraps, planks and empty cans stay, supplies are taken" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        CHECK_FALSE( npc_autonomy::worth_carrying( guy, item( itype_scrap ) ) );
        CHECK_FALSE( npc_autonomy::worth_carrying( guy, item( itype_2x4 ) ) );
        CHECK_FALSE( npc_autonomy::worth_carrying( guy, item( itype_can_drink ) ) );
        CHECK( npc_autonomy::worth_carrying( guy, item( itype_bandages ) ) );
        CHECK( npc_autonomy::worth_carrying( guy, item( itype_flashlight ) ) );
        CHECK( npc_autonomy::worth_carrying( guy, item( itype_can_beans ) ) );
    }
    SECTION( "NPC autonomy off: the game's own test" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        CHECK( npc_autonomy::worth_carrying( guy, item( itype_2x4 ) ) );
    }
}
