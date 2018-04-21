#include "stream_widget.hpp"

#include <QShortcut>

StreamWidget::StreamWidget(libvlc::Instance &inst, void *hwnd, QWidget *parent):
    QSplitter(parent),
    video(inst, this),
    chat(hwnd, this),
    _chat_position(ChatPosition::Right),
    _chat_size(20),
    _video_size(80)
{
    setSizes(QList<int>() << _video_size << _chat_size);
    addWidget(&video);
    addWidget(&chat);

    auto sh_toggle_chat_left = new QShortcut(QKeySequence("ctrl+left"), this);
    QObject::connect(sh_toggle_chat_left, &QShortcut::activated, [this] {
        reposition(ChatPosition::Left);
    });
    auto sh_toggle_chat_right = new QShortcut(QKeySequence("ctrl+right"), this);
    QObject::connect(sh_toggle_chat_right, &QShortcut::activated, [this] {
        reposition(ChatPosition::Right);
    });
}

void StreamWidget::reposition(ChatPosition position) {
    if (chat.isVisible()) {
        _chat_size = sizes()[indexOf(&chat)];
        _video_size = sizes()[indexOf(&video)];
    }

    switch (position) {
        case ChatPosition::Right: insertWidget(1, &chat); break;
        case ChatPosition::Left: insertWidget(0, &chat); break;
    }

    if (_chat_position != position) {
        QList<int> new_sizes;
        new_sizes.insert(indexOf(&chat), _chat_size);
        new_sizes.insert(indexOf(&video), _video_size);
        setSizes(new_sizes);
    }

    chat.setVisible(!chat.isVisible() || _chat_position != position);

    _chat_position = position;
}
