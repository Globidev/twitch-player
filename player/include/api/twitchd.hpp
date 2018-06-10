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

struct SegmentMetadata {
    quint32 broadc_s;
    QString cmd;
    quint32 ingest_r, ingest_s;
    double stream_offset;
    quint64 transc_r, transc_s;
};

struct TwitchdAPI: APIClient {
    using stream_index_response_t = response_t<StreamIndex>;
    stream_index_response_t stream_index(QString);

    using metadata_response_t = response_t<SegmentMetadata>;
    metadata_response_t metadata(QString, QString, QString);

    using daemon_version_response_t = response_t<QString>;
    daemon_version_response_t daemon_version();

    using daemon_quit_response_t = response_t<void>;
    daemon_quit_response_t daemon_quit();

    static QString playback_url(QString, QString, QString);
};
