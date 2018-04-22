#include "main_window.hpp"
#include "constants.hpp"

#include <QApplication>
#include <QProcess>
#include <QTimer>

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

    // QTimer::singleShot(3000, [&] {
    //     main_window.add_stream("pokimane", 0, 1);
    //     QTimer::singleShot(3000, [&] {
    //         main_window.add_stream("loltyler1", 1, 1);
    //         QTimer::singleShot(3000, [&] {
    //             main_window.add_stream("asmongold", 1, 0);
    //         });
    //     });
    // });

    main_window.show();

    return app.exec();
}
