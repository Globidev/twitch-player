#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "widgets/vlc_log_viewer.hpp"
#include "widgets/options_dialog.hpp"

#include "native/capabilities.hpp"

#include "constants.hpp"

#include <QKeyEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QSettings>
#include <QMessageBox>

static void focus_pane(QWidget *pane) {
    pane->setFocus();
    pane->repaint();
}

MainWindow::MainWindow(libvlc::Instance &video_context, QWidget *parent) :
    QMainWindow(parent),
    _video_context(video_context),
    _vlc_log_viewer(new VLCLogViewer(video_context)),
    _grid(new ResizableGrid(this)),
    _central_widget(new QStackedWidget(this)),
    _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);

    setCentralWidget(_central_widget);
    _central_widget->addWidget(_grid);

    auto add_action = [this](auto action, auto slot) {
        QObject::connect(action, &QAction::triggered, slot);
    };

    auto active_position = [this] {
        return _grid->widget_position(focused_pane());
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
    add_action(
        _ui->actionStreamZoom,
        [this](bool on) { on ? zoom() : unzoom(); }
    );
        //
    add_action(
        _ui->actionAddStreamLeft,
        [=] { unzoom(); add_pane(active_position()); }
    );
    add_action(
        _ui->actionAddStreamRight,
        [=] { unzoom(); add_pane(active_position().right()); }
    );
    add_action(
        _ui->actionAddStreamUp,
        [=] { unzoom(); add_pane(active_position()); }
    );
    add_action(
        _ui->actionAddStreamDown,
        [=] { unzoom(); add_pane(active_position().down()); }
    );
    add_action(
        _ui->actionRemoveStream,
        [=] { unzoom(); remove_pane(focused_pane()); }
    );
        //
    add_action(
        _ui->actionFocusLeft,
        [=] { unzoom(); move_focus(active_position().left()); }
    );
    add_action(
        _ui->actionFocusRight,
        [=] { unzoom(); move_focus(active_position().right()); }
    );
    add_action(
        _ui->actionFocusUp,
        [=] { unzoom(); move_focus(active_position().up()); }
    );
    add_action(
        _ui->actionFocusDown,
        [=] { unzoom(); move_focus(active_position().down()); }
    );
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
    // Focus might have updated
    for (auto pane: _panes)
        pane->repaint();
    QMainWindow::mousePressEvent(event);
}

StreamContainer * MainWindow::add_pane(Position pos) {
    auto stream_container = new StreamContainer(_video_context, this);
    _grid->insert_widget(pos, stream_container);
    stream_container->setFocus();
    stream_container->activateWindow();
    _panes.push_back(stream_container);
    return stream_container;
}

void MainWindow::remove_pane(StreamContainer *pane) {
    auto pane_it = std::find(_panes.begin(), _panes.end(), pane);

    if (pane_it != _panes.end()) {
        _grid->remove_widget(pane);
        pane->deleteLater();
        auto next_pane = _panes.erase(pane_it);

        if (!_panes.empty()) {
            auto focus_target_it = next_pane == _panes.begin()
                                 ? next_pane : std::prev(next_pane);
            focus_pane(*focus_target_it);
        }
    }
}

StreamContainer * MainWindow::focused_pane() {
    auto focusedWidget = qApp->focusWidget();

    auto pane_it = std::find_if(_panes.begin(), _panes.end(), [=](auto pane) {
        return pane->isAncestorOf(focusedWidget);
    });

    if (pane_it != _panes.end())
        return *pane_it;
    else
        return nullptr;
}

void MainWindow::move_focus(Position pos) {
    auto focus_target = _grid->closest_widget(pos);
    if (focus_target)
        focus_pane(focus_target);
}

bool MainWindow::is_zoomed() {
    return _central_widget->currentWidget() != _grid;
}

void MainWindow::zoom() {
    if (is_zoomed()) {
        _ui->actionStreamZoom->setChecked(true);
        return ;
    }

    if (auto active_pane = focused_pane(); active_pane) {
        _zoomed_position = _grid->widget_position(active_pane);
        _central_widget->addWidget(active_pane);
        _central_widget->setCurrentWidget(active_pane);
        _ui->actionStreamZoom->setChecked(true);
    }
    else {
        _ui->actionStreamZoom->setChecked(false);
    }
}

void MainWindow::unzoom() {
    if (!is_zoomed()) {
        _ui->actionStreamZoom->setChecked(false);
        return ;
    }

    auto zoomed_pane = _central_widget->currentWidget();
    _central_widget->removeWidget(zoomed_pane);
    _grid->insert_widget(_zoomed_position, zoomed_pane);
    zoomed_pane->show();
    focus_pane(zoomed_pane);

    _ui->actionStreamZoom->setChecked(false);
}
