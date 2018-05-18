#include "ui/main_window.hpp"
#include "ui_main_window.h"

#include "ui/widgets/pane.hpp"
#include "ui/widgets/stream_widget.hpp"
#include "ui/widgets/video_widget.hpp"
#include "ui/widgets/foreign_widget.hpp"
#include "ui/tools/vlc_log_viewer.hpp"

#include <QStackedWidget>
#include <QKeyEvent>
#include <QShortcut>

static void focus_pane(QWidget *pane) {
    pane->setFocus();
    pane->repaint();
}

MainWindow::MainWindow(libvlc::Instance &video_context, QWidget *parent) :
    QMainWindow(parent),
    _ui(std::make_unique<Ui::MainWindow>()),
    _video_context(video_context),
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

void MainWindow::mousePressEvent(QMouseEvent *event) {
    // Focus might have updated
    for (auto pane: _panes)
        pane->repaint();
    QMainWindow::mousePressEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event) {
    // Focus might have updated
    for (auto pane: _panes)
        pane->repaint();
    QMainWindow::wheelEvent(event);
}

Pane * MainWindow::add_pane(Position pos) {
    auto stream_container = new Pane(_video_context, this);
    _grid->insert_widget(pos, stream_container);
    stream_container->setFocus();
    stream_container->activateWindow();
    _panes.push_back(stream_container);
    return stream_container;
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

    // Windows workaround for weird display issues
    if (auto active_pane = focused_pane(); active_pane)
        active_pane->stream()->chat()->redraw();

    _ui->actionStreamZoom->setChecked(false);
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
