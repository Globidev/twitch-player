#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace constants {
    static const auto TWITCHD_PATH = R"(C:\Users\depar\dev\twitch-player\twitchd\target\release\twitchd.exe)";
    static const auto TWITCHD_CLIENT_ID = "alcht5gave31yctuzv48v2i1hlwq53";

    static const char * const VLC_ARGV[] = {
        "--verbose=0",
        "--file-caching=1000",
        "--live-caching=1000",
        "--disc-caching=1000",
        "--network-caching=1000",
    };
    static const auto VLC_ARGC = sizeof(VLC_ARGV) / sizeof(*VLC_ARGV);
}

#endif // CONSTANTS_HPP
