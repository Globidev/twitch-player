#include "widgets/stream_container.hpp"
#include "widgets/stream_picker.hpp"
#include "widgets/stream_widget.hpp"

#include "libvlc.hpp"

#include <QHboxLayout>

StreamContainer::StreamContainer(libvlc::Instance &video_ctx, QWidget *parent):
    QWidget(parent),
    _layout(new QHBoxLayout(this)),
    _picker(new StreamPicker(this)),
    _stream(new StreamWidget(video_ctx, this))
{
    QObject::connect(
        _picker,
        &StreamPicker::stream_picked,
        [this](QString channel) { play(channel); });

    setLayout(_layout);

    _layout->addWidget(_picker);
    _layout->setContentsMargins(QMargins());
    setContentsMargins(QMargins());
}

void StreamContainer::play(QString channel) {
    _picker->hide();
    _layout->removeWidget(_picker);
    repaint();
    _layout->addWidget(_stream);
    _stream->play(channel);
}
