#include "widgets/filters_tool.hpp"
#include "ui_filters_tool.h"

#include "libvlc.hpp"

FiltersTool::FiltersTool(libvlc::MediaPlayer &mp, QWidget *parent):
    QWidget(parent),
    _ui(new Ui::FiltersTool)
{
    _ui->setupUi(this);
    setWindowFlags(Qt::Tool);

    _ui->videoFiltersGroupBox->setChecked(mp.video_filters_enabled());

    _ui->sliderContrast->setValue(mp.get_contrast() * 100);
    _ui->sliderBrightness->setValue(mp.get_brightness() * 100);
    _ui->sliderHue->setValue(mp.get_hue());
    _ui->sliderSaturation->setValue(mp.get_saturation() * 100);
    _ui->sliderGamma->setValue(mp.get_gamma() * 100);

    QObject::connect(_ui->videoFiltersGroupBox, &QGroupBox::toggled, [&](bool on) {
        mp.enable_video_filters(on);
    });

    QObject::connect(_ui->sliderContrast, &QSlider::valueChanged, [&](int value) {
        mp.set_contrast(static_cast<float>(value) / 100.f);
    });
    QObject::connect(_ui->sliderBrightness, &QSlider::valueChanged, [&](int value) {
        mp.set_brightness(static_cast<float>(value) / 100.f);
    });
    QObject::connect(_ui->sliderHue, &QSlider::valueChanged, [&](int value) {
        mp.set_hue(value);
    });
    QObject::connect(_ui->sliderSaturation, &QSlider::valueChanged, [&](int value) {
        mp.set_saturation(static_cast<float>(value) / 100.f);
    });
    QObject::connect(_ui->sliderGamma, &QSlider::valueChanged, [&](int value) {
        mp.set_gamma(static_cast<float>(value) / 100.f);
    });
}

FiltersTool::~FiltersTool() {
    delete _ui;
}
