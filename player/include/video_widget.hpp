#ifndef VIDEO_WIDGET_HPP
#define VIDEO_WIDGET_HPP

#include "libvlc.hpp"

#include <optional>
#include <QWidget>
#include <QTimer>

class VolumeOverlay: public QWidget
{
public:
    VolumeOverlay(QWidget *parent = nullptr):
        QWidget(parent)
    {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_PaintOnScreen);

        setAttribute(Qt::WA_TransparentForMouseEvents);

        timer.setInterval(2500);
        QObject::connect(&timer, &QTimer::timeout, [this] {
            text.reset();
            repaint();
        });
    }

    void show_volume(int);
    void show_muted(bool);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    std::optional<QString> text;
    QTimer timer;
};

class VideoWidget: public QWidget
{
public:
    VideoWidget(libvlc::Instance &, QWidget * = nullptr);

    void play(QString);
    void set_volume(int);
    void set_muted(bool);

    void update_overlay_position();

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
    VolumeOverlay *_overlay;

    int _vol = 35;
    bool _muted = false;
};

#endif // VIDEO_WIDGET_HPP
