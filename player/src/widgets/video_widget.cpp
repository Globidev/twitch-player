#include "widgets/video_widget.hpp"

#include "api/twitchd.hpp"

#include <QApplication>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QShortcut>
#include <QTimer>

template <class Slot>
static void delayed(VideoWidget *parent, int ms, Slot slot) {
    auto timer = new QTimer(parent);
    QObject::connect(timer, &QTimer::timeout, slot);
    timer->setSingleShot(true);
    timer->start(ms);
}

VideoWidget::VideoWidget(libvlc::Instance &instance, QWidget *parent):
    QWidget(parent),
    _instance(instance),
    _media_player(libvlc::MediaPlayer(instance)),
    _overlay(new VideoOverlay(this)),
    _move_filter(new MovementFilter(*this))
{
    window()->installEventFilter(_move_filter);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::WheelFocus);
    _media_player.set_renderer((void *)winId());
    _media_player.set_volume(_vol);
    _overlay->show();
    update_overlay_position();
    setMinimumSize(160, 90);
}

void VideoWidget::play(QString channel, QString quality) {
    auto location = TwitchdAPI::playback_url(channel, quality);
    _media.emplace(_instance, location.toStdString().c_str());
    _media_player.set_media(*_media);
    _media_player.play();
}

int VideoWidget::volume() const {
    return _vol;
}

void VideoWidget::set_volume(int volume) {
    _vol = volume;
    _media_player.set_volume(_muted ? 0 : _vol);
    _overlay->show_text(QString::number(_vol) + " %");
}

bool VideoWidget::muted() const {
    return _muted;
}

void VideoWidget::set_muted(bool muted) {
    _muted = muted;
    set_volume(_vol);
    _overlay->show_text(muted ? "Muted" : "Unmuted");
}

void VideoWidget::fast_forward() {
    _media_player.set_position(1.0f);
    _overlay->show_text("Fast forward...");
}

void VideoWidget::update_overlay_position() {
    _overlay->resize(size());
    _overlay->move(mapToGlobal(pos()) - pos());
}

void VideoWidget::wheelEvent(QWheelEvent *event) {
    setFocus();
    auto delta = qApp->keyboardModifiers().testFlag(Qt::ShiftModifier)
               ? 1 : 5;
    int new_volume;
    if (event->angleDelta().y() > 0)
        new_volume = std::min(_vol + delta, 100);
    else
        new_volume = std::max(0, _vol - delta);
    set_volume(new_volume);
    QWidget::wheelEvent(event);
}

void VideoWidget::resizeEvent(QResizeEvent *event) {
    update_overlay_position();
}

void VideoWidget::showEvent(QShowEvent *event) {
    // have to actually wait for it to be shown ...
    delayed(this, 250, [this] { update_overlay_position(); });
}

void VideoWidget::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::MouseButton::LeftButton))
        _last_drag_position = event->globalPos();

    QWidget::mousePressEvent(event);
}

void VideoWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::MouseButton::LeftButton)) {
        auto delta = event->globalPos() - _last_drag_position;
        if (delta.manhattanLength() > 10) {
            window()->move(window()->pos() + delta);
            _last_drag_position = event->globalPos();
        }
    }
    else
        QWidget::mouseMoveEvent(event);
}

void VideoWidget::keyPressEvent(QKeyEvent *event) {
    QWidget::keyPressEvent(event);
    // keypresses might trigger parent's shortcuts which might modify the
    // overlay. It's hard to detect without adding strong coupling with the
    // parent so for now let's just schedule an overlay update for the next
    // event loop iteration
    delayed(this, 0, [this] { update_overlay_position(); });
}

MovementFilter::MovementFilter(VideoWidget & video_widget):
    QObject(&video_widget),
    _video_widget(video_widget)
{ }

bool MovementFilter::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Type::Move)
        _video_widget.update_overlay_position();
    return QObject::eventFilter(watched, event);
}

VideoOverlay::VideoOverlay(QWidget *parent):
    QWidget(parent),
    _timer(new QTimer(this))
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    _text_font.setBold(true);
    _text_font.setPointSize(40);
    _text_font.setWeight(40);

    _timer->setInterval(2500);
    QObject::connect(_timer, &QTimer::timeout, [this] {
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
    _timer->start();
    repaint();
}
