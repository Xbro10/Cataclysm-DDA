// Xbro10 fork: Discord "Playing" card (Rich Presence). See discord_presence.h.
#include "discord_presence.h"

#include <chrono>
#include <cstring>
#include <ctime>
#include <optional>
#include <sstream>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_utility.h"
#include "coordinates.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "json.h"
#include "options.h"
#include "path_info.h"
#include "profession.h"
#include "scenario.h"
#include "string_formatter.h"
#include "type_id.h"
#include "worldfactory.h"

#if defined(_WIN32)
#include "platform_win.h"
#else
#include <cstdlib>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace discord_presence
{

static std::string get_str( const JsonObject &jo, const std::string &name )
{
    return jo.has_string( name ) ? jo.get_string( name ) : std::string();
}

static place parse_place( const JsonObject &jo )
{
    jo.allow_omitted_members();
    place p;
    p.text = get_str( jo, "text" );
    p.image = get_str( jo, "image" );
    p.image_text = get_str( jo, "image_text" );
    return p;
}

config parse_config( const JsonObject &jo )
{
    jo.allow_omitted_members();
    config cfg;
    cfg.app_id = get_str( jo, "application_id" );
    cfg.small_image = get_str( jo, "small_image" );
    cfg.button_label = get_str( jo, "button_label" );
    cfg.button_url = get_str( jo, "button_url" );
    if( jo.has_object( "menu" ) ) {
        cfg.menu = parse_place( jo.get_object( "menu" ) );
    }
    if( jo.has_object( "places" ) ) {
        JsonObject places = jo.get_object( "places" );
        places.allow_omitted_members();
        for( const JsonMember &m : places ) {
            cfg.places[m.name()] = parse_place( m.get_object() );
        }
    }
    return cfg;
}

static std::string join_dot( const std::vector<std::string> &parts )
{
    std::string out;
    for( const std::string &p : parts ) {
        if( p.empty() ) {
            continue;
        }
        if( !out.empty() ) {
            out += " · ";
        }
        out += p;
    }
    return out;
}

std::string activity_json( const config &cfg, const snapshot &s )
{
    place where = cfg.menu;
    if( !s.in_menu ) {
        auto it = cfg.places.find( s.dimension.empty() ? "default" : s.dimension );
        if( it == cfg.places.end() ) {
            it = cfg.places.find( "default" );
        }
        if( it != cfg.places.end() ) {
            where = it->second;
        }
    }
    std::ostringstream os;
    JsonOut jsout( os );
    jsout.start_object();
    jsout.member( "details", where.text.empty() ? std::string( "Playing" ) : where.text );
    if( !s.in_menu ) {
        // Days survived on every card (user request); Detailed adds what you're doing.
        std::vector<std::string> state;
        const int survived = std::max( 0, s.day - 1 );
        state.push_back( survived == 0 ? std::string( "First day" ) : survived == 1 ?
                         std::string( "Survived 1 day" ) : string_format( "Survived %d days", survived ) );
        // Followers in words (Discord's own party counter, "(2 of 2)", read like a day count).
        if( s.followers > 0 ) {
            state.push_back( s.followers == 1 ? std::string( "1 follower" ) :
                             string_format( "%d followers", s.followers ) );
        }
        if( s.detailed ) {
            state.push_back( s.doing );
        }
        jsout.member( "state", join_dot( state ) );
    }
    if( !s.in_menu && s.started_at > 0 ) {
        jsout.member( "timestamps" );
        jsout.start_object();
        jsout.member( "start", s.started_at );
        jsout.end_object();
    }
    jsout.member( "assets" );
    jsout.start_object();
    if( !where.image.empty() ) {
        jsout.member( "large_image", where.image );
        // Without a small picture, the big one's hover text says who you're playing.
        const std::string hover = !s.in_menu && cfg.small_image.empty() && !s.who.empty() ? s.who :
                                  where.image_text;
        if( !hover.empty() ) {
            jsout.member( "large_text", hover );
        }
    }
    if( !cfg.small_image.empty() ) {
        jsout.member( "small_image", cfg.small_image );
        if( !s.in_menu && !s.who.empty() ) {
            jsout.member( "small_text", s.who );
        }
    }
    jsout.end_object();
    if( !cfg.button_label.empty() && !cfg.button_url.empty() ) {
        jsout.member( "buttons" );
        jsout.start_array();
        jsout.start_object();
        jsout.member( "label", cfg.button_label );
        jsout.member( "url", cfg.button_url );
        jsout.end_object();
        jsout.end_array();
    }
    jsout.end_object();
    return os.str();
}

// ---- The connection to the Discord desktop app ----
// Frames are: int32 opcode, int32 length (little endian), then that many bytes of JSON.
// Opcodes: 0 handshake, 1 frame. Discord's replies are read and thrown away.

namespace
{
using clock = std::chrono::steady_clock;

class connection
{
    public:
        bool is_open() const {
            return open_;
        }
        bool open( const std::string &app_id ) {
            for( int i = 0; i < 10 && !open_; ++i ) {
                open_ = try_open( i );
            }
            if( !open_ ) {
                return false;
            }
            std::ostringstream os;
            JsonOut jsout( os );
            jsout.start_object();
            jsout.member( "v", 1 );
            jsout.member( "client_id", app_id );
            jsout.end_object();
            if( !write_frame( 0, os.str() ) ) {
                close();
                return false;
            }
            return true;
        }
        bool write_frame( int32_t opcode, const std::string &json ) {
            std::string frame( 8, '\0' );
            const int32_t len = static_cast<int32_t>( json.size() );
            std::memcpy( frame.data(), &opcode, 4 );
            std::memcpy( frame.data() + 4, &len, 4 );
            frame += json;
            if( !write_all( frame ) ) {
                close();
                return false;
            }
            return true;
        }
        // Read and discard whatever Discord sent, without waiting.
        void drain();
        void close();
        ~connection() {
            close();
        }
    private:
        bool try_open( int index );
        bool write_all( const std::string &bytes );
        bool open_ = false;
#if defined(_WIN32)
        HANDLE pipe_ = INVALID_HANDLE_VALUE;
#else
        int sock_ = -1;
#endif
};

#if defined(_WIN32)
bool connection::try_open( int index )
{
    const std::string name = string_format( "\\\\.\\pipe\\discord-ipc-%d", index );
    pipe_ = CreateFileA( name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0,
                         nullptr );
    return pipe_ != INVALID_HANDLE_VALUE;
}

bool connection::write_all( const std::string &bytes )
{
    DWORD written = 0;
    return WriteFile( pipe_, bytes.data(), static_cast<DWORD>( bytes.size() ), &written, nullptr ) &&
           written == bytes.size();
}

void connection::drain()
{
    if( !open_ ) {
        return;
    }
    DWORD avail = 0;
    while( PeekNamedPipe( pipe_, nullptr, 0, nullptr, &avail, nullptr ) ) {
        if( avail == 0 ) {
            return;
        }
        char buf[512];
        DWORD got = 0;
        if( !ReadFile( pipe_, buf, std::min<DWORD>( avail, sizeof( buf ) ), &got, nullptr ) ) {
            break;
        }
    }
    close();
}

void connection::close()
{
    if( pipe_ != INVALID_HANDLE_VALUE ) {
        CloseHandle( pipe_ );
        pipe_ = INVALID_HANDLE_VALUE;
    }
    open_ = false;
}

int64_t process_id()
{
    return static_cast<int64_t>( GetCurrentProcessId() );
}
#else
bool connection::try_open( int index )
{
    const char *dir = nullptr;
    for( const char *var : {
             "XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP"
         } ) {
        dir = std::getenv( var );
        if( dir != nullptr ) {
            break;
        }
    }
    const std::string path = string_format( "%s/discord-ipc-%d", dir ? dir : "/tmp", index );
    sock_ = socket( AF_UNIX, SOCK_STREAM, 0 );
    if( sock_ < 0 ) {
        return false;
    }
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    std::strncpy( addr.sun_path, path.c_str(), sizeof( addr.sun_path ) - 1 );
    if( connect( sock_, reinterpret_cast<sockaddr *>( &addr ), sizeof( addr ) ) != 0 ) {
        ::close( sock_ );
        sock_ = -1;
        return false;
    }
    return true;
}

bool connection::write_all( const std::string &bytes )
{
    size_t done = 0;
    while( done < bytes.size() ) {
#if defined(MSG_NOSIGNAL)
        const ssize_t n = send( sock_, bytes.data() + done, bytes.size() - done, MSG_NOSIGNAL );
#else
        const ssize_t n = send( sock_, bytes.data() + done, bytes.size() - done, 0 );
#endif
        if( n <= 0 ) {
            return false;
        }
        done += static_cast<size_t>( n );
    }
    return true;
}

void connection::drain()
{
    if( !open_ ) {
        return;
    }
    char buf[512];
    while( true ) {
        const ssize_t n = recv( sock_, buf, sizeof( buf ), MSG_DONTWAIT );
        if( n > 0 ) {
            continue;
        }
        if( n == 0 ) {
            close();
        }
        return;
    }
}

void connection::close()
{
    if( sock_ >= 0 ) {
        ::close( sock_ );
        sock_ = -1;
    }
    open_ = false;
}

int64_t process_id()
{
    return static_cast<int64_t>( getpid() );
}
#endif

struct presence_state {
    std::optional<config> cfg;
    connection conn;
    std::optional<clock::time_point> last_connect_try;
    std::optional<clock::time_point> last_send;
    std::string last_activity;
    int64_t started_at = 0;
    bool in_menu = true;
    int nonce = 0;
};

presence_state &state()
{
    static presence_state s;
    return s;
}

const config &get_config()
{
    presence_state &st = state();
    if( !st.cfg ) {
        st.cfg = config();
        read_from_file_optional_json( PATH_INFO::datadir_path() / "discord_presence.json",
        [&]( const JsonValue & jv ) {
            st.cfg = parse_config( jv.get_object() );
        } );
    }
    return *st.cfg;
}

std::string option_value()
{
    return get_option<std::string>( "DISCORD_PRESENCE" );
}

std::string set_activity_payload( const std::string &activity )
{
    presence_state &st = state();
    return string_format( "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":%lld,\"activity\":%s},\"nonce\":\"%d\"}",
                          static_cast<long long>( process_id() ), activity, ++st.nonce );
}

// Sends the activity if it changed, at most once every 15 seconds (Discord's limit).
void send( const snapshot &s, bool force )
{
    presence_state &st = state();
    const config &cfg = get_config();
    if( cfg.app_id.empty() ) {
        return;
    }
    const clock::time_point now = clock::now();
    if( !st.conn.is_open() ) {
        if( st.last_connect_try && now - *st.last_connect_try < std::chrono::seconds( 30 ) ) {
            return;
        }
        st.last_connect_try = now;
        if( !st.conn.open( cfg.app_id ) ) {
            return;
        }
        st.last_activity.clear();
    }
    st.conn.drain();
    if( !force && st.last_send && now - *st.last_send < std::chrono::seconds( 15 ) ) {
        return;
    }
    const std::string activity = activity_json( cfg, s );
    if( activity == st.last_activity ) {
        return;
    }
    if( st.conn.is_open() && st.conn.write_frame( 1, set_activity_payload( activity ) ) ) {
        st.last_activity = activity;
        st.last_send = now;
    }
}

void clear_card()
{
    presence_state &st = state();
    if( st.conn.is_open() ) {
        st.conn.write_frame( 1, set_activity_payload( "null" ) );
        st.conn.close();
    }
    st.last_activity.clear();
}

snapshot game_snapshot()
{
    presence_state &st = state();
    snapshot s;
    s.in_menu = st.in_menu;
    s.started_at = st.started_at;
    s.detailed = option_value() == "detailed";
    if( st.in_menu || g == nullptr ) {
        s.in_menu = true;
        return s;
    }
    const avatar &u = get_avatar();
    s.dimension = g->get_dimension_prefix().str();
    // The normal world can be another place when a mod replaces it (the Backrooms mod makes it
    // Level 0): a "default:<mod id>" entry wins while that mod is active.
    if( ( s.dimension.empty() || s.dimension == "default" ) && world_generator &&
        world_generator->active_world ) {
        for( const mod_id &mod : world_generator->active_world->active_mod_order ) {
            const std::string key = "default:" + mod.str();
            if( get_config().places.count( key ) ) {
                s.dimension = key;
                break;
            }
        }
    }
    s.day = to_days<int>( calendar::turn - calendar::start_of_game ) + 1;
    s.followers = static_cast<int>( g->get_follower_list().size() );
    if( u.in_sleep_state() ) {
        s.doing = "Sleeping";
    } else if( u.in_vehicle && u.controlling_vehicle ) {
        s.doing = "Driving";
    }
    std::vector<std::string> who;
    if( get_scenario() != nullptr ) {
        who.push_back( get_scenario()->gender_appropriate_name( u.male ) );
    }
    if( u.prof != nullptr ) {
        who.push_back( u.prof->gender_appropriate_name( u.male ) );
    }
    s.who = join_dot( who );
    return s;
}
} // namespace

void show_menu()
{
    presence_state &st = state();
    st.in_menu = true;
    if( option_value() == "off" ) {
        clear_card();
        return;
    }
    send( game_snapshot(), true );
}

void game_started()
{
    presence_state &st = state();
    st.in_menu = false;
    st.started_at = static_cast<int64_t>( std::time( nullptr ) );
    if( option_value() == "off" ) {
        return;
    }
    send( game_snapshot(), true );
}

void update()
{
    presence_state &st = state();
    if( option_value() == "off" ) {
        if( st.conn.is_open() ) {
            clear_card();
        }
        return;
    }
    // Cheap checks first: this runs every turn.
    const clock::time_point now = clock::now();
    if( st.last_send && now - *st.last_send < std::chrono::seconds( 15 ) ) {
        return;
    }
    if( !st.conn.is_open() && st.last_connect_try &&
        now - *st.last_connect_try < std::chrono::seconds( 30 ) ) {
        return;
    }
    if( get_config().app_id.empty() ) {
        return;
    }
    send( game_snapshot(), false );
}

} // namespace discord_presence
