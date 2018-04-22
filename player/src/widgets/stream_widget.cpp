#include "widgets/stream_widget.hpp"
#include "widgets/video_widget.hpp"
#include "widgets/foreign_widget.hpp"

#include "libvlc.hpp"

#include <QKeyEvent>
#include <QSplitter>

StreamWidget::StreamWidget(libvlc::Instance &inst, void *hwnd, QWidget *parent):
    QWidget(parent),
    video(new VideoWidget(inst, this)),
    chat(new ForeignWidget(hwnd, this)),
    _splitter(new QSplitter(this)),
    _layout(new QHBoxLayout(this))
{
    setLayout(_layout);

    _layout->setContentsMargins(QMargins());
    _layout->addWidget(_splitter);

    _splitter->addWidget(video);
    _splitter->addWidget(chat);
    _splitter->setSizes(QList<int>() << _video_size << _chat_size);
}

void StreamWidget::keyPressEvent(QKeyEvent * event) {
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
        switch (event->key()) {
            case Qt::Key_Left:
                reposition(ChatPosition::Left);
                break;
            case Qt::Key_Right:
                reposition(ChatPosition::Right);
                break;
        }
    }
    QWidget::keyPressEvent(event);
}

void StreamWidget::reposition(ChatPosition position) {
    // Save sizes
    if (chat->isVisible()) {
        _chat_size = _splitter->sizes()[_splitter->indexOf(chat)];
        _video_size = _splitter->sizes()[_splitter->indexOf(video)];
    }

    // Potentially swap the layout
    switch (position) {
        case ChatPosition::Right: _splitter->insertWidget(1, chat); break;
        case ChatPosition::Left: _splitter->insertWidget(0, chat); break;
    }

    // Restore sizes
    if (_chat_position != position) {
        QList<int> new_sizes;
        new_sizes.insert(_splitter->indexOf(chat), _chat_size);
        new_sizes.insert(_splitter->indexOf(video), _video_size);
        _splitter->setSizes(new_sizes);
    }

    chat->setVisible(!chat->isVisible() || _chat_position != position);

    _chat_position = position;
}
