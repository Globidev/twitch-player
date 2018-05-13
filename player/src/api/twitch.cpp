#include "api/twitch.hpp"

#include "constants.hpp"

#include <QUrlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "api/oauth.hpp"

static QList<StreamData> parse_stream_data(const QByteArray & raw) {
    QList<StreamData> parsed;
    auto json_data = QJsonDocument::fromJson(raw).object();

    for (auto raw_stream: json_data["streams"].toArray()) {
        auto stream_obj = raw_stream.toObject();
        auto channel_data_obj = stream_obj["channel"].toObject();

        auto channel_data = ChannelData {
            channel_data_obj["name"].toString(),
            channel_data_obj["display_name"].toString(),
            channel_data_obj["status"].toString()
        };

        auto thumbnail_url = stream_obj["preview"].toObject()["medium"].toString();

        auto stream_data = StreamData {
            channel_data,
            thumbnail_url,
            stream_obj["game"].toString(),
            static_cast<uint32_t>(stream_obj["viewers"].toInt())
        };

        parsed << stream_data;
    }

    return parsed;
}

TwitchAPI::streams_response_t TwitchAPI::stream_search(QString query) {
    QUrl url { "https://api.twitch.tv/kraken/search/streams" };

    QUrlQuery url_query;
    url_query.addQueryItem("q", query);
    url_query.addQueryItem("offset", "0");
    url_query.addQueryItem("limit", "32");
    url.setQuery(url_query);

    QNetworkRequest request { url };
    // request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Client-ID", constants::TWITCHD_CLIENT_ID);

    return get(request).then(&parse_stream_data);
}

TwitchAPI::streams_response_t TwitchAPI::top_streams() {
    QUrl url { "https://api.twitch.tv/kraken/streams" };

    QUrlQuery url_query;
    url_query.addQueryItem("offset", "0");
    url_query.addQueryItem("limit", "32");
    url.setQuery(url_query);

    QNetworkRequest request { url };
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Client-ID", constants::TWITCHD_CLIENT_ID);

    return get(request).then(&parse_stream_data);
}

TwitchAPI::streams_response_t TwitchAPI::followed_streams(const QString & token) {
    QUrl url { "https://api.twitch.tv/kraken/streams/followed" };

    QUrlQuery url_query;
    url_query.addQueryItem("offset", "0");
    url_query.addQueryItem("limit", "32");
    url.setQuery(url_query);

    QNetworkRequest request { url };
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Client-ID", constants::TWITCHD_CLIENT_ID);
    request.setRawHeader("Authorization", QString("OAuth %1").arg(token).toLocal8Bit());

    auto retry_if_unauthorized = [=](BadStatus error) {
        if (error.status == 401) {
            auto oauth = std::make_shared<OAuth>();
            return oauth->query_token().then([=](QString token) mutable {
                oauth.reset();
                return followed_streams(token);
            });
        }
        else
            return streams_response_t::reject(error);
    };

    return get(request)
        .then(&parse_stream_data)
        .fail(retry_if_unauthorized);
}
