#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "widgets/vlc_log_viewer.hpp"
#include "widgets/options_dialog.hpp"

#include "native/capabilities.hpp"

#include "constants.hpp"

#include <QKeyEvent>
#include <QSplitter>
#include <QSettings>
#include <QMessageBox>

static auto to_string_vector(const QStringList & list) {
    auto qstring_vector = list.toVector();
    std::vector<std::string> string_vector;

    std::transform(
        qstring_vector.begin(),
        qstring_vector.end(),
        std::back_inserter(string_vector),
        [](auto & str) { return str.toStdString(); }
    );

    return string_vector;
}

static auto borrow_string_data(const std::vector<std::string> & string_vec) {
    std::vector<const char*> borrowed;

    std::transform(
        string_vec.begin(),
        string_vec.end(),
        std::back_inserter(borrowed),
        [](auto & str) { return str.c_str(); }
    );

    return borrowed;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _main_splitter(new QSplitter(Qt::Vertical, this)),
    _ui(new Ui::MainWindow)
{
    using namespace constants::settings::vlc;

    QSettings settings;
    auto vlc_args = settings.value(KEY_VLC_ARGS, DEFAULT_VLC_ARGS)
        .toStringList();
    auto vlc_string_args = to_string_vector(vlc_args);
    auto vlc_argv = borrow_string_data(vlc_string_args);

    auto argc = vlc_argv.size();
    auto argv = vlc_argv.data();
    _video_context = std::make_unique<libvlc::Instance>(argc, argv);
    if (!&*_video_context) {
        QMessageBox::critical(this,
            "libvlc error",
            "Failed to initialize libvlc, resetting options..."
        );
        settings.remove(KEY_VLC_ARGS);
        qApp->exit(-1);
        return;
    }
    _vlc_log_viewer = new VLCLogViewer(*_video_context);

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
        auto [row, col] = find_focused_stream();
        add_picker(row, col + 1);
    });
    add_action(_ui->actionAddStreamVertically, [this] {
        auto [row, col] = find_focused_stream();
        add_picker(row + 1, col);
    });
    add_action(_ui->actionRemoveStream, [this] {
        auto [row, col] = find_focused_stream();
        if (row < rows.size()) {
            if (col < rows[row]->count()) {
                auto stream_widget = rows[row]->widget(col);
                _streams.erase(static_cast<StreamContainer *>(stream_widget));
                stream_widget->deleteLater();
            }
        }
    });

    add_action(_ui->actionLogs, [this] {
        _vlc_log_viewer->show();
        _vlc_log_viewer->raise();
    });

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

void MainWindow::mousePressEvent(QMouseEvent *) {
    for (auto s: _streams)
        s->repaint();
}

void MainWindow::add_picker(int row, int col) {
    QSplitter *sp;
    if (row >= rows.size()) {
        sp = new QSplitter(this);
        _main_splitter->addWidget(sp);
        if (_main_splitter->count() > 1) {
            QList<int> sizes;
            for (auto i = 0; i < _main_splitter->count(); ++i)
                sizes << 100;
            _main_splitter->setSizes(sizes);
        }
        rows.push_back(sp);
    } else {
        sp = rows[row];
    }

    auto stream_container = new StreamContainer(*_video_context, this);

    sp->insertWidget(col, stream_container);
    if (sp->count() > 1) {
        QList<int> sizes;
        for (auto i = 0; i < sp->count(); ++i)
            sizes << 100;
        sp->setSizes(sizes);
    }
    _streams.insert(stream_container);
    stream_container->setFocus();
    stream_container->activateWindow();
}

void MainWindow::add_stream(int row, int col, QString channel) {
    add_picker(row, col);
    (*_streams.rbegin())->play(channel);
}

void MainWindow::toggle_fullscreen() {
    auto state = windowState();
    state.setFlag(Qt::WindowState::WindowFullScreen, !isFullScreen());
    setWindowState(state);
}

std::pair<int, int> MainWindow::find_focused_stream() {
    auto focusedWidget = qApp->focusWidget();

    for (auto row = 0; row < rows.size(); ++row) {
        for (auto col = 0; col < rows[row]->count(); ++col) {
            if (rows[row]->widget(col)->isAncestorOf(focusedWidget))
                return { row, col };
        }
    }

    return { 0, 0 };
}
