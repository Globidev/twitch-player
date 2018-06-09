#include "api/twitchd.hpp"

#include <QUrlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static StreamIndex parse_stream_index_data(const QByteArray &raw) {
    auto json_data = QJsonDocument::fromJson(raw).object();

    QList<PlaylistInfo> playlist_infos;
    for (auto raw_pl_info: json_data["playlist_infos"].toArray()) {
        auto pl_info_obj = raw_pl_info.toObject();
        auto media_info_obj = pl_info_obj["media_info"].toObject();
        auto stream_info_obj = pl_info_obj["stream_info"].toObject();
        auto resolution_obj = stream_info_obj["resolution"].toObject();

        auto resolution = Resolution {
            static_cast<uint32_t>(resolution_obj["width"].toInt()),
            static_cast<uint32_t>(resolution_obj["height"].toInt()),
        };

        auto stream_info = StreamInfo {
            resolution,
            static_cast<uint64_t>(stream_info_obj["bandwidth"].toInt())
        };

        auto media_info = MediaInfo {
            media_info_obj["name"].toString(),
            media_info_obj["group_id"].toString()
        };

        auto url = pl_info_obj["url"].toString();

        auto playlist_info = PlaylistInfo {
            stream_info,
            media_info,
            url
        };

        playlist_infos << playlist_info;
    }

    return StreamIndex { playlist_infos };
}

static SegmentMetadata parse_metadata(const QByteArray &raw) {
    auto json_data = QJsonDocument::fromJson(raw).object();

    return SegmentMetadata {
        static_cast<quint32>(json_data["broadc_s"].toInt()),
        json_data["cmd"].toString(),
        static_cast<quint32>(json_data["ingest_r"].toInt()),
        static_cast<quint32>(json_data["ingest_r"].toInt()),
        json_data["stream_offset"].toDouble(),
        static_cast<quint64>(json_data["transc_r"].toDouble()),
        static_cast<quint64>(json_data["transc_r"].toDouble()),
    };
}

TwitchdAPI::stream_index_response_t TwitchdAPI::stream_index(QString channel) {
    QUrl url { "http://127.0.0.1:7777/stream_index" };

    QUrlQuery url_query;
    url_query.addQueryItem("channel", channel);
    url.setQuery(url_query);

    QNetworkRequest request { url };

    return get(request).then(&parse_stream_index_data);
}

TwitchdAPI::metadata_response_t TwitchdAPI::metadata(QString channel, QString quality, QString key) {
    QUrl url { "http://127.0.0.1:7777/meta" };

    QUrlQuery url_query;
    url_query.addQueryItem("channel", channel);
    if (!quality.isEmpty())
        url_query.addQueryItem("quality", quality);
    url_query.addQueryItem("key", key);
    url.setQuery(url_query);

    QNetworkRequest request { url };

    return get(request).then(&parse_metadata);
}

QString TwitchdAPI::playback_url(QString channel, QString quality, QString meta_key) {
    QUrl url { "http://127.0.0.1:7777/play" };

    QUrlQuery url_query;
    url_query.addQueryItem("channel", channel);
    if (!quality.isEmpty())
        url_query.addQueryItem("quality", quality);
    url_query.addQueryItem("meta_key", meta_key);
    url.setQuery(url_query);

    return url.toString(QUrl::FullyEncoded);
}
