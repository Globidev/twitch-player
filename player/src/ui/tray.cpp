#include "ui/tray.hpp"

#include "api/pubsub.hpp"

#include "prelude/timer.hpp"

#include "constants.hpp"

#include <QSettings>

SystemTray::SystemTray(TwitchPubSub &pubsub):
    QSystemTrayIcon(QIcon(":/icons/icon.ico")),
    _pubsub(pubsub)
{
    QObject::connect(&pubsub, &TwitchPubSub::channel_went_live, [=](auto channel) {
        channel_to_play = channel;

        auto alert_title = QString("%1 just went live!").arg(channel);
        auto alert_message = QString("Click to play");

        showMessage(alert_title, alert_message);

        delayed(this, 10'000, [=] { channel_to_play = std::nullopt; });
    });

    QObject::connect(this, &QSystemTrayIcon::messageClicked, [=] {
        if (channel_to_play)
            emit channel_open_requested(*channel_to_play);
    });

    _menu.addAction("Quit", [] { qApp->quit(); });

    setContextMenu(&_menu);

    using namespace constants::settings::notifications;

    QSettings settings;

    auto pubsub_channels = settings
        .value(KEY_PUBSUB_CHANNELS, DEFAULT_PUBSUB_CHANNELS)
        .toStringList();

    for (auto channel: pubsub_channels)
        pubsub.listen_to_channel(channel)
            .fail([=](QString error) {
                auto alert_title = "Failed to monitor channel";
                auto alert_message = QString("Error while trying to monitor %1: %2")
                    .arg(channel)
                    .arg(error);

                showMessage(alert_title, alert_message, QSystemTrayIcon::Critical);
            });
}
