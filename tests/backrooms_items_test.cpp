// Xbro10 fork: Backrooms Expanded's items.
// Hidden test; run with the Backrooms mods loaded:
//   Cataclysm-test-vcpkg-static-Release-x64.exe --mods=backrooms,backrooms_expanded "[backrooms_items]"
// The Flamero-Mado spear: a fuel-fed spear you light and put out like the chainsaw. Lit, it adds heat
// damage, burns its fuel and goes out by itself when the tank is empty.

#include <string>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "damage.h"
#include "flag.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
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
