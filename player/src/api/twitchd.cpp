#include "api/twitchd.hpp"

#include "constants.hpp"

#include <QSettings>

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

static QUrl endpoint(const QString &path) {
    using namespace constants::settings::daemon;

    auto [address, port] = []() -> std::pair<QString, quint16> {
        QSettings settings;
        if (settings.value(KEY_MANAGED, DEFAULT_MANAGED).toBool()) {
            return {
                settings.value(KEY_HOST_MANAGED, DEFAULT_HOST_MANAGED).toString(),
                settings.value(KEY_PORT_MANAGED, DEFAULT_PORT_MANAGED).value<quint16>()
            };
        }
        else {
            return {
                settings.value(KEY_HOST_UNMANAGED, DEFAULT_HOST_UNMANAGED).toString(),
                settings.value(KEY_PORT_UNMANAGED, DEFAULT_PORT_UNMANAGED).value<quint16>()
            };
        }
    }();

    return QUrl { QString("http://%1:%2/%3").arg(address).arg(port).arg(path) };
}

TwitchdAPI::stream_index_response_t TwitchdAPI::stream_index(QString channel) {
    using namespace constants::settings::oauth;

    auto url = endpoint("stream_index");

    QSettings settings;
    auto oauth = settings.value(ACCESS_TOKEN_KEY).toString();

    QUrlQuery url_query;
    url_query.addQueryItem("channel", channel);
    if (!oauth.isEmpty())
        url_query.addQueryItem("oauth", oauth);
    url.setQuery(url_query);

    QNetworkRequest request { url };

    return get(request).then(&parse_stream_index_data);
}

TwitchdAPI::metadata_response_t TwitchdAPI::metadata(QString channel, QString quality, QString key) {
    auto url = endpoint("meta");

    QUrlQuery url_query;
    url_query.addQueryItem("channel", channel);
    if (!quality.isEmpty())
        url_query.addQueryItem("quality", quality);
    url_query.addQueryItem("key", key);
    url.setQuery(url_query);

    QNetworkRequest request { url };

    return get(request).then(&parse_metadata);
}

TwitchdAPI::daemon_version_response_t TwitchdAPI::daemon_version() {
    auto url = endpoint("version");

    QNetworkRequest request { url };

    return get(request)
        .then([](const QByteArray &raw) {
            return QString(raw);
        });
}

TwitchdAPI::daemon_quit_response_t TwitchdAPI::daemon_quit() {
    auto url = endpoint("quit");

    QNetworkRequest request { url };

    return post(request).then([](const QByteArray &) { });
}

QString TwitchdAPI::playback_url(QString channel, QString quality, QString meta_key) {
    using namespace constants::settings::oauth;

    auto url = endpoint("play");

    QSettings settings;
    auto oauth = settings.value(ACCESS_TOKEN_KEY).toString();

    QUrlQuery url_query;
    url_query.addQueryItem("channel", channel);
    if (!quality.isEmpty())
        url_query.addQueryItem("quality", quality);
    if (!oauth.isEmpty())
        url_query.addQueryItem("oauth", oauth);
    url_query.addQueryItem("meta_key", meta_key);
    url.setQuery(url_query);

    return url.toString(QUrl::FullyEncoded);
}
