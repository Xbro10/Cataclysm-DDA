// Hidden test; run with Survivor Minds loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=survivor_minds "[survivor_minds]"
//
// Prompt 5.4: the morning journal entry mentions loneliness and fear when the fork's NPC
// autonomy keeps those needs high, and says nothing about them when the variables don't
// exist (an official build).

#include <array>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "dialogue.h"
#include "effect_on_condition.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"

static const effect_on_condition_id
effect_on_condition_EOC_SM_JOURNAL_WRITE_ENTRY( "EOC_SM_JOURNAL_WRITE_ENTRY" );

// The wordings in data/mods/Survivor_Minds/journal/journal_lines.json.
static const std::array<std::string, 4> lonely_lines{ {
        "Haven't talked to anyone in days.", "Caught myself talking to the walls.",
        "Would give a lot for a real conversation.", "Nobody to tell any of this to."
    }
};
static const std::array<std::string, 4> scared_lines{ {
        "Couldn't settle.  Kept watching the dark.", "Every sound had me reaching for something.",
        "Didn't feel safe for a second.", "Something's out there.  I can feel it."
    }
};

static bool mentions_any( const std::string &text, const std::array<std::string, 4> &lines )
{
    for( const std::string &line : lines ) {
        if( text.find( line ) != std::string::npos ) {
            return true;
        }
    }
    return false;
}

TEST_CASE( "survivor_minds_journal_mentions_loneliness_and_fear", "[.][survivor_minds]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 7_hours );
    npc &guy = spawn_npc( get_avatar().pos_bub().xy() + point( 3, 0 ), "test_talker" );
    // Writes one morning entry and returns it.
    const auto write_entry = [&guy]() {
        guy.remove_value( "sm_journal_text" );
        guy.remove_value( "sm_journal_day" );
        dialogue d( get_talker_for( guy ), nullptr );
        effect_on_condition_EOC_SM_JOURNAL_WRITE_ENTRY->activate( d );
        const diag_value *text = guy.maybe_get_value( "sm_journal_text" );
        REQUIRE( text != nullptr );
        return text->str();
    };

    SECTION( "no autonomy needs at all (an official build): no extra line" ) {
        const std::string entry = write_entry();
        INFO( entry );
        CHECK_FALSE( mentions_any( entry, lonely_lines ) );
        CHECK_FALSE( mentions_any( entry, scared_lines ) );
    }
    SECTION( "calm and in company: no extra line" ) {
        guy.set_value( "autonomy_need_social", 30.0 );
        guy.set_value( "autonomy_need_safety", 10.0 );
        const std::string entry = write_entry();
        INFO( entry );
        CHECK_FALSE( mentions_any( entry, lonely_lines ) );
        CHECK_FALSE( mentions_any( entry, scared_lines ) );
    }
    SECTION( "lonely (Social 60+): they write about it" ) {
        guy.set_value( "autonomy_need_social", 60.0 );
        const std::string entry = write_entry();
        INFO( entry );
        CHECK( mentions_any( entry, lonely_lines ) );
        CHECK_FALSE( mentions_any( entry, scared_lines ) );
    }
    SECTION( "scared (Safety 50+): they write about it" ) {
        guy.set_value( "autonomy_need_safety", 50.0 );
        const std::string entry = write_entry();
        INFO( entry );
        CHECK( mentions_any( entry, scared_lines ) );
        CHECK_FALSE( mentions_any( entry, lonely_lines ) );
    }
    SECTION( "both: both lines" ) {
        guy.set_value( "autonomy_need_social", 90.0 );
        guy.set_value( "autonomy_need_safety", 90.0 );
        const std::string entry = write_entry();
        INFO( entry );
        CHECK( mentions_any( entry, lonely_lines ) );
        CHECK( mentions_any( entry, scared_lines ) );
    }
}

static const effect_on_condition_id
effect_on_condition_EOC_SM_JOURNAL_MORNING( "EOC_SM_JOURNAL_MORNING" );
static const effect_on_condition_id
effect_on_condition_EOC_SM_DEBUG_JOURNAL_NOW_FOLLOWER( "EOC_SM_DEBUG_JOURNAL_NOW_FOLLOWER" );

static const faction_id faction_your_followers( "your_followers" );

// Reported 2026-11-19: a guard left far away all morning never wrote that day's entry (only
// 6-10 AM counted, and NPCs outside the loaded area don't run). Now they write it as soon as
// they're loaded again that day.
TEST_CASE( "survivor_minds_journal_written_later_the_same_day", "[.][survivor_minds]" )
{
    clear_map();
    clear_avatar();
    npc &guy = spawn_npc( get_avatar().pos_bub().xy() + point( 3, 0 ), "test_talker" );
    guy.remove_value( "sm_journal_day" );
    const auto due = [&guy]( const time_duration &time_of_day ) {
        set_time( calendar::turn_zero + 2_days + time_of_day );
        const_dialogue d( get_const_talker_for( guy ), nullptr );
        return effect_on_condition_EOC_SM_JOURNAL_MORNING->test_condition( d );
    };
    CHECK_FALSE( due( 5_hours ) );
    CHECK( due( 7_hours ) );
    CHECK( due( 15_hours ) );
    // Written today: not again.
    dialogue write( get_talker_for( guy ), nullptr );
    effect_on_condition_EOC_SM_JOURNAL_WRITE_ENTRY->activate( write );
    CHECK_FALSE( due( 15_hours ) );
}

TEST_CASE( "survivor_minds_debug_journal_now_includes_guards", "[.][survivor_minds]" )
{
    clear_map();
    clear_avatar();
    set_time( calendar::turn_zero + 15_hours );
    npc &guy = spawn_npc( get_avatar().pos_bub().xy() + point( 3, 0 ), "test_talker" );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_NULL );
    guy.set_mission( NPC_MISSION_GUARD_ALLY );
    REQUIRE( guy.is_player_ally() );
    REQUIRE_FALSE( guy.is_following() );
    guy.remove_value( "sm_journal_text" );
    dialogue d( get_talker_for( guy ), nullptr );
    effect_on_condition_EOC_SM_DEBUG_JOURNAL_NOW_FOLLOWER->activate( d );
    const diag_value *text = guy.maybe_get_value( "sm_journal_text" );
    REQUIRE( text != nullptr );
    CHECK( text->str().find( "Day " ) != std::string::npos );
}
