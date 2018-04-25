#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "widgets/vlc_log_viewer.hpp"
#include "widgets/options_dialog.hpp"

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

    auto add_action = [this](auto action, auto slot) {
        QObject::connect(action, &QAction::triggered, slot);
    };

    add_action(_ui->actionFullScreen, [this] { toggle_fullscreen(); });
    add_action(_ui->actionToggleWindowBorders, [this] {
        auto win_handle = reinterpret_cast<WindowHandle>(window()->winId());
        toggle_window_borders(win_handle);
    });
    add_action(_ui->actionAddStreamHorizontally, [this] {
        add_picker(0, rows[0]->count());
    });
    add_action(_ui->actionAddStreamVertically, [this] {
        add_picker(rows.size(), 0);
    });
    add_action(_ui->actionLogs, [this] { _vlc_log_viewer->show(); });

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
