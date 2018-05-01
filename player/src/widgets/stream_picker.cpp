#include "widgets/stream_picker.hpp"
#include "ui_stream_picker.h"

#include "widgets/flow_layout.hpp"
#include "widgets/stream_card.hpp"
#include "widgets/quality_picker_dialog.hpp"

#include "api/twitch.hpp"

#include "constants.hpp"

StreamPicker::StreamPicker(QWidget *parent):
    QWidget(parent),
    _ui(new Ui::StreamPicker),
    _stream_presenter(new QWidget(this)),
    _api(new TwitchAPI(this))
{
    _ui->setupUi(this);

    new FlowLayout(_stream_presenter);
    _ui->streamArea->setWidget(_stream_presenter);

    fetch_streams(_api->top_streams());

    QObject::connect(_ui->searchBox, &QLineEdit::textChanged, [this](auto query) {
        fetch_streams(_api->stream_search(query));
    });

    QObject::connect(_ui->searchBox, &QLineEdit::returnPressed, [this] {
        emit stream_picked(_ui->searchBox->text(), QString());
    });
}

StreamPicker::~StreamPicker() {
    delete _ui;
}

void StreamPicker::focusInEvent(QFocusEvent *event) {
    QWidget::focusInEvent(event);
    _ui->searchBox->setFocus();
}

void StreamPicker::fetch_streams(StreamPromise * query) {
    if (current_query)
        current_query->abort();

    current_query = query;

    QObject::connect(current_query, &StreamPromise::finished, [=](auto stream_data) {
        auto layout = _stream_presenter->layout();

        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != nullptr) {
            item->widget()->deleteLater();
            delete item;
        }

        for (auto data: stream_data) {
            auto stream_card = new StreamCard(data, _stream_presenter);
            layout->addWidget(stream_card);
            QObject::connect(stream_card, &StreamCard::clicked, [this](auto channel) {
                bool picked;
                auto quality = QualityPickerDialog::pick(channel, this, &picked);
                if (picked)
                    emit stream_picked(channel, quality);
            });
        }

        current_query->deleteLater();
        current_query = nullptr;
    });
}
