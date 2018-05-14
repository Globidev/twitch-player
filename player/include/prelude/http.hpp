#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QtPromise>

class APIClient {
public:
    auto get(const QNetworkRequest &request) {
        using QtPromise::QPromise;

        return QPromise<QByteArray>([=](auto& resolve, auto& reject) {
            auto reply = _http_client.get(request);

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

protected:
    template <class T>
    using response_t = QtPromise::QPromise<T>;

private:
    QNetworkAccessManager _http_client;
};
