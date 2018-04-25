#include "libvlc.hpp"

// LibVLC uses ssize_t, which is POSIX...
#if defined(_MSC_VER)
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif
#include <vlc/vlc.h>

libvlc::LogLevel reify_log_level(int level) {
    switch (level) {
        case LIBVLC_DEBUG:   return libvlc::LogLevel::Debug;
        case LIBVLC_NOTICE:  return libvlc::LogLevel::Notice;
        case LIBVLC_WARNING: return libvlc::LogLevel::Warning;
        case LIBVLC_ERROR:   return libvlc::LogLevel::Error;
        default:             return libvlc::LogLevel::Unknown;
    }
}

static void on_log(void *data, int level, const vlc_log_t *, const char *fmt, va_list args) {
    auto callback = reinterpret_cast<libvlc::Instance::log_cb_t *>(data);
    char buff[4096];

    auto size = vsnprintf(buff, sizeof buff, fmt, args);
    if (size > 0) {
        auto entry = libvlc::LogEntry {
            reify_log_level(level),
            { buff, static_cast<std::string::size_type>(size) }
        };
        (*callback)(entry);
    }
}

namespace libvlc {

Instance::Instance():
    CWrapper(libvlc_new(0, nullptr), libvlc_release)
{ }

void Instance::set_log_callback(log_cb_t cb) {
    _cb = cb;
    libvlc_log_set(&*this, &on_log, (void*)&_cb);
}

void Instance::unset_log_callback() {
    libvlc_log_unset(&*this);
}

MediaPlayer::MediaPlayer(Instance &instance):
    CWrapper(libvlc_media_player_new(&instance), libvlc_media_player_release)
{
    libvlc_video_set_mouse_input(&*this, false);
    libvlc_video_set_key_input(&*this, false);
}

void MediaPlayer::set_media(Media &media) {
    libvlc_media_player_set_media(&*this, &media);
}

void MediaPlayer::set_renderer(void *hwnd) {
    libvlc_media_player_set_hwnd(&*this, hwnd);
}

void MediaPlayer::play() {
    libvlc_media_player_play(&*this);
}

void MediaPlayer::set_volume(int volume) {
    libvlc_audio_set_volume(&*this, volume);
}

void MediaPlayer::set_position(float rate) {
    libvlc_media_player_set_position(&*this, rate);
}

Media::Media(Instance &instance, const char *location):
    CWrapper(libvlc_media_new_location(&instance, location), libvlc_media_release)
{ }

}
