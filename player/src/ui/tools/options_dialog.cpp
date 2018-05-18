#include "ui/tools/options_dialog.hpp"
#include "ui_options_dialog.h"

#include "constants.hpp"

#include <QFileDialog>
#include <QKeySequenceEdit>
#include <QSettings>

constexpr auto LIST_WIDGET_ITEM_FLAGS = Qt::ItemIsEnabled
                                      | Qt::ItemIsEditable
                                      | Qt::ItemIsSelectable;

OptionsDialog::OptionsDialog(QWidget *parent):
    QDialog(parent),
    _ui(std::make_unique<Ui::OptionsDialog>())
{
    setModal(true);
    _ui->setupUi(this);

    load_settings();

    QObject::connect(
        _ui->buttonBox,
        &QDialogButtonBox::accepted,
        [this] { save_settings(); emit settings_changed(); }
    );

    QObject::connect(_ui->browseChatRendererPath, &QPushButton::clicked, [this] {
        auto path = QFileDialog::getOpenFileName(this, "Chat renderer path");
        if (!path.isEmpty())
            _ui->chatRendererPath->setText(path);
    });

    QObject::connect(_ui->libvlcOptionsListAdd, &QPushButton::clicked, [this] {
        auto new_item = new QListWidgetItem("...", _ui->libvlcOptionsList);
        new_item->setFlags(LIST_WIDGET_ITEM_FLAGS);
        _ui->libvlcOptionsList->setCurrentItem(new_item);
        _ui->libvlcOptionsList->editItem(new_item);
    });

    QObject::connect(_ui->libvlcOptionsListDel, &QPushButton::clicked, [this] {
        qDeleteAll(_ui->libvlcOptionsList->selectedItems());
    });
}

OptionsDialog::~OptionsDialog() = default;

void OptionsDialog::load_settings() {
    using namespace constants::settings::chat_renderer;
    using namespace constants::settings::vlc;
    using namespace constants::settings::shortcuts;

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

    auto keybinds_layout = new QVBoxLayout(_ui->tabKeybinds);
    for (auto sh_desc: ALL_SHORTCUTS) {
        auto sequence_str = settings
            .value(sh_desc.setting_key, sh_desc.default_key_sequence)
            .toString();
        auto sequence = QKeySequence(sequence_str);
        auto shortcut_layout = new QHBoxLayout;
        auto label = new QLabel(QString("%1:").arg(sh_desc.action_text));
        label->setMinimumWidth(130);
        shortcut_layout->addWidget(label);
        auto keybind_edit = new QKeySequenceEdit(sequence);
        shortcut_layout->addWidget(keybind_edit);
        _keybind_edits.push_back({ sh_desc.setting_key, keybind_edit });
        keybinds_layout->addLayout(shortcut_layout);
    }
    _ui->tabKeybinds->setLayout(keybinds_layout);
}

void OptionsDialog::save_settings() {
    using namespace constants::settings::chat_renderer;
    using namespace constants::settings::vlc;
    using namespace constants::settings::shortcuts;

    QSettings settings;

    settings.setValue(KEY_CHAT_RENDERER_PATH, _ui->chatRendererPath->text());
    settings.setValue(KEY_CHAT_RENDERER_ARGS, _ui->chatRendererArgs->text().split(';'));

    QStringList libvlc_options;
    for (auto i = 0; i < _ui->libvlcOptionsList->count(); ++i)
        libvlc_options << _ui->libvlcOptionsList->item(i)->text();
    settings.setValue(KEY_VLC_ARGS, libvlc_options);

    for (auto [setting_key, sequence_edit]: _keybind_edits)
        settings.setValue(setting_key, sequence_edit->keySequence().toString());
}
