#include "widgets/video_widget.hpp"

#include "widgets/video_controls.hpp"

#include "vlc_event_watcher.hpp"

#include "constants.hpp"
#include "native/capabilities.hpp"

#include <QApplication>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QShortcut>
#include <QSettings>
#include <QTimer>

template <class Slot>
static void delayed(VideoWidget *parent, int ms, Slot slot) {
    auto timer = new QTimer(parent);
    QObject::connect(timer, &QTimer::timeout, slot);
    timer->setSingleShot(true);
    timer->start(ms);
}

static auto quality_names(const StreamIndex & index) {
    QStringList qualities;

    for (auto pl_info: index.playlist_infos)
        qualities << pl_info.media_info.name;

    return qualities;
}

VideoWidget::VideoWidget(libvlc::Instance &instance, QWidget *parent):
    QWidget(parent),
    _instance(instance),
    _media_player(libvlc::MediaPlayer(instance)),
    _overlay(new VideoOverlay(this)),
    _controls(new VideoControls(this)),
    _move_filter(new MovementFilter(*this)),
    _event_watcher(new VLCEventWatcher(_media_player, this))
{
    window()->installEventFilter(_move_filter);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFocusPolicy(Qt::WheelFocus);
    _media_player.set_renderer((void *)winId());
    _media_player.set_volume(_vol);
    update_overlay_position();
    setMinimumSize(160, 90);
    setMouseTracking(true);

    _controls->set_volume(_vol);
    QObject::connect(_controls, &VideoControls::volume_changed, [=](int vol) {
        set_volume(vol);
        activateWindow();
    });
    QObject::connect(_controls, &VideoControls::muted_changed, [=](bool muted) {
        set_muted(muted);
    });
    QObject::connect(_controls, &VideoControls::fast_forward, [=] {
        fast_forward();
    });
    QObject::connect(_controls, &VideoControls::quality_changed, [=](auto quality) {
        play(_current_channel, quality);
        activateWindow();
    });

    QObject::connect(_event_watcher, &VLCEventWatcher::new_event, [=](auto event, float f) {
        using namespace libvlc;
        constexpr auto event_name = [](auto ev) {
            switch (ev) {
            case MediaPlayer::Event::MediaChanged: return "MediaChanged";
            case MediaPlayer::Event::Opening: return "Opening";
            case MediaPlayer::Event::Buffering: return "Buffering";
            case MediaPlayer::Event::Playing: return "Playing";
            case MediaPlayer::Event::Paused: return "Paused";
            case MediaPlayer::Event::Stopped: return "Stopped";
            case MediaPlayer::Event::Forward: return "Forward";
            case MediaPlayer::Event::Backward: return "Backward";
            case MediaPlayer::Event::EndReached: return "EndReached";
            case MediaPlayer::Event::EncounteredError: return "EncounteredError";
            case MediaPlayer::Event::Muted: return "Muted";
            case MediaPlayer::Event::Unmuted: return "Unmuted";
            case MediaPlayer::Event::AudioVolume: return "AudioVolume";
            case MediaPlayer::Event::AudioDevice: return "AudioDevice";
            case MediaPlayer::Event::ScrambledChanged: return "ScrambledChanged";
            case MediaPlayer::Event::Corked: return "Corked";
            case MediaPlayer::Event::Uncorked: return "Uncorked";
            case MediaPlayer::Event::Unknown: return "Unknown";
            }
        };

        qDebug() << "EVENT:" << event_name(event) << f;
        switch (event) {
            case MediaPlayer::Event::Opening:
                _overlay->set_buffering(true);
                break;
            case MediaPlayer::Event::Buffering:
                _overlay->set_buffering(f != 100.f);
                break;
            case MediaPlayer::Event::EndReached:
            case MediaPlayer::Event::Stopped:
            case MediaPlayer::Event::EncounteredError:
                play(_current_channel, _current_quality);
                break;
        }
    });
}

void VideoWidget::play(QString channel, QString quality) {
    _current_channel = channel;
    _current_quality = quality;

    auto location = TwitchdAPI::playback_url(channel, quality);

    _media.emplace(_instance, location.toStdString().c_str());
    _media_player.set_media(*_media);
    _media_player.play();

    _controls->clear_qualities();
    if (stream_index_reponse)
        stream_index_reponse->cancel();
    stream_index_reponse = _api.stream_index(channel);
    stream_index_reponse->then([=](auto index) {
        auto qualities = quality_names(index);
        _controls->set_qualities(quality, qualities);
        stream_index_reponse.reset();
    });

    _overlay->show();

    if (!quality.isEmpty()) {
        using namespace constants::settings::streams;
        QSettings settings;
        settings.setValue(KEY_LAST_QUALITY_FOR(channel), quality);
    }
}

int VideoWidget::volume() const {
    return _vol;
}

void VideoWidget::set_volume(int volume) {
    _vol = volume;
    _media_player.set_volume(_muted ? 0 : _vol);
    _controls->set_volume(_vol);
    _overlay->show_text(QString::number(_vol) + " %");
}

bool VideoWidget::muted() const {
    return _muted;
}

void VideoWidget::set_muted(bool muted) {
    _muted = muted;
    set_volume(_vol);
    _controls->set_muted(_muted);
    _overlay->show_text(muted ? "Muted" : "Unmuted");
}

