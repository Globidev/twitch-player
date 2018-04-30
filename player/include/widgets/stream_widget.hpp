#ifndef STREAM_WIDGET_HPP
#define STREAM_WIDGET_HPP

#include <QWidget>

class VideoWidget;
class ForeignWidget;

namespace libvlc {
struct Instance;
}

class QSplitter;
class QHBoxLayout;

enum class ChatPosition { Left, Right };

class StreamWidget: public QWidget {
public:
    StreamWidget(libvlc::Instance &, QWidget * = nullptr);

    void play(QString, QString);

    void reposition_chat(ChatPosition);
    void resize_chat(ChatPosition);

    VideoWidget *video() const;
    ForeignWidget *chat() const;

private:
    QSplitter *_splitter;
    QHBoxLayout *_layout;
    VideoWidget *_video;
    ForeignWidget *_chat;

    int _chat_size = 50, _video_size = 50;
    ChatPosition _chat_position = ChatPosition::Right;
};

#endif // STREAM_WIDGET_HPP
