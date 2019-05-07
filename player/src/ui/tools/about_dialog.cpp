#include "ui/tools/about_dialog.hpp"

#include "ui_about_dialog.h"

constexpr auto VERSION = "0.9.23";

AboutDialog::AboutDialog(QWidget *parent):
    QDialog(parent),
    _ui(std::make_unique<Ui::AboutDialog>())
{
    setModal(true);
    _ui->setupUi(this);

    _ui->labelVersion->setText(QString("Twitch Player version %1").arg(VERSION));
}

AboutDialog::~AboutDialog() = default;
