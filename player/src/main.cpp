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
    MainWindow main_window;

    if (args.size() >= 2)
        main_window.add_stream(args[1]);
    else
        main_window.add_stream("lirik");

    main_window.show();

    return app.exec();
}
