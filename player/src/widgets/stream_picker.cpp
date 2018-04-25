#include "widgets/stream_picker.hpp"
#include "ui_stream_picker.h"

#include <QInputDialog>

StreamPicker::StreamPicker(QWidget *parent):
    QWidget(parent),
    _ui(new Ui::StreamPicker)
{
    _ui->setupUi(this);
    setAutoFillBackground(true);

    QObject::connect(_ui->pick_button, &QPushButton::clicked, [this] {
        bool picked;
        auto channel = QInputDialog::getText(this,
            "New stream", "channel",
            QLineEdit::Normal, QString(), &picked
        );
        if (picked && !channel.isEmpty())
            emit stream_picked(channel);
    });
}

StreamPicker::~StreamPicker() {
    delete _ui;
}
