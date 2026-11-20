#include <list>
#include <memory>
#include <optional>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_id.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "flag.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "npc_attack.h"
#include "options_helpers.h"
#include "overmap_ui.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

class Creature;

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_combat_exoskeleton_medium( "combat_exoskeleton_medium" );
static const itype_id itype_combat_exoskeleton_medium_on( "combat_exoskeleton_medium_on" );
static const itype_id itype_heavy_battery_cell( "heavy_battery_cell" );
static const itype_id itype_knife_large( "knife_large" );
static const itype_id itype_modular_m16a4( "modular_m16a4" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_wearable_light( "wearable_light" );

static const mtype_id mon_zombie( "mon_zombie" );

static const string_id<npc_template> npc_template_test_talker( "test_talker" );

static const weather_type_id weather_sunny( "sunny" );

static constexpr point_bub_ms main_npc_start{ 50, 50 };
static constexpr tripoint_bub_ms main_npc_start_tripoint{ main_npc_start, 0 };

namespace npc_attack_setup
{
static npc &spawn_main_npc()
{
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    get_player_character().setpos( here, { main_npc_start, -1 } );
    REQUIRE( creatures.creature_at<Creature>( main_npc_start_tripoint ) == nullptr );
    const character_id model_id = here.place_npc( main_npc_start, npc_template_test_talker );

    npc &model_npc = *g->find_npc( model_id );
    clear_character( model_npc );
    model_npc.setpos( here, main_npc_start_tripoint );

    g->load_npcs();

    REQUIRE( creatures.creature_at<npc>( main_npc_start_tripoint ) != nullptr );

    return model_npc;
}

static npc &respawn_main_npc()
{
    npc *guy = get_creature_tracker().creature_at<npc>( main_npc_start_tripoint );
    if( guy ) {
        guy->die( &get_map(), nullptr );
    }
    return spawn_main_npc();
}

static monster *spawn_zombie_at_range( const int range )
{
    return g->place_critter_at( mon_zombie, main_npc_start_tripoint + tripoint( range, 0, 0 ) );
}
} // namespace npc_attack_setup

TEST_CASE( "NPC_faces_zombies", "[npc_attack]" )
{
    map &here = get_map();

    get_player_character().setpos( here, main_npc_start_tripoint );
    clear_map_and_put_player_underground();
    clear_vehicles();
    scoped_weather_override sunny_weather( weather_sunny );
    npc &main_npc = npc_attack_setup::respawn_main_npc();
    // Allied NPC for behavior testing purposes
    main_npc.set_fac( faction_your_followers );

    GIVEN( "There is a zombie 1 tile away" ) {
        monster *zombie = npc_attack_setup::spawn_zombie_at_range( 1 );

        WHEN( "NPC only has a large knife" ) {
            item weapon( itype_knife_large );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item()->typeId() == itype_knife_large );

            THEN( "NPC attempts to melee the enemy target" ) {
                main_npc.evaluate_best_attack( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                CHECK( melee_attack );
                const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                CHECK( rating.value() );
                CHECK( *rating.value() > 0 );
                CHECK( rating.target() == zombie->pos_bub() );
            }
        }
        WHEN( "NPC only has an m16a4" ) {
            arm_shooter( main_npc, itype_modular_m16a4 );

            WHEN( "NPC is allowed to use loud ranged weapons" ) {
                main_npc.rules.set_flag( ally_rule::use_guns );
                REQUIRE( main_npc.rules.has_flag( ally_rule::use_guns ) );
                main_npc.rules.clear_flag( ally_rule::use_silent );
                REQUIRE( !main_npc.rules.has_flag( ally_rule::use_silent ) );

                THEN( "NPC tries to shoot the enemy target" ) {
                    main_npc.evaluate_best_attack( zombie );
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_gun *ranged_attack = dynamic_cast<npc_attack_gun *>( attack.get() );
                    CHECK( ranged_attack );
                    const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                    CHECK( rating.value() );
                    CHECK( *rating.value() > 0 );
                    CHECK( rating.target() == zombie->pos_bub() );
                }
            }
            WHEN( "NPC isn't allowed to use loud weapons" ) {
                main_npc.rules.set_flag( ally_rule::use_silent );
                REQUIRE( main_npc.rules.has_flag( ally_rule::use_silent ) );

                THEN( "NPC can't fire his weapon" ) {
                    main_npc.evaluate_best_attack( zombie );
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_gun *ranged_attack = dynamic_cast<npc_attack_gun *>( attack.get() );
                    CHECK( !ranged_attack );
                }
            }
            WHEN( "NPC isn't allowed to use ranged weapons" ) {
                main_npc.rules.clear_flag( ally_rule::use_guns );
                REQUIRE( !main_npc.rules.has_flag( ally_rule::use_guns ) );

                THEN( "NPC can't fire his weapon" ) {
                    main_npc.evaluate_best_attack( zombie );
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_gun *ranged_attack = dynamic_cast<npc_attack_gun *>( attack.get() );
                    CHECK( !ranged_attack );
                }
            }
        }
        WHEN( "NPC only has a bunch of rocks" ) {
            item weapon( itype_rock );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item()->typeId() == itype_rock );

            THEN( "NPC doesn't bother throwing the rocks so close" ) {
                main_npc.evaluate_best_attack( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_throw *throw_attack = dynamic_cast<npc_attack_throw *>( attack.get() );
                CHECK( !throw_attack );
            }
        }
        WHEN( "NPC has an exoskeleton" ) {
            main_npc.clear_worn();
            item armor( itype_combat_exoskeleton_medium );
            std::optional<std::list<item>::iterator> wear_success = main_npc.wear_item( armor );
            item &worn_armor = **wear_success;

            REQUIRE( wear_success );

            // If the flag gets removed from power armor, some other item with the flag will need to replace it.
            REQUIRE( main_npc.worn_with_flag( flag_COMBAT_TOGGLEABLE ) );

            WHEN( "NPC has a battery for their armor" ) {

                item battery = item( itype_heavy_battery_cell );
                battery.ammo_set( battery.ammo_default() );
                worn_armor.put_in( battery, pocket_type::MAGAZINE_WELL );

                REQUIRE( worn_armor.ammo_remaining( ) > 0 );

                THEN( "NPC activates their exoskeleton successfully" ) {

                    // target is not exposed, so regen_ai_cache is used to have the npc re-assess threat and store the target.
                    main_npc.regen_ai_cache();
                    main_npc.method_of_attack();
                    CHECK( main_npc.is_wearing( itype_combat_exoskeleton_medium_on ) );
                    CHECK( !main_npc.is_wearing( itype_combat_exoskeleton_medium ) );
                }
            }

            WHEN( "NPC has no power supply for their exoskeleton" ) {
                THEN( "NPC fails to activate their exoskeleton" ) {
                    main_npc.regen_ai_cache();
                    main_npc.method_of_attack();
                    CHECK( main_npc.is_wearing( itype_combat_exoskeleton_medium ) );
                    CHECK( !main_npc.is_wearing( itype_combat_exoskeleton_medium_on ) );
                }
            }
        }
        WHEN( "NPC has a headlamp" ) {
            main_npc.clear_worn();

            item headlamp( itype_wearable_light );
            std::optional<std::list<item>::iterator> wear_success = main_npc.wear_item( headlamp );
            REQUIRE( wear_success );

            // If the flag gets added, some other item without the flag will need to replace it.
            REQUIRE( !main_npc.worn_with_flag( flag_COMBAT_TOGGLEABLE ) );

            // This test is to ensure NPCs do not randomly activate items in their inventory.
            THEN( "NPC does not activate their headlamp" ) {
                main_npc.regen_ai_cache();
                main_npc.method_of_attack();
                CHECK( main_npc.is_wearing( itype_wearable_light ) );
            }
        }
    }
    GIVEN( "There is a zombie 5 tiles away" ) {
        monster *zombie = npc_attack_setup::spawn_zombie_at_range( 5 );

        WHEN( "NPC only has a large knife" ) {
            item weapon( itype_knife_large );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item()->typeId() == itype_knife_large );

            THEN( "NPC attempts to melee the enemy target" ) {
                main_npc.evaluate_best_attack( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                CHECK( melee_attack );
                const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                CHECK( rating.value() );
                CHECK( *rating.value() > 0 );
                CHECK( rating.target() == zombie->pos_bub() );
            }
        }

        WHEN( "NPC only has a bunch of rocks" ) {
            item weapon( itype_rock, calendar::turn, 5 );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item()->typeId() == itype_rock );

            THEN( "NPC throws rocks at the zombie" ) {
                main_npc.evaluate_best_attack( zombie );
                const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                npc_attack_throw *throw_attack = dynamic_cast<npc_attack_throw *>( attack.get() );
                CHECK( throw_attack );
            }
        }
    }
    GIVEN( "There is a zombie 1 tile away and another 8 tiles away" ) {
        monster *zombie = npc_attack_setup::spawn_zombie_at_range( 1 );
        monster *zombie_far = npc_attack_setup::spawn_zombie_at_range( 8 );

        WHEN( "NPC only has a large knife" ) {
            item weapon( itype_knife_large );
            main_npc.set_wielded_item( weapon );
            REQUIRE( main_npc.get_wielded_item()->typeId() == itype_knife_large );

            WHEN( "NPC is targetting closest zombie" ) {
                main_npc.evaluate_best_attack( zombie );

                THEN( "NPC tries to attack closest zombie" ) {
                    const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                    npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                    CHECK( melee_attack );
                    const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                    CHECK( rating.value() );
                    CHECK( *rating.value() > 0 );
                    CHECK( rating.target() == zombie->pos_bub() );
                }
            }
            WHEN( "NPC is targetting farthest zombie" ) {
                WHEN( "Furthest zombie is at full HP" ) {
                    main_npc.evaluate_best_attack( zombie_far );

                    THEN( "NPC tries to attack closest zombie" ) {
                        const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                        npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                        CHECK( melee_attack );
                        const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                        CHECK( rating.value() );
                        CHECK( *rating.value() > 0 );
                        CHECK( rating.target() == zombie->pos_bub() );
                    }
                }
                WHEN( "Furthest zombie is at low HP" ) {
                    zombie_far->set_hp( 1 );
                    main_npc.evaluate_best_attack( zombie_far );

                    THEN( "NPC tries to attack furthest zombie" ) {
                        const std::shared_ptr<npc_attack> &attack = main_npc.get_current_attack();
                        npc_attack_melee *melee_attack = dynamic_cast<npc_attack_melee *>( attack.get() );
                        CHECK( melee_attack );
                        const npc_attack_rating &rating = main_npc.get_current_attack_evaluation();
                        CHECK( rating.value() );
                        CHECK( *rating.value() > 0 );
                        CHECK( rating.target() == zombie_far->pos_bub() );
                    }
                }
            }
        }
    }
    GIVEN( "There is no zombie nearby. " ) {
        WHEN( "NPC is wearing active exoskeleton. " ) {
            item armor( itype_combat_exoskeleton_medium_on );
            armor.activate();
            std::optional<std::list<item>::iterator> wear_success = main_npc.wear_item( armor );
            REQUIRE( wear_success );

            THEN( "NPC deactivates their exoskeleton. " ) {
                // This is somewhat cheating, but going up one level is testing all of npc::move.
                main_npc.cleanup_on_no_danger();

                CHECK( !main_npc.is_wearing( itype_combat_exoskeleton_medium_on ) );
                CHECK( main_npc.is_wearing( itype_combat_exoskeleton_medium ) );
            }
        }
    }
}

