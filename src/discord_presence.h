#pragma once
#ifndef CATA_SRC_DISCORD_PRESENCE_H
#define CATA_SRC_DISCORD_PRESENCE_H

// Xbro10 fork: Discord "Playing" card (Rich Presence). The game talks to the Discord desktop app over
// its local IPC connection; no Discord library is used. Settings (the application id, the picture for
// each dimension, the button) come from data/discord_presence.json, so they can change without a rebuild.
// Option DISCORD_PRESENCE: off / basic / detailed. Everything fails quietly when Discord isn't running.

#include <cstdint>
#include <map>
#include <string>

class JsonObject;

namespace discord_presence
{

// One dimension's line and picture on the card.
struct place {
    std::string text;       // e.g. "Level 1 · The Parking Deck (Class 1)"
    std::string image;      // asset key uploaded to the Discord application
    std::string image_text; // shown when hovering the picture
};

struct config {
    std::string app_id;                   // empty = the card is never sent
    std::map<std::string, place> places;  // by dimension id; "default" for the normal world
    place menu;                           // shown in the main menu
    std::string small_image;
    std::string button_label;
    std::string button_url;
};

// What the game is doing, gathered each update.
struct snapshot {
    bool in_menu = false;
    std::string dimension;     // dimension id, "" for the normal world
    int day = 1;
    int followers = 0;
    std::string doing;         // "Sleeping", "Driving", "" ...
    std::string who;           // "Breakroom Hire · Nursing Assistant"
    int64_t started_at = 0;    // unix seconds, for the timer
    bool detailed = false;
};

config parse_config( const JsonObject &jo );
// The SET_ACTIVITY "activity" object as JSON text (pure, for tests).
std::string activity_json( const config &cfg, const snapshot &s );

// Called from the game: in the main menu, once when a world is loaded, and every turn (throttled).
void show_menu();
void game_started();
void update();

} // namespace discord_presence

#endif // CATA_SRC_DISCORD_PRESENCE_H
