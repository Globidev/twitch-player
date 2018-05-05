#include "widgets/video_controls.hpp"
#include "ui_video_controls.h"

#include <QTimer>
#include <QMouseEvent>
#include <QPainter>
#include <QWindow>

VideoControls::VideoControls(QWidget *parent):
    QWidget(parent),
    _ui(new Ui::VideoControls),
    _appearTimer(new QTimer(this))
{
    _ui->setupUi(this);

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::FocusPolicy::NoFocus);
    // setAttribute(Qt::WA_TransparentForMouseEvents);

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

    using Highlighted = void (QComboBox::*)(int);
    auto highlighted = static_cast<Highlighted>(&QComboBox::highlighted);
    QObject::connect(_ui->qualityCombo, highlighted, [=](auto) {
        _appearTimer->start();
    });
}

VideoControls::~VideoControls() {
    delete _ui;
}

void VideoControls::set_volume(int volume) {
    _ui->volumeSlider->setValue(volume);
}

void VideoControls::set_muted(bool muted) {
    _muted = muted;
    set_volume_icon();
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
    QWidget::mousePressEvent(event);
}

void VideoControls::changeEvent(QEvent *event) {
    if (event->type() == QEvent::ActivationChange
        && isActiveWindow()
        && window()->windowHandle()->isActive()
        && parentWidget()->hasFocus())
    {
        emit activated();
    }

    QWidget::changeEvent(event);
}

void VideoControls::paintEvent(QPaintEvent *event) {
    QPainter painter { this };

    QLinearGradient grad(rect().topLeft(), rect().bottomLeft());
    grad.setColorAt(0, QColor(0, 0, 0, 0x00));
    grad.setColorAt(1, QColor(0, 0, 0, 0xB0));

    painter.fillRect(rect(), QBrush(grad));

    QWidget::paintEvent(event);
}
