#include "ui/widgets/pane.hpp"

#include "ui/widgets/stream_picker.hpp"
#include "ui/widgets/stream_widget.hpp"

#include "libvlc/bindings.hpp"

#include <QHboxLayout>
#include <QPainter>
#include <QApplication>

constexpr auto border_width = 1;

Pane::Pane(libvlc::Instance &video_ctx, QWidget *parent):
    QWidget(parent),
    _layout(new QHBoxLayout(this)),
    _picker(new StreamPicker(this)),
    _stream(new StreamWidget(video_ctx, this))
{
    QObject::connect(
        _picker,
        &StreamPicker::stream_picked,
        [this](QString channel, QString quality) { play(channel, quality); }
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

void Pane::play(QString channel, QString quality) {
    _picker->hide();

    _layout->removeWidget(_picker);
    _layout->addWidget(_stream);

    _stream->setFocus();
    _stream->show();
    _stream->play(channel, quality);

    repaint();
}

StreamWidget *Pane::stream() const {
    return _stream;
}

void Pane::paintEvent(QPaintEvent *event) {
    if (isAncestorOf(qApp->focusWidget())) {
        QPainter painter(this);

        painter.setPen({
            QColor(0x39, 0x2e, 0x5c),
            static_cast<qreal>(border_width),
            Qt::PenStyle::DotLine
        });
        painter.drawRect(
            border_width, border_width,
            width() - border_width * 2,
            height() - border_width * 2
        );
    }

    QWidget::paintEvent(event);
}

void Pane::focusOutEvent(QFocusEvent *event) {
    QWidget::focusOutEvent(event);
    repaint();
}

void Pane::focusInEvent(QFocusEvent *event) {
    QWidget::focusInEvent(event);
    _layout->itemAt(0)->widget()->setFocus();
    repaint();
}
