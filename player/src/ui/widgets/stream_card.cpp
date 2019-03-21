#include "ui/widgets/stream_card.hpp"
#include "ui_stream_card.h"

#include <QHBoxLayout>

#include <QNetworkAccessManager>
#include <QNetworkReply>

StreamCard::StreamCard(StreamData data, QWidget *parent):
    QWidget(parent),
    _ui(std::make_unique<Ui::StreamCard>()),
    _uptime_widget(new QWidget(nullptr)),
    _uptime_layout(new QHBoxLayout(_uptime_widget)),
    _uptime_logo(new QLabel(this)),
    _uptime_label(new QLabel(this)),
    _data(data)
{
    _ui->setupUi(this);

    _ui->channelName->setText(data.channel.display_name);
    _ui->title->setText(data.channel.title);
    _ui->gameName->setText(data.current_game);
    _ui->viewerCount->setText(QString("%1 viewers").arg(data.viewcount));

    _uptime_widget->setParent(_ui->preview);
    _uptime_widget->setStyleSheet("background-color: rgba(0, 0, 0, 0.1);");
    _uptime_layout->addWidget(_uptime_logo);
    _uptime_layout->addWidget(_uptime_label);

    _uptime_logo->setPixmap(QPixmap(":/icons/clock.svg"));
    _uptime_logo->setScaledContents(true);
    _uptime_logo->setMaximumSize(16, 16);

    auto uptime_secs = data.created_at.secsTo(QDateTime::currentDateTime());
    auto uptime_hours = uptime_secs / 3600;
    auto uptime_minutes = (uptime_secs - uptime_hours * 3600) / 60;
    _uptime_label->setText(QString("%1:%2").arg(uptime_hours).arg(uptime_minutes, 2, 10, QChar('0')));

    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::white);
    _uptime_label->setPalette(palette);

    _uptime_widget->show();
    _uptime_widget->move(_ui->preview->width() - _uptime_widget->width(), 0);

    auto http_client = new QNetworkAccessManager(this);
    http_client->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    auto preview_reply = http_client->get(QNetworkRequest(QUrl(data.preview)));

    QObject::connect(preview_reply, &QNetworkReply::finished, [=] {
        auto data = preview_reply->readAll();
        _ui->preview->setPixmap(QPixmap::fromImage(QImage::fromData(data)));
        preview_reply->deleteLater();
    });
}

StreamCard::~StreamCard() = default;

void StreamCard::mousePressEvent(QMouseEvent *) {
    emit clicked(_data.channel.name);
}
