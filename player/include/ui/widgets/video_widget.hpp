#pragma once

#include "libvlc/bindings.hpp"

#include "api/twitchd.hpp"

#include <QWidget>
#include <optional>

class VideoControls;
class VideoDetails;
class VLCEventWatcher;

class VideoWidget: public QWidget {
public:
    VideoWidget(libvlc::Instance &, QWidget * = nullptr);
    ~VideoWidget();

    void play(QString, QString);

    int volume() const;
    void set_volume(int);

    bool muted() const;
    void set_muted(bool);

    void fast_forward();

    void hint_layout_change();

    libvlc::MediaPlayer & media_player();

    VideoControls & controls() const;

protected:
    void wheelEvent(QWheelEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private:
    libvlc::Instance & _instance;
    libvlc::MediaPlayer _media_player;
    std::optional<libvlc::Media> _media;

    VideoDetails *_details;
    VideoControls *_controls;

    VLCEventWatcher *_event_watcher;

    int _vol;
    bool _muted;

    QPoint _last_drag_position;

    TwitchdAPI _api;

    QString _current_channel, _current_quality, _current_meta_key;
    std::optional<SegmentMetadata> _current_metadata;

    QTimer *_retry_timer;

    void update_overlay_position();

};
