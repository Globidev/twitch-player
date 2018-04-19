#ifndef VIDEO_WIDGET_HPP
#define VIDEO_WIDGET_HPP

#include "libvlc.hpp"

#include <optional>
#include <QWidget>

class VideoWidget: public QWidget
{
public:
    VideoWidget(libvlc::Instance &, QWidget * = nullptr);

    void play(QString);
    void set_volume(int volume);

private:
    libvlc::Instance & _instance;
    libvlc::MediaPlayer _media_player;
    std::optional<libvlc::Media> _media;
};

#endif // VIDEO_WIDGET_HPP

//QWidget *overlay;
//overlay = new W(&w);
//overlay->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
//overlay->setAttribute(Qt::WA_NoSystemBackground);
//overlay->setAttribute(Qt::WA_TranslucentBackground);
//overlay->setAttribute(Qt::WA_PaintOnScreen);
//overlay->setWindowOpacity(1);
//overlay->show();
