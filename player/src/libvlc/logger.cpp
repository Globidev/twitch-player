#include "libvlc/logger.hpp"

#include <QTimer>

VLCLogger::VLCLogger(libvlc::Instance &video_context, QObject *parent):
    QObject(parent),
    _video_context(video_context)
{
    video_context.set_log_callback([this](libvlc::LogEntry entry) {
        _queue.push(std::move(entry));
    });

    auto poll_timer = new QTimer(this);
    QObject::connect(poll_timer, &QTimer::timeout, [this] {
        auto opt_entry = _queue.try_pop();
        while (opt_entry) {
            emit newLogEntry(std::move(*opt_entry));
            opt_entry = _queue.try_pop();
        }
    });
    poll_timer->start(250);
}

VLCLogger::~VLCLogger() {
    _video_context.unset_log_callback();
}
