#pragma once

#include <QDialog>

#include <vector>
#include <functional>

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

    void update_daemon(QString, std::function<void ()>);

    std::unique_ptr<Ui::OptionsDialog> _ui;
    std::vector<std::pair<QString, QKeySequenceEdit *>> _keybind_edits;
};
