#ifndef STREAM_PICKER_HPP
#define STREAM_PICKER_HPP

#include <QWidget>

#include "api/twitch.hpp"

namespace Ui {
    class StreamPicker;
}

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

    TwitchAPI _api;
    TwitchAPI::streams_response_t current_query;

    void fetch_streams(TwitchAPI::streams_response_t);
    void channel_picked(QString);
};


#endif // STREAM_PICKER_HPP
