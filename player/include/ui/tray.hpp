#pragma once

#include <QSystemTrayIcon>
#include <QMenu>

#include <optional>

class TwitchPubSub;

class SystemTray: public QSystemTrayIcon {
    Q_OBJECT

public:
    SystemTray(TwitchPubSub &);

signals:
    void channel_open_requested(QString);

private:
    TwitchPubSub &_pubsub;

    std::optional<QString> channel_to_play;

    QMenu _menu;
};
