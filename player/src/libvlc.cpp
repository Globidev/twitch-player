#include "libvlc.hpp"
#include "constants.hpp"

// LibVLC uses ssize_t, which is POSIX...
#if defined(_MSC_VER)
  #include <BaseTsd.h>
  typedef SSIZE_T ssize_t;
#endif
#include <vlc/vlc.h>

namespace libvlc {

Instance::Instance():
    CWrapper(libvlc_new(constants::VLC_ARGC, constants::VLC_ARGV), libvlc_release)
{ }

MediaPlayer::MediaPlayer(Instance &instance):
    CWrapper(libvlc_media_player_new(&instance), libvlc_media_player_release)
{ }

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

Media::Media(Instance &instance, const char *location):
    CWrapper(libvlc_media_new_location(&instance, location), libvlc_media_release)
{ }

}
