#ifndef OAUTH_HPP
#define OAUTH_HPP

#include <QObject>
#include <QtPromise>

class QTcpServer;
class QNetworkRequest;

class OAuth: public QObject {
    Q_OBJECT
public:
    OAuth(QObject * = nullptr);

    QtPromise::QPromise<QString> query_token();

signals:
    void token_ready(QString);

private:
    QTcpServer *_local_server;

    void fetch_token(const QUrl &);
    void save_token_data(const QByteArray &);
};

#endif // OAUTH_HPP
