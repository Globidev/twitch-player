#ifndef LIBVLC_HPP
#define LIBVLC_HPP

#include <memory>

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_media_t;

namespace libvlc {

template <class T>
using CDeleter = void (*)(T*);

template <class T>
using CResource = std::unique_ptr<T, CDeleter<T>>;

template <class T>
struct CWrapper {
    CWrapper(T* resource_ptr, CDeleter<T> deleter):
        resource(resource_ptr, deleter) { }

    auto operator &() const { return resource.get(); }

private:
    using resource_t = CResource<T>;

    resource_t resource;
};

struct Instance;
struct MediaPlayer;
struct Media;

struct Instance: CWrapper<libvlc_instance_t> {
    Instance();
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
