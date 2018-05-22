#pragma once

#include <QWidget>
#include <QImage>

#include <optional>

#include "api/twitch.hpp"

namespace Ui {
class StreamDetails;
}

class VideoDetails: public QWidget {
public:
    VideoDetails(QWidget * = nullptr);
    ~VideoDetails();

    void show_state(const QString &);
    void set_buffering(bool);
    void show_stream_details();

    void set_channel(const QString &);

    void hide_stream_details();

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    QString _state_text;
    QTimer *_state_timer;
    QFont _state_text_font;
    bool _show_state_text = false;

    QImage _spinner;
    QTimer *_spinner_timer;
    int _spinner_angle = 0;
    bool _show_spinner = false;

    std::unique_ptr<QWidget> _stream_details_widget;
    std::unique_ptr<Ui::StreamDetails> _stream_details_ui;
    QString _channel;
    QTimer *_stream_details_timer;
    bool _show_stream_details = false;
    bool _has_valid_stream_details = false;

    QNetworkAccessManager *_http_client;

    void draw_state_text();
    void draw_spinner();
    void draw_stream_details();

    void fetch_channel_details();
    void fetch_channel_logo(const QString &);

    TwitchAPI _api;
};
