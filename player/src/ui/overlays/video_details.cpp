#include "ui/overlays/video_details.hpp"
#include "ui_stream_details.h"

#include "prelude/timer.hpp"

#include <QTimer>
#include <QMouseEvent>
#include <QPainter>

#include <QPushButton>

#include "ui/native/capabilities.hpp"

VideoDetails::VideoDetails(QWidget *parent):
    QWidget(parent),
    _state_timer(new QTimer(this)),
    _spinner(":/images/kappa.png"),
    _spinner_timer(new QTimer(this)),
    _stream_details_widget(std::make_unique<QWidget>()),
    _stream_details_ui(std::make_unique<Ui::StreamDetails>()),
    _stream_details_timer(new QTimer(this)),
    _http_client(new QNetworkAccessManager(this))
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    _state_text_font.setBold(true);
    _state_text_font.setPointSize(40);
    _state_text_font.setWeight(40);

    _state_timer->setInterval(2500);
    QObject::connect(_state_timer, &QTimer::timeout, [this] {
        _show_state_text = false;
        repaint();
    });

    _spinner_timer->setInterval(20);
    QObject::connect(_spinner_timer, &QTimer::timeout, [this] {
        _spinner_angle = (_spinner_angle + 6) % 360;
        repaint();
    });

    _stream_details_timer->setInterval(2500);
    QObject::connect(_stream_details_timer, &QTimer::timeout, [this] {
        _show_stream_details = false;
        repaint();
    });

    _stream_details_ui->setupUi(_stream_details_widget.get());

    interval(this, 60 * 1000, [=] { fetch_channel_details(); });

    _http_client->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    set_transparent((WindowHandle)winId());
}

void VideoDetails::mouseReleaseEvent(QMouseEvent *event) {
    event->ignore();
    parentWidget()->activateWindow();
};

void VideoDetails::paintEvent(QPaintEvent *event) {
    if(_show_stream_details && _has_valid_stream_details)
        draw_stream_details();

    if (_show_state_text)
        draw_state_text();

    if (_show_spinner)
        draw_spinner();

    QWidget::paintEvent(event);
}

void VideoDetails::draw_state_text() {
    QPainter painter { this };

    painter.setFont(_state_text_font);
    painter.setPen(Qt::white);

    QRect br;
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, _state_text, &br);

    QLinearGradient gradient(br.topLeft(), br.bottomLeft());
    gradient.setColorAt(0, QColor(0, 0, 0, 0xB0));
    gradient.setColorAt(1, QColor(0, 0, 0, 0x00));

    painter.fillRect(br, QBrush(gradient));
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, _state_text);
}

void VideoDetails::draw_spinner() {
    QPainter painter { this };

    auto centered_rect = _spinner.rect();
    auto aspect_ratio = (float)centered_rect.height() / centered_rect.width();
    if (rect().width() < centered_rect.width()) {
        auto new_width = rect().width();
        centered_rect.setWidth(new_width);
        centered_rect.setHeight(new_width * aspect_ratio);
    }
    if (rect().height() < centered_rect.height()) {
        auto new_height = rect().height();
        centered_rect.setHeight(new_height);
        centered_rect.setWidth(new_height / aspect_ratio);
    }
    centered_rect.moveCenter(rect().center());
    painter.translate(rect().center());
    painter.rotate(_spinner_angle);
    painter.drawImage(
        centered_rect.translated(-rect().center()),
        _spinner
    );
}

void VideoDetails::draw_stream_details() {
    // QPainter painter { this };

    _stream_details_widget->resize(width(), _stream_details_widget->height());
    _stream_details_widget->render(this);
}

void VideoDetails::show_state(const QString &state) {
    _state_text = state;
    _state_timer->start();
    _show_state_text = true;
    repaint();
}

void VideoDetails::set_buffering(bool enabled) {
    if (enabled == _show_spinner)
        return ;
    _show_spinner = enabled;
    _spinner_angle = 0;
    if (enabled) _spinner_timer->start();
    else         _spinner_timer->stop();
    repaint();
}

void VideoDetails::show_stream_details() {
    _stream_details_timer->start();
    _show_stream_details = true;
    repaint();
}

void VideoDetails::set_channel(const QString &channel) {
    _channel = channel;
    _stream_details_ui->channelLogo->setPixmap(QPixmap{});
    _has_valid_stream_details = false;
    fetch_channel_details();
}

void VideoDetails::fetch_channel_details() {
    auto fill_stream_info = [=](StreamData data) {
        if (_stream_details_ui->channelLogo->pixmap()->isNull())
            fetch_channel_logo(data.channel.logo_url);

        _stream_details_ui->labelChannel->setText(_channel.toUpper());
        _stream_details_ui->labelTitle->setText(data.channel.title);
        _stream_details_ui->labelPlaying->setText(data.current_game);
        _stream_details_ui->labelViewcount->setText(QString::number(data.viewcount));

        _has_valid_stream_details = true;
        repaint();
    };

    auto fetch_stream = [=](QList<ChannelData> channels) {
        auto channel_it = std::find_if(
            channels.begin(), channels.end(),
            [=](ChannelData channel) { return channel.name == _channel; }
        );
        if (channel_it != channels.end())
            return _api.stream(channel_it->id);
        else
            return TwitchAPI::stream_response_t::reject("Channel not found");
    };

    _api.channel_search(_channel)
        .then(fetch_stream)
        .then(fill_stream_info);
}

void VideoDetails::fetch_channel_logo(const QString &url) {
    auto request = QNetworkRequest(QUrl(url));
    auto preview_reply = _http_client->get(request);

    QObject::connect(preview_reply, &QNetworkReply::finished, [=] {
        auto data = preview_reply->readAll();
        auto pixmap = QPixmap::fromImage(QImage::fromData(data));
        _stream_details_ui->channelLogo->setPixmap(pixmap);
        preview_reply->deleteLater();
    });
}
