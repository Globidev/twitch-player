#include "widgets/stream_card.hpp"
#include "ui_stream_card.h"

#include <QHBoxLayout>

#include <QNetworkAccessManager>
#include <QNetworkReply>

StreamCard::StreamCard(StreamData data, QWidget *parent):
    QWidget(parent),
    _ui(new Ui::StreamCard),
    _data(data)
{
    _ui->setupUi(this);

    _ui->channelName->setText(data.channel.display_name);
    _ui->title->setText(data.channel.title);
    _ui->gameName->setText(data.current_game);
    _ui->viewerCount->setText(QString("%1 viewers").arg(data.viewcount));

    auto http_client = new QNetworkAccessManager(this);
    http_client->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    auto preview_reply = http_client->get(QNetworkRequest(QUrl(data.preview)));

    QObject::connect(preview_reply, &QNetworkReply::finished, [=] {
        auto data = preview_reply->readAll();
        _ui->preview->setPixmap(QPixmap::fromImage(QImage::fromData(data)));
        preview_reply->deleteLater();
    });
}

StreamCard::~StreamCard() {
    delete _ui;
}

void StreamCard::mousePressEvent(QMouseEvent *) {
    emit clicked(_data.channel.name);
}
