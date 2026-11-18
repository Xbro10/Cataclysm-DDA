// Xbro10 fork: Backrooms Expanded's items.
// Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_items]"
// The Flamero-Mado spear: a fuel-fed spear you light and put out like the chainsaw. Lit, it adds heat
// damage, burns its fuel and goes out by itself when the tank is empty.
// The spawn check: every Backrooms Expanded and Survivor Minds item group (ids starting br, NC_BR or NC_SM) is
// spawned many times; nothing may come out loose that should be packed (a liquid outside a container, a gun
// lying beside an empty-welled magazine that fits it). Overflowing containers already fail through debugmsg.

#include <map>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "damage.h"
#include "enums.h"
#include "flag.h"
#include "item.h"
#include "item_factory.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "magic.h"
#include "map_helpers.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "string_formatter.h"
#include "type_id.h"

static const damage_type_id damage_heat( "heat" );
static const damage_type_id damage_stab( "stab" );

static const item_group_id Item_spawn_data_br_flamero_cache( "br_flamero_cache" );

static const itype_id itype_br_flamero_mado_off( "br_flamero_mado_off" );
static const itype_id itype_br_flamero_mado_on( "br_flamero_mado_on" );
static const itype_id itype_lamp_oil( "lamp_oil" );

static bool backrooms_items_loaded()
{
    if( !itype_br_flamero_mado_off.is_valid() ) {
        WARN( "Needs --mods=backrooms,backrooms_expanded" );
        return false;
    }
    return true;
}

TEST_CASE( "flamero_mado_spear_lights_burns_and_goes_out", "[.][backrooms_items]" )
{
    if( !backrooms_items_loaded() ) {
        return;
    }
    clear_avatar();
    clear_map_without_vision();
    avatar &you = get_avatar();
    map &here = get_map();

    item spear( itype_br_flamero_mado_off );
    // Unlit it is a plain (good) spear: it pierces and doesn't burn.
    CHECK( spear.damage_melee( damage_stab ) > 0 );
    CHECK( spear.damage_melee( damage_heat ) == 0 );

    SECTION( "an empty tank won't light" ) {
        REQUIRE( you.wield( spear ) );
        you.invoke_item( &*you.get_wielded_item() );
        CHECK( you.get_wielded_item()->typeId() == itype_br_flamero_mado_off );
    }

    SECTION( "kerosene lights it; lit, it burns as well as pierces, then runs dry and goes out" ) {
        spear.ammo_set( itype_lamp_oil, 100 );
        REQUIRE( spear.ammo_remaining() == 100 );
        REQUIRE( you.wield( spear ) );
        you.invoke_item( &*you.get_wielded_item() );
        item_location lit = you.get_wielded_item();
        REQUIRE( lit->typeId() == itype_br_flamero_mado_on );
        CHECK( lit->active );
        CHECK( lit->damage_melee( damage_heat ) > 0 );
        CHECK( lit->damage_melee( damage_stab ) == spear.damage_melee( damage_stab ) );
        CHECK( lit->has_flag( flag_FLAMING ) );

        // Burn it out: one charge every 3 turns (lighting it is free).
        int turns = 0;
        while( you.get_wielded_item()->typeId() == itype_br_flamero_mado_on && turns < 1000 ) {
            calendar::turn += 1_turns;
            you.process_items( &here );
            turns++;
        }
        CHECK( you.get_wielded_item()->typeId() == itype_br_flamero_mado_off );
        CHECK( you.get_wielded_item()->ammo_remaining() == 0 );
        CHECK( turns > 2 * 100 );
        CHECK( turns <= 4 * 100 );
    }

    SECTION( "using it again puts it out and keeps the fuel" ) {
        spear.ammo_set( itype_lamp_oil, 100 );
        REQUIRE( you.wield( spear ) );
        you.invoke_item( &*you.get_wielded_item() );
        REQUIRE( you.get_wielded_item()->typeId() == itype_br_flamero_mado_on );
        you.invoke_item( &*you.get_wielded_item() );
        CHECK( you.get_wielded_item()->typeId() == itype_br_flamero_mado_off );
        CHECK_FALSE( you.get_wielded_item()->active );
        CHECK( you.get_wielded_item()->ammo_remaining() == 100 );
    }
}

TEST_CASE( "flamero_mado_cache_spawns_a_spear_with_some_fuel", "[.][backrooms_items]" )
{
    if( !backrooms_items_loaded() ) {
        return;
    }
    for( int i = 0; i < 50; i++ ) {
        bool found = false;
        for( const item &it : item_group::items_from( Item_spawn_data_br_flamero_cache ) ) {
            if( it.typeId() == itype_br_flamero_mado_off ) {
                found = true;
                CHECK( it.ammo_remaining() <= 250 );
            }
        }
        CHECK( found );
    }
}

static bool is_mod_group( const item_group_id &id )
{
    const std::string &s = id.str();
    return s.rfind( "br", 0 ) == 0 || s.rfind( "NC_BR", 0 ) == 0 || s.rfind( "NC_SM", 0 ) == 0;
}

