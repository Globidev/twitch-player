#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "native/capabilities.hpp"

#include <QKeyEvent>
#include <QShortcut>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _main_splitter(new QSplitter(Qt::Vertical, this)),
    _ui(new Ui::MainWindow),
    _sh_full_screen(new QShortcut(QKeySequence::FullScreen, this))
{
    QObject::connect(
        _sh_full_screen,
        &QShortcut::activated,
        [this] { toggle_fullscreen(); }
    );

    _ui->setupUi(this);
    _ui->verticalLayout->addWidget(_main_splitter);
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Alt) {
        auto win_handle = reinterpret_cast<WindowHandle>(window()->winId());
        toggle_window_borders(win_handle);
    }
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
