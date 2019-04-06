#pragma once

#include <QStringList>
#include <vector>

#define Constant static const auto

namespace constants {
#ifdef _WIN32
    Constant TWITCHD_PATH = "twitchd.exe";
#else
    Constant TWITCHD_PATH = "./twitchd";
#endif
    Constant TWITCHD_CLIENT_ID = "alcht5gave31yctuzv48v2i1hlwq53";

    Constant OAUTH_REDIRECT_URI_PORT = 4242;
    Constant OAUTH_REDIRECT_URI = QString("http://localhost:%1")
        .arg(OAUTH_REDIRECT_URI_PORT);
    Constant OAUTH_SCOPES =
        "user_follows_edit user_subscriptions chat_login user_read";

    namespace settings {
        namespace oauth {
            Constant ACCESS_TOKEN_KEY = "oauth/access-token";
            Constant REFRESH_TOKEN_KEY = "oauth/refresh-token";
        }

        namespace chat_renderer {
            Constant KEY_CHAT_RENDERER_ALWAYS_OPEN = "chat-renderer/always-open";
            Constant DEFAULT_CHAT_RENDERER_ALWAYS_OPEN = true;

            Constant KEY_CHAT_RENDERER_PATH = "chat-renderer/paths";
            // TODO: Improve default chat renderer path logic
            Constant CHROME_PATH =
#ifdef _WIN32
                R"(C:\Program Files (x86)\Google\Chrome\Application\chrome.exe)";
#elif __APPLE__
                R"(/Applications/Google Chrome.app/Contents/MacOS/Google Chrome)";
#elif __linux__
                R"(/bin/google-chrome-stable)";
#endif
            Constant DEFAULT_CHAT_RENDERER_PATH = CHROME_PATH;

            Constant KEY_CHAT_RENDERER_ARGS = "chat-renderer/args";
            Constant DEFAULT_CHAT_RENDERER_ARGS = QStringList()
                << "--app=https://www.twitch.tv/popout/${channel}/chat?popout=";

            Constant KEY_CHAT_RENDERER_TITLE_HINT = "chat-renderer/title-hint";
            Constant DEFAULT_CHAT_RENDERER_TITLE_HINT = "Twitch";
        }

        namespace streams {
            Constant KEY_LAST_QUALITY_FOR = [](auto channel) {
                return QString("streams/last_quality/%1").arg(channel);
            };
        }

        namespace ui {
            Constant KEY_LAST_VOLUME = "ui/last_volume";
            Constant DEFAULT_VOLUME = 50;
            Constant KEY_LAST_MUTE = "ui/last_mute";
            Constant DEFAULT_LAST_MUTE = false;
            Constant KEY_LAST_CHAT_SIZE_PERCENT = "ui/last_chat_size_percent";
            Constant DEFAULT_CHAT_SIZE_PERCENT = 25.f;
            Constant KEY_LAST_CHAT_POSITION = "ui/last_chat_position";
            Constant DEFAULT_CHAT_POSITION = 1; // Right
            Constant KEY_MAIN_WINDOW_GEOMETRY = "ui/main_window_geometry";
            Constant DEFAULT_MAIN_WINDOW_GEOMETRY = QByteArray();

            Constant KEY_ALWAYS_MINIMIZE_TO_TRAY = "ui/always_minimize_to_tray";
            Constant DEFAULT_ALWAYS_MINIMIZE_TO_TRAY = true;
            Constant KEY_DRAG_TO_MOVE = "ui/drag_to_move";
            Constant DEFAULT_DRAG_TO_MOVE = true;
        }

        namespace vlc {
            Constant KEY_VLC_ARGS = "vlc/args";
            Constant DEFAULT_VLC_ARGS = QStringList()
                << "--network-caching=1000";
        }

