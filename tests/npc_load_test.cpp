// Xbro10 fork: overloaded NPCs lighten their load (src/npc_load.{h,cpp}). With NPC autonomy on, an NPC
// over its weight limit puts down junk it picked up itself, cheapest per kilo first, and keeps food,
// ammo for its gun, favourites and what it wears or wields. What you trade to a follower is a favourite.

#include <list>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "faction.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "npc_load.h"
#include "npctrade.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "trade_ui.h"
#include "type_id.h"
#include "units.h"

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_apple( "apple" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_duffelbag( "duffelbag" );
static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_glockmag( "glockmag" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_steel_lump( "steel_lump" );
static const itype_id itype_scrap( "scrap" );

static npc &spawn_load_follower()
{
    npc &guy = spawn_npc( point_bub_ms( 62, 60 ), "test_talker" );
    clear_character( guy );
    guy.setpos( get_map(), tripoint_bub_ms( 62, 60, 0 ) );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.set_fac( faction_your_followers );
    // A real bag: the debug backpack makes what it holds weigh nothing. A weaker NPC is easier to overload.
    item pack( itype_duffelbag );
    REQUIRE( guy.wear_item( pack, false ) );
    guy.set_str_bonus( -4 );
    item gun( itype_glock_19 );
    REQUIRE( guy.wield( gun ) );
    return guy;
}

// Fills the bag with 1 kg steel lumps until the NPC is a few kilos over its limit.
static void overload( npc &guy )
{
    for( int i = 0; i < 200 && guy.weight_carried() <= guy.weight_capacity() + 3_kilogram; i++ ) {
        guy.i_add( item( itype_steel_lump ) );
    }
    REQUIRE( guy.weight_carried() > guy.weight_capacity() );
}

static int carried( const npc &guy, const itype_id &id )
{
    return guy.amount_of( id );
}

TEST_CASE( "overloaded_npc_puts_down_junk_and_keeps_what_it_needs", "[npc][autonomy][npc_load]" )
{
    clear_map_without_vision();
    clear_avatar();
    override_option autonomy( "NPC_AUTONOMY", "true" );
    npc &guy = spawn_load_follower();

    guy.i_add( item( itype_apple ) );
    guy.i_add( item( itype_glockmag ) );
    item favourite( itype_hammer );
    favourite.set_favorite( true );
    guy.i_add( favourite );
    guy.i_add( item( itype_scrap ) );
    overload( guy );

    SECTION( "it picks only junk, cheapest per kilo first, and just enough of it" ) {
        const std::vector<item_location> drop = npc_load::what_to_drop( guy );
        REQUIRE_FALSE( drop.empty() );
        units::mass dropped = 0_gram;
        for( const item_location &loc : drop ) {
            CAPTURE( loc->typeId().str() );
            CHECK( ( loc->typeId() == itype_steel_lump || loc->typeId() == itype_scrap ) );
            CHECK_FALSE( loc->is_favorite );
            dropped += loc->weight();
        }
        CHECK( guy.weight_carried() - dropped <= guy.weight_capacity() );
        // Without the last one it would still be over: no more than needed.
        CHECK( guy.weight_carried() - dropped + drop.back()->weight() > guy.weight_capacity() );
        CHECK( drop.front()->typeId() == itype_scrap );
    }

    SECTION( "it puts it down on its tile and keeps food, its ammo, favourites and its gun" ) {
        REQUIRE( npc_load::lighten_load( guy ) );
        process_activity( guy );
        CHECK( guy.weight_carried() <= guy.weight_capacity() );
        CHECK( carried( guy, itype_apple ) == 1 );
        CHECK( carried( guy, itype_glockmag ) == 1 );
        CHECK( carried( guy, itype_hammer ) == 1 );
        CHECK( guy.get_wielded_item()->typeId() == itype_glock_19 );
        CHECK( guy.is_wearing( itype_duffelbag ) );
        CHECK_FALSE( get_map().i_at( guy.pos_bub() ).empty() );
        // It looks again only after a minute.
        guy.i_add( item( itype_steel_lump ) );
        overload( guy );
        CHECK_FALSE( npc_load::lighten_load( guy ) );
        calendar::turn += 2_minutes;
        CHECK( npc_load::lighten_load( guy ) );
    }
}

TEST_CASE( "npc_under_its_limit_or_without_autonomy_keeps_everything", "[npc][autonomy][npc_load]" )
{
    clear_map_without_vision();
    clear_avatar();
    SECTION( "autonomy off" ) {
        override_option autonomy( "NPC_AUTONOMY", "false" );
        npc &guy = spawn_load_follower();
        overload( guy );
        CHECK_FALSE( npc_load::lighten_load( guy ) );
    }
    SECTION( "under the limit" ) {
        override_option autonomy( "NPC_AUTONOMY", "true" );
        npc &guy = spawn_load_follower();
        guy.i_add( item( itype_steel_lump ) );
        REQUIRE( guy.weight_carried() <= guy.weight_capacity() );
        CHECK( npc_load::what_to_drop( guy ).empty() );
        CHECK_FALSE( npc_load::lighten_load( guy ) );
    }
}

TEST_CASE( "what_you_trade_to_a_follower_is_a_favourite", "[npc][autonomy][npc_load]" )
{
    clear_map_without_vision();
    clear_avatar();
    avatar &you = get_avatar();
    item pack( itype_debug_backpack );
    REQUIRE( you.wear_item( pack, false ) );
    item_location lump = you.i_add( item( itype_steel_lump ) );
    REQUIRE( lump );

    const bool on = GENERATE( true, false );
    CAPTURE( on );
    override_option autonomy( "NPC_AUTONOMY", on ? "true" : "false" );
    npc &guy = spawn_load_follower();

    trade_selector::select_t stuff{ { lump, 1 } };
    std::list<item_location *> from_map;
    npc_trading::transfer_items( stuff, you, guy, from_map, false );
    REQUIRE( carried( guy, itype_steel_lump ) == 1 );
    bool favourite = false;
    guy.visit_items( [&favourite]( const item_location & it ) {
        if( it->typeId() == itype_steel_lump ) {
            favourite = it->is_favorite;
        }
        return VisitResponse::NEXT;
    } );
    CHECK( favourite == on );
}