// TODO: Add scenarios for:
// - NPCs carrying a mix of weapons
// - NPCs trying to shoot through allies

// Xbro10 fork: an NPC behind a chain-link fence shoots through it (arrows and bullets fly through chain-link,
// projectile_passes_mesh) instead of walking round or climbing over to fight; a wall still blocks the shot.
TEST_CASE( "NPC_shoots_through_chain_link", "[npc_attack]" )
{
    map &here = get_map();
    get_player_character().setpos( here, main_npc_start_tripoint );
    clear_map_and_put_player_underground();
    clear_vehicles();
    scoped_weather_override sunny_weather( weather_sunny );
    npc &main_npc = npc_attack_setup::respawn_main_npc();
    main_npc.set_fac( faction_your_followers );
    main_npc.set_str_base( 14 );
    main_npc.reset_stats();
    main_npc.rules.set_flag( ally_rule::use_guns );
    main_npc.rules.clear_flag( ally_rule::use_silent );

    const std::string weapon = GENERATE( "compbow", "modular_m16a4" );
    CAPTURE( weapon );
    arm_shooter( main_npc, itype_id( weapon ) );
    monster *zombie = npc_attack_setup::spawn_zombie_at_range( 4 );
    REQUIRE( zombie );
    // Tough enough to take every shot, so it is still there for the wall half.
    zombie->set_hp( 100000 );
    const tripoint_bub_ms fence = main_npc_start_tripoint + tripoint( 2, 0, 0 );

    // Play the NPC's turns: count it as firing once the loaded ammo goes down.
    const auto fires_within_ten_turns = [&]() {
        main_npc.evaluate_best_attack( zombie );
        npc_attack_gun *gun = dynamic_cast<npc_attack_gun *>( main_npc.get_current_attack().get() );
        if( !gun ) {
            return false;
        }
        const int loaded = main_npc.get_wielded_item()->ammo_remaining();
        for( int turn = 0; turn < 10; turn++ ) {
            main_npc.set_moves( 100 );
            gun->use( main_npc, zombie->pos_bub() );
            if( main_npc.get_wielded_item()->ammo_remaining() < loaded ) {
                return true;
            }
        }
        return false;
    };

    // A wall counts as blocking (the NPC would go looking for a clear shot): the shot rates lower than through chain-link.
    const auto shot_rating = [&]( const char *ter ) {
        here.ter_set( fence, ter_id( ter ) );
        main_npc.evaluate_best_attack( zombie );
        REQUIRE( dynamic_cast<npc_attack_gun *>( main_npc.get_current_attack().get() ) );
        return main_npc.get_current_attack_evaluation().value().value_or( 0 );
    };
    const int through_wall = shot_rating( "t_wall" );
    const int through_fence = shot_rating( "t_chainfence" );
    CHECK( through_wall < through_fence );

    CHECK( fires_within_ten_turns() );
    CHECK( main_npc.pos_bub() == main_npc_start_tripoint );
}

