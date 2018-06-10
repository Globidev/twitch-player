#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QtPromise>

class APIClient {
private:
    QNetworkAccessManager _http_client;

    auto send_request(const QNetworkRequest &request, const QByteArray &verb) {
        using QtPromise::QPromise;

        return QPromise<QByteArray>([=](auto& resolve, auto& reject) {
            auto reply = _http_client.sendCustomRequest(request, verb);

            QObject::connect(reply, &QNetworkReply::finished, [=]() {
                auto error = reply->error();

                if (error == QNetworkReply::NoError)
                    resolve(reply->readAll());
                else
                    reject(error);

                reply->deleteLater();
            });

        });
    }

public:
    auto get(const QNetworkRequest &request) {
        return send_request(request, "GET");
    }

    auto post(const QNetworkRequest &request) {
        return send_request(request, "POST");
    }

protected:
    template <class T>
    using response_t = QtPromise::QPromise<T>;
};
