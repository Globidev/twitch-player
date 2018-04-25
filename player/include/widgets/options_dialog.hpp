#ifndef OPTIONS_DIALOG_HPP
#define OPTIONS_DIALOG_HPP

#include <QDialog>

namespace Ui {
    class OptionsDialog;
}

class OptionsDialog: public QDialog {
public:
    OptionsDialog(QWidget * = nullptr);
    ~OptionsDialog();
private:
    void load_settings();
    void save_settings();

    Ui::OptionsDialog *_ui;
};

#endif // OPTIONS_DIALOG_HPP
