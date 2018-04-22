#ifndef VIDEO_WIDGET_HPP
#define VIDEO_WIDGET_HPP

#include "libvlc.hpp"

#include <optional>
#include <QWidget>
#include <QTimer>
#include <QFont>

class VideoOverlay: public QWidget {
public:
    VideoOverlay(QWidget * = nullptr);

    void show_text(QString);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    std::optional<QString> _text;
    QTimer _timer;
    QFont _text_font;
};

class VideoWidget;

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

    void play(QString);
    void set_volume(int);
    void set_muted(bool);

protected:
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void showEvent(QShowEvent *) override;

private:
    libvlc::Instance & _instance;
    libvlc::MediaPlayer _media_player;
    std::optional<libvlc::Media> _media;
    VideoOverlay *_overlay;
    friend class MovementFilter;
    MovementFilter _move_filter;

    int _vol = 35;
    bool _muted = false;

    void update_overlay_position();
};

#endif // VIDEO_WIDGET_HPP
