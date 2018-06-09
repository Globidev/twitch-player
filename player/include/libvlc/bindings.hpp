#pragma once

#include "libvlc/types.hpp"

#include "prelude/c_wrapper.hpp"

#include <functional>
#include <vector>
#include <string>

namespace libvlc {

struct Instance: CWrapper<libvlc_instance_t> {
    Instance(std::vector<std::string>);

    using log_cb_t = std::function<void (LogEntry)>;

    void set_log_callback(log_cb_t);
    void unset_log_callback();

private:
    log_cb_t _cb;
};

struct Media: CWrapper<libvlc_media_t> {
    Media(Instance &, const char *);
};

struct Equalizer: CWrapper<libvlc_equalizer_t> {
    Equalizer();
};

struct MediaPlayer: CWrapper<libvlc_media_player_t> {
    MediaPlayer(Instance &);

    void set_media(Media &);
    void set_renderer(void *);
    void play();
    void stop();
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

    using event_cb_t = std::function<void (Event)>;

    void set_event_callback(event_cb_t);

private:
    Equalizer _equalizer;
    event_cb_t _cb;

    // For some reason, `libvlc_video_get_adjust` does not work for
    // libvlc_adjust_Enable.
    // Keeping track of the enabling manually...
    bool _adjust_enabled = false;
};

}
