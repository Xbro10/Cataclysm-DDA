// Xbro10 fork: overloaded NPCs lighten their load (see npc_load.h).
#include "npc_load.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor_definitions.h"
#include "calendar.h"
#include "flag.h"
#include "item.h"
#include "item_location.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "player_activity.h"
#include "point.h"
#include "string_formatter.h"
#include "translations.h"
#include "units.h"
#include "visitable.h"

static const char *const var_load_checked = "autonomy_load_checked_at";

namespace
{

bool feeds_weapon( const item &weapon, const item &it )
{
    if( !weapon.is_gun() ) {
        return false;
    }
    if( it.is_magazine() ) {
        return weapon.magazine_compatible().count( it.typeId() ) > 0;
    }
    if( it.is_ammo() ) {
        return weapon.ammo_types().count( it.ammo_type() ) > 0;
    }
    return false;
}

double turns_now()
{
    return static_cast<double>( to_turns<int>( calendar::turn - calendar::turn_zero ) );
}

} // namespace

namespace npc_load
{

bool keeps( const npc &guy, const item &it )
{
    if( it.has_flag( flag_FIREWOOD ) ) {
        return true;
    }
    const item_location weapon = guy.get_wielded_item();
    bool keep = false;
    it.visit_items( [&]( const item * e, item * ) {
        if( e->is_favorite || e->is_comestible() || ( weapon && feeds_weapon( *weapon, *e ) ) ) {
            keep = true;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    return keep;
}

std::vector<item_location> what_to_drop( npc &guy )
{
    units::mass over = guy.weight_carried() - guy.weight_capacity();
    if( over <= 0_gram ) {
        return {};
    }
    // Loose things in their pockets and bags: the top-level contents of what they wear. Worn clothes
    // and the wielded item themselves are never candidates.
    std::vector<std::pair<double, item_location>> candidates;
    guy.visit_items( [&]( const item_location & loc ) {
        if( !loc.has_parent() ) {
            return VisitResponse::NEXT;
        }
        if( !loc.parent_item().has_parent() && loc->weight() > 0_gram && !keeps( guy, *loc ) ) {
            const double per_kg = guy.value( *loc ) / units::to_kilogram( loc->weight() );
            candidates.emplace_back( per_kg, loc );
        }
        return VisitResponse::SKIP;
    } );
    std::stable_sort( candidates.begin(), candidates.end(),
    []( const auto & a, const auto & b ) {
        return a.first < b.first;
    } );
    std::vector<item_location> ret;
    for( const auto &c : candidates ) {
        if( over <= 0_gram ) {
            break;
        }
        ret.push_back( c.second );
        over -= c.second->weight();
    }
    return ret;
}

bool lighten_load( npc &guy )
{
    if( !npc_autonomy::enabled() || guy.weight_carried() <= guy.weight_capacity() ) {
        return false;
    }
    const diag_value *checked = guy.maybe_get_value( var_load_checked );
    const double now = turns_now();
    if( checked != nullptr && checked->is_dbl() &&
        now - checked->dbl() < to_turns<int>( 1_minutes ) ) {
        return false;
    }
    guy.set_value( var_load_checked, now );

    const std::vector<item_location> drop = what_to_drop( guy );
    if( drop.empty() ) {
        return false;
    }
    std::vector<drop_or_stash_item_info> to_drop;
    to_drop.reserve( drop.size() );
    for( const item_location &loc : drop ) {
        to_drop.emplace_back( loc, loc->count() );
    }
    const std::string first = drop.front()->tname();
    guy.say( drop.size() == 1 ?
             string_format( _( "Too heavy.  Leaving the %s here." ), first ) :
             string_format( _( "Too heavy.  Leaving the %s and some other junk here." ), first ) );
    guy.assign_activity( drop_activity_actor( to_drop, tripoint_rel_ms::zero, false ) );
    return true;
}

} // namespace npc_load