        namespace daemon {
            Constant KEY_MANAGED = "daemon/managed";
            Constant DEFAULT_MANAGED = true;
            Constant KEY_HOST_MANAGED = "daemon/host";
            Constant DEFAULT_HOST_MANAGED = "127.0.0.1";
            Constant KEY_PORT_MANAGED = "daemon/port";
            Constant DEFAULT_PORT_MANAGED = 7777;
            Constant KEY_HOST_UNMANAGED = "daemon/host_unmanaged";
            Constant DEFAULT_HOST_UNMANAGED = "twitch.glo.bi";
            Constant KEY_PORT_UNMANAGED = "daemon/port_unmanaged";
            Constant DEFAULT_PORT_UNMANAGED = 7777;
            Constant KEY_CACHE_TIMEOUT = "daemon/cache_timeout";
            Constant DEFAULT_CACHE_TIMEOUT = 60;
            Constant KEY_PLAYLIST_FETCH_INTERVAL = "daemon/playlist_fetch_interval";
            Constant DEFAULT_PLAYLIST_FETCH_INTERVAL = 1000;
            Constant KEY_PLAYER_INACTIVE_TIMEOUT = "daemon/player_inactive_timeout";
            Constant DEFAULT_PLAYER_INACTIVE_TIMEOUT = 60;
            Constant KEY_PLAYER_FETCH_TIMEOUT = "daemon/player_fetch_timeout";
            Constant DEFAULT_PLAYER_FETCH_TIMEOUT = 10;
            Constant KEY_PLAYER_VIDEO_CHUNKS_SIZE = "daemon/player_video_chunks_size";
            Constant DEFAULT_PLAYER_VIDEO_CHUNKS_SIZE = 131072;
            Constant KEY_PLAYER_MAX_SINK_BUFFER_SIZE = "daemon/player_max_sink_buffer_size";
            Constant DEFAULT_PLAYER_MAX_SINK_BUFFER_SIZE = 10000000;
        }

        namespace notifications {
            Constant KEY_PUBSUB_CHANNELS = "notifications/pubsub_channels";
            Constant DEFAULT_PUBSUB_CHANNELS = QStringList();
        }

        namespace shortcuts {
            struct ActionnableShortcutDescriptor {
                QString action_text;
                QString default_key_sequence;
                QString setting_key;
            };

            #define Shortcut static const ActionnableShortcutDescriptor

