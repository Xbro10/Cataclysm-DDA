// Xbro10 fork: Gabriel (Survivor Minds), the Spearman loadout keeps his spear.
// Run with the mods:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=survivor_minds,xbro_additions "[survivor_minds]"

#include <memory>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "creature_tracker.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "npc_attack.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"

static const faction_id faction_your_followers( "your_followers" );

static const mtype_id mon_zombie( "mon_zombie" );

// Reported 2026-11-24: Gabriel dropped his spear for the hatchet on his belt in a fight and left it.
TEST_CASE( "gabriel_keeps_his_spear_in_a_fight", "[.][survivor_minds]" )
{
    clear_map();
    clear_avatar();
    map &here = get_map();
    const tripoint_bub_ms spot = get_avatar().pos_bub() + tripoint( 5, 0, 0 );
    npc &guy = spawn_npc( spot.xy(), "sm_gabriel_spear" );
    guy.setpos( here, spot );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    REQUIRE( guy.get_wielded_item() );
    const itype_id spear = guy.get_wielded_item()->typeId();
    std::string report = string_format( "wielding %s (%.1f)", spear.str(),
                                        guy.evaluate_weapon( *guy.get_wielded_item() ) );
    guy.visit_items( [&]( item_location node ) {
        if( node->is_melee() && node.get_item() != guy.get_wielded_item().get_item() ) {
            report += string_format( "; carries %s (%.1f)", node->typeId().str(),
                                     guy.evaluate_weapon( *node ) );
        }
        return VisitResponse::NEXT;
    } );
    INFO( report );
    monster *zombie = g->place_critter_at( mon_zombie, spot + tripoint( 1, 0, 0 ) );
    REQUIRE( zombie );
    for( const int hp : { zombie->get_hp_max(), 10, 1 } ) {
        CAPTURE( hp );
        zombie->set_hp( hp );
        guy.evaluate_best_attack( zombie );
        const std::shared_ptr<npc_attack> &attack = guy.get_current_attack();
        REQUIRE( attack );
        guy.set_moves( 100 );
        attack->use( guy, guy.get_current_attack_evaluation().target() );
        REQUIRE( guy.get_wielded_item() );
        CHECK( guy.get_wielded_item()->typeId() == spear );
        for( const item &it : here.i_at( guy.pos_bub() ) ) {
            CHECK( it.typeId() != spear );
        }
        if( zombie->is_dead() ) {
            break;
        }
    }
}
