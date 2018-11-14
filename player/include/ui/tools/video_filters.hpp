#pragma once

#include <QWidget>

#include <memory>

namespace Ui {
class VideoFilters;
};

namespace libvlc {
struct MediaPlayer;
}

class VideoFilters: public QWidget {
public:
    VideoFilters(libvlc::MediaPlayer &, QWidget * = nullptr);

private:
    std::unique_ptr<Ui::VideoFilters> _ui;
};
