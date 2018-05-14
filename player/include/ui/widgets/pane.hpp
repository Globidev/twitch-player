#pragma once

#include <QWidget>

class QHBoxLayout;
class StreamPicker;
class StreamWidget;

namespace libvlc {
    struct Instance;
}

class Pane: public QWidget {
public:
    Pane(libvlc::Instance &, QWidget * = nullptr);

    void play(QString, QString = QString());

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
