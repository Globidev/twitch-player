#ifndef TWITCHD_HPP
#define TWITCHD_HPP

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

struct Resolution {
    uint32_t width, height;
};

struct StreamInfo {
    Resolution resolution;
    uint64_t bandwidth;
};

struct MediaInfo {
    QString name;
    QString group_id;
};

struct PlaylistInfo {
    StreamInfo stream_info;
    MediaInfo media_info;
    QString url;
};

struct StreamIndex {
    QList<PlaylistInfo> playlist_infos;
};

class StreamIndexPromise: public QObject {
    Q_OBJECT
public:
    StreamIndexPromise(QNetworkReply *, QObject * = nullptr);
    ~StreamIndexPromise();

    void abort();

signals:
    void finished(const StreamIndex &);

private:
    QNetworkReply *_reply;
};

class TwitchdAPI: public QObject {
public:
    TwitchdAPI(QObject * = nullptr);
    StreamIndexPromise *stream_index(QString);

    static QString playback_url(QString, QString);

private:
    QNetworkAccessManager *_http_client;
};

#endif // TWITCHD_HPP
