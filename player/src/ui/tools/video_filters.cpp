#include "ui/tools/video_filters.hpp"
#include "ui_filters_tool.h"

#include "libvlc/bindings.hpp"

VideoFilters::VideoFilters(libvlc::MediaPlayer &mp, QWidget *parent):
    QWidget(parent),
    _ui(std::make_unique<Ui::VideoFilters>())
{
    _ui->setupUi(this);
    setWindowFlags(Qt::Tool);

    auto setup_slider = [&](auto factor, auto slider, auto get, auto set) {
        slider->setValue((mp.*get)() * factor);
        auto update = [=, &mp](int value) { (mp.*set)(value / factor); };
        QObject::connect(slider, &QSlider::valueChanged, update);
    };

    _ui->videoFiltersGroupBox->setChecked(mp.video_filters_enabled());
    QObject::connect(_ui->videoFiltersGroupBox, &QGroupBox::toggled, [&](bool on) {
        mp.enable_video_filters(on);
    });

    using MP = libvlc::MediaPlayer;
    setup_slider(100.f, _ui->sliderContrast,   &MP::get_contrast,   &MP::set_contrast);
    setup_slider(100.f, _ui->sliderBrightness, &MP::get_brightness, &MP::set_brightness);
    setup_slider(1,     _ui->sliderHue,        &MP::get_hue,        &MP::set_hue);
    setup_slider(100.f, _ui->sliderSaturation, &MP::get_saturation, &MP::set_saturation);
    setup_slider(100.f, _ui->sliderGamma,      &MP::get_gamma,      &MP::set_gamma);
}
