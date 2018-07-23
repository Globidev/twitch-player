#include "ui/main_window.hpp"
#include "ui_main_window.h"

#include "ui/widgets/pane.hpp"
#include "ui/widgets/stream_widget.hpp"
#include "ui/widgets/video_widget.hpp"
#include "ui/widgets/foreign_widget.hpp"
#include "ui/overlays/video_controls.hpp"
#include "ui/tools/vlc_log_viewer.hpp"

#include "prelude/timer.hpp"

#include <QStackedWidget>
#include <QKeyEvent>
#include <QShortcut>

static void focus_pane(QWidget *pane) {
    pane->setFocus();
    pane->repaint();
}

MainWindow::MainWindow(libvlc::Instance &video_context, TwitchPubSub &pubsub, QWidget *parent):
    QMainWindow(parent),
    _ui(std::make_unique<Ui::MainWindow>()),
    _video_context(video_context),
    _pubsub(pubsub),
    _vlc_log_viewer(std::make_unique<VLCLogViewer>(video_context)),
    _grid(new SplitterGrid(this)),
    _central_widget(new QStackedWidget(this))
{
    _ui->setupUi(this);

    setCentralWidget(_central_widget);
    _central_widget->addWidget(_grid);

    setup_shortcuts();
}

MainWindow::~MainWindow() = default;

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::ALT && event->key() == Qt::Key_Alt) {
        auto menu_visible = !_ui->menuBar->isVisible();
        _ui->menuBar->setVisible(menu_visible);
        for (auto [_key, pair]: _shortcuts) {
            auto [shortcut, _action] = pair;
            shortcut->setEnabled(!menu_visible);
        }
    }
    QMainWindow::keyPressEvent(event);
}

Pane * MainWindow::add_pane(Position pos) {
    auto pane = new Pane(_video_context, this);

    QObject::connect(
        pane,
        &Pane::remove_requested,
        [=] {
            // For some unknown reasons, the foreign widget doesn't get properly
            // released unless we schedule the removing for later...
            delayed(this, 250, [=] { remove_pane(pane); });
        }
    );

    QObject::connect(
        pane,
        &Pane::zoom_requested,
        [=](bool on) { on ? zoom(pane) : unzoom(); }
    );

    QObject::connect(
        pane,
        &Pane::fullscreen_requested,
        [=](bool on) { set_fullscreen(on); }
    );

    _grid->insert_widget(pos, pane);
    pane->setFocus();
    pane->activateWindow();
    _panes.push_back(pane);

    return pane;
}

void MainWindow::remove_pane(Pane *pane) {
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

Pane * MainWindow::focused_pane() {
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
    for (auto pane: _panes)
        pane->repaint();
}

void MainWindow::move_pane(Position from, Position to) {
    auto focused = focused_pane();
    _grid->swap(from, to);
    focus_pane(focused);
    for (auto pane: _panes)
        pane->repaint();
}

void MainWindow::set_fullscreen(bool on) {
    setWindowState(windowState().setFlag(Qt::WindowFullScreen, on));
    _action_fullscreen->setChecked(on);
    for (auto pane: _panes)
        pane->stream()->video()->controls().set_fullscreen(on);
}

bool MainWindow::is_zoomed() {
    return _central_widget->currentWidget() != _grid;
}

void MainWindow::zoom(Pane *pane) {
    if (is_zoomed()) {
        _action_stream_zoom->setChecked(true);
        return ;
    }

    _zoomed_position = _grid->widget_position(pane);
    _central_widget->addWidget(pane);
    _central_widget->setCurrentWidget(pane);
    _action_stream_zoom->setChecked(true);

    for (auto pane: _panes) {
        pane->stream()->video()->controls().set_zoomed(true);
        pane->stream()->video()->hint_layout_change();
    }
}

void MainWindow::unzoom() {
    if (!is_zoomed()) {
        _action_stream_zoom->setChecked(false);
        return ;
    }

    auto zoomed_pane = _central_widget->currentWidget();
    _central_widget->removeWidget(zoomed_pane);
    _grid->insert_widget(_zoomed_position, zoomed_pane);
    zoomed_pane->show();
    focus_pane(zoomed_pane);

    // Windows workaround for weird display issues
    if (auto active_pane = focused_pane(); active_pane)
        active_pane->stream()->chat()->redraw();

    _action_stream_zoom->setChecked(false);
    for (auto pane: _panes)
        pane->stream()->video()->controls().set_zoomed(false);
}

void MainWindow::setup_audio_devices() {
    auto active_pane = focused_pane();

    if (!active_pane)
        return ;

    _audio_devices_menu->clear();
    auto & mp = active_pane->stream()->video()->media_player();
    auto current_device_id = mp.get_current_device_id();
    for (auto device: mp.audio_devices()) {
        auto description = QString::fromStdString(device.description);
        auto action = _audio_devices_menu->addAction(description);
        if (device.id == current_device_id) {
            action->setCheckable(true);
            action->setChecked(true);
        }
        QObject::connect(action, &QAction::triggered, [=] {
            if (auto active_pane = focused_pane(); active_pane) {
                active_pane->stream()->video()->media_player()
                    .set_audio_device(device.id);
            }
        });
    }
}
