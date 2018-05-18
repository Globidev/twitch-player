#pragma once

#include <QWidget>

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

    void appear();

    void clear_qualities();
    void set_qualities(QString, QStringList);

protected:
    void mousePressEvent(QMouseEvent *) override;
    void paintEvent(QPaintEvent *) override;

signals:
    void volume_changed(int);
    void muted_changed(bool);
    void fast_forward();
    void quality_changed(QString);

private:
    void set_volume_icon();

    std::unique_ptr<Ui::VideoControls> _ui;

    QTimer *_appearTimer;

    bool _muted = false;
};
