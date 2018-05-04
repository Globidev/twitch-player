#ifndef QUALITY_PICKER_DIALOG_HPP
#define QUALITY_PICKER_DIALOG_HPP

#include <QDialog>

#include "api/twitchd.hpp"

namespace Ui {
    class QualityPickerDialog;
}

class QMovie;

class QualityPickerDialog: public QDialog {
public:
    QualityPickerDialog(QString, QWidget * = nullptr);
    ~QualityPickerDialog();

    static QString pick(QString, QWidget * = nullptr, bool * = nullptr);
private:
    Ui::QualityPickerDialog *_ui;
    QMovie *_spinner_movie;

    TwitchdAPI _api;
    TwitchdAPI::stream_index_response_t _stream_index_query;
};

#endif // QUALITY_PICKER_DIALOG_HPP
