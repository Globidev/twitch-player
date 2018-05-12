#include "libvlc.hpp"

// LibVLC uses ssize_t, which is POSIX...
#if defined(_MSC_VER)
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif
#include <vlc/vlc.h>

#include <algorithm>
#include <array>

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
    auto callback = reinterpret_cast<Instance::log_cb_t *>(data);
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

bool MediaPlayer::video_filters_enabled() {
    return static_cast<bool>(libvlc_video_get_adjust_float(&*this, libvlc_adjust_Enable));
}

void MediaPlayer::enable_video_filters(bool on) {
    libvlc_video_set_adjust_int(&*this, libvlc_adjust_Enable, static_cast<int>(on));
}

float MediaPlayer::get_contrast() {
    return libvlc_video_get_adjust_float(&*this, libvlc_adjust_Contrast);
}

void MediaPlayer::set_contrast(float contrast) {
    libvlc_video_set_adjust_float(&*this, libvlc_adjust_Contrast, contrast);
}

float MediaPlayer::get_brightness() {
    return libvlc_video_get_adjust_float(&*this, libvlc_adjust_Brightness);
}

void MediaPlayer::set_brightness(float brightness) {
    libvlc_video_set_adjust_float(&*this, libvlc_adjust_Brightness, brightness);
}

float MediaPlayer::get_hue() {
    return libvlc_video_get_adjust_float(&*this, libvlc_adjust_Hue);
}

void MediaPlayer::set_hue(float hue) {
    libvlc_video_set_adjust_float(&*this, libvlc_adjust_Hue, hue);
}

float MediaPlayer::get_saturation() {
    return libvlc_video_get_adjust_float(&*this, libvlc_adjust_Saturation);
}

void MediaPlayer::set_saturation(float saturation) {
    libvlc_video_set_adjust_float(&*this, libvlc_adjust_Saturation, saturation);
}

float MediaPlayer::get_gamma() {
    return libvlc_video_get_adjust_float(&*this, libvlc_adjust_Gamma);
}

void MediaPlayer::set_gamma(float gamma) {
    libvlc_video_set_adjust_float(&*this, libvlc_adjust_Gamma, gamma);
}

std::vector<AudioDevice> MediaPlayer::audio_devices() {
    std::vector<AudioDevice> devices;

    auto device = libvlc_audio_output_device_enum(&*this);
    while (device) {
        devices.push_back({ device->psz_device, device->psz_description });
        device = device->p_next;
    }
    libvlc_audio_output_device_list_release(device);

    return devices;
}

std::string MediaPlayer::get_current_device_id() {
    auto raw_device = libvlc_audio_output_device_get(&*this);
    if (raw_device) {
        std::string device { raw_device };
        libvlc_free(raw_device);
        return device;
    }
    else {
        return { };
    }
}

void MediaPlayer::set_audio_device(std::string device_id) {
    libvlc_audio_output_device_set(&*this, nullptr, device_id.c_str());
}

static constexpr std::array<libvlc_event_e, 17> media_player_events = {
    libvlc_MediaPlayerMediaChanged,
    libvlc_MediaPlayerOpening,
    libvlc_MediaPlayerBuffering,
    libvlc_MediaPlayerPlaying,
    libvlc_MediaPlayerPaused,
    libvlc_MediaPlayerStopped,
    libvlc_MediaPlayerForward,
    libvlc_MediaPlayerBackward,
    libvlc_MediaPlayerEndReached,
    libvlc_MediaPlayerEncounteredError,
    libvlc_MediaPlayerMuted,
    libvlc_MediaPlayerUnmuted,
    libvlc_MediaPlayerAudioVolume,
    libvlc_MediaPlayerAudioDevice,
    libvlc_MediaPlayerScrambledChanged,
    libvlc_MediaPlayerCorked,
    libvlc_MediaPlayerUncorked,
};

static auto reify_event(const libvlc_event_t *p_event) {
    switch (p_event->type) {
        case libvlc_MediaPlayerMediaChanged:     return MediaPlayer::Event::MediaChanged;
        case libvlc_MediaPlayerOpening:          return MediaPlayer::Event::Opening;
        case libvlc_MediaPlayerBuffering:        return MediaPlayer::Event::Buffering;
        case libvlc_MediaPlayerPlaying:          return MediaPlayer::Event::Playing;
        case libvlc_MediaPlayerPaused:           return MediaPlayer::Event::Paused;
        case libvlc_MediaPlayerStopped:          return MediaPlayer::Event::Stopped;
        case libvlc_MediaPlayerForward:          return MediaPlayer::Event::Forward;
        case libvlc_MediaPlayerBackward:         return MediaPlayer::Event::Backward;
        case libvlc_MediaPlayerEndReached:       return MediaPlayer::Event::EndReached;
        case libvlc_MediaPlayerEncounteredError: return MediaPlayer::Event::EncounteredError;
        case libvlc_MediaPlayerMuted:            return MediaPlayer::Event::Muted;
        case libvlc_MediaPlayerUnmuted:          return MediaPlayer::Event::Unmuted;
        case libvlc_MediaPlayerAudioVolume:      return MediaPlayer::Event::AudioVolume;
        case libvlc_MediaPlayerAudioDevice:      return MediaPlayer::Event::AudioDevice;

        case libvlc_MediaPlayerScrambledChanged: return MediaPlayer::Event::ScrambledChanged;
        case libvlc_MediaPlayerCorked:           return MediaPlayer::Event::Corked;
        case libvlc_MediaPlayerUncorked:         return MediaPlayer::Event::Uncorked;

        default:                                 return MediaPlayer::Event::Unknown;
    }
}

static auto on_event(const libvlc_event_t *p_event, void *data) {
    auto callback = reinterpret_cast<MediaPlayer::event_cb_t *>(data);

    auto reified_event = reify_event(p_event);
    (*callback)(reified_event, p_event->u.media_player_buffering.new_cache);
}

void MediaPlayer::set_event_callback(event_cb_t cb) {
    _cb = cb;
    if (auto event_manager = libvlc_media_player_event_manager(&*this); event_manager) {
        for (auto event: media_player_events)
            libvlc_event_attach(event_manager, event, &on_event, (void*)&_cb);
    }
}

Media::Media(Instance &instance, const char *location):
    CWrapper(libvlc_media_new_location(&instance, location), libvlc_media_release)
{ }

}
