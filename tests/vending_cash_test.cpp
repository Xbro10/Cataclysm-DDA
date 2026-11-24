// Xbro10 fork: vending machines take paper cash and coins (iexamine::vending, iexamine_helper::vending_pay_cash),
// paying with as few and as small bills as cover the price, change in coins. Straps and bundles don't count.

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "iexamine.h"
#include "item.h"
#include "player_helpers.h"
#include "type_id.h"

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_coin_dollar( "coin_dollar" );
static const itype_id itype_coin_quarter( "coin_quarter" );
static const itype_id itype_money_one( "money_one" );
static const itype_id itype_money_strap_one( "money_strap_one" );
static const itype_id itype_money_twenty( "money_twenty" );

TEST_CASE( "vending_machines_take_cash_and_give_change", "[iexamine][vending]" )
{
    clear_avatar();
    avatar &you = get_avatar();
    you.wear_item( item( itype_backpack ), false, false );

    SECTION( "a twenty for a $1.25 snack gives $18.75 back in coins" ) {
        you.i_add( item( itype_money_twenty, calendar::turn ) );
        REQUIRE( iexamine_helper::vending_cash_value( you ) == 2000 );
        CHECK( iexamine_helper::vending_pay_cash( you, 125 ) );
        CHECK( you.amount_of( itype_money_twenty ) == 0 );
        CHECK( you.amount_of( itype_coin_dollar ) == 18 );
        CHECK( you.amount_of( itype_coin_quarter ) == 3 );
        CHECK( iexamine_helper::vending_cash_value( you ) == 1875 );
    }

    SECTION( "the smallest bills that cover it are used, the rest kept" ) {
        you.i_add( item( itype_money_one, calendar::turn ) );
        you.i_add( item( itype_money_one, calendar::turn ) );
        you.i_add( item( itype_money_twenty, calendar::turn ) );
        CHECK( iexamine_helper::vending_pay_cash( you, 150 ) );
        CHECK( you.amount_of( itype_money_twenty ) == 1 );
        CHECK( you.amount_of( itype_money_one ) == 0 );
        CHECK( you.amount_of( itype_coin_quarter ) == 2 );
        CHECK( iexamine_helper::vending_cash_value( you ) == 2050 );
    }

    SECTION( "not enough cash takes nothing" ) {
        you.i_add( item( itype_money_one, calendar::turn ) );
        CHECK_FALSE( iexamine_helper::vending_pay_cash( you, 150 ) );
        CHECK( you.amount_of( itype_money_one ) == 1 );
    }

    SECTION( "a currency strap isn't fed into a vending machine" ) {
        you.i_add( item( itype_money_strap_one, calendar::turn ) );
        CHECK( iexamine_helper::vending_cash_value( you ) == 0 );
    }
}
