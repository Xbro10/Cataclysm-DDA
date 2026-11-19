// Xbro10 fork: a character template made with a mod's scenario (say Backrooms Expanded's Breakroom Hire)
// is offered in every world. Loading it in a world without that mod used to pop up "Tried to use
// non-existent scenario"; now it quietly falls back to the generic scenario. Saves still report it.

#include <fstream>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "debug.h"
#include "filesystem.h"
#include "path_info.h"
#include "player_difficulty.h"
#include "scenario.h"

TEST_CASE( "template_with_a_missing_scenario_loads_quietly", "[chargen]" )
{
    const cata_path dir = PATH_INFO::templatedir_path();
    assure_dir_exist( dir );
    const std::string name = "xbro10_missing_scenario_test";
    const cata_path file = dir / ( name + ".template" );
    {
        std::ofstream out( file.get_unrelative_path() );
        out << R"json([ { "limit": 2 }, { "scenario": "no_such_mod_scenario", "profession": "unemployed" } ])json";
    }

    avatar u;
    pool_type pool = pool_type::FREEFORM;
    std::string msg;
    bool loaded = false;
    msg = capture_debugmsg_during( [&]() {
        loaded = u.load_template( name, pool );
    } );
    remove_file( file );

    CHECK( loaded );
    CHECK( msg.empty() );
    CHECK( get_scenario() == scenario::generic() );
}
