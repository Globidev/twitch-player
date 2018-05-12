#include "widgets/stream_picker.hpp"
#include "ui_stream_picker.h"

#include "widgets/flow_layout.hpp"
#include "widgets/stream_card.hpp"

#include "api/oauth.hpp"

#include "constants.hpp"

#include <QSettings>

StreamPicker::StreamPicker(QWidget *parent):
    QWidget(parent),
    _ui(new Ui::StreamPicker),
    _channels_stream_presenter(new QWidget(this)),
    _followed_stream_presenter(new QWidget(this))
{
    _ui->setupUi(this);

    new FlowLayout(_channels_stream_presenter);
    _ui->channelsStreamArea->setWidget(_channels_stream_presenter);
    new FlowLayout(_followed_stream_presenter);
    _ui->followedStreamArea->setWidget(_followed_stream_presenter);

    QSettings settings;

    auto access_token = settings
        .value(constants::oauth::ACCESS_TOKEN_KEY)
        .toString();

    fetch_streams(_api.top_streams(), _channels_stream_presenter);

    if (!access_token.isEmpty())
        fetch_streams(_api.followed_streams(access_token), _followed_stream_presenter);

    QObject::connect(_ui->searchBox, &QLineEdit::textChanged, [this](auto query) {
        if (query.isEmpty())
            fetch_streams(_api.top_streams(), _channels_stream_presenter);
        else
            fetch_streams(_api.stream_search(query), _channels_stream_presenter);
    });

    QObject::connect(_ui->searchBox, &QLineEdit::returnPressed, [this] {
        channel_picked(_ui->searchBox->text());
    });
}

StreamPicker::~StreamPicker() {
    delete _ui;
}

void StreamPicker::focusInEvent(QFocusEvent *event) {
    QWidget::focusInEvent(event);
    _ui->searchBox->setFocus();
}

void StreamPicker::channel_picked(QString channel) {
    using namespace constants::settings::streams;

    QSettings settings;
    auto quality = settings
        .value(KEY_LAST_QUALITY_FOR(channel))
        .toString();

    emit stream_picked(channel, quality);
}

void StreamPicker::fetch_streams(TwitchAPI::streams_response_t query, QWidget *container) {
    auto & current_query = current_queries[container];

    query->then([=](auto stream_data) {
        auto layout = container->layout();

        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {
            item->widget()->deleteLater();
            delete item;
        }

        for (auto data: stream_data) {
            auto stream_card = new StreamCard(data, container);
            layout->addWidget(stream_card);
            QObject::connect(stream_card, &StreamCard::clicked, [this](auto channel) {
                channel_picked(channel);
            });
        }

        current_queries[container].reset();
    });

    query->bad_status([=](int status) {
        if (status == 401) {
            auto oauth = new OAuth(this);
            QObject::connect(oauth, &OAuth::token_ready, [=](auto access_token) {
                fetch_streams(_api.followed_streams(access_token), _followed_stream_presenter);
            });
            oauth->query_token();
        }
    });

    if (current_query)
        current_query->cancel();

    current_queries[container] = std::move(query);
}
