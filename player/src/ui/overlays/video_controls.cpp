#include "ui/overlays/video_controls.hpp"
#include "ui_video_controls.h"

#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QWindow>

VideoControls::VideoControls(QWidget *parent):
    QWidget(parent),
    _ui(std::make_unique<Ui::VideoControls>()),
    _appearTimer(new QTimer(this))
{
    _ui->setupUi(this);

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_NoSystemBackground);

    _appearTimer->setInterval(2500);
    _appearTimer->setSingleShot(true);
    QObject::connect(_appearTimer, &QTimer::timeout, [=] {
        hide();
    });

    QObject::connect(_ui->volumeSlider, &QSlider::valueChanged, [=](int vol) {
        _appearTimer->start();
        emit volume_changed(vol);
    });

    using IndexChanged = void (QComboBox::*)(const QString &);
    auto index_changed = static_cast<IndexChanged>(&QComboBox::currentIndexChanged);
    QObject::connect(_ui->qualityCombo, index_changed, [=](auto pick) {
        emit quality_changed(pick);
    });

    using Activated = void (QComboBox::*)(int);
    auto activated = static_cast<Activated>(&QComboBox::activated);
    QObject::connect(_ui->qualityCombo, activated, [=](auto) {
        parentWidget()->activateWindow();
    });

    using Highlighted = void (QComboBox::*)(int);
    auto highlighted = static_cast<Highlighted>(&QComboBox::highlighted);
    QObject::connect(_ui->qualityCombo, highlighted, [=](auto) {
        _appearTimer->start();
    });
}

VideoControls::~VideoControls() = default;

void VideoControls::set_volume(int volume) {
    QSignalBlocker blocker(this);

    _ui->volumeSlider->setValue(volume);
}

void VideoControls::set_muted(bool muted) {
    _muted = muted;
    set_volume_icon();
}

void VideoControls::set_zoomed(bool zoomed) {
    _zoomed = zoomed;
    set_zoomed_icon();
}

void VideoControls::set_fullscreen(bool fullscreen) {
    _fullscreen = fullscreen;
    set_fullscreen_icon();
}

void VideoControls::appear() {
    show();
    _appearTimer->start();
}

void VideoControls::clear_qualities() {
    QSignalBlocker blocker(this);

    _ui->qualityCombo->clear();
}

void VideoControls::set_qualities(QString quality, QStringList qualities) {
    QSignalBlocker blocker(this);

    _ui->qualityCombo->addItems(qualities);
    _ui->qualityCombo->setCurrentText(quality);
}

void VideoControls::set_volume_icon() {
    auto icon_path = _muted ? ":/icons/volume_off.png" : ":/icons/volume_on.png";
    _ui->volumeLabel->setPixmap(QPixmap(icon_path));
}

void VideoControls::set_zoomed_icon() {
    auto icon_path = _zoomed ? ":/icons/unzoom.png" : ":/icons/zoom.png";
    _ui->zoomLabel->setPixmap(QPixmap(icon_path));
}

void VideoControls::set_fullscreen_icon() {
    auto icon_path = _fullscreen ? ":/icons/fullscreen_exit.png" : ":/icons/fullscreen_enter.png";
    _ui->fullscreenLabel->setPixmap(QPixmap(icon_path));
}

void VideoControls::mousePressEvent(QMouseEvent *event) {
    auto widget_clicked = [=](auto widget) {
        return widget->rect().contains(event->pos() - widget->pos());
    };

    if (widget_clicked(_ui->volumeLabel)) {
        _muted = !_muted;
        emit muted_changed(_muted);
    }
    else if (widget_clicked(_ui->fastForwardLabel)) {
        emit fast_forward();
    }
    else if (widget_clicked(_ui->layoutLeftLabel)) {
        emit layout_left_requested();
    }
    else if (widget_clicked(_ui->layoutRightLabel)) {
        emit layout_right_requested();
    }
    else if (widget_clicked(_ui->zoomLabel)) {
        _zoomed = !_zoomed;
        emit zoom_requested(_zoomed);
    }
    else if (widget_clicked(_ui->fullscreenLabel)) {
        _fullscreen = !_fullscreen;
        emit fullscreen_requested(_fullscreen);
    }
    else if (widget_clicked(_ui->browseLabel)) {
        emit browse_requested();
    }
    else if (widget_clicked(_ui->removeLabel)) {
        emit remove_requested();
    }

    QWidget::mousePressEvent(event);
    parentWidget()->activateWindow();
}

void VideoControls::paintEvent(QPaintEvent *event) {
    QPainter painter { this };

    QLinearGradient grad(rect().topLeft(), rect().bottomLeft());
    grad.setColorAt(0, QColor(0, 0, 0, 0x00));
    grad.setColorAt(1, QColor(0, 0, 0, 0xB0));

    painter.fillRect(rect(), QBrush(grad));

    QWidget::paintEvent(event);
}
