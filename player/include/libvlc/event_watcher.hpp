#pragma once

#include <QObject>

#include "libvlc/bindings.hpp"

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
