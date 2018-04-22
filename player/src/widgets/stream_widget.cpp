#include "widgets/stream_widget.hpp"
#include "widgets/video_widget.hpp"
#include "widgets/foreign_widget.hpp"

#include "libvlc.hpp"

#include <QShortcut>

StreamWidget::StreamWidget(libvlc::Instance &inst, void *hwnd, QWidget *parent):
    QSplitter(parent),
    video(new VideoWidget(inst, this)),
    chat(new ForeignWidget(hwnd, this)),
    _chat_position(ChatPosition::Right),
    _chat_size(20),
    _video_size(80),
    _sh_toggle_chat_left(new QShortcut(QKeySequence("ctrl+left"), this)),
    _sh_toggle_chat_right(new QShortcut(QKeySequence("ctrl+right"), this))
{
    QObject::connect(_sh_toggle_chat_left, &QShortcut::activated, [this] {
        reposition(ChatPosition::Left);
    });
    QObject::connect(_sh_toggle_chat_right, &QShortcut::activated, [this] {
        reposition(ChatPosition::Right);
    });

    setSizes(QList<int>() << _video_size << _chat_size);
    addWidget(video);
    addWidget(chat);
}

void StreamWidget::reposition(ChatPosition position) {
    // Save sizes
    if (chat->isVisible()) {
        _chat_size = sizes()[indexOf(chat)];
        _video_size = sizes()[indexOf(video)];
    }

    // Potentially swap the layout
    switch (position) {
        case ChatPosition::Right: insertWidget(1, chat); break;
        case ChatPosition::Left: insertWidget(0, chat); break;
    }

    // Restore sizes
    if (_chat_position != position) {
        QList<int> new_sizes;
        new_sizes.insert(indexOf(chat), _chat_size);
        new_sizes.insert(indexOf(video), _video_size);
        setSizes(new_sizes);
    }

    chat->setVisible(!chat->isVisible() || _chat_position != position);

    _chat_position = position;
}
