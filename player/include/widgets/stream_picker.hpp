#ifndef STREAM_PICKER_HPP
#define STREAM_PICKER_HPP

#include <QWidget>

#include <unordered_map>

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
    QWidget *_channels_stream_presenter;
    QWidget *_followed_stream_presenter;

    TwitchAPI _api;
    std::unordered_map<QWidget *, TwitchAPI::streams_response_t> current_queries;

    void fetch_streams(TwitchAPI::streams_response_t, QWidget *);
    void channel_picked(QString);
};


#endif // STREAM_PICKER_HPP
