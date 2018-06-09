#pragma once

#include <string>
#include <variant>

// LibVLC uses ssize_t, which is POSIX...
#if defined(_MSC_VER)
  #include <BaseTsd.h>
  using ssize_t = SSIZE_T;
#endif

// libvlc Forwards
struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;
struct libvlc_equalizer_t;
struct libvlc_event_manager_t;
struct vlc_log_t;
//

namespace libvlc {

// Bindings forwards
struct Instance;
struct Media;
struct Equalizer;
struct MediaPlayer;
//

namespace events {
    struct Opening { };
    struct Playing { };
    struct TimeChanged { int64_t new_time; };
    struct Buffering { float cache_percent; };
    struct Stopped { };
    struct EndReached { };
    struct EncounteredError { };
    struct Unknown { };
}

using Event = std::variant<
    events::Opening,
    events::Playing,
    events::TimeChanged,
    events::Buffering,
    events::Stopped,
    events::EndReached,
    events::EncounteredError,
    events::Unknown
>;

enum class LogLevel {
    Debug,
    Notice,
    Warning,
    Error,
    Unknown
};

struct LogEntry {
    LogLevel level;
    std::string text;
};

struct AudioDevice {
    std::string id;
    std::string description;
};

}
