#pragma once

#include <QWidget>
#include <QImage>

#include <optional>

class VideoDetails: public QWidget {
public:
    VideoDetails(QWidget * = nullptr);

    void show_state(const QString &);
    void set_buffering(bool);
    void show_stream_info();

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    QString _state_text;
    QTimer *_state_timer;
    QFont _state_text_font;
    bool _show_state_text = false;

    QImage _spinner;
    QTimer *_spinner_timer;
    int _spinner_angle = 0;
    bool _show_spinner = false;

    void draw_state_text();
    void draw_spinner();
};
