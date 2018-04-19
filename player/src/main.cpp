#include "main_window.hpp"
#include "video_widget.hpp"
#include "libvlc.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    auto video_context = libvlc::Instance { };

    QApplication app { argc, argv };

    VideoWidget w { video_context };

    w.play("dreamhackcs");
    w.set_volume(50);
    w.show();

    return app.exec();
}
