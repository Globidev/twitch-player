#include "api/twitch.hpp"

#include "api/oauth.hpp"

#include "constants.hpp"

#include <QUrlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static ChannelData extract_channel(const QJsonObject & channel_obj) {
    return {
        channel_obj["name"].toString(),
        channel_obj["display_name"].toString(),
        channel_obj["status"].toString(),
        channel_obj["logo"].toString(),
        static_cast<uint32_t>(channel_obj["_id"].toInt())
    };
}

static StreamData extract_stream(const QJsonObject & stream_obj) {
    auto channel_data = extract_channel(stream_obj["channel"].toObject());

    return {
        channel_data,
        stream_obj["preview"].toObject()["medium"].toString(),
        stream_obj["game"].toString(),
        static_cast<uint32_t>(stream_obj["viewers"].toInt())
    };
}

static StreamData parse_stream_data(const QByteArray & raw) {
    auto json_data = QJsonDocument::fromJson(raw).object();

    return extract_stream(json_data["stream"].toObject());
}

static QList<StreamData> parse_streams_data(const QByteArray & raw) {
    QList<StreamData> parsed;
    auto json_data = QJsonDocument::fromJson(raw).object();

    for (auto raw_stream: json_data["streams"].toArray())
        parsed << extract_stream(raw_stream.toObject());

    return parsed;
}

static QList<ChannelData> parse_channels_data(const QByteArray & raw) {
    QList<ChannelData> parsed;
    auto json_data = QJsonDocument::fromJson(raw).object();

    for (auto raw_channel: json_data["channels"].toArray())
        parsed << extract_channel(raw_channel.toObject());

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

    return get(request).then(&parse_streams_data);
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

    return get(request).then(&parse_streams_data);
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

    auto retry_if_unauthorized = [=](QNetworkReply::NetworkError error) {
        if (error == QNetworkReply::AuthenticationRequiredError) {
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
        .then(&parse_streams_data)
        .fail(retry_if_unauthorized);
}

TwitchAPI::stream_response_t TwitchAPI::stream(uint32_t channel_id) {
    QUrl url { QString("https://api.twitch.tv/kraken/streams/%1").arg(channel_id) };

    QNetworkRequest request { url };
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Client-ID", constants::TWITCHD_CLIENT_ID);

    return get(request).then(&parse_stream_data);
}

TwitchAPI::channels_response_t TwitchAPI::channel_search(const QString &channel_name) {
    QUrl url { "https://api.twitch.tv/kraken/search/channels" };

    QUrlQuery url_query;
    url_query.addQueryItem("query", channel_name);
    url_query.addQueryItem("offset", "0");
    url_query.addQueryItem("limit", "32");
    url.setQuery(url_query);

    QNetworkRequest request { url };
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Client-ID", constants::TWITCHD_CLIENT_ID);

    return get(request).then(&parse_channels_data);
}
