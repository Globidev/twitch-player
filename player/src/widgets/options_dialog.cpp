#include "widgets/options_dialog.hpp"
#include "ui_options_dialog.h"

#include "constants.hpp"

#include <QFileDialog>
#include <QSettings>

constexpr auto LIST_WIDGET_ITEM_FLAGS = Qt::ItemIsEnabled
                                      | Qt::ItemIsEditable
                                      | Qt::ItemIsSelectable;

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

    QObject::connect(_ui->libvlcOptionsListAdd, &QPushButton::clicked, [this] {
        auto new_item = new QListWidgetItem("...", _ui->libvlcOptionsList);
        new_item->setFlags(LIST_WIDGET_ITEM_FLAGS);
        _ui->libvlcOptionsList->editItem(new_item);
    });

    QObject::connect(_ui->libvlcOptionsListDel, &QPushButton::clicked, [this] {
        for (auto item: _ui->libvlcOptionsList->selectedItems())
            delete item;
    });
}

OptionsDialog::~OptionsDialog() {
    delete _ui;
}


void OptionsDialog::load_settings() {
    using namespace constants::settings::paths;
    using namespace constants::settings::vlc;

    QSettings settings;

    auto chat_renderer_path = settings
        .value(KEY_CHAT_RENDERER_PATH, DEFAULT_CHAT_RENDERER_PATH)
        .toString();
    _ui->chatRendererPath->setText(chat_renderer_path);

    auto chat_renderer_args = settings
        .value(KEY_CHAT_RENDERER_ARGS, DEFAULT_CHAT_RENDERER_ARGS)
        .toStringList();
    _ui->chatRendererArgs->setText(chat_renderer_args.join(';'));

    auto libvlc_options = settings
        .value(KEY_VLC_ARGS, DEFAULT_VLC_ARGS)
        .toStringList();
    for (auto option: libvlc_options) {
        auto item = new QListWidgetItem(option, _ui->libvlcOptionsList);
        item->setFlags(LIST_WIDGET_ITEM_FLAGS);
    }
}

void OptionsDialog::save_settings() {
    using namespace constants::settings::paths;
    using namespace constants::settings::vlc;

    QSettings settings;

    settings.setValue(KEY_CHAT_RENDERER_PATH, _ui->chatRendererPath->text());
    settings.setValue(KEY_CHAT_RENDERER_ARGS, _ui->chatRendererArgs->text().split(';'));

    QStringList libvlc_options;
    for (auto i = 0; i < _ui->libvlcOptionsList->count(); ++i)
        libvlc_options << _ui->libvlcOptionsList->item(i)->text();
    settings.setValue(KEY_VLC_ARGS, libvlc_options);
}
