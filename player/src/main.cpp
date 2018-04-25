#include "main_window.hpp"
#include "constants.hpp"

#include <QApplication>
#include <QProcess>
#include <QTimer>

int main(int argc, char *argv[]) {
    QProcess::startDetached(constants::TWITCHD_PATH, QStringList()
        << "--client-id" << constants::TWITCHD_CLIENT_ID
    );

    QCoreApplication::setOrganizationName("GlobiCorp");
    QCoreApplication::setApplicationName("Twitch Player");

    QApplication app { argc, argv };

    MainWindow main_window;

    auto args = app.arguments();
    if (args.size() >= 2) {
        auto channel = [&] {
            if (args[1].startsWith("twitch-player://"))
                return args[1].remove("twitch-player://").remove("/");
            else
                return args[1];
        }();
        main_window.add_stream(0, 0, channel);
    }
    else {
        main_window.add_picker(0, 0);
    }

    main_window.show();

    return app.exec();
}
