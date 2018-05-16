#include "ui/overlays/video_details.hpp"

#include <QTimer>
#include <QMouseEvent>
#include <QPainter>

VideoDetails::VideoDetails(QWidget *parent):
    QWidget(parent),
    _state_timer(new QTimer(this)),
    _spinner_timer(new QTimer(this)),
    _spinner(":/images/kappa.png")
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    _state_text_font.setBold(true);
    _state_text_font.setPointSize(40);
    _state_text_font.setWeight(40);

    _state_timer->setInterval(2500);
    QObject::connect(_state_timer, &QTimer::timeout, [this] {
        _show_state_text = false;
        repaint();
    });

    _spinner_timer->setInterval(20);
    QObject::connect(_spinner_timer, &QTimer::timeout, [this] {
        _spinner_angle = (_spinner_angle + 6) % 360;
        repaint();
    });
}

void VideoDetails::mouseReleaseEvent(QMouseEvent *event) {
    event->ignore();
    parentWidget()->activateWindow();
};

void VideoDetails::paintEvent(QPaintEvent *event) {
    if (_show_state_text)
        draw_state_text();

    if (_show_spinner)
        draw_spinner();

    QWidget::paintEvent(event);
}

void VideoDetails::draw_state_text() {
    QPainter painter { this };

    painter.setFont(_state_text_font);
    painter.setPen(Qt::white);

    QRect br;
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, _state_text, &br);

    QLinearGradient gradient(br.topLeft(), br.bottomLeft());
    gradient.setColorAt(0, QColor(0, 0, 0, 0xB0));
    gradient.setColorAt(1, QColor(0, 0, 0, 0x00));

    painter.fillRect(br, QBrush(gradient));
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, _state_text);
}

void VideoDetails::draw_spinner() {
    QPainter painter { this };

    auto centered_rect = _spinner.rect();
    auto aspect_ratio = (float)centered_rect.height() / centered_rect.width();
    if (rect().width() < centered_rect.width()) {
        auto new_width = rect().width();
        centered_rect.setWidth(new_width);
        centered_rect.setHeight(new_width * aspect_ratio);
    }
    if (rect().height() < centered_rect.height()) {
        auto new_height = rect().height();
        centered_rect.setHeight(new_height);
        centered_rect.setWidth(new_height / aspect_ratio);
    }
    centered_rect.moveCenter(rect().center());
    painter.translate(rect().center());
    painter.rotate(_spinner_angle);
    painter.drawImage(
        centered_rect.translated(-rect().center()),
        _spinner
    );
}

void VideoDetails::show_state(const QString &state) {
    _state_text = state;
    _state_timer->start();
    _show_state_text = true;
    repaint();
}

void VideoDetails::set_buffering(bool enabled) {
    if (enabled == _show_spinner)
        return ;
    _show_spinner = enabled;
    _spinner_angle = 0;
    if (enabled) _spinner_timer->start();
    else         _spinner_timer->stop();
    repaint();
}

void VideoDetails::show_stream_info() {

}
