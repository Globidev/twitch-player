#ifndef VIDEO_WIDGET_HPP
#define VIDEO_WIDGET_HPP

#include "libvlc.hpp"

#include <QWidget>

#include <optional>

class VideoOverlay: public QWidget {
public:
    VideoOverlay(QWidget * = nullptr);

    void show_text(QString);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    std::optional<QString> _text;
    QTimer *_timer;
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

    int volume() const;
    void set_volume(int);

    bool muted() const;
    void set_muted(bool);

    void fast_forward();

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

    friend class MovementFilter;
    MovementFilter *_move_filter;

    int _vol = 35;
    bool _muted = false;

    QPoint _last_drag_position;

    void update_overlay_position();
};

#endif // VIDEO_WIDGET_HPP
