#ifndef STREAM_WIDGET_HPP
#define STREAM_WIDGET_HPP

#include <QSplitter>

class VideoWidget;
class ForeignWidget;

namespace libvlc {
struct Instance;
}

class StreamWidget: public QSplitter {
public:
    StreamWidget(libvlc::Instance &, void *, QWidget * = nullptr);

    VideoWidget *video;
    ForeignWidget *chat;

private:
    enum class ChatPosition { Left, Right };

    void reposition(ChatPosition);

    int _chat_size, _video_size;
    ChatPosition _chat_position;

    QShortcut *_sh_toggle_chat_left;
    QShortcut *_sh_toggle_chat_right;
};

#endif // STREAM_WIDGET_HPP
