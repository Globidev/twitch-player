#pragma once

#include <QString>

namespace daemon_control {
    struct Status {
        bool running = false;
        QString version;
    };

    bool start();
    bool stop();

    Status status();
};
