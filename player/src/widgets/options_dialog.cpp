#include "widgets/options_dialog.hpp"
#include "ui_options_dialog.h"

#include "constants.hpp"

#include <QFileDialog>
#include <QSettings>

OptionsDialog::OptionsDialog(QWidget *parent):
    QDialog(parent),
    _ui(new Ui::OptionsDialog)
{
    setModal(true);
    _ui->setupUi(this);

    load_settings();

    QObject::connect(
        _ui->buttonBox,
        &QDialogButtonBox::accepted,
        [this] { save_settings(); }
    );

    QObject::connect(_ui->browseChatRendererPath, &QPushButton::clicked, [this] {
        auto path = QFileDialog::getOpenFileName(this, "Chat renderer path");
        if (!path.isEmpty())
            _ui->chatRendererPath->setText(path);
    });

}

OptionsDialog::~OptionsDialog() {
    delete _ui;
}


void OptionsDialog::load_settings() {
    using namespace constants::settings::paths;

    QSettings settings;

    auto chat_renderer_path = settings
        .value(KEY_CHAT_RENDERER_PATH, DEFAULT_CHAT_RENDERER_PATH)
        .toString();
    _ui->chatRendererPath->setText(chat_renderer_path);

    auto chat_renderer_args = settings
        .value(KEY_CHAT_RENDERER_ARGS, DEFAULT_CHAT_RENDERER_ARGS)
        .toStringList();
    _ui->chatRendererArgs->setText(chat_renderer_args.join(';'));
}

void OptionsDialog::save_settings() {
    using namespace constants::settings::paths;

    QSettings settings;

    settings.setValue(KEY_CHAT_RENDERER_PATH, _ui->chatRendererPath->text());
    settings.setValue(KEY_CHAT_RENDERER_ARGS, _ui->chatRendererArgs->text().split(';'));
}
