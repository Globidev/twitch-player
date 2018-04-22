#include "widgets/video_widget.hpp"

#include <QWheelEvent>
#include <QPainter>

VideoWidget::VideoWidget(libvlc::Instance &instance, QWidget *parent):
    QWidget(parent),
    _instance(instance),
    _media_player(libvlc::MediaPlayer(instance)),
    _overlay(new VideoOverlay(this)),
    _move_filter(*this)
{
    window()->installEventFilter(&_move_filter);
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
    _overlay->resize(size());
    _overlay->move(mapToGlobal(pos()) - pos());
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
    // have to actually wait for it to be shown ...
    QTimer::singleShot(250, [this] {
        update_overlay_position();
    });
}

MovementFilter::MovementFilter(VideoWidget & video_widget):
    _video_widget(video_widget)
{ }

bool MovementFilter::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Type::Move)
        _video_widget.update_overlay_position();
    return QObject::eventFilter(watched, event);
}

VideoOverlay::VideoOverlay(QWidget *parent):
    QWidget(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    _text_font.setBold(true);
    _text_font.setPointSize(40);
    _text_font.setWeight(40);

    _timer.setInterval(2500);
    QObject::connect(&_timer, &QTimer::timeout, [this] {
        _text.reset();
        repaint();
    });
}

void VideoOverlay::paintEvent(QPaintEvent *event) {
    if (_text) {
        QPainter painter { this };

        painter.setFont(_text_font);
        painter.setPen(Qt::white);

        painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, *_text);
    }
}

void VideoOverlay::show_text(QString new_text) {
    _text.emplace(new_text);
    _timer.start();
    repaint();
}
