#pragma once

#include <QMainWindow>
#include <QMap>

#include <vector>
#include <tuple>

#include "ui/layouts/splitter_grid.hpp"

namespace Ui {
class MainWindow;
}

namespace libvlc {
struct Instance;
}

class Pane;
class VLCLogViewer;
class QStackedWidget;

class MainWindow : public QMainWindow {
public:
    MainWindow(libvlc::Instance &, QWidget * = nullptr);
    ~MainWindow();

    Pane *add_pane(Position);
    void remove_pane(Pane *);

protected:
    void changeEvent(QEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    libvlc::Instance &_video_context;

    VLCLogViewer *_vlc_log_viewer;

    std::vector<Pane *> _panes;
    SplitterGrid *_grid;
    QStackedWidget *_central_widget;

    Ui::MainWindow *_ui;

    Position _zoomed_position;

    Pane *focused_pane();

    void move_focus(Position);
    void move_pane(Position, Position);

    bool is_zoomed();
    void zoom();
    void unzoom();

    std::vector<std::pair<QString, std::pair<QShortcut *, QAction*>>> _shortcuts;

    void setup_shortcuts();
    void remap_shortcuts();

    QMenu *_audio_devices_menu;
    void setup_audio_devices();
};