            Shortcut TOGGLE_FULL_SCREEN    { "Toggle Full Screen",    "F11",                  "shortcuts/full_screen" };
            Shortcut TOGGLE_WINDOW_BORDERS { "Toggle Window Borders", "F10",                  "shortcuts/window_borders" };
            Shortcut TOGGLE_ALWAYS_ON_TOP  { "Toggle Always On Top",  "Ctrl+A",               "shortcuts/always_on_top" };
            Shortcut TOGGLE_STREAM_ZOOM    { "Toggle Stream Zoom",    "Ctrl+Z",               "shortcuts/stream_zoom" };
            Shortcut ADD_PANE_LEFT         { "Add Pane left",         "Ctrl+Alt+Left",        "shortcuts/add_pane_left" };
            Shortcut ADD_PANE_RIGHT        { "Add Pane right",        "Ctrl+Alt+Right",       "shortcuts/add_pane_right" };
            Shortcut ADD_PANE_UP           { "Add Pane up",           "Ctrl+Alt+Up",          "shortcuts/add_pane_up" };
            Shortcut ADD_PANE_DOWN         { "Add Pane down",         "Ctrl+Alt+Down",        "shortcuts/add_pane_down" };
            Shortcut ADD_CHAT_PANE_LEFT    { "Add Chat Pane left",    "Shift+Alt+Left",       "shortcuts/add_chat_pane_left" };
            Shortcut ADD_CHAT_PANE_RIGHT   { "Add Chat Pane right",   "Shift+Alt+Right",      "shortcuts/add_chat_pane_right" };
            Shortcut ADD_CHAT_PANE_UP      { "Add Chat Pane up",      "Shift+Alt+Up",         "shortcuts/add_chat_pane_up" };
            Shortcut ADD_CHAT_PANE_DOWN    { "Add Chat Pane down",    "Shift+Alt+Down",       "shortcuts/add_chat_pane_down" };
            Shortcut REMOVE_ACTIVE_PANE    { "Remove Active Pane",    "Ctrl+Del",             "shortcuts/remove_active_pane" };
            Shortcut MOVE_FOCUS_LEFT       { "Move Focus Left",       "Left",                 "shortcuts/move_focus_left" };
            Shortcut MOVE_FOCUS_RIGHT      { "Move Focus Right",      "Right",                "shortcuts/move_focus_right" };
            Shortcut MOVE_FOCUS_UP         { "Move Focus Up",         "Up",                   "shortcuts/move_focus_up" };
            Shortcut MOVE_FOCUS_DOWN       { "Move Focus Down",       "Down",                 "shortcuts/move_focus_down" };
            Shortcut TOGGLE_CHAT_LEFT      { "Toggle Chat Left",      "Ctrl+Left",            "shortcuts/toggle_chat_left" };
            Shortcut TOGGLE_CHAT_RIGHT     { "Toggle Chat Right",     "Ctrl+Right",           "shortcuts/toggle_chat_right" };
            Shortcut RESIZE_CHAT_LEFT      { "Resize Chat Left",      "Ctrl+Shift+Left",      "shortcuts/resize_chat_left" };
            Shortcut RESIZE_CHAT_RIGHT     { "Resize Chat Right",     "Ctrl+Shift+Right",     "shortcuts/resize_chat_right" };
            Shortcut MOVE_PANE_LEFT        { "Move Pane Left",        "Ctrl+Alt+Shift+Left",  "shortcuts/move_pane_left" };
            Shortcut MOVE_PANE_RIGHT       { "Move Pane Right",       "Ctrl+Alt+Shift+Right", "shortcuts/move_pane_right" };
            Shortcut MOVE_PANE_UP          { "Move Pane Up",          "Ctrl+Alt+Shift+Up",    "shortcuts/move_pane_up" };
            Shortcut MOVE_PANE_DOWN        { "Move Pane Down",        "Ctrl+Alt+Shift+Down",  "shortcuts/move_pane_down" };
            Shortcut ROTATE_LAYOUT         { "Rotate Layout",         "Ctrl+R",               "shortcuts/rotate_layout" };
            Shortcut TOGGLE_MUTE           { "Toggle Mute",           "M",                    "shortcuts/toggle_mute" };
            Shortcut FAST_FORWARD          { "Fast Forward",          "F",                    "shortcuts/fast_forward" };
            Shortcut FILTERS_TOOL          { "Filters",               "Ctrl+E",               "shortcuts/effects_and_filters" };

            #undef Shortcut

            Constant ALL_SHORTCUTS = std::vector<ActionnableShortcutDescriptor> {
                TOGGLE_FULL_SCREEN, TOGGLE_WINDOW_BORDERS, TOGGLE_ALWAYS_ON_TOP, TOGGLE_STREAM_ZOOM,
                ADD_PANE_LEFT, ADD_PANE_RIGHT, ADD_PANE_UP, ADD_PANE_DOWN,
                ADD_CHAT_PANE_LEFT, ADD_CHAT_PANE_RIGHT, ADD_CHAT_PANE_UP, ADD_CHAT_PANE_DOWN,
                REMOVE_ACTIVE_PANE, MOVE_FOCUS_LEFT, MOVE_FOCUS_RIGHT, MOVE_FOCUS_UP,
                MOVE_FOCUS_DOWN, TOGGLE_CHAT_LEFT, TOGGLE_CHAT_RIGHT, RESIZE_CHAT_LEFT,
                RESIZE_CHAT_RIGHT, MOVE_PANE_LEFT, MOVE_PANE_RIGHT, MOVE_PANE_UP,
                MOVE_PANE_DOWN, ROTATE_LAYOUT, TOGGLE_MUTE, FAST_FORWARD, FILTERS_TOOL
            };
        }
    }
}

#undef Constant
