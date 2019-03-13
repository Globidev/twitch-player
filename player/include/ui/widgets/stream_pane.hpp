#pragma once

#include <QWidget>

#include <memory>

class QHBoxLayout;
class StreamPicker;
class StreamWidget;

namespace libvlc {
    struct Instance;
}

class StreamPane: public QWidget {
    Q_OBJECT

public:
    StreamPane(libvlc::Instance &, QWidget * = nullptr);
    ~StreamPane();

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
