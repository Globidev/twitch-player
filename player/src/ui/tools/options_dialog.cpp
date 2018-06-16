#include "ui/tools/options_dialog.hpp"
#include "ui_options_dialog.h"

#include "process/daemon_control.hpp"

#include "prelude/timer.hpp"

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

    QObject::connect(_ui->daemonManagedGroupbox, &QGroupBox::toggled, [this](bool on) {
        _ui->daemonUnmanagedGroupbox->setChecked(!on);
        QSettings settings;
        settings.setValue(constants::settings::daemon::KEY_MANAGED, on);
        update_daemon("Querying...", [] { });
    });
    QObject::connect(_ui->daemonUnmanagedGroupbox, &QGroupBox::toggled, [this](bool on) {
        _ui->daemonManagedGroupbox->setChecked(!on);
        QSettings settings;
        settings.setValue(constants::settings::daemon::KEY_MANAGED, !on);
        update_daemon("Querying...", [] { });
    });
    QObject::connect(_ui->daemonIndexCacheTimeout, &QSlider::valueChanged, [this](int value) {
        _ui->daemonIndexCacheTimeoutLabel->setText(QString("%1 seconds").arg(value));
    });
    QObject::connect(_ui->daemonPlaylistFetchInterval, &QSlider::valueChanged, [this](int value) {
        _ui->daemonPlaylistFetchIntervalLabel->setText(QString("%1 milliseconds").arg(value));
    });
    QObject::connect(_ui->daemonPlayerFetchTimeout, &QSlider::valueChanged, [this](int value) {
        _ui->daemonPlayerFetchTimeoutLabel->setText(QString("%1 seconds").arg(value));
    });
    QObject::connect(_ui->daemonPlayerInactiveTimeout, &QSlider::valueChanged, [this](int value) {
        _ui->daemonPlayerInactiveTimeoutLabel->setText(QString("%1 seconds").arg(value));
    });

    QObject::connect(_ui->daemonStart, &QPushButton::clicked, [this] {
        update_daemon("Starting...", [] { daemon_control::start(); });
    });

    QObject::connect(_ui->daemonStop, &QPushButton::clicked, [this] {
        update_daemon("Stopping...", [] { daemon_control::stop(); });
    });

    load_settings();
}

OptionsDialog::~OptionsDialog() = default;

void OptionsDialog::load_settings() {
    using namespace constants::settings::chat_renderer;
    using namespace constants::settings::vlc;
    using namespace constants::settings::shortcuts;
    using namespace constants::settings::daemon;

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

    auto keybinds_layout = new QVBoxLayout(_ui->keybindsFrame->widget());
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
    _ui->keybindsFrame->widget()->setLayout(keybinds_layout);

    update_daemon("Querying...", [] { });

    _ui->daemonManagedHost->setText(settings.value(KEY_HOST_MANAGED, DEFAULT_HOST_MANAGED).toString());
    _ui->daemonManagedPort->setValue(settings.value(KEY_PORT_MANAGED, DEFAULT_PORT_MANAGED).value<quint16>());

    _ui->daemonIndexCacheTimeout->setValue(settings.value(KEY_CACHE_TIMEOUT, DEFAULT_CACHE_TIMEOUT).toInt());
    _ui->daemonPlaylistFetchInterval->setValue(settings.value(KEY_PLAYLIST_FETCH_INTERVAL, DEFAULT_PLAYLIST_FETCH_INTERVAL).toInt());
    _ui->daemonPlayerFetchTimeout->setValue(settings.value(KEY_PLAYER_FETCH_TIMEOUT, DEFAULT_PLAYER_FETCH_TIMEOUT).toInt());
    _ui->daemonPlayerInactiveTimeout->setValue(settings.value(KEY_PLAYER_INACTIVE_TIMEOUT, DEFAULT_PLAYER_INACTIVE_TIMEOUT).toInt());

    _ui->daemonUnmanagedHost->setText(settings.value(KEY_HOST_UNMANAGED, DEFAULT_HOST_UNMANAGED).toString());
    _ui->daemonUnmanagedPort->setValue(settings.value(KEY_PORT_UNMANAGED, DEFAULT_PORT_UNMANAGED).value<quint16>());

    if (!settings.value(KEY_MANAGED, DEFAULT_MANAGED).toBool())
        _ui->daemonUnmanagedGroupbox->setChecked(true);
}

void OptionsDialog::save_settings() {
    using namespace constants::settings::chat_renderer;
    using namespace constants::settings::vlc;
    using namespace constants::settings::shortcuts;
    using namespace constants::settings::daemon;

    QSettings settings;

    settings.setValue(KEY_CHAT_RENDERER_PATH, _ui->chatRendererPath->text());
    settings.setValue(KEY_CHAT_RENDERER_ARGS, _ui->chatRendererArgs->text().split(';'));

    QStringList libvlc_options;
    for (auto i = 0; i < _ui->libvlcOptionsList->count(); ++i)
        libvlc_options << _ui->libvlcOptionsList->item(i)->text();
    settings.setValue(KEY_VLC_ARGS, libvlc_options);

    for (auto [setting_key, sequence_edit]: _keybind_edits)
        settings.setValue(setting_key, sequence_edit->keySequence().toString());

    settings.setValue(KEY_HOST_MANAGED, _ui->daemonManagedHost->text());
    settings.setValue(KEY_PORT_MANAGED, _ui->daemonManagedPort->value());

    settings.setValue(KEY_CACHE_TIMEOUT, _ui->daemonIndexCacheTimeout->value());
    settings.setValue(KEY_PLAYLIST_FETCH_INTERVAL, _ui->daemonPlaylistFetchInterval->value());
    settings.setValue(KEY_PLAYER_FETCH_TIMEOUT, _ui->daemonPlayerFetchTimeout->value());
    settings.setValue(KEY_PLAYER_INACTIVE_TIMEOUT, _ui->daemonPlayerInactiveTimeout->value());

    settings.setValue(KEY_HOST_UNMANAGED, _ui->daemonUnmanagedHost->text());
    settings.setValue(KEY_PORT_UNMANAGED, _ui->daemonUnmanagedPort->value());

    settings.setValue(KEY_MANAGED, _ui->daemonManagedGroupbox->isChecked());
}

void OptionsDialog::update_daemon(QString message, std::function<void ()> action) {
    using namespace constants::settings::daemon;

    _ui->daemonStart->setEnabled(false);
    _ui->daemonStop->setEnabled(false);
    _ui->daemonStatus->setText(message);

    action();

    auto daemon_status = daemon_control::status();

    QSettings settings;

    auto managed = settings.value(KEY_MANAGED, DEFAULT_MANAGED).toBool();

    auto [status, color] = [=]() -> std::pair<QString, QColor> {
        if (daemon_status.running) {
            return {
                QString("Running (%1)").arg(daemon_status.version),
                QColor(Qt::green)
            };
        }
        else {
            return { "Stopped", QColor(Qt::red) };
        }
    }();

    status.prepend(managed ? "": "[remote] ");
    _ui->daemonStatus->setText(status);
    QPalette palette;
    palette.setColor(QPalette::WindowText, color);
    _ui->daemonStatus->setPalette(palette);

    _ui->daemonStart->setEnabled(!daemon_status.running && managed);
    _ui->daemonStop->setEnabled(daemon_status.running && managed);
}
