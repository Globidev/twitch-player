#include "process/daemon_control.hpp"

#include "api/twitchd.hpp"

#include "constants.hpp"

#include <QProcess>
#include <QSettings>

struct DaemonSettings {
    QString host;
    quint16 port;

    QString client_id;

    quint32 cache_timeout_s;
    quint32 playlist_fetch_interval_ms;
    quint32 player_inactive_timeout_s;
    quint32 player_fetch_timeout_s;
    quint32 player_video_chunks_size;
    quint32 player_max_sink_buffer_size;
};

static DaemonSettings load_settings() {
    using namespace constants::settings::daemon;
    QSettings settings;

    return DaemonSettings {
        settings.value(KEY_HOST_MANAGED, DEFAULT_HOST_MANAGED).toString(),
        settings.value(KEY_PORT_MANAGED, DEFAULT_PORT_MANAGED).value<quint16>(),

        constants::TWITCHD_CLIENT_ID,

        settings.value(KEY_CACHE_TIMEOUT, DEFAULT_CACHE_TIMEOUT).value<quint32>(),
        settings.value(KEY_PLAYLIST_FETCH_INTERVAL, DEFAULT_PLAYLIST_FETCH_INTERVAL).value<quint32>(),
        settings.value(KEY_PLAYER_INACTIVE_TIMEOUT, DEFAULT_PLAYER_INACTIVE_TIMEOUT).value<quint32>(),
        settings.value(KEY_PLAYER_FETCH_TIMEOUT, DEFAULT_PLAYER_FETCH_TIMEOUT).value<quint32>(),
        settings.value(KEY_PLAYER_VIDEO_CHUNKS_SIZE, DEFAULT_PLAYER_VIDEO_CHUNKS_SIZE).value<quint32>(),
        settings.value(KEY_PLAYER_MAX_SINK_BUFFER_SIZE, DEFAULT_PLAYER_MAX_SINK_BUFFER_SIZE).value<quint32>(),
    };
}

namespace daemon_control {

bool start() {
    auto settings = load_settings();

    auto arguments = QStringList()
        << "--client-id" << settings.client_id
        << "--host" << settings.host
        << "--port" << QString::number(settings.port)
        << "--index-cache-timeout" << QString("%1s").arg(settings.cache_timeout_s)
        << "--playlist-fetch-interval" << QString("%1ms").arg(settings.playlist_fetch_interval_ms)
        << "--player-inactive-timeout" << QString("%1s").arg(settings.player_inactive_timeout_s)
        << "--player-fetch-timeout" << QString("%1s").arg(settings.player_fetch_timeout_s)
        << "--player-video-chunks-size" << QString("%1").arg(settings.player_video_chunks_size)
        << "--player-max-sink-buffer-size" << QString("%1").arg(settings.player_max_sink_buffer_size)
        ;

    return QProcess::startDetached(constants::TWITCHD_PATH, arguments);
}

bool stop() {
    bool stopped = false;

    TwitchdAPI api;

    auto quit_response = api.daemon_quit()
        .tap([&] {
            stopped = true;
        });

    quit_response.timeout(2000).wait();

    return stopped;
}

Status status() {
    using namespace constants::settings::daemon;
    QSettings settings;

    Status status;
    TwitchdAPI api;

    status.managed = settings.value(KEY_MANAGED, DEFAULT_MANAGED).toBool();

    auto version_response = api.daemon_version()
        .tap([&](QString version) {
            status.running = true;
            status.version = version;
        });

    version_response.timeout(2000).wait();

    return status;
}

}