// Reported 2026-11-19: a Breakroom archer in a sandbag nest never shot a zombie on the far side of a
// half sandbag wall and the yard's chain-link. Low cover you can shoot over doesn't block the shot.
TEST_CASE( "NPC_shoots_over_sandbags_and_through_chain_link", "[npc_attack]" )
{
    map &here = get_map();
    get_player_character().setpos( here, main_npc_start_tripoint );
    clear_map_and_put_player_underground();
    clear_vehicles();
    scoped_weather_override sunny_weather( weather_sunny );
    npc &main_npc = npc_attack_setup::respawn_main_npc();
    main_npc.set_fac( faction_your_followers );
    main_npc.set_str_base( 14 );
    main_npc.reset_stats();
    main_npc.rules.set_flag( ally_rule::use_guns );
    main_npc.rules.clear_flag( ally_rule::use_silent );
    arm_shooter( main_npc, itype_id( "compbow" ) );
    monster *zombie = npc_attack_setup::spawn_zombie_at_range( 4 );
    REQUIRE( zombie );
    zombie->set_hp( 100000 );
    here.furn_set( main_npc_start_tripoint + tripoint( 1, 0, 0 ), furn_id( "f_sandbag_half" ) );
    here.ter_set( main_npc_start_tripoint + tripoint( 3, 0, 0 ), ter_id( "t_chainfence" ) );

    main_npc.evaluate_best_attack( zombie );
    npc_attack_gun *gun = dynamic_cast<npc_attack_gun *>( main_npc.get_current_attack().get() );
    REQUIRE( gun );
    const int loaded = main_npc.get_wielded_item()->ammo_remaining();
    bool fired = false;
    for( int turn = 0; turn < 10 && !fired; turn++ ) {
        main_npc.set_moves( 100 );
        gun->use( main_npc, zombie->pos_bub() );
        fired = main_npc.get_wielded_item()->ammo_remaining() < loaded;
    }
    CHECK( fired );
    CHECK( main_npc.pos_bub() == main_npc_start_tripoint );
}
