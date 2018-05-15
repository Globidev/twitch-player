#pragma once

#include <QObject>

#include "libvlc/types.hpp"

#include "prelude/sync.hpp"

class VLCLogger: public QObject {
    Q_OBJECT

public:
    VLCLogger(libvlc::Instance &, QObject * = nullptr);
    ~VLCLogger();

signals:
    void new_log_entry(libvlc::LogEntry);

private:
    libvlc::Instance &_video_context;

    using LogEntryQueue = sync::Queue<libvlc::LogEntry>;
    LogEntryQueue _queue;
};
