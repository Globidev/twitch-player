#pragma once

#include <QWidget>

#include <unordered_map>

#include "api/twitch.hpp"

#include "prelude/promise.hpp"

namespace Ui {
    class StreamPicker;
}

class StreamPicker: public QWidget {
    Q_OBJECT

public:
    StreamPicker(QWidget * = nullptr);

protected:
    void focusInEvent(QFocusEvent *) override;

signals:
    void stream_picked(QString, QString);

private:
    std::unique_ptr<Ui::StreamPicker> _ui;

    QWidget *_channels_stream_presenter;
    QWidget *_followed_stream_presenter;

    TwitchAPI _api;
    std::unordered_map<QWidget *, CancelToken> current_queries;

    void fetch_streams(TwitchAPI::streams_response_t, QWidget *);
    void present_streams(QWidget *, QList<StreamData>);

    void channel_picked(QString);
};
