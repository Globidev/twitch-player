#pragma once

#include "prelude/http.hpp"

struct Resolution {
    uint32_t width, height;
};

struct StreamInfo {
    Resolution resolution;
    uint64_t bandwidth;
};

struct MediaInfo {
    QString name;
    QString group_id;
};

struct PlaylistInfo {
    StreamInfo stream_info;
    MediaInfo media_info;
    QString url;
};

struct StreamIndex {
    QList<PlaylistInfo> playlist_infos;
};

struct TwitchdAPI: APIClient {
    using stream_index_response_t = response_t<StreamIndex>;

    stream_index_response_t stream_index(QString);
    static QString playback_url(QString, QString);
};
