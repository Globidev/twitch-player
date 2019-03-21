#pragma once

#include <QWidget>

#include "api/twitch.hpp"

namespace Ui {
    class StreamCard;
}

class QHBoxLayout;
class QLabel;

class StreamCard: public QWidget {
    Q_OBJECT

public:
    StreamCard(StreamData, QWidget * = nullptr);
    ~StreamCard();

protected:
    void mousePressEvent(QMouseEvent *) override;

signals:
    void clicked(QString);

private:
    std::unique_ptr<Ui::StreamCard> _ui;

    QWidget *_uptime_widget;
    QHBoxLayout *_uptime_layout;
    QLabel *_uptime_logo;
    QLabel *_uptime_label;

    StreamData _data;
};
