#pragma once

#include "prelude/http.hpp"

struct ChannelData {
    QString name, display_name;
    QString title;
    QString logo_url;
    uint32_t id;
};

struct StreamData {
    ChannelData channel;
    QString preview;
    QString current_game;
    uint32_t viewcount;
};

struct TwitchAPI: APIClient {
    using streams_response_t = response_t<QList<StreamData>>;

    streams_response_t stream_search(QString);
    streams_response_t top_streams();
    streams_response_t followed_streams(const QString &);

    using stream_response_t = response_t<StreamData>;
    stream_response_t stream(uint32_t);

    using channels_response_t = response_t<QList<ChannelData>>;
    channels_response_t channel_search(const QString &);
};
