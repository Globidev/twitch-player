#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QMap>

#include <vector>
#include <tuple>

#include "widgets/resizable_grid.hpp"

namespace Ui {
class MainWindow;
}

namespace libvlc {
struct Instance;
}

class StreamContainer;
class VLCLogViewer;
class QStackedWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow(libvlc::Instance &, QWidget * = nullptr);
    ~MainWindow();

    StreamContainer * add_pane(Position);
    void remove_pane(StreamContainer *);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    libvlc::Instance &_video_context;

    VLCLogViewer *_vlc_log_viewer;

    std::vector<StreamContainer *> _panes;
    ResizableGrid *_grid;
    QStackedWidget *_central_widget;

    Ui::MainWindow *_ui;

    Position _zoomed_position;

    StreamContainer * focused_pane();

    void move_focus(Position);
    void move_pane(Position, Position);

    bool is_zoomed();
    void zoom();
    void unzoom();

    std::vector<std::pair<QString, std::pair<QShortcut *, QAction*>>> _shortcuts;

    void setup_shortcuts();
    void remap_shortcuts();
};

#endif // MAIN_WINDOW_HPP
