#ifndef STREAM_PICKER_HPP
#define STREAM_PICKER_HPP

#include <QWidget>

namespace Ui {
    class StreamPicker;
}

class StreamPicker: public QWidget {
    Q_OBJECT

public:
    StreamPicker(QWidget * = nullptr);
    ~StreamPicker();

signals:
    void stream_picked(QString);

private:
    Ui::StreamPicker *_ui;
};


#endif // STREAM_PICKER_HPP
