#include "constants.hpp"

#include "libvlc/bindings.hpp"

#include "process/daemon_control.hpp"

#include "ui/main_window.hpp"
#include "ui/widgets/pane.hpp"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>

#include <optional>

constexpr auto EXPECTED_DAEMON_VERSION = "1.1.2";

struct Options {
    std::optional<QString> initial_channel;
};

using VLCArgs = std::vector<std::string>;

static Options parse_options(QStringList args);
static VLCArgs load_vlc_args();
static void handle_vlc_init_failure();
static bool init_daemon();

int main(int argc, char *argv[]) {
    QApplication app { argc, argv };

    app.setOrganizationName("GlobiCorp");
    app.setApplicationName("Twitch Player");

    libvlc::Instance video_context { load_vlc_args() };

    if (!video_context.init_success()) {
        handle_vlc_init_failure();
        return EXIT_FAILURE;
    }

    if (!init_daemon()) {
        return EXIT_FAILURE;
    }

    MainWindow main_window { video_context };
    auto pane = main_window.add_pane(Position { 0, 0 });

    auto options = parse_options(app.arguments());
    if (options.initial_channel)
        pane->play(*options.initial_channel);

    main_window.show();

    return app.exec();
}

static Options parse_options(QStringList args) {
    if (args.size() <= 1)
        return { };

    auto channel = [channel_arg = args[1]] {
        constexpr auto protocol_prefix = "twitch-player://";
        // Check if the program was called using the registered protocol
        // The format is "protocol-name://channel/"
        if (channel_arg.startsWith(protocol_prefix))
            return channel_arg.mid(static_cast<int>(strlen(protocol_prefix)))
                              .chopped(1); // Trailing slash
        else
            return channel_arg;
    }();

    return { channel };
}

static VLCArgs load_vlc_args() {
    using namespace constants::settings::vlc;

    QSettings settings;

    auto vlc_args_as_qlist = settings.value(KEY_VLC_ARGS, DEFAULT_VLC_ARGS)
        .toStringList();

    VLCArgs vlc_args;
    std::transform(
        vlc_args_as_qlist.begin(), vlc_args_as_qlist.end(),
        std::back_inserter(vlc_args),
        [](auto & q_str) { return q_str.toStdString(); }
    );

    return vlc_args;
}

static void handle_vlc_init_failure() {
    // Reset libvlc options
    QSettings settings;
    settings.remove(constants::settings::vlc::KEY_VLC_ARGS);

    auto error_message = QString(
        "Failed to initialize libvlc\n"
        "Resetting options...\n"
        "Try running the program again"
    );

    QMessageBox::critical(nullptr, "libvlc error", error_message);
}

static bool init_daemon() {
    auto status = daemon_control::status();

    if (!status.managed)
        return true;

    auto start_daemon = [] {
        if (!daemon_control::start()) {
            auto error_message = QString(
                "Failed to start the daemon process\n"
                "Make sure that you have the correct permissions to run it"
            );
            QMessageBox::critical(nullptr, "Daemon error", error_message);
            return false;
        }
        else {
            return true;
        }
    };

    if (!status.running) {
        if (!start_daemon())
            return false;
    }
    else if (status.version != EXPECTED_DAEMON_VERSION) {
        if (!daemon_control::stop()) {
            auto warning_message = QString(
                "Failed to gracefully stop the daemon process\n"
                "Your daemon is out of date and might lack a shutdown capability\n"
                "Please try to manually stop the process called \"twitchd\" and run the app again\n"
                "If you choose to ignore that warning, your experience might not be optimal"
            );
            QMessageBox::warning(nullptr, "Daemon warning", warning_message);
            return true;
        }

        if (!start_daemon())
            return false;
    }

    return true;
}
