#pragma once

#include <QJsonObject>

#include <QtPromise>

class QWebSocket;
class QTimer;

struct PendingOrder {
    QtPromise::QPromiseResolve<void> resolve;
    QtPromise::QPromiseReject<void> reject;
};

class TwitchPubSub: public QObject {
    Q_OBJECT

public:
    TwitchPubSub(QObject * = nullptr);

    QtPromise::QPromise<void> listen_to_channel(QString);
    void unlisten_to_channel(QString);

signals:
    void channel_went_live(QString);
    void channel_went_offline(QString);

private:
    void send_message(QJsonObject);
    void process_message(QJsonObject);

    QWebSocket *_ws;
    QTimer *_ping_timer;

    QMap<QString, PendingOrder> _pending_orders;

    QList<QJsonObject> _order_queue;
    QList<QJsonObject> _orders_issued;
};
