#ifndef STREAM_CONTAINER_HPP
#define STREAM_CONTAINER_HPP

#include <QWidget>

class QHBoxLayout;
class StreamPicker;
class StreamWidget;

namespace libvlc {
    struct Instance;
}

class StreamContainer: public QWidget {
public:
    StreamContainer(libvlc::Instance &, QWidget * = nullptr);

    void play(QString);

    StreamWidget *stream() const;

protected:
    void paintEvent(QPaintEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void focusInEvent(QFocusEvent *) override;

private:
    QHBoxLayout *_layout;
    StreamPicker *_picker;
    StreamWidget *_stream;
};

#endif // STREAM_CONTAINER_HPP
