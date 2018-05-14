#include "libvlc/event_watcher.hpp"

#include "libvlc/bindings.hpp"

#include <QTimer>

VLCEventWatcher::VLCEventWatcher(libvlc::MediaPlayer & mp, QObject *parent):
    QObject(parent)
{
    mp.set_event_callback([this](auto event) {
        _queue.push(std::move(event));
    });

    auto poll_timer = new QTimer(this);
    QObject::connect(poll_timer, &QTimer::timeout, [this] {
        while (!_queue.empty()) {
            auto event = std::move(_queue.pop());
            emit new_event(std::move(event));
        }
    });
    poll_timer->start(250);
}
