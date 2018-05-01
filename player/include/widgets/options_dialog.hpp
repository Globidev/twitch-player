#ifndef OPTIONS_DIALOG_HPP
#define OPTIONS_DIALOG_HPP

#include <QDialog>
#include <vector>

namespace Ui {
    class OptionsDialog;
}

class QKeySequenceEdit;

class OptionsDialog: public QDialog {
    Q_OBJECT
public:
    OptionsDialog(QWidget * = nullptr);
    ~OptionsDialog();
signals:
    void settings_changed();
private:
    void load_settings();
    void save_settings();

    Ui::OptionsDialog *_ui;
    std::vector<std::pair<QString, QKeySequenceEdit *>> _keybind_edits;
};

#endif // OPTIONS_DIALOG_HPP
