#pragma once

#include <QObject>

#include "libvlc/types.hpp"

#include "prelude/sync.hpp"

class VLCEventWatcher: public QObject {
    Q_OBJECT
public:
    VLCEventWatcher(libvlc::MediaPlayer &, QObject * = nullptr);
signals:
    void new_event(libvlc::Event);
private:
    sync::Queue<libvlc::Event> _queue;
};
