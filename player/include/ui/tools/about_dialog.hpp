#pragma once

#include <QDialog>

namespace Ui {
    class AboutDialog;
}

class AboutDialog: public QDialog {
public:
    AboutDialog(QWidget * = nullptr);
    ~AboutDialog();

private:
    std::unique_ptr<Ui::AboutDialog> _ui;
};
