#include "widgets/stream_widget.hpp"
#include "widgets/video_widget.hpp"
#include "widgets/foreign_widget.hpp"

#include "libvlc.hpp"
#include "win.hpp"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QSplitter>
#include <QProcess>
#include <QTimer>

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

void StreamWidget::play(QString channel) {
    _video->play(channel);

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
            _chat->grab(*handle_it);
            timer->deleteLater();
        }
    });
    timer->start(250);
}

void StreamWidget::keyPressEvent(QKeyEvent * event) {
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
        switch (event->key()) {
            case Qt::Key_Left:
                reposition(ChatPosition::Left);
                break;
            case Qt::Key_Right:
                reposition(ChatPosition::Right);
                break;
        }
    }
    QWidget::keyPressEvent(event);
}

void StreamWidget::reposition(ChatPosition position) {
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
