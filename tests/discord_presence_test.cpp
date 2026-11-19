// Xbro10 fork: the Discord "Playing" card's text (src/discord_presence.cpp). No Discord needed.
#include <string>

#include "cata_catch.h"
#include "discord_presence.h"
#include "flexbuffer_json.h"
#include "json_loader.h"
#include "cata_utility.h"
#include "path_info.h"

static discord_presence::config sample_config()
{
    const std::string json = R"json({
        "application_id": "123",
        "small_image": "fork_logo",
        "button_label": "Get the fork",
        "button_url": "https://example.com/releases",
        "menu": { "text": "In the main menu", "image": "cdda_menu" },
        "places": {
            "default": { "text": "Surviving the Cataclysm", "image": "cdda_world" },
            "default:backrooms": { "text": "Level 0", "image": "level0" },
            "backrooms_level_2": { "text": "Level 2 (Class 3)", "image": "level2", "image_text": "Pipes" }
        }
    })json";
    JsonValue jv = json_loader::from_string( json );
    return discord_presence::parse_config( jv.get_object() );
}

TEST_CASE( "discord_presence_card_text", "[discord_presence]" )
{
    const discord_presence::config cfg = sample_config();
    CHECK( cfg.app_id == "123" );
    CHECK( cfg.places.size() == 3 );

    discord_presence::snapshot s;
    SECTION( "main menu" ) {
        s.in_menu = true;
        const std::string a = discord_presence::activity_json( cfg, s );
        CHECK( a.find( "\"details\":\"In the main menu\"" ) != std::string::npos );
        CHECK( a.find( "\"large_image\":\"cdda_menu\"" ) != std::string::npos );
        CHECK( a.find( "timestamps" ) == std::string::npos );
        CHECK( a.find( "\"state\"" ) == std::string::npos );
        CHECK( a.find( "party" ) == std::string::npos );
    }
    SECTION( "basic, Level 2: level line, timer, button, days survived, party size" ) {
        s.dimension = "backrooms_level_2";
        s.started_at = 1000;
        s.day = 4;
        s.followers = 2;
        s.who = "Pipe Rat";
        const std::string a = discord_presence::activity_json( cfg, s );
        CHECK( a.find( "\"details\":\"Level 2 (Class 3)\"" ) != std::string::npos );
        CHECK( a.find( "\"large_image\":\"level2\"" ) != std::string::npos );
        CHECK( a.find( "\"large_text\":\"Pipes\"" ) != std::string::npos );
        CHECK( a.find( "\"start\":1000" ) != std::string::npos );
        CHECK( a.find( "\"small_text\":\"Pipe Rat\"" ) != std::string::npos );
        CHECK( a.find( "\"label\":\"Get the fork\"" ) != std::string::npos );
        CHECK( a.find( "\"state\":\"Survived 3 days\"" ) != std::string::npos );
        // You and two followers: Discord shows "(3 of 3)".
        CHECK( a.find( "\"party\":{\"id\":\"cdda-party\",\"size\":[3,3]}" ) != std::string::npos );
    }
    SECTION( "first day, alone: no party counter" ) {
        s.day = 1;
        const std::string a = discord_presence::activity_json( cfg, s );
        CHECK( a.find( "\"state\":\"First day\"" ) != std::string::npos );
        CHECK( a.find( "party" ) == std::string::npos );
    }
    SECTION( "detailed adds what you're doing" ) {
        s.dimension = "default:backrooms";
        s.detailed = true;
        s.day = 12;
        s.followers = 3;
        s.doing = "Sleeping";
        const std::string a = discord_presence::activity_json( cfg, s );
        CHECK( a.find( "\"details\":\"Level 0\"" ) != std::string::npos );
        CHECK( a.find( "\"state\":\"Survived 11 days · Sleeping\"" ) != std::string::npos );
        CHECK( a.find( "\"size\":[4,4]" ) != std::string::npos );
    }
    SECTION( "an unknown dimension falls back to the normal world" ) {
        s.dimension = "some_other_mod_dimension";
        const std::string a = discord_presence::activity_json( cfg, s );
        CHECK( a.find( "\"details\":\"Surviving the Cataclysm\"" ) != std::string::npos );
    }
}

TEST_CASE( "discord_presence_one_picture_hover_shows_who", "[discord_presence]" )
{
    // With no small picture (the shipped setup), the big picture's hover text says who you play.
    discord_presence::config cfg = sample_config();
    cfg.small_image.clear();
    discord_presence::snapshot s;
    s.dimension = "backrooms_level_2";
    s.who = "Pipe Rat · Plumber";
    const std::string a = discord_presence::activity_json( cfg, s );
    CHECK( a.find( "\"large_text\":\"Pipe Rat" ) != std::string::npos );
    CHECK( a.find( "small_image" ) == std::string::npos );
}

TEST_CASE( "discord_presence_shipped_config_parses", "[discord_presence]" )
{
    // The real data/discord_presence.json must parse (application_id may still be empty).
    bool read = false;
    discord_presence::config cfg;
    read_from_file_optional_json( PATH_INFO::datadir_path() / "discord_presence.json",
    [&]( const JsonValue & jv ) {
        cfg = discord_presence::parse_config( jv.get_object() );
        read = true;
    } );
    REQUIRE( read );
    CHECK( cfg.places.count( "default" ) == 1 );
    CHECK( cfg.places.count( "default:backrooms" ) == 1 );
    CHECK( !cfg.button_url.empty() );
}
