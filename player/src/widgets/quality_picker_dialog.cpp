#include "widgets/quality_picker_dialog.hpp"
#include "ui_quality_picker_dialog.h"

#include "api/twitchd.hpp"

#include <QEventLoop>
#include <QMovie>

#include <QNetworkAccessManager>

QualityPickerDialog::QualityPickerDialog(QString channel, QWidget *parent):
    QDialog(parent),
    _ui(new Ui::QualityPickerDialog),
    _spinner_movie(new QMovie(":/gifs/spinner.gif")),
    _api(new TwitchdAPI(this))
{
    _ui->setupUi(this);
    _ui->spinner->setMovie(_spinner_movie);
    _spinner_movie->start();
    _spinner_movie->setScaledSize(_ui->spinner->size());

    auto query = _api->stream_index(channel);
    QObject::connect(query, &StreamIndexPromise::finished, [=](StreamIndex data) {
        QStringList qualities;
        for (auto playlist_info: data.playlist_infos)
            qualities << playlist_info.media_info.name;
        _ui->qualityCombo->addItems(qualities);
        _ui->qualityCombo->setEnabled(true);
        _ui->spinner->hide();
        query->deleteLater();
    });
}

QualityPickerDialog::~QualityPickerDialog() {
    delete _ui;
}

QString QualityPickerDialog::pick(QString channel, QWidget *parent, bool *picked_ptr) {
    QEventLoop loop;
    QString quality;
    bool picked;

    auto dialog = new QualityPickerDialog(channel, parent);
    QObject::connect(dialog, &QDialog::finished, [&](auto result) {
        picked = (result == QDialog::Accepted);
        quality = dialog->_ui->qualityCombo->currentText();
        loop.quit();
    });

    dialog->show();
    loop.exec();

    if (picked_ptr)
        *picked_ptr = picked;

    if (picked)
        return quality;
    else
        return QString();
}
