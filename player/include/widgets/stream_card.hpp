#ifndef STREAM_CARD_HPP
#define STREAM_CARD_HPP

#include <QWidget>

#include "api/twitch.hpp"

namespace Ui {
    class StreamCard;
}

class StreamCard: public QWidget {
    Q_OBJECT

public:
    StreamCard(StreamData, QWidget * = nullptr);
    StreamCard::~StreamCard();

protected:
    void mousePressEvent(QMouseEvent *) override;

signals:
    void clicked(QString);

private:
    Ui::StreamCard *_ui;
    StreamData _data;
};

#endif // STREAM_CARD_HPP
