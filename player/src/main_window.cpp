#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "widgets/stream_widget.hpp"
#include "widgets/video_widget.hpp"
#include "widgets/foreign_widget.hpp"
#include "widgets/vlc_log_viewer.hpp"
#include "widgets/options_dialog.hpp"

#include "native/capabilities.hpp"

#include "constants.hpp"

#include <QKeyEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QShortcut>

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

    setup_shortcuts();
}

MainWindow::~MainWindow() {
    delete _vlc_log_viewer;
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->modifiers() == Qt::ALT && event->key() == Qt::Key_Alt) {
        auto menu_visible = !_ui->menuBar->isVisible();
        _ui->menuBar->setVisible(menu_visible);
        for (auto [_, pair]: _shortcuts) {
            auto [shortcut, _] = pair;
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
    for (auto pane: _panes)
        pane->repaint();
}

void MainWindow::move_pane(Position from, Position to) {
    auto focused = focused_pane();
    _grid->swap(from, to);
    focus_pane(focused);
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
