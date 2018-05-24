#pragma once

#include <QWidget>

class QHBoxLayout;
class StreamPicker;
class StreamWidget;

namespace libvlc {
    struct Instance;
}

class Pane: public QWidget {
    Q_OBJECT

public:
    Pane(libvlc::Instance &, QWidget * = nullptr);
    ~Pane();

    void play(QString, QString = QString());

    StreamWidget *stream() const;

protected:
    void paintEvent(QPaintEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void focusInEvent(QFocusEvent *) override;

signals:
    void remove_requested();
    void zoom_requested(bool);
    void fullscreen_requested(bool);

private:
    libvlc::Instance & _video_ctx;

    QHBoxLayout *_layout;
    std::unique_ptr<StreamPicker> _picker;
    std::unique_ptr<StreamWidget> _stream;

    void setup_picker();
    void setup_stream();
};
