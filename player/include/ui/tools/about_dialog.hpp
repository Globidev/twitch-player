#pragma once

#include <QDialog>

#include <memory>

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
