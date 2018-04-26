#include "widgets/stream_container.hpp"
#include "widgets/stream_picker.hpp"
#include "widgets/stream_widget.hpp"

#include "libvlc.hpp"

#include <QHboxLayout>
#include <QPainter>
#include <QApplication>

constexpr auto border_width = 1;

StreamContainer::StreamContainer(libvlc::Instance &video_ctx, QWidget *parent):
    QWidget(parent),
    _layout(new QHBoxLayout(this)),
    _picker(new StreamPicker(this)),
    _stream(new StreamWidget(video_ctx, this))
{
    QObject::connect(
        _picker,
        &StreamPicker::stream_picked,
        [this](QString channel) { play(channel); }
    );

    _stream->hide();
    setLayout(_layout);

    _layout->addWidget(_picker);

    setFocusPolicy(Qt::StrongFocus);
    auto margins = QMargins(border_width, border_width, border_width, border_width);
    _layout->setContentsMargins(margins);
    setContentsMargins(margins);
    setAutoFillBackground(true);
}

void StreamContainer::play(QString channel) {
    _picker->hide();
    _layout->removeWidget(_picker);

    repaint();

    _layout->addWidget(_stream);
    _stream->show();
    _stream->play(channel);
}

void StreamContainer::paintEvent(QPaintEvent *event) {
    if (isAncestorOf(qApp->focusWidget())) {
        QPainter painter(this);

        painter.setPen(QPen(Qt::cyan));
        painter.drawRect(
            border_width, border_width,
            width() - border_width * 2,
            height() - border_width * 2
        );
    }

    QWidget::paintEvent(event);
}

void StreamContainer::focusOutEvent(QFocusEvent *) {
    repaint();
}

void StreamContainer::focusInEvent(QFocusEvent *) {
    repaint();
}
