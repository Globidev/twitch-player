#include "libvlc/logger.hpp"

#include "libvlc/bindings.hpp"

#include "prelude/timer.hpp"

VLCLogger::VLCLogger(libvlc::Instance &video_context, QObject *parent):
    QObject(parent),
    _video_context(video_context)
{
    video_context.set_log_callback([this](libvlc::LogEntry entry) {
        _queue.push(std::move(entry));
    });

    auto poll_entries = [this] {
        auto opt_entry = _queue.try_pop();
        while (opt_entry) {
            emit new_log_entry(std::move(*opt_entry));
            opt_entry = _queue.try_pop();
        }
    };

    interval(this, 250, poll_entries);
}

VLCLogger::~VLCLogger() {
    _video_context.unset_log_callback();
}
