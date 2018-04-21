#include "video_widget.hpp"

#include <QWheelEvent>
#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QTimer>

VideoWidget::VideoWidget(libvlc::Instance &instance, QWidget *parent):
    QWidget(parent),
    _instance(instance),
    _media_player(libvlc::MediaPlayer(instance)),
    _overlay(new VolumeOverlay(this))
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::StrongFocus);
    _media_player.set_renderer((void *)winId());
    _media_player.set_volume(_vol);
    _overlay->show();
    update_overlay_position();
}

void VideoWidget::play(QString channel) {
    QString location = "http://localhost:7777/play?channel=" + channel;
    _media.emplace(_instance, location.toStdString().c_str());
    _media_player.set_media(*_media);
    _media_player.play();
}

void VideoWidget::set_volume(int volume) {
    _vol = volume;
    _media_player.set_volume(_muted ? 0 : _vol);
    _overlay->show_text(QString::number(_vol) + " %");
}

void VideoWidget::set_muted(bool muted) {
    _muted = muted;
    set_volume(_vol);
    _overlay->show_text(muted ? "Muted" : "Unmuted");
}

void VideoWidget::update_overlay_position() {
    QPoint p = mapToGlobal(pos());
    _overlay->resize(size());
    _overlay->move(p);
    _overlay->raise();
}

void VideoWidget::wheelEvent(QWheelEvent *event) {
    int new_volume;
    if (event->angleDelta().y() > 0)
        new_volume = std::min(_vol + 5, 100);
    else
        new_volume = std::max(0, _vol - 5);
    set_volume(new_volume);
    QWidget::wheelEvent(event);
}

void VideoWidget::keyPressEvent(QKeyEvent *event) {
    qDebug() << event;
    if (event->key() == Qt::Key_M) {
        set_muted(!_muted);
    }
    if (event->key() == Qt::Key_F) {
        _media_player.set_position(1.0f);
        _overlay->show_text("Fast forward...");
    }

    QWidget::keyPressEvent(event);
}

void VideoWidget::moveEvent(QMoveEvent *event) {
    update_overlay_position();
}

void VideoWidget::resizeEvent(QResizeEvent *event) {
    update_overlay_position();
}

void VideoWidget::showEvent(QShowEvent *event) {
    // have to actually wait for it to be shown...
    QTimer::singleShot(250, [this] {
        update_overlay_position();
    });
}

void VolumeOverlay::paintEvent(QPaintEvent *event) {
    if (text) {
        QFont font;
        font.setBold(true);
        font.setPointSize(40);
        font.setWeight(40);

        QPainter painter { this };

        painter.setFont(font);
        painter.setPen(Qt::white);

        painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, *text);
    }
}

void VolumeOverlay::show_text(QString new_text) {
    text.emplace(new_text);
    repaint();
    timer.start();
}
