#pragma once

#include <QObject>

#include "libvlc/bindings.hpp"

#include "prelude/sync.hpp"

class VLCLogger: public QObject {
    Q_OBJECT

public:
    VLCLogger(libvlc::Instance &, QObject * = nullptr);
    ~VLCLogger();

signals:
    void newLogEntry(libvlc::LogEntry);

private:
    libvlc::Instance &_video_context;

    sync::Queue<libvlc::LogEntry> _queue;
};
