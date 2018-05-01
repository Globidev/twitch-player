#ifndef STREAM_PICKER_HPP
#define STREAM_PICKER_HPP

#include <QWidget>

namespace Ui {
    class StreamPicker;
}

class QNetworkAccessManager;
class QNetworkReply;
class TwitchAPI;
class StreamPromise;

class StreamPicker: public QWidget {
    Q_OBJECT

public:
    StreamPicker(QWidget * = nullptr);
    ~StreamPicker();

protected:
    void focusInEvent(QFocusEvent *) override;

signals:
    void stream_picked(QString, QString);

private:
    Ui::StreamPicker *_ui;
    QWidget *_stream_presenter;

    TwitchAPI *_api;
    StreamPromise *current_query = nullptr;

    void fetch_streams(StreamPromise *);
};


#endif // STREAM_PICKER_HPP
