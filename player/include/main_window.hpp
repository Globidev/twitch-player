#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>

#include <vector>

namespace Ui {
class MainWindow;
}

namespace libvlc {
struct Instance;
}

class ResizableGrid;
class StreamContainer;
class VLCLogViewer;

struct PlacedStream {
    StreamContainer *container;
    int row, column;
};

class MainWindow : public QMainWindow {
public:
    MainWindow(libvlc::Instance &, QWidget * = nullptr);
    ~MainWindow();

    StreamContainer * add_container(int, int);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private:
    libvlc::Instance &_video_context;

    VLCLogViewer *_vlc_log_viewer;

    std::vector<PlacedStream> _streams;
    ResizableGrid *_grid;

    Ui::MainWindow *_ui;

    std::pair<int, int> find_focused_stream();
    void remove_stream(int, int);
};

#endif // MAIN_WINDOW_HPP
