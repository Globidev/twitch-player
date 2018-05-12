#ifndef VLC_EVENT_WATCHER_HPP
#define VLC_EVENT_WATCHER_HPP

#include <QObject>

#include "libvlc.hpp"

#include "prelude/sync.hpp"

class VLCEventWatcher: public QObject {
    Q_OBJECT
public:
    VLCEventWatcher(libvlc::MediaPlayer &, QObject * = nullptr);
signals:
    void new_event(libvlc::MediaPlayer::Event, float);
private:
    sync::Queue<std::pair<libvlc::MediaPlayer::Event, float>> _queue;
};

#endif // VLC_EVENT_WATCHER_HPP
