#ifndef VIDEO_WIDGET_HPP
#define VIDEO_WIDGET_HPP

#include "libvlc.hpp"

#include "api/twitchd.hpp"

#include <QWidget>
#include <QImage>

#include <optional>

class VideoOverlay: public QWidget {
public:
    VideoOverlay(QWidget * = nullptr);

    void show_text(QString);
    void set_buffering(bool);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    std::optional<QString> _text;
    QTimer *_timer, *_spinner_timer;
    QFont _text_font;
    QImage _spinner;
    bool _show_spinner = false;
    int _spinner_angle = 0;
};

class VideoWidget;
class VideoControls;
class VLCEventWatcher;

class MovementFilter: public QObject {
public:
    MovementFilter(VideoWidget &);
protected:
    bool eventFilter(QObject *, QEvent *) override;
private:
    VideoWidget &_video_widget;
};

class VideoWidget: public QWidget {
public:
    VideoWidget(libvlc::Instance &, QWidget * = nullptr);

    void play(QString, QString);

    int volume() const;
    void set_volume(int);

    bool muted() const;
    void set_muted(bool);

    void fast_forward();

    libvlc::MediaPlayer & media_player();

protected:
    void wheelEvent(QWheelEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;

private:
    libvlc::Instance & _instance;
    libvlc::MediaPlayer _media_player;
    std::optional<libvlc::Media> _media;

    VideoOverlay *_overlay;
    VideoControls *_controls;

    friend class MovementFilter;
    MovementFilter *_move_filter;

    VLCEventWatcher *_event_watcher;

    int _vol = 35;
    bool _muted = false;

    QPoint _last_drag_position;

    TwitchdAPI _api;
    TwitchdAPI::stream_index_response_t stream_index_reponse;

    QString _current_channel, _current_quality;

    void update_overlay_position();
};

#endif // VIDEO_WIDGET_HPP
