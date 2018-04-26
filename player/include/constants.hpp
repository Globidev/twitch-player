#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <QStringList>

namespace constants {
    static const auto TWITCHD_PATH = "twitchd.exe";
    static const auto TWITCHD_CLIENT_ID = "alcht5gave31yctuzv48v2i1hlwq53";

    namespace settings {
        namespace paths {
            static const auto KEY_CHAT_RENDERER_PATH = "paths/chat-renderer";
            // TODO: Improve default chat renderer path logic
            static const auto CHROME_PATH =
                R"(C:\Program Files (x86)\Google\Chrome\Application\chrome.exe)";
            static const auto DEFAULT_CHAT_RENDERER_PATH = CHROME_PATH;

            static const auto KEY_CHAT_RENDERER_ARGS = "chat-renderer/args";
            static const auto DEFAULT_CHAT_RENDERER_ARGS = QStringList()
                << "--app=https://www.twitch.tv/popout/${channel}/chat?popout=";
        }

        namespace vlc {
            static const auto KEY_VLC_ARGS = "vlc/args";
            static const auto DEFAULT_VLC_ARGS = QStringList()
                << "--network-caching=3000";
        }
    }
}

#endif // CONSTANTS_HPP
