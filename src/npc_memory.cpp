#include "npc_memory.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "calendar.h"
#include "game.h"
#include "line.h"
#include "math_parser_diag_value.h"
#include "npc.h"
#include "npc_autonomy.h"
#include "point.h"
#include "type_id.h"

namespace npc_memory
{

namespace
{

// The saved NPC var holding the list.
constexpr const char *var_places = "autonomy_places";

int now_turns()
{
    return to_turns<int>( calendar::turn - calendar::turn_zero );
}

std::string this_dimension()
{
    return g == nullptr ? std::string() : g->get_dimension_prefix().str();
}

// How much a kind matters when the memory is full: a danger spot or water is worth keeping
// over a looted room.
int importance( place_kind kind )
{
    switch( kind ) {
        case place_kind::danger:
            return 5;
        case place_kind::water:
            return 4;
        case place_kind::safe:
            return 3;
        case place_kind::camp:
            return 2;
        case place_kind::looted:
        case place_kind::last:
            break;
    }
    return 1;
}

// How long a memory lasts without seeing the place again.
time_duration lasts_for( place_kind kind )
{
    switch( kind ) {
        case place_kind::danger:
            return 3_days;
        case place_kind::looted:
            return 7_days;
        case place_kind::safe:
            return 10_days;
        case place_kind::water:
        case place_kind::camp:
        case place_kind::last:
            break;
    }
    return 30_days;
}

bool faded( const place &p, const int now )
{
    return now - p.seen > to_turns<int>( lasts_for( p.kind ) );
}

// Lower is dropped first: importance, minus a point per day since they saw it.
double keep_score( const place &p, const int now )
{
    return importance( p.kind ) - static_cast<double>( now - p.seen ) / to_turns<double>( 1_days );
}

std::vector<place> stored( const npc &guy )
{
    const diag_value *val = guy.maybe_get_value( var_places );
    if( val == nullptr || !val->is_str() ) {
        return {};
    }
    return decode( val->str() );
}

void store( npc &guy, const std::vector<place> &list )
{
    if( list.empty() ) {
        guy.remove_value( var_places );
    } else {
        guy.set_value( var_places, encode( list ) );
    }
}

} // namespace

int same_place_radius( place_kind kind )
{
    return kind == place_kind::danger ? 5 : 3;
}

std::string encode( const std::vector<place> &list )
{
    std::string out;
    for( const place &p : list ) {
        if( !out.empty() ) {
            out += ';';
        }
        out += std::to_string( static_cast<int>( p.kind ) ) + ',' + std::to_string( p.pos.x() ) + ',' +
               std::to_string( p.pos.y() ) + ',' + std::to_string( p.pos.z() ) + ',' +
               std::to_string( p.seen ) + ',' + p.dimension;
    }
    return out;
}

std::vector<place> decode( const std::string &text )
{
    std::vector<place> list;
    std::istringstream entries( text );
    std::string entry;
    while( std::getline( entries, entry, ';' ) ) {
        std::istringstream fields( entry );
        std::string field;
        std::vector<std::string> parts;
        while( std::getline( fields, field, ',' ) ) {
            parts.push_back( field );
        }
        // An empty dimension leaves the last field off.
        if( parts.size() < 5 ) {
            continue;
        }
        try {
            const int kind = std::stoi( parts[0] );
            if( kind < 0 || kind >= static_cast<int>( place_kind::last ) ) {
                continue;
            }
            place p;
            p.kind = static_cast<place_kind>( kind );
            p.pos = tripoint_abs_ms( std::stoi( parts[1] ), std::stoi( parts[2] ), std::stoi( parts[3] ) );
            p.seen = std::stoi( parts[4] );
            p.dimension = parts.size() > 5 ? parts[5] : std::string();
            list.push_back( p );
        } catch( const std::exception & ) {
            // A damaged entry is just forgotten.
            continue;
        }
    }
    return list;
}

std::vector<place> places( const npc &guy )
{
    std::vector<place> list = stored( guy );
    const int now = now_turns();
    list.erase( std::remove_if( list.begin(), list.end(), [now]( const place & p ) {
        return faded( p, now );
    } ), list.end() );
    return list;
}

int count( const npc &guy )
{
    return static_cast<int>( places( guy ).size() );
}

void remember( npc &guy, const place_kind kind, const tripoint_abs_ms &pos )
{
    if( !npc_autonomy::enabled() || kind == place_kind::last ) {
        return;
    }
    std::vector<place> list = places( guy );
    const int now = now_turns();
    const std::string dimension = this_dimension();
    for( place &p : list ) {
        if( p.kind == kind && p.dimension == dimension && p.pos.z() == pos.z() &&
            rl_dist( p.pos, pos ) <= same_place_radius( kind ) ) {
            // Seen again: it's still there.
            p.pos = pos;
            p.seen = now;
            store( guy, list );
            return;
        }
    }
    list.push_back( { kind, pos, dimension, now } );
    while( static_cast<int>( list.size() ) > max_places ) {
        const auto drop = std::min_element( list.begin(), list.end(),
        [now]( const place & a, const place & b ) {
            const double score_a = keep_score( a, now );
            const double score_b = keep_score( b, now );
            return score_a != score_b ? score_a < score_b : a.seen < b.seen;
        } );
        list.erase( drop );
    }
    store( guy, list );
}

void forget( npc &guy, const place_kind kind, const tripoint_abs_ms &pos )
{
    std::vector<place> list = places( guy );
    const std::string dimension = this_dimension();
    const size_t before = list.size();
    list.erase( std::remove_if( list.begin(), list.end(), [&]( const place & p ) {
        return p.kind == kind && p.dimension == dimension && p.pos.z() == pos.z() &&
               rl_dist( p.pos, pos ) <= same_place_radius( kind );
    } ), list.end() );
    if( list.size() != before ) {
        store( guy, list );
    }
}

std::optional<tripoint_abs_ms> nearest( const npc &guy, const place_kind kind,
                                        const std::function<bool( const tripoint_abs_ms & )> &usable )
{
    const std::string dimension = this_dimension();
    std::optional<tripoint_abs_ms> best;
    int best_dist = std::numeric_limits<int>::max();
    for( const place &p : places( guy ) ) {
        if( p.kind != kind || p.dimension != dimension || ( usable && !usable( p.pos ) ) ) {
            continue;
        }
        const int dist = rl_dist( guy.pos_abs(), p.pos );
        if( dist < best_dist ) {
            best_dist = dist;
            best = p.pos;
        }
    }
    return best;
}

} // namespace npc_memory
