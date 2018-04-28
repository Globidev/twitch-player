#include "vlc_logger.hpp"

#include <QTimer>

void LogEntryQueue::push_work(libvlc::LogEntry item) {
    lock_t lock { _work_mutex };

    _work.push(item);
}

std::optional<libvlc::LogEntry> LogEntryQueue::try_pop() {
    lock_t lock { _work_mutex };

    if (_work.empty())
        return std::nullopt;

    libvlc::LogEntry item = _work.front();
    _work.pop();

    // MSVC bug: cannot return from `item` or `{ item }`
    return std::optional<libvlc::LogEntry>{ item };
}

VLCLogger::VLCLogger(libvlc::Instance &video_context, QObject *parent):
    QObject(parent),
    _video_context(video_context)
{
    video_context.set_log_callback([this](libvlc::LogEntry entry) {
        _queue.push_work(entry);
    });

    auto poll_timer = new QTimer(this);
    QObject::connect(poll_timer, &QTimer::timeout, [this] {
        auto entry = _queue.try_pop();
        while (entry) {
            emit newLogEntry(*entry);
            // MSVC bug: cannot assign to an optional value for some reason
            auto next_entry = _queue.try_pop();
            entry.swap(next_entry);
        }
    });
    poll_timer->start(250);
}

VLCLogger::~VLCLogger() {
    _video_context.unset_log_callback();
}
