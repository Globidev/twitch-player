#include "video_widget.hpp"

VideoWidget::VideoWidget(libvlc::Instance &instance, QWidget *parent):
    QWidget(parent), _instance(instance), _media_player(libvlc::MediaPlayer(instance))
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    _media_player.set_renderer((void *)winId());
}

void VideoWidget::play(QString channel) {
    QString location = "http://localhost:7777/play?channel=" + channel;
    _media.emplace(_instance, location.toStdString().c_str());
    _media_player.set_media(*_media);
    _media_player.play();
}

void VideoWidget::set_volume(int volume) {
    _media_player.set_volume(volume);
}
