#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "coordinates.h"
#include "cata_catch.h"
#include "character.h"
#include "dialogue.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "math_parser.h"
#include "player_helpers.h"
#include "talker.h"
#include "type_id.h"

static const itype_id itype_butter( "butter" );
static const itype_id itype_caff_gum( "caff_gum" );
static const itype_id itype_ghee( "ghee" );
static const itype_id itype_raw_butter( "raw_butter" );

TEST_CASE( "consumption_count_math_function", "[math_parser][consumption]" )
{
    clear_avatar();
    avatar &guy = get_avatar();
    math_exp testexp;

    dialogue d( get_talker_for( guy ), std::make_unique<talker>() );

    SECTION( "returns 0 with no consumption history" ) {
        guy.consumption_history.clear();
        REQUIRE( testexp.parse( "u_consumption_count('butter')" ) );
        CHECK( testexp.eval( d ) == Approx( 0 ) );
    }

    SECTION( "counts matching items in history" ) {
        guy.consumption_history.clear();
        item butter( itype_butter );
        guy.consumption_history.emplace_back( butter );
        guy.consumption_history.emplace_back( butter );
        guy.consumption_history.emplace_back( butter );

        REQUIRE( testexp.parse( "u_consumption_count('butter')" ) );
        CHECK( testexp.eval( d ) == Approx( 3 ) );

        // Different item should not count
        REQUIRE( testexp.parse( "u_consumption_count('apple')" ) );
        CHECK( testexp.eval( d ) == Approx( 0 ) );
    }

    SECTION( "hours kwarg filters by time window" ) {
        guy.consumption_history.clear();
        item butter( itype_butter );

        // Add a recent event (current turn)
        guy.consumption_history.emplace_back( butter );

        // Add an old event (10 hours ago)
        consumption_event old_event( butter );
        old_event.time = calendar::turn - 10_hours;
        guy.consumption_history.push_back( old_event );

        // Default 48h window should see both
        REQUIRE( testexp.parse( "u_consumption_count('butter')" ) );
        CHECK( testexp.eval( d ) == Approx( 2 ) );

        // 6h window should see only the recent one
        REQUIRE( testexp.parse( "u_consumption_count('butter', 'hours': 6)" ) );
        CHECK( testexp.eval( d ) == Approx( 1 ) );

        // 12h window should see both
        REQUIRE( testexp.parse( "u_consumption_count('butter', 'hours': 12)" ) );
        CHECK( testexp.eval( d ) == Approx( 2 ) );
    }

    SECTION( "eats_like groups related foods together" ) {
        guy.consumption_history.clear();
        item butter( itype_butter );
        item ghee( itype_ghee );
        item raw_butter( itype_raw_butter );

        guy.consumption_history.emplace_back( butter );
        guy.consumption_history.emplace_back( ghee );
        guy.consumption_history.emplace_back( raw_butter );

        // All three should count when querying butter (ghee and raw_butter have eats_like: butter)
        REQUIRE( testexp.parse( "u_consumption_count('butter')" ) );
        CHECK( testexp.eval( d ) == Approx( 3 ) );

        // Querying ghee directly should also count all three (resolves to butter)
        REQUIRE( testexp.parse( "u_consumption_count('ghee')" ) );
        CHECK( testexp.eval( d ) == Approx( 3 ) );
    }
}

TEST_CASE( "consumption_count_total_math_function", "[math_parser][consumption]" )
{
    clear_avatar();
    avatar &guy = get_avatar();
    math_exp testexp;

    dialogue d( get_talker_for( guy ), std::make_unique<talker>() );

    SECTION( "returns 0 with no history" ) {
        guy.consumption_history.clear();
        REQUIRE( testexp.parse( "u_consumption_count_total()" ) );
        CHECK( testexp.eval( d ) == Approx( 0 ) );
    }

    SECTION( "counts all items regardless of type" ) {
        guy.consumption_history.clear();
        item butter( itype_butter );
        item ghee( itype_ghee );
        guy.consumption_history.emplace_back( butter );
        guy.consumption_history.emplace_back( butter );
        guy.consumption_history.emplace_back( ghee );

        REQUIRE( testexp.parse( "u_consumption_count_total()" ) );
        CHECK( testexp.eval( d ) == Approx( 3 ) );
    }

    SECTION( "hours kwarg filters by time window" ) {
        guy.consumption_history.clear();
        item butter( itype_butter );

        guy.consumption_history.emplace_back( butter );

        consumption_event old_event( butter );
        old_event.time = calendar::turn - 10_hours;
        guy.consumption_history.push_back( old_event );

        REQUIRE( testexp.parse( "u_consumption_count_total()" ) );
        CHECK( testexp.eval( d ) == Approx( 2 ) );

        REQUIRE( testexp.parse( "u_consumption_count_total('hours': 6)" ) );
        CHECK( testexp.eval( d ) == Approx( 1 ) );
    }
}

// Xbro10 fork (2026-11-23): chewing gum (a med with a use action) eaten from a tile next to the
// eater logged "Couldn't find item caffeinated chewing gum to form item_location": the item was
// looked for only on the eater and their own tile. The caller's location is used now.
TEST_CASE( "chewing_gum_from_the_next_tile", "[consumption]" )
{
    clear_avatar();
    clear_map_without_vision();
    avatar &guy = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms next = guy.pos_bub() + tripoint::east;
    item &gum = here.add_item( next, item( itype_caff_gum ) );
    REQUIRE( gum.typeId() == itype_caff_gum );
    item_location loc( map_cursor( &here, next ), &gum );
    // A debugmsg fails the test.
    CHECK( guy.consume( loc, true ) != trinary::NONE );
}
