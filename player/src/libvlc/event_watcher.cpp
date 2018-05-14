#include "libvlc/event_watcher.hpp"

#include <QTimer>

VLCEventWatcher::VLCEventWatcher(libvlc::MediaPlayer & mp, QObject *parent):
    QObject(parent)
{
    mp.set_event_callback([this](auto event, float v) {
        _queue.push({event, v});
    });

    auto poll_timer = new QTimer(this);
    QObject::connect(poll_timer, &QTimer::timeout, [this] {
        while (!_queue.empty()) {
            auto event = std::move(_queue.pop());
            emit new_event(event.first, event.second);
        }
    });
    poll_timer->start(250);
}
