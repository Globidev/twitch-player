#include "libvlc/event_watcher.hpp"

#include "libvlc/bindings.hpp"

#include "prelude/timer.hpp"

VLCEventWatcher::VLCEventWatcher(libvlc::MediaPlayer & mp, QObject *parent):
    QObject(parent)
{
    mp.set_event_callback([this](auto event) {
        _queue.push(std::move(event));
    });

    auto poll_events = [this] {
        auto opt_event = _queue.try_pop();
        while (opt_event) {
            emit new_event(std::move(*opt_event));
            opt_event = _queue.try_pop();
        }
    };

    interval(this, 250, poll_events);
}
