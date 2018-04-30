#include "api/twitch.hpp"

#include "constants.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

StreamPromise::StreamPromise(QNetworkReply *reply, QObject *parent):
    QObject(parent),
    _reply(reply)
{
    QObject::connect(_reply, &QNetworkReply::finished, [=] {
        if (_reply->isOpen())
            emit finished(parse_stream_data(_reply->readAll()));
    });
}

StreamPromise::~StreamPromise() {
    _reply->deleteLater();
}

void StreamPromise::abort() {
    _reply->abort();
}

TwitchAPI::TwitchAPI(QObject *parent):
    _http_client(new QNetworkAccessManager(parent))
{ }

StreamPromise *TwitchAPI::stream_search(QString query) {
    QUrl url { "https://api.twitch.tv/kraken/search/streams" };

    QUrlQuery url_query;
    url_query.addQueryItem("q", query);
    url_query.addQueryItem("offset", "0");
    url_query.addQueryItem("limit", "32");
    url.setQuery(url_query);

    QNetworkRequest request { url };
    // request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Client-ID", constants::TWITCHD_CLIENT_ID);

    return new StreamPromise(_http_client->get(request), this);
}

StreamPromise *TwitchAPI::top_streams() {
    QUrl url { "https://api.twitch.tv/kraken/streams" };

    QUrlQuery url_query;
    url_query.addQueryItem("offset", "0");
    url_query.addQueryItem("limit", "32");
    url.setQuery(url_query);

    QNetworkRequest request { url };
    request.setRawHeader("Accept", "application/vnd.twitchtv.v5+json");
    request.setRawHeader("Client-ID", constants::TWITCHD_CLIENT_ID);

    return new StreamPromise(_http_client->get(request), this);
}
