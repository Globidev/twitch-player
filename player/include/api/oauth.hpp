#pragma once

#include <QObject>
#include <QtPromise>

class QTcpServer;
class QNetworkRequest;

using QtPromise::QPromise;

class OAuth: public QObject {
    Q_OBJECT

public:
    OAuth(QObject * = nullptr);

    QPromise<QString> query_token();

signals:
    void token_ready(QString);

private:
    QTcpServer *_local_server;

    void fetch_token(const QUrl &);
    void save_token_data(const QByteArray &);
};