TEST_CASE( "backrooms_item_groups_spawn_packed", "[.][backrooms_items]" )
{
    if( !backrooms_items_loaded() ) {
        return;
    }
    std::vector<item_group_id> groups;
    for( const item_group_id &id : item_controller->get_all_group_names() ) {
        if( is_mod_group( id ) ) {
            groups.push_back( id );
        }
    }
    REQUIRE( groups.size() > 20 );

    // group -> what came out loose, and how often.
    std::map<std::string, std::map<std::string, int>> loose;
    for( const item_group_id &id : groups ) {
        for( int i = 0; i < 300; i++ ) {
            const item_group::ItemList spawned = item_group::items_from( id, calendar::turn_zero );
            for( const item &it : spawned ) {
                if( it.made_of( phase_id::LIQUID ) ) {
                    loose[id.str()]["liquid outside a container: " + it.typeId().str()]++;
                }
                if( !it.is_gun() || it.magazine_integral() || it.magazine_current() ) {
                    continue;
                }
                for( const item &other : spawned ) {
                    if( other.is_magazine() && it.can_contain( other ).success() ) {
                        loose[id.str()]["gun " + it.typeId().str() + " beside its magazine " +
                                        other.typeId().str()]++;
                    }
                }
            }
        }
    }
    std::string report;
    for( const auto &g : loose ) {
        for( const auto &what : g.second ) {
            report += string_format( "\n  %s: %s (%d of 300 spawns)", g.first, what.first, what.second );
        }
    }
    INFO( "Checked " << groups.size() << " item groups." << report );
    CHECK( loose.empty() );
}

TEST_CASE( "guard_office_pistols_spawn_loaded", "[.][backrooms_items]" )
{
    if( !backrooms_items_loaded() ) {
        return;
    }
    int pistols = 0;
    for( int i = 0; i < 300; i++ ) {
        for( const item &it : item_group::items_from( item_group_id( "br1_security_guns" ),
                calendar::turn_zero ) ) {
            if( !it.is_gun() || it.magazine_integral() ) {
                continue;
            }
            pistols++;
            CAPTURE( it.typeId().str() );
            REQUIRE( it.magazine_current() != nullptr );
            CHECK( it.ammo_remaining() > 0 );
        }
    }
    CHECK( pistols > 100 );
}

TEST_CASE( "backrooms_wiki_finds_work", "[.][backrooms_items]" )
{
    if( !backrooms_items_loaded() ) {
        return;
    }
    clear_avatar();
    clear_map_without_vision();
    avatar &you = get_avatar();
    const itype_id firesalt( "br_firesalt" );
    const itype_id moth_jelly( "br_moth_jelly" );
    const itype_id liquid_pain( "br_liquid_pain" );
    const bodypart_id torso( "torso" );

    SECTION( "firesalt fuels the Flamero-Mado spear" ) {
        item spear( itype_br_flamero_mado_off );
        spear.ammo_set( firesalt, 50 );
        REQUIRE( spear.ammo_remaining() == 50 );
        REQUIRE( you.wield( spear ) );
        you.invoke_item( &*you.get_wielded_item() );
        CHECK( you.get_wielded_item()->typeId() == itype_br_flamero_mado_on );
    }

    SECTION( "moth jelly mends a few hit points" ) {
        you.set_part_hp_cur( torso, 20 );
        item jelly( moth_jelly );
        you.consume( jelly, /*force=*/true );
        CHECK( you.get_part_hp_cur( torso ) > 20 );
    }

    SECTION( "cloudy water is liquid pain" ) {
        REQUIRE( you.get_pain() == 0 );
        item drink( liquid_pain );
        you.consume( drink, /*force=*/true );
        CHECK( you.get_pain() >= 30 );
    }

    SECTION( "deathmoths leave moth jelly" ) {
        int jelly = 0;
        for( int i = 0; i < 200; i++ ) {
            for( const item &it : item_group::items_from( item_group_id( "br_deathmoth_drops" ),
                    calendar::turn_zero ) ) {
                jelly += it.typeId() == moth_jelly ? 1 : 0;
            }
        }
        CHECK( jelly > 30 );
    }
}

TEST_CASE( "npcs_never_drink_cloudy_water_on_their_own", "[.][backrooms_items]" )
{
    if( !backrooms_items_loaded() ) {
        return;
    }
    clear_avatar();
    clear_map_without_vision();
    npc &guy = spawn_npc( point_bub_ms( 62, 60 ), "test_talker" );
    clear_character( guy );
    guy.set_thirst( 400 );

    item pain( itype_id( "br_liquid_pain" ) );
    REQUIRE( pain.has_flag( flag_id( "NPC_REFUSES" ) ) );
    guy.i_add( pain.in_its_container() );
    CHECK_FALSE( guy.consume_food( npc::consume_filter::drink_only ) );
    CHECK( guy.find_nearby_food( npc::consume_filter::drink_only ).empty() );

    // Control: real water in the same pack is drunk.
    guy.i_add( item( itype_id( "water_clean" ) ).in_its_container() );
    CHECK( guy.consume_food( npc::consume_filter::drink_only ) );
}

TEST_CASE( "firesalt_tools_and_the_burning_spear", "[.][backrooms_items]" )
{
    if( !backrooms_items_loaded() ) {
        return;
    }
    SECTION( "the lit spear carries its on-hit fire spell, the unlit one doesn't" ) {
        const item lit( itype_br_flamero_mado_on );
        const item unlit( itype_br_flamero_mado_off );
        CHECK( lit.is_relic() );
        CHECK_FALSE( unlit.is_relic() );
        CHECK( spell_id( "br_flamero_ignite" ).is_valid() );
    }
    SECTION( "the firesalt twist starts fires and the torch outlasts a plain one" ) {
        const item twist( itype_id( "br_firesalt_twist" ) );
        CHECK( twist.has_flag( flag_id( "FIRESTARTER" ) ) );
        CHECK( twist.ammo_remaining() == 5 );
        const item torch( itype_id( "br_firesalt_torch_lit" ) );
        const item plain( itype_id( "torch_lit" ) );
        CHECK( torch.ammo_remaining() > plain.ammo_remaining() );
    }
}
