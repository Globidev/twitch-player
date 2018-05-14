#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QtPromise>

struct BadStatus {
    int status;
    QByteArray reply;
};

class APIClient {
public:
    auto get(const QNetworkRequest &request) {
        using QtPromise::QPromise;

        return QPromise<QByteArray>([=](auto& resolve, auto& reject) {
            auto reply = _http_client.get(request);

            QObject::connect(reply, &QNetworkReply::finished, [=]() {
                if (reply->error() == QNetworkReply::NoError) {
                    auto status = reply
                        ->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                        .toInt();
                    auto data = reply->readAll();

                    if (status == 200)
                        resolve(data);
                    else
                        reject(BadStatus { status, data });
                }
                else
                    reject(reply->error());

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
