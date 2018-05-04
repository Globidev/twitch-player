#ifndef FILTERS_TOOL_HPP
#define FILTERS_TOOL_HPP

#include <QWidget>

namespace Ui {
class FiltersTool;
};

namespace libvlc {
struct MediaPlayer;
}

class FiltersTool: public QWidget {
public:
    FiltersTool(libvlc::MediaPlayer &, QWidget * = nullptr);
    ~FiltersTool();

private:
    Ui::FiltersTool *_ui;
};


#endif // FILTERS_TOOL_HPP
