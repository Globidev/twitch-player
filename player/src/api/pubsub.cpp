#include "api/pubsub.hpp"

#include <QWebSocket>
#include <QTimer>

#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>

#include "prelude/timer.hpp"

constexpr auto WS_URL = "wss://pubsub-edge.twitch.tv";

static QString gen_nonce() {
    const QString charset {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    };

    QString key;

    for (int i = 0; i < 32; ++i) {
        int index = qrand() % charset.length();
        key.append(charset.at(index));
    }

    return key;
}

static QJsonObject ping_message() {
    return {
        { "type", "PING" }
    };
}

struct ListenOrder {
    static constexpr char type[] = "LISTEN";

    const QStringList topics;

    QJsonObject encode() const {
        return {
            { "topics", QJsonArray::fromStringList(topics) }
        };
    }

    static ListenOrder playback(QString channel) {
        auto playback_topic = QString("video-playback.%1").arg(channel);

        return { QStringList() << playback_topic };
    }
};

struct UnlistenOrder {
    static constexpr char type[] = "UNLISTEN";

    const QStringList topics;

    QJsonObject encode() const {
        return {
            { "topics", QJsonArray::fromStringList(topics) }
        };
    }

    static UnlistenOrder playback(QString channel) {
        auto playback_topic = QString("video-playback.%1").arg(channel);

        return { QStringList() << playback_topic };
    }
};

template <class Order>
static QJsonObject encode_order(Order order, QString nonce) {
    return {
        { "type",  Order::type },
        { "nonce", nonce },
        { "data",  order.encode() },
    };
}

TwitchPubSub::TwitchPubSub(QObject *parent):
    QObject(parent),
    _ws(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this)),
    _ping_timer(new QTimer(this))
{
    QObject::connect(_ws, &QWebSocket::connected, [this] {
        _ping_timer->start();

        for (auto order: _order_queue)
            send_message(order);

        _order_queue.clear();
    });

    QObject::connect(_ws, &QWebSocket::disconnected, [this] {
        _ping_timer->stop();

        _order_queue = _orders_issued;
        _orders_issued.clear();

        delayed(this, 2000, [this] { _ws->open(QUrl { WS_URL }); });
    });

    QObject::connect(_ws, &QWebSocket::textMessageReceived, [this](auto raw_message) {
        auto message = QJsonDocument::fromJson(raw_message.toLocal8Bit());
        process_message(message.object());
    });

    QObject::connect(_ping_timer, &QTimer::timeout, [this] {
        send_message(ping_message());
    });

    _ping_timer->setInterval(4 * 60 * 1000);

    _ws->open(QUrl { WS_URL });
}

QtPromise::QPromise<void> TwitchPubSub::listen_to_channel(QString channel) {
    auto listen_order = ListenOrder::playback(channel);
    auto nonce = gen_nonce();

    auto promise = QtPromise::QPromise<void>([=](auto resolve, auto reject) {
        _pending_orders.insert(nonce, PendingOrder { resolve, reject });
    });

    auto order_message = encode_order(listen_order, nonce);

    if (_ws->state() != QAbstractSocket::SocketState::ConnectedState) {
        _order_queue.append(order_message);
    } else {
        send_message(order_message);
    }

    return promise;
}

void TwitchPubSub::unlisten_to_channel(QString channel) {
    auto unlisten_order = UnlistenOrder::playback(channel);

    auto order_message = encode_order(unlisten_order, gen_nonce());

    send_message(order_message);
}

void TwitchPubSub::send_message(QJsonObject message) {
    qDebug() << ">>" << message;

    _ws->sendTextMessage(QJsonDocument(message).toJson());
}

void TwitchPubSub::process_message(QJsonObject message) {
    qDebug() << "<<" << message;

    auto type = message["type"].toString();

    if (type == "RESPONSE") {
        auto nonce = message["nonce"].toString();
        auto pending_order_it = _pending_orders.find(nonce);

        if (pending_order_it != _pending_orders.end()) {
            auto error = message["error"].toString();

            if (error.isEmpty())
                pending_order_it->resolve();
            else
                pending_order_it->reject(error);

            _pending_orders.remove(nonce);
        }
    }
    else if (type == "MESSAGE") {
        auto data = message["data"].toObject();
        auto topic = data["topic"].toString();
        auto raw_data_message = data["message"].toString().toLocal8Bit();
        auto data_message = QJsonDocument::fromJson(raw_data_message).object();
        auto data_message_type = data_message["type"].toString();

        if (data_message_type == "stream-up") {
            auto channel = topic.right(QString("video-playback.").length());
            emit channel_went_live(channel);
        }
    }
}
