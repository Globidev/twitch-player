#ifndef LIBVLC_HPP
#define LIBVLC_HPP

#include <memory>
#include <functional>
#include <vector>
#include <string>

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;
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

struct MediaPlayer: CWrapper<libvlc_media_player_t> {
    MediaPlayer(Instance &);

    void set_media(Media &);
    void set_renderer(void *);
    void play();
    void set_volume(int);
    void set_position(float);
};

struct Media: CWrapper<libvlc_media_t> {
    Media(Instance &, const char *);
};

}

#endif // LIBVLC_HPP
