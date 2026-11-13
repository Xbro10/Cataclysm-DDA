#include "npc_autonomy.h"

#include <vector>

#include "avatar.h"
#include "dialogue_chatbin.h"
#include "messages.h"
#include "mission.h"
#include "npc.h"
#include "options.h"
#include "translations.h"

namespace npc_autonomy
{

bool enabled()
{
    return get_option<bool>( "NPC_AUTONOMY" );
}

void on_follower_leaves( npc &guy )
{
    if( !enabled() ) {
        return;
    }
    const avatar &player_character = get_avatar();
    // Copy: failing a mission can change the giver's list.
    const std::vector<mission *> assigned = guy.chatbin.missions_assigned;
    for( mission *miss : assigned ) {
        if( miss == nullptr || !miss->in_progress() ||
            miss->get_assigned_player_id() != player_character.getID() ) {
            continue;
        }
        miss->fail();
        add_msg( m_bad, _( "Quest cancelled: %1$s left you, so \"%2$s\" can't be handed in." ),
                 guy.get_name(), miss->name() );
    }
    guy.chatbin.missions_assigned.clear();
}

} // namespace npc_autonomy
