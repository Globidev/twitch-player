#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;
struct libvlc_equalizer_t;
struct libvlc_event_manager_t;
struct vlc_log_t;

namespace libvlc {

template <class T>
using CDeleter = void (*)(T*);

template <class T>
using CResource = std::unique_ptr<T, CDeleter<T>>;

template <class T>
struct CWrapper {
    CWrapper(T* resource_ptr, CDeleter<T> deleter):
        resource(resource_ptr, deleter)
    { }

    auto operator &()   const { return resource.get(); }
    bool init_success() const { return resource.get() != nullptr; }

private:
    using resource_t = CResource<T>;

    resource_t resource;
};

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

struct Instance;
struct MediaPlayer;
struct Media;

struct Instance: CWrapper<libvlc_instance_t> {
    Instance(std::vector<std::string>);

    using log_cb_t = std::function<void (LogEntry)>;

    void set_log_callback(log_cb_t);
    void unset_log_callback();

private:
    log_cb_t _cb;
};

struct Equalizer: CWrapper<libvlc_equalizer_t> {
    Equalizer();
};

struct MediaPlayer: CWrapper<libvlc_media_player_t> {
    MediaPlayer(Instance &);

    void set_media(Media &);
    void set_renderer(void *);
    void play();
    void set_volume(int);
    void set_position(float);

    bool video_filters_enabled();
    void enable_video_filters(bool);

    float get_contrast();
    void set_contrast(float);
    float get_brightness();
    void set_brightness(float);
    float get_hue();
    void set_hue(float);
    float get_saturation();
    void set_saturation(float);
    float get_gamma();
    void set_gamma(float);

    std::vector<AudioDevice> audio_devices();
    std::string get_current_device_id();
    void set_audio_device(std::string);

    enum class Event {
        MediaChanged,
        Opening,
        Buffering,
        Playing,
        Paused,
        Stopped,
        Forward,
        Backward,
        EndReached,
        EncounteredError,
        Muted,
        Unmuted,
        AudioVolume,
        AudioDevice,

        ScrambledChanged,
        Corked,
        Uncorked,

        Unknown,
    };

    using event_cb_t = std::function<void (Event, float)>;

    void set_event_callback(event_cb_t);

private:
    Equalizer _equalizer;
    event_cb_t _cb;
};

struct Media: CWrapper<libvlc_media_t> {
    Media(Instance &, const char *);
};

}
