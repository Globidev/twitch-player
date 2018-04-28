#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "widgets/vlc_log_viewer.hpp"
#include "widgets/options_dialog.hpp"
#include "widgets/resizable_grid.hpp"

#include "native/capabilities.hpp"

#include "constants.hpp"

#include <QKeyEvent>
#include <QSplitter>
#include <QSettings>
#include <QMessageBox>

MainWindow::MainWindow(libvlc::Instance &video_context, QWidget *parent) :
    QMainWindow(parent),
    _video_context(video_context),
    _vlc_log_viewer(new VLCLogViewer(video_context)),
    _grid(new ResizableGrid(this)),
    _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);

    setCentralWidget(_grid);

    auto add_action = [this](auto action, auto slot) {
        QObject::connect(action, &QAction::triggered, slot);
    };

    // View
    add_action(_ui->actionFullScreen, [this](bool on) {
        setWindowState(windowState().setFlag(Qt::WindowFullScreen, on));
    });
    add_action(_ui->actionToggleWindowBorders, [this](bool on) {
        auto win_handle = reinterpret_cast<WindowHandle>(window()->winId());
        toggle_window_borders(win_handle, on);
    });
    add_action(_ui->actionAlwaysOnTop, [this](bool on) {
        auto win_handle = reinterpret_cast<WindowHandle>(window()->winId());
        toggle_always_on_top(win_handle, on);
    });
    add_action(_ui->actionAddStreamHorizontally, [this] {
        auto [row, col] = find_focused_stream();
        add_container(row, col + 1);
    });
    add_action(_ui->actionAddStreamVertically, [this] {
        auto [row, col] = find_focused_stream();
        add_container(row + 1, col);
    });
    add_action(_ui->actionRemoveStream, [this] {
        auto [row, col] = find_focused_stream();
        remove_stream(row, col);
    });
    // Tools
    add_action(_ui->actionLogs, [this] {
        _vlc_log_viewer->show();
        _vlc_log_viewer->raise();
    });
    // Preferences
    add_action(_ui->actionOptions, [this] {
        auto options_dialog = new OptionsDialog(this);
        options_dialog->show();
    });
}

MainWindow::~MainWindow() {
    delete _vlc_log_viewer;
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::ActivationChange)
        _ui->menuBar->setVisible(isActiveWindow());
    QMainWindow::changeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    QMainWindow::keyPressEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    for (auto s: _streams)
        s.container->repaint();
    QMainWindow::mousePressEvent(event);
}

StreamContainer * MainWindow::add_container(int row, int col) {
    auto stream_container = new StreamContainer(_video_context, this);
    _grid->insert_widget(row, col, stream_container);
    stream_container->setFocus();
    stream_container->activateWindow();
    _streams.push_back(PlacedStream { stream_container, row, col });
    return stream_container;
}

std::pair<int, int> MainWindow::find_focused_stream() {
    auto focusedWidget = qApp->focusWidget();

    auto stream_it = std::find_if(
        _streams.begin(), _streams.end(),
        [=](auto stored_stream) {
            return stored_stream.container->isAncestorOf(focusedWidget);
        }
    );

    if (stream_it != _streams.end())
        return { stream_it->row, stream_it->column };

    return { 0, 0 };
}

void MainWindow::remove_stream(int row, int col) {
    auto stream_it = std::find_if(
        _streams.begin(), _streams.end(),
        [=](auto stored_stream) {
            return stored_stream.row == row
                && stored_stream.column == col;
        }
    );

    if (stream_it != _streams.end()) {
        stream_it->container->deleteLater();
        auto next = _streams.erase(stream_it);

        if (!_streams.empty()) {
            auto focus_target_it = next == _streams.begin()
                                 ? next : std::prev(next);
            focus_target_it->container->setFocus();
            focus_target_it->container->repaint();
        }
    }
}
