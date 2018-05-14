#include "ui/widgets/stream_widget.hpp"

#include "ui/widgets/video_widget.hpp"
#include "ui/widgets/foreign_widget.hpp"

#include "libvlc/bindings.hpp"
#include "constants.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QSplitter>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QMessageBox>

static auto find_chat_windows(QString renderer_path) {
    auto normalized_path = renderer_path.replace("/", "\\").toStdString();
    return find_windows_by_title_and_pname("Twitch", normalized_path);
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

StreamWidget::StreamWidget(libvlc::Instance &inst, QWidget *parent):
    QWidget(parent),
    _splitter(new QSplitter(this)),
    _layout(new QHBoxLayout(this)),
    _video(new VideoWidget(inst, this)),
    _chat(new ForeignWidget(this))
{
    setLayout(_layout);

    _layout->setContentsMargins(QMargins());
    _layout->addWidget(_splitter);

    _splitter->addWidget(_video);
    _splitter->addWidget(_chat);
    _splitter->setSizes(QList<int>() << _video_size << _chat_size);
}

void StreamWidget::play(QString channel, QString quality) {
    using namespace constants::settings::chat_renderer;

    _video->play(channel, quality);

    QSettings settings;
    auto renderer_path = settings
        .value(KEY_CHAT_RENDERER_PATH, DEFAULT_CHAT_RENDERER_PATH)
        .toString();
    auto raw_renderer_args = settings
        .value(KEY_CHAT_RENDERER_ARGS, DEFAULT_CHAT_RENDERER_ARGS)
        .toStringList();
    QStringList renderer_args;
    for (auto raw_arg: raw_renderer_args)
        renderer_args.append(raw_arg.replace("${channel}", channel));
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

void StreamWidget::reposition_chat(ChatPosition position) {
    // Save sizes
    if (_chat->isVisible()) {
        _chat_size = _splitter->sizes()[_splitter->indexOf(_chat)];
        _video_size = _splitter->sizes()[_splitter->indexOf(_video)];
    }

    // Potentially swap the layout
    switch (position) {
        case ChatPosition::Right: _splitter->insertWidget(1, _chat); break;
        case ChatPosition::Left: _splitter->insertWidget(0, _chat); break;
    }

    // Restore sizes
    if (_chat_position != position) {
        QList<int> new_sizes;
        new_sizes.insert(_splitter->indexOf(_chat), _chat_size);
        new_sizes.insert(_splitter->indexOf(_video), _video_size);
        _splitter->setSizes(new_sizes);
    }

    _chat->setVisible(!_chat->isVisible() || _chat_position != position);

    _chat_position = position;
}

void StreamWidget::resize_chat(ChatPosition position) {
    _chat_size = _splitter->sizes()[_splitter->indexOf(_chat)];
    _video_size = _splitter->sizes()[_splitter->indexOf(_video)];

    auto delta = (_chat_size + _video_size) * 2 / 100;

    if (position == _chat_position) {
        _chat_size -= delta; _video_size += delta;
    }
    else {
        _chat_size += delta; _video_size -= delta;
    }

    QList<int> new_sizes;
    new_sizes.insert(_splitter->indexOf(_chat), _chat_size);
    new_sizes.insert(_splitter->indexOf(_video), _video_size);
    _splitter->setSizes(new_sizes);
}

VideoWidget *StreamWidget::video() const {
    return _video;
}

ForeignWidget *StreamWidget::chat() const {
    return _chat;
}
