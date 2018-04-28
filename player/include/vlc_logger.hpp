#ifndef VLC_LOGGER_HPP
#define VLC_LOGGER_HPP

#include "libvlc.hpp"

#include <QObject>

#include <optional>
#include <queue>
#include <mutex>

struct LogEntryQueue {
    void push_work(libvlc::LogEntry item);

    std::optional<libvlc::LogEntry> try_pop();

private:
    using lock_t = std::unique_lock<std::mutex>;

    std::mutex _work_mutex;
    std::queue<libvlc::LogEntry> _work;
};

class VLCLogger: public QObject {
    Q_OBJECT

public:
    VLCLogger(libvlc::Instance &, QObject * = nullptr);
    ~VLCLogger();

signals:
    void newLogEntry(libvlc::LogEntry);

private:
    libvlc::Instance &_video_context;
    LogEntryQueue _queue;
};

#endif // VLC_LOGGER_HPP
