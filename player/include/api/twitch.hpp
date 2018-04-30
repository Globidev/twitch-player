#ifndef TWITCH_HPP
#define TWITCH_HPP

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

struct ChannelData {
    QString name, display_name;
    QString title;
};

struct StreamData {
    ChannelData channel;
    QString preview;
    QString current_game;
    uint32_t viewcount;
};

class StreamPromise: public QObject {
    Q_OBJECT
public:
    StreamPromise(QNetworkReply *, QObject * = nullptr);
    ~StreamPromise();

    void abort();

signals:
    void finished(const QList<StreamData> &);

private:
    QNetworkReply *_reply;
};

class TwitchAPI: public QObject {
public:
    TwitchAPI(QObject * = nullptr);
    StreamPromise *stream_search(QString);
    StreamPromise *top_streams();

private:
    QNetworkAccessManager *_http_client;
};

#endif // TWITCH_HPP
