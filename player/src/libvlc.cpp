#include "libvlc.hpp"

// LibVLC uses ssize_t, which is POSIX...
#if defined(_MSC_VER)
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif
#include <vlc/vlc.h>

#include <algorithm>

namespace libvlc {

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

static auto create_instance_from_args(std::vector<std::string> args) {
    std::vector<const char*> raw_args_borrowed;

    std::transform(
        args.begin(), args.end(),
        std::back_inserter(raw_args_borrowed),
        [](auto & str) { return str.c_str(); }
    );

    auto argc = static_cast<int>(args.size());
    auto argv = raw_args_borrowed.data();

    return libvlc_new(argc, argv);
}

Instance::Instance(std::vector<std::string> args):
    CWrapper(create_instance_from_args(args), libvlc_release)
{ }

void Instance::set_log_callback(log_cb_t cb) {
    _cb = cb;
    libvlc_log_set(&*this, &on_log, (void*)&_cb);
}

void Instance::unset_log_callback() {
    libvlc_log_unset(&*this);
}

Equalizer::Equalizer():
    CWrapper(libvlc_audio_equalizer_new(), libvlc_audio_equalizer_release)
{ }

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
    libvlc_audio_set_volume(&*this, std::min(volume, 100));

    // Really not sure about the default pre amp value, seems to be 10 on my
    // machine...
    auto pre_amp_value = (volume > 100)
                       ? 10 + static_cast<float>(volume - 100) / 10.
                       : 10;
    libvlc_audio_equalizer_set_preamp(&_equalizer, pre_amp_value);
    libvlc_media_player_set_equalizer(&*this, &_equalizer);
}

void MediaPlayer::set_position(float rate) {
    libvlc_media_player_set_position(&*this, rate);
}

Media::Media(Instance &instance, const char *location):
    CWrapper(libvlc_media_new_location(&instance, location), libvlc_media_release)
{ }

}
