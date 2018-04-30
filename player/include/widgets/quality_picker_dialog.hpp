#ifndef QUALITY_PICKER_DIALOG_HPP
#define QUALITY_PICKER_DIALOG_HPP

#include <QDialog>

namespace Ui {
    class QualityPickerDialog;
}

class QMovie;
class TwitchdAPI;

class QualityPickerDialog: public QDialog {
public:
    QualityPickerDialog(QString, QWidget * = nullptr);
    ~QualityPickerDialog();

    static QString pick(QString, QWidget * = nullptr, bool * = nullptr);
private:
    Ui::QualityPickerDialog *_ui;
    QMovie *_spinner_movie;

    TwitchdAPI *_api;
};

#endif // QUALITY_PICKER_DIALOG_HPP
