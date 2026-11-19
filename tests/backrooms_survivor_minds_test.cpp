// Xbro10 fork, Prompt 5.5: Backrooms Expanded's Survivor Minds traces.
// Hidden test; run with both mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded,survivor_minds "[backrooms_sm]"
//
// With Survivor Minds on, mod_interactions/survivor_minds fills Backrooms Expanded's empty mark
// hooks (weight 0) with chalk-mark furniture, and the dead survivor at an overrun camp carries a
// journal with its last entries already written.

#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "mapgen_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

TEST_CASE( "overrun_camp_holds_a_readable_journal_and_marks", "[.][backrooms_sm]" )
{
    if( !furn_str_id( "f_sm_mark_looted" ).is_valid() ||
        !furn_str_id( "f_br1_runner_board" ).is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded,survivor_minds" );
        return;
    }
    map &here = get_map();
    clear_map_without_vision();
    clear_avatar();
    const tripoint_abs_omt omt = get_avatar().pos_abs_omt();
    const tripoint_bub_ms corner = here.get_bub( project_to<coords::ms>( omt ) );

    SECTION( "the body carries a journal with an owner and last entries" ) {
        manual_mapgen( omt, manual_nested_mapgen, nested_mapgen_id( "br1_overrun_camp_core" ) );
        int journals = 0;
        for( const item &it : here.i_at( corner + point( 4, 3 ) ) ) {
            if( it.typeId() != itype_id( "sm_journal" ) ) {
                continue;
            }
            journals++;
            const std::string owner = it.get_var( "sm_journal_owner" );
            const std::string text = it.get_var( "sm_journal_text" );
            CAPTURE( owner, text );
            CHECK_FALSE( owner.empty() );
            CHECK( text.find( "Day " ) == 0 );
            CHECK( text.find( '\n' ) != std::string::npos );
        }
        CHECK( journals == 1 );
        // The picked-over crates are marked.
        CHECK( here.furn( corner + point( 9, 2 ) ) == furn_str_id( "f_sm_mark_looted" ) );
    }
    SECTION( "the mark hooks draw Survivor Minds' marks" ) {
        const struct {
            const char *hook;
            const char *furn;
        } marks[] = {
            { "br_sm_mark_camp", "f_sm_mark_camp" },
            { "br_sm_mark_danger", "f_sm_mark_danger" },
            { "br_sm_mark_looted", "f_sm_mark_looted" },
            { "br_sm_mark_water_n", "f_sm_mark_water_n" },
            { "br_sm_mark_water_e", "f_sm_mark_water_e" },
            { "br_sm_mark_water_s", "f_sm_mark_water_s" },
            { "br_sm_mark_water_w", "f_sm_mark_water_w" },
        };
        for( const auto &m : marks ) {
            CAPTURE( m.hook );
            here.furn_set( corner, furn_str_id::NULL_ID() );
            manual_mapgen( omt, manual_nested_mapgen, nested_mapgen_id( m.hook ) );
            CHECK( here.furn( corner ) == furn_str_id( m.furn ) );
        }
    }
}
