#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_widget.hpp"
#include "widgets/video_widget.hpp"

#include "win.hpp"

#include <QProcess>
#include <QTimer>
#include <QKeyEvent>
#include <QShortcut>

static const auto CHROME_PATH =
    R"(C:\Program Files (x86)\Google\Chrome\Application\chrome.exe)";

static auto find_chat_windows() {
    return find_windows_by_title_and_pname("Twitch", CHROME_PATH);
}

static auto find_new_chat_windows(const FindWindowResult & previous_results) {
    auto new_results = find_chat_windows();

    FindWindowResult new_windows;
    std::set_difference(
        new_results.begin(), new_results.end(),
        previous_results.begin(), previous_results.end(),
        std::inserter(new_windows, new_windows.begin())
    );

    return new_windows;
}

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
        auto handle = (HWND)window()->winId();
        auto style = GetWindowLong(handle, GWL_STYLE);
        SetWindowLong(handle, GWL_STYLE, style ^ (WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU));
        if (GetWindowLong(handle, GWL_STYLE) != style) {
            SetWindowPos(handle, nullptr, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(handle, nullptr, TRUE);
        }
    }
    QMainWindow::keyPressEvent(event);
}

MainWindow::MainWindow(QString channel, QWidget *parent):
    MainWindow(parent)
{
    add_stream(channel, 0, 0);
}

void MainWindow::add_stream(QString channel, int row, int col) {
    auto pre_launch_windows = find_chat_windows();
    QProcess::startDetached(CHROME_PATH, QStringList()
        << "--app=https://www.twitch.tv/popout/" + channel + "/chat?popout="
    );

    auto timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, [=] {
        auto chat_windows = find_new_chat_windows(pre_launch_windows);

        if (auto handle_it = chat_windows.begin();
            handle_it != chat_windows.end())
        {
            add_stream_impl(channel, *handle_it, row, col);
            timer->deleteLater();
        }
    });
    timer->start(250);
}

void MainWindow::add_stream_impl(QString channel, HWND chat_window_handle, int row, int col) {
    QSplitter *sp;
    if (row >= rows.size()) {
        sp = new QSplitter(this);
        _main_splitter->addWidget(sp);
        rows.push_back(sp);
    } else {
        sp = rows[row];
    }

    auto stream_widget = new StreamWidget {
        _video_context,
        chat_window_handle,
        this
    };

    sp->insertWidget(col, stream_widget);
    stream_widget->video->play(channel);
    _streams.push_back(stream_widget);
}

void MainWindow::toggle_fullscreen() {
    auto state = windowState();
    state.setFlag(Qt::WindowState::WindowFullScreen, !isFullScreen());
    setWindowState(state);
}
