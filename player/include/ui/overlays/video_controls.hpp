#pragma once

#include <QWidget>

#include <memory>

namespace Ui {
class VideoControls;
}

class QTimer;

class VideoControls: public QWidget {
    Q_OBJECT

public:
    VideoControls(QWidget * = nullptr);
    ~VideoControls();

    void set_volume(int);
    void set_muted(bool);

    void set_zoomed(bool);
    void set_fullscreen(bool);

    void appear();

    void clear_qualities();
    void set_qualities(QString, QStringList);

    void set_delay(float);

protected:
    void mousePressEvent(QMouseEvent *) override;
    void paintEvent(QPaintEvent *) override;

signals:
    void fast_forward();
    void muted_changed(bool);
    void volume_changed(int);
    void quality_changed(QString);

    void layout_left_requested();
    void layout_right_requested();
    void zoom_requested(bool);
    void fullscreen_requested(bool);

    void browse_requested();
    void remove_requested();

private:
    void set_volume_icon();
    void set_zoomed_icon();
    void set_fullscreen_icon();

    std::unique_ptr<Ui::VideoControls> _ui;

    QTimer *_appearTimer;

    bool _muted = false;
    bool _fullscreen = false;
    bool _zoomed = false;
};
