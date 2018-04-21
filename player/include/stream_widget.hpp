#ifndef STREAM_WIDGET_HPP
#define STREAM_WIDGET_HPP

#include "video_widget.hpp"
#include "foreign_widget.hpp"

#include "libvlc.hpp"

#include <QSplitter>

struct StreamWidget: QSplitter {
    StreamWidget(libvlc::Instance &, void *, QWidget * = nullptr);

    VideoWidget video;
    ForeignWidget chat;

private:
    enum class ChatPosition { Left, Right };

    void reposition(ChatPosition position);

    int _chat_size, _video_size;
    ChatPosition _chat_position;
};

#endif // STREAM_WIDGET_HPP
