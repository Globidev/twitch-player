#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "widgets/vlc_log_viewer.hpp"

#include "native/capabilities.hpp"

#include <QKeyEvent>
#include <QShortcut>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _main_splitter(new QSplitter(Qt::Vertical, this)),
    _ui(new Ui::MainWindow),
    _vlc_log_viewer(new VLCLogViewer(_video_context))
{
    _ui->setupUi(this);
    _ui->verticalLayout->addWidget(_main_splitter);

    QObject::connect(
        _ui->actionFullScreen,
        &QAction::triggered,
        [this] { toggle_fullscreen(); }
    );

    QObject::connect(
        _ui->actionToggleWIndowBorders,
        &QAction::triggered,
        [this] {
            auto win_handle = reinterpret_cast<WindowHandle>(window()->winId());
            toggle_window_borders(win_handle);
        }
    );

    QObject::connect(
        _ui->actionLogs,
        &QAction::triggered,
        [this] { _vlc_log_viewer->show(); }
    );

    QObject::connect(
        _ui->actionAddStreamHorizontally,
        &QAction::triggered,
        [this] { add_picker(0, rows[0]->count()); }
    );

    QObject::connect(
        _ui->actionAddStreamVertically,
        &QAction::triggered,
        [this] { add_picker(rows.size(), 0); }
    );
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

void MainWindow::add_picker(int row, int col) {
    QSplitter *sp;
    if (row >= rows.size()) {
        sp = new QSplitter(this);
        _main_splitter->addWidget(sp);
        rows.push_back(sp);
    } else {
        sp = rows[row];
    }

    auto stream_container = new StreamContainer(_video_context, this);

    sp->insertWidget(col, stream_container);
    _streams.push_back(stream_container);
}

void MainWindow::add_stream(int row, int col, QString channel) {
    add_picker(row, col);
    _streams.back()->play(channel);
}

void MainWindow::toggle_fullscreen() {
    auto state = windowState();
    state.setFlag(Qt::WindowState::WindowFullScreen, !isFullScreen());
    setWindowState(state);
}
