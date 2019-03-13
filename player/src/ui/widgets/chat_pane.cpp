#include "ui/widgets/chat_pane.hpp"
#include "ui/widgets/foreign_widget.hpp"
#include "ui/native/capabilities.hpp"

#include "constants.hpp"

#include <QHBoxLayout>
#include <QSettings>
#include <QProcess>
#include <QMessageBox>
#include <QTimer>

#include <regex>

constexpr auto border_width = 1;

static auto find_chat_windows(QString renderer_path) {
    using namespace constants::settings::chat_renderer;

    auto normalized_path = renderer_path.replace("/", "\\").toStdString();

    QSettings settings;
    auto title_hint = settings
        .value(KEY_CHAT_RENDERER_TITLE_HINT,DEFAULT_CHAT_RENDERER_TITLE_HINT)
        .toString();
    auto title_hint_pattern = std::regex { title_hint.toLocal8Bit().data() };

    return find_windows_by_title_and_pname(title_hint_pattern, normalized_path);
}

static auto find_new_chat_windows(QString renderer_path,
                                  const FindWindowResult & previous_results) {
    auto new_results = find_chat_windows(renderer_path);

    FindWindowResult new_windows;
    std::set_difference(
        new_results.begin(), new_results.end(),
        previous_results.begin(), previous_results.end(),
        std::inserter(new_windows, new_windows.begin())
    );

    return new_windows;
}

ChatPane::ChatPane(QWidget *parent):
    QWidget(parent),
    _layout(new QHBoxLayout(this)),
    _chat(new ForeignWidget(this))
{
    _layout->addWidget(_chat);
    setLayout(_layout);

    auto margins = QMargins(border_width, border_width, border_width, border_width);
    _layout->setContentsMargins(margins);
    setContentsMargins(margins);

    using namespace constants::settings::chat_renderer;

    QSettings settings;

    auto renderer_path = settings
        .value(KEY_CHAT_RENDERER_PATH, DEFAULT_CHAT_RENDERER_PATH)
        .toString();
    auto raw_renderer_args = settings
        .value(KEY_CHAT_RENDERER_ARGS, DEFAULT_CHAT_RENDERER_ARGS)
        .toStringList();
    QStringList renderer_args;
    for (auto raw_arg: raw_renderer_args)
        renderer_args.append(raw_arg.replace("${channel}", "none"));
    auto pre_launch_windows = find_chat_windows(renderer_path);
    bool started = QProcess::startDetached(renderer_path, renderer_args);

    if (!started)
        QMessageBox::critical(
            window(),
            "Error starting chat renderer",
            QString("Failed to start the chat process: %1").arg(renderer_path)
        );
    else {
        auto timer = new QTimer(this);
        QObject::connect(timer, &QTimer::timeout, [=] {
            auto chat_windows = find_new_chat_windows(renderer_path, pre_launch_windows);

            if (auto handle_it = chat_windows.begin();
                handle_it != chat_windows.end())
            {
                _chat->grab(*handle_it);
                activateWindow();
                timer->deleteLater();
            }
        });
        timer->start(250);
    }
}

ChatPane::~ChatPane() = default;
