#pragma once

#include "prelude/http.hpp"

struct ChannelData {
    QString name, display_name;
    QString title;
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
    streams_response_t followed_streams(const QString & token);
};
