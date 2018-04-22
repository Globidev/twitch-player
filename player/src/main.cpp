#include "main_window.hpp"
#include "constants.hpp"

#include <QApplication>
#include <QProcess>

int main(int argc, char *argv[]) {
    QProcess::startDetached(constants::TWITCHD_PATH, QStringList()
        << "--client-id" << constants::TWITCHD_CLIENT_ID
    );

    QApplication app { argc, argv };

    auto args = app.arguments();
    auto channel = [&] {
        if (args[1].startsWith("twitch-player://"))
            return args[1].remove("twitch-player://").remove("/");
        else
            return args[1];
    }();

    MainWindow main_window { channel };

    main_window.show();

    return app.exec();
}
