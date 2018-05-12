#ifndef VLC_LOGGER_HPP
#define VLC_LOGGER_HPP

#include <QObject>

#include "libvlc.hpp"

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

#endif // VLC_LOGGER_HPP
