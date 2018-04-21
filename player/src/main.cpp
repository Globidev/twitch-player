#include "main_window.hpp"
#include "constants.hpp"

#include <QApplication>
#include <QProcess>

#include "video_widget.hpp"

int main(int argc, char *argv[]) {
    QProcess::startDetached(constants::TWITCHD_PATH, QStringList()
        << "--client-id" << constants::TWITCHD_CLIENT_ID
    );

    QApplication app { argc, argv };

    auto args = app.arguments();
    MainWindow main_window;

    if (args.size() >= 2) {
        if (args[1].startsWith("twitch-player://")) {
            auto channel = args[1].remove("twitch-player://").remove("/");
            main_window.add_stream(channel);
        }
        else
            main_window.add_stream(args[1]);
    }
    // else
    //     main_window.add_stream("lirik");

    main_window.show();

    // libvlc::Instance i;
    // VideoWidget w { i };
    // w.play("lirik");
    // w.set_volume(20);
    // w.show();

    return app.exec();
}
