#include "api/oauth.hpp"

#include "constants.hpp"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QSettings>

#include <QDesktopServices>

static auto access_code_url(const QByteArray & code) {
    QUrl url { "https://id.twitch.tv/oauth2/token" };

    QUrlQuery url_query;
    url_query.addQueryItem("client_id", constants::TWITCHD_CLIENT_ID);
    url_query.addQueryItem("client_secret", "8mpn5c01v3k5fdi094x6275cn18du7");
    url_query.addQueryItem("code", code);
    url_query.addQueryItem("grant_type", "authorization_code");
    url_query.addQueryItem("redirect_uri", constants::OAUTH_REDIRECT_URI);
    url.setQuery(url_query);

    return url;
}

static auto refresh_token_url(const QString & refresh_token) {
    QUrl url { "https://id.twitch.tv/oauth2/token" };

    QUrlQuery url_query;
    url_query.addQueryItem("grant_type", "refresh_token");
    url_query.addQueryItem("refresh_token", refresh_token);
    url_query.addQueryItem("client_id", constants::TWITCHD_CLIENT_ID);
    url_query.addQueryItem("client_secret", "8mpn5c01v3k5fdi094x6275cn18du7");
    url.setQuery(url_query);

    return url;
}

static auto authorize_url() {
    QUrl url { "https://id.twitch.tv/oauth2/authorize" };

    QUrlQuery url_query;
    url_query.addQueryItem("client_id", constants::TWITCHD_CLIENT_ID);
    url_query.addQueryItem("redirect_uri", constants::OAUTH_REDIRECT_URI);
    url_query.addQueryItem("response_type", "code");
    url_query.addQueryItem("scope", constants::OAUTH_SCOPES);
    url.setQuery(url_query);

    return url;
}

OAuth::OAuth(QObject *parent):
    QObject(parent),
    _local_server(new QTcpServer(this))
{
    QObject::connect(_local_server, &QTcpServer::newConnection, [=] {
        auto client = _local_server->nextPendingConnection();
        QObject::connect(client, &QTcpSocket::readyRead, [=] {
            while (client->canReadLine()) {
                auto line = client->readLine();
                auto code_param_index = line.indexOf("GET /?code=");

                if (code_param_index != -1) {
                    auto code = line.mid(code_param_index + 11, 30);
                    fetch_token(access_code_url(code));
                }

                if (client->atEnd()) {
                    client->write("HTTP/1.1 200 OK\r\n\r\n<h1>OK</h1>");
                    client->close();
                }
            }
        });
    });

    _local_server->listen(
        QHostAddress::LocalHost,
        constants::OAUTH_REDIRECT_URI_PORT
    );
}

QtPromise::QPromise<QString> OAuth::query_token() {
    QSettings settings;

    auto refresh_token = settings
        .value(constants::oauth::REFRESH_TOKEN_KEY)
        .toString();

    if (!refresh_token.isEmpty())
        fetch_token(refresh_token_url(refresh_token));
    else
        QDesktopServices::openUrl(authorize_url());

    return QtPromise::QPromise<QString>([=](const auto & resolve, auto) {
        QObject::connect(this, &OAuth::token_ready, [=](auto token) {
            resolve(token);
        });
    });
}

void OAuth::save_token_data(const QByteArray &raw_data) {
    auto json_data = QJsonDocument::fromJson(raw_data).object();

    auto access_token = json_data["access_token"].toString();
    auto refresh_token = json_data["refresh_token"].toString();

    QSettings settings;

    settings.setValue(constants::oauth::ACCESS_TOKEN_KEY, access_token);
    settings.setValue(constants::oauth::REFRESH_TOKEN_KEY, refresh_token);

    emit token_ready(access_token);
}

void OAuth::fetch_token(const QUrl &url) {
    auto http_client = new QNetworkAccessManager(this);
    auto token_reply = http_client->post(QNetworkRequest { url }, QByteArray());

    QObject::connect(token_reply, &QNetworkReply::finished, [=] {
        auto status = token_reply
            ->attribute(QNetworkRequest::HttpStatusCodeAttribute)
            .toInt();

        if (status == 200) {
            auto token_data = token_reply->readAll();
            save_token_data(token_data);
        }

        token_reply->deleteLater();
    });
}
