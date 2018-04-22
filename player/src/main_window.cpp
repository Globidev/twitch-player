#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_widget.hpp"

#include "win.hpp"

#include <QProcess>
#include <QSplitter>
#include <QTimer>
#include <QShortcut>

static const auto CHROME_PATH =
    R"(C:\Program Files (x86)\Google\Chrome\Application\chrome.exe)";

static auto find_chat_windows() {
    return find_windows_by_title_and_pname("Twitch", CHROME_PATH);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
    _main_splitter(new QSplitter(Qt::Vertical, this)),
{
    ui->setupUi(this);

    auto sh_full_screen = new QShortcut(QKeySequence::FullScreen, this);
    QObject::connect(sh_full_screen, &QShortcut::activated, [this] {
        auto state = windowState();
        if (isFullScreen()) state &= ~Qt::WindowState::WindowFullScreen;
        else                state |=  Qt::WindowState::WindowFullScreen;
        setWindowState(state);
    });
}

MainWindow::MainWindow(QString channel, QWidget *parent):
    MainWindow(parent)
{
    add_stream(channel);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::add_stream(QString channel, int row, int col) {
    auto pre_launch_windows = find_chat_windows();
    QProcess::startDetached(CHROME_PATH, QStringList()
        << "--app=https://www.twitch.tv/popout/" + channel + "/chat?popout="
    );

    auto timer = new QTimer { this };
    QObject::connect(timer, &QTimer::timeout, [=] {
        auto windows = find_chat_windows();

        if (!windows.size())
            return;

        FindWindowResult best_candidates;
        std::set_difference(
            windows.begin(), windows.end(),
            pre_launch_windows.begin(), pre_launch_windows.end(),
            std::inserter(best_candidates, best_candidates.begin())
        );

        if (!best_candidates.size())
            return;

        auto chat_window_handle = *best_candidates.begin();

        auto widget = new StreamWidget { _video_context, chat_window_handle, this };
        ui->gridLayout->addWidget(widget, 0, 0);
        widget->video.play(channel);
        streams.insert(widget);
        timer->stop();
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
