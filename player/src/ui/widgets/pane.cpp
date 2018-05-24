#include "ui/widgets/pane.hpp"

#include "ui/widgets/stream_picker.hpp"
#include "ui/widgets/stream_widget.hpp"
#include "ui/widgets/video_widget.hpp"
#include "ui/overlays/video_controls.hpp"
#include "ui/utils/event_notifier.hpp"

#include "libvlc/bindings.hpp"

#include "prelude/timer.hpp"

#include <QHBoxLayout>
#include <QPainter>
#include <QApplication>

constexpr auto border_width = 1;

static const auto FOCUS_INVALIDATING_EVENTS = {
    QEvent::MouseButtonRelease,
    QEvent::KeyRelease
};

Pane::Pane(libvlc::Instance &video_ctx, QWidget *parent):
    QWidget(parent),
    _video_ctx(video_ctx),
    _layout(new QHBoxLayout(this)),
    _picker(std::make_unique<StreamPicker>(this)),
    _stream(std::make_unique<StreamWidget>(video_ctx, this))
{
    auto notifier = new EventNotifier(FOCUS_INVALIDATING_EVENTS, this);
    window()->installEventFilter(notifier);

    QObject::connect(notifier, &EventNotifier::new_event, [=] {
        repaint();
    });

    setup_picker();
    setup_stream();

    _stream->hide();
    setLayout(_layout);

    _layout->addWidget(_picker.get());

    setFocusPolicy(Qt::StrongFocus);
    auto margins = QMargins(border_width, border_width, border_width, border_width);
    _layout->setContentsMargins(margins);
    setContentsMargins(margins);
    setAutoFillBackground(true);
}

Pane::~Pane() = default;

void Pane::play(QString channel, QString quality) {
    _picker->hide();

    _layout->removeWidget(_picker.get());
    _layout->addWidget(_stream.get());

    _stream->setFocus();
    _stream->show();
    _stream->play(channel, quality);

    repaint();
}

StreamWidget *Pane::stream() const {
    return _stream.get();
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
    if (auto item = _layout->itemAt(0); item)
        item->widget()->setFocus();
    repaint();
}

void Pane::setup_picker() {
    auto play_stream = [=](QString channel, QString quality) {
        play(channel, quality);
    };

    QObject::connect( _picker.get(), &StreamPicker::stream_picked, play_stream);
}

void Pane::setup_stream() {
    auto on_browse = [=] {
        // For some unknown reasons, the foreign widget doesn't get properly
        // released unless we schedule the removing for later...
        delayed(this, 250, [=] {
            _layout->removeWidget(_stream.get());

            _picker = std::make_unique<StreamPicker>(this);
            _stream = std::make_unique<StreamWidget>(_video_ctx, this);
            setup_picker();
            setup_stream();

            _layout->addWidget(_picker.get());

            _picker->show();
        });
    };

    QObject::connect(
        &_stream->video()->controls(),
        &VideoControls::browse_requested,
        on_browse
    );

    QObject::connect(
        &_stream->video()->controls(),
        &VideoControls::remove_requested,
        [=] { emit remove_requested(); }
    );
    QObject::connect(
        &_stream->video()->controls(),
        &VideoControls::zoom_requested,
        [=](bool on) {
            emit zoom_requested(on);
            _stream->video()->controls().set_zoomed(on);
        }
    );
    QObject::connect(
        &_stream->video()->controls(),
        &VideoControls::fullscreen_requested,
        [=](bool on) {
            emit fullscreen_requested(on);
            _stream->video()->controls().set_fullscreen(on);
        }
    );
}