void VideoWidget::fast_forward() {
    _media_player.set_position(1.0f);
    _overlay->show_text("Fast forward...");
}

libvlc::MediaPlayer & VideoWidget::media_player() {
    return _media_player;
}

void VideoWidget::update_overlay_position() {
    auto top_left = mapToGlobal(pos()) - pos();
    auto bottom_left = top_left + QPoint(0, height());

    _overlay->resize(size());
    _overlay->move(top_left);
    _controls->resize(width(), _controls->height());
    _controls->move(bottom_left - QPoint(0, _controls->height()));
}

void VideoWidget::wheelEvent(QWheelEvent *event) {
    setFocus();
    auto delta = qApp->keyboardModifiers().testFlag(Qt::ShiftModifier)
               ? 1 : 5;
    int new_volume;
    if (event->angleDelta().y() > 0)
        new_volume = std::min(_vol + delta, 200);
    else
        new_volume = std::max(0, _vol - delta);
    set_volume(new_volume);
    QWidget::wheelEvent(event);
}

void VideoWidget::resizeEvent(QResizeEvent *event) {
    update_overlay_position();
}

void VideoWidget::showEvent(QShowEvent *event) {
    // have to actually wait for it to be shown ...
    delayed(this, 250, [this] { update_overlay_position(); });
}

void VideoWidget::mousePressEvent(QMouseEvent *event) {
    _controls->appear();

    if (event->buttons().testFlag(Qt::MouseButton::LeftButton))
        _last_drag_position = event->globalPos();

    QWidget::mousePressEvent(event);
}

void VideoWidget::mouseMoveEvent(QMouseEvent *event) {
    _controls->appear();

    if (event->buttons().testFlag(Qt::MouseButton::LeftButton)) {
        auto delta = event->globalPos() - _last_drag_position;
        if (delta.manhattanLength() > 10) {
            window()->move(window()->pos() + delta);
            _last_drag_position = event->globalPos();
        }
    }
    else
        QWidget::mouseMoveEvent(event);
}

void VideoWidget::keyPressEvent(QKeyEvent *event) {
    QWidget::keyPressEvent(event);
    // keypresses might trigger parent's shortcuts which might modify the
    // overlay. It's hard to detect without adding strong coupling with the
    // parent so for now let's just schedule an overlay update for the next
    // event loop iteration
    delayed(this, 0, [this] { update_overlay_position(); });
}

MovementFilter::MovementFilter(VideoWidget & video_widget):
    QObject(&video_widget),
    _video_widget(video_widget)
{ }

bool MovementFilter::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Type::Move)
        _video_widget.update_overlay_position();
    return QObject::eventFilter(watched, event);
}

VideoOverlay::VideoOverlay(QWidget *parent):
    QWidget(parent),
    _timer(new QTimer(this)),
    _spinner_timer(new QTimer(this)),
    _spinner(":/images/kappa.png")
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    _text_font.setBold(true);
    _text_font.setPointSize(40);
    _text_font.setWeight(40);

    _timer->setInterval(2500);
    QObject::connect(_timer, &QTimer::timeout, [this] {
        _text.reset();
        repaint();
    });

    _spinner_timer->setInterval(20);
    QObject::connect(_spinner_timer, &QTimer::timeout, [this] {
        _spinner_angle = (_spinner_angle + 6) % 360;
        repaint();
    });
}

void VideoOverlay::mouseReleaseEvent(QMouseEvent *event) {
    event->ignore();
    parentWidget()->activateWindow();
};

void VideoOverlay::paintEvent(QPaintEvent *event) {
    QPainter painter { this };

    painter.setFont(_text_font);
    painter.setPen(Qt::white);

    if (_text) {
        QRect br;
        painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, *_text, &br);

        QLinearGradient gradient(br.topLeft(), br.bottomLeft());
        gradient.setColorAt(0, QColor(0, 0, 0, 0xB0));
        gradient.setColorAt(1, QColor(0, 0, 0, 0x00));

        painter.fillRect(br, QBrush(gradient));
        painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, *_text);
    }

    if (_show_spinner) {
        auto centered_rect = _spinner.rect();
        auto aspect_ratio = (float)centered_rect.height() / centered_rect.width();
        if (rect().width() < centered_rect.width()) {
            auto new_width = rect().width();
            centered_rect.setWidth(new_width);
            centered_rect.setHeight(new_width * aspect_ratio);
        }
        if (rect().height() < centered_rect.height()) {
            auto new_height = rect().height();
            centered_rect.setHeight(new_height);
            centered_rect.setWidth(new_height / aspect_ratio);
        }
        centered_rect.moveCenter(rect().center());
        painter.translate(rect().center());
        painter.rotate(_spinner_angle);
        painter.drawImage(
            centered_rect.translated(-rect().center()),
            _spinner
        );
    }
}

void VideoOverlay::show_text(QString new_text) {
    _text.emplace(new_text);
    _timer->start();
    repaint();
}

void VideoOverlay::set_buffering(bool enabled) {
    if (enabled == _show_spinner)
        return ;
    _show_spinner = enabled;
    _spinner_angle = 0;
    if (enabled) _spinner_timer->start();
    else         _spinner_timer->stop();
    repaint();
}
