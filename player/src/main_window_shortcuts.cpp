#include "main_window.hpp"
#include "ui_main_window.h"

#include "widgets/stream_container.hpp"
#include "widgets/stream_widget.hpp"
#include "widgets/video_widget.hpp"
#include "widgets/foreign_widget.hpp"
#include "widgets/vlc_log_viewer.hpp"
#include "widgets/options_dialog.hpp"
#include "widgets/filters_tool.hpp"

#include "native/capabilities.hpp"

#include "constants.hpp"

#include "api/oauth.hpp"

#include <QKeyEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QShortcut>

void MainWindow::setup_shortcuts() {
    using namespace constants::settings::shortcuts;

    QSettings settings;

    auto add_shortcut_impl = [=, &settings](auto menu, auto sh_desc) {
        auto sequence_str = settings
            .value(sh_desc.setting_key, sh_desc.default_key_sequence)
            .toString();

        auto sequence = QKeySequence(sequence_str);

        auto shortcut = new QShortcut(sequence, this);
        shortcut->setEnabled(false);

        auto action = menu->addAction(sh_desc.action_text);
        action->setShortcut(sequence);

        auto pair = std::pair<QShortcut *, QAction *> { shortcut, action };
        _shortcuts.push_back({sh_desc.setting_key, pair});

        return pair;
    };

    auto add_shortcut = [=](auto menu, auto sh_desc, auto slot) {
        auto [shortcut, action] = add_shortcut_impl(menu, sh_desc);

        QObject::connect(shortcut, &QShortcut::activated, slot);
        QObject::connect(action, &QAction::triggered, slot);

        return action;
    };

    auto add_toggle = [=](auto menu, auto sh_desc, auto slot) {
        auto [shortcut, action] = add_shortcut_impl(menu, sh_desc);

        // MSVC BUG: cannot capture action by value ???
        auto toggle_slot = [=, action = action] {
            auto on = !action->isChecked();
            slot(on);
            action->setChecked(on);
        };

        QObject::connect(shortcut, &QShortcut::activated, toggle_slot);
        QObject::connect(action, &QAction::triggered, slot);

        action->setCheckable(true);

        return action;
    };

    auto add_action = [this](auto action, auto slot) {
        QObject::connect(action, &QAction::triggered, slot);
    };

    auto active_pos = [this] {
        return _grid->widget_position(focused_pane());
    };

    auto window_handle = [this] {
        return reinterpret_cast<WindowHandle>(window()->winId());
    };

    auto still = [](auto pos) { return pos; };
    auto left  = [=](auto pos) { return pos.left(_grid->orientation()); };
    auto right = [=](auto pos) { return pos.right(_grid->orientation()); };
    auto up    = [=](auto pos) { return pos.up(_grid->orientation()); };
    auto down  = [=](auto pos) { return pos.down(_grid->orientation()); };

    auto with_active_stream = [=](auto action) {
        if (auto active_pane = focused_pane(); active_pane)
            action(active_pane->stream());
    };

    // View
    auto toggle_fullscreen = [=](bool on) {
        setWindowState(windowState().setFlag(Qt::WindowFullScreen, on));
    };
    auto toggle_window_borders = [=](bool on) {
        ::toggle_window_borders(window_handle(), on);
    };
    auto toggle_always_on_top = [=](bool on) {
        ::toggle_always_on_top(window_handle(), on);
    };
    auto toggle_stream_zoon = [=](bool on) {
        on ? zoom() : unzoom();
    };

    add_toggle(_ui->menuView, TOGGLE_FULL_SCREEN,    toggle_fullscreen);
    add_toggle(_ui->menuView, TOGGLE_WINDOW_BORDERS, toggle_window_borders)
        ->setChecked(true);
    add_toggle(_ui->menuView, TOGGLE_ALWAYS_ON_TOP, toggle_always_on_top);
    add_toggle(_ui->menuView, TOGGLE_STREAM_ZOOM,   toggle_stream_zoon);
        //
    _ui->menuView->addSeparator();
    auto add_pane_at  = [=](auto dpos) {
        return [=] { unzoom(); add_pane(dpos(active_pos())); };
    };
    add_shortcut(_ui->menuView, ADD_PANE_LEFT,  add_pane_at(still));
    add_shortcut(_ui->menuView, ADD_PANE_RIGHT, add_pane_at(right));
    add_shortcut(_ui->menuView, ADD_PANE_UP,    add_pane_at(up));
    add_shortcut(_ui->menuView, ADD_PANE_DOWN,  add_pane_at(down));

    auto remove_active_pane = [=] { unzoom(); remove_pane(focused_pane()); };
    add_shortcut(_ui->menuView, REMOVE_ACTIVE_PANE, remove_active_pane);
        //
    _ui->menuView->addSeparator();
    auto move_focus_at = [=](auto dpos) {
        return [=] { unzoom(); move_focus(dpos(active_pos())); };
    };
    add_shortcut(_ui->menuView, MOVE_FOCUS_LEFT,  move_focus_at(left));
    add_shortcut(_ui->menuView, MOVE_FOCUS_RIGHT, move_focus_at(right));
    add_shortcut(_ui->menuView, MOVE_FOCUS_UP,    move_focus_at(up));
    add_shortcut(_ui->menuView, MOVE_FOCUS_DOWN,  move_focus_at(down));
        //
    _ui->menuView->addSeparator();
    auto toggle_chat = [=](auto pos) {
        return [=] { with_active_stream([=](auto stream) { stream->reposition_chat(pos); }); };
    };
    add_shortcut(_ui->menuView, TOGGLE_CHAT_LEFT,  toggle_chat(ChatPosition::Left));
    add_shortcut(_ui->menuView, TOGGLE_CHAT_RIGHT, toggle_chat(ChatPosition::Right));
    auto resize_chat = [=](auto pos) {
        return [=] { with_active_stream([=](auto stream) { stream->resize_chat(pos); }); };
    };
    add_shortcut(_ui->menuView, RESIZE_CHAT_LEFT,  resize_chat(ChatPosition::Left));
    add_shortcut(_ui->menuView, RESIZE_CHAT_RIGHT, resize_chat(ChatPosition::Right));
        //
    _ui->menuView->addSeparator();
    auto move_pane_at = [=](auto dpos) {
        return [=] { unzoom(); auto pos = active_pos(); move_pane(pos, dpos(pos)); };
    };
    add_shortcut(_ui->menuView, MOVE_PANE_LEFT,  move_pane_at(left));
    add_shortcut(_ui->menuView, MOVE_PANE_RIGHT, move_pane_at(right));
    add_shortcut(_ui->menuView, MOVE_PANE_UP,    move_pane_at(up));
    add_shortcut(_ui->menuView, MOVE_PANE_DOWN,  move_pane_at(down));
        //
    _ui->menuView->addSeparator();
    auto rotate_layout = [=] { unzoom(); _grid->rotate(); };
    add_shortcut(_ui->menuView, ROTATE_LAYOUT, rotate_layout);
    // Playback
    auto toggle_mute = [=] {
        with_active_stream([=](auto stream) { stream->video()->set_muted(!stream->video()->muted()); });
    };
    add_shortcut(_ui->menuPlayback, TOGGLE_MUTE, toggle_mute);
    auto fast_forward = [=] {
        with_active_stream([=](auto stream) { stream->video()->fast_forward(); });
    };
    add_shortcut(_ui->menuPlayback, FAST_FORWARD, fast_forward);
    _audio_devices_menu = _ui->menuPlayback->addMenu("Audio Devices");
    QObject::connect(_audio_devices_menu, &QMenu::aboutToShow, [=] {
        setup_audio_devices();
    });
    add_shortcut(_ui->menuPlayback, FILTERS_TOOL, [=] {
        if (auto active_pane = focused_pane(); active_pane) {
            auto & mp = active_pane->stream()->video()->media_player();
            auto tool = new FiltersTool(mp, this);
            tool->show();
        }
    });
    // Tools
    add_action(_ui->actionLogs, [this] {
        _vlc_log_viewer->show();
        _vlc_log_viewer->raise();
    });
    add_action(_ui->actionLoginWithTwitch, [this] {
        {
            QSettings settings;

            settings.remove(constants::oauth::ACCESS_TOKEN_KEY);
            settings.remove(constants::oauth::REFRESH_TOKEN_KEY);
        }

        auto oauth = std::make_shared<OAuth>();
        oauth->query_token().then([=](QString) mutable {
            oauth.reset();
        });
    });
    // Preferences
    add_action(_ui->actionOptions, [this] {
        auto options_dialog = new OptionsDialog(this);
        QObject::connect(options_dialog, &OptionsDialog::settings_changed, [this] {
            remap_shortcuts();
        });
        options_dialog->show();
    });
}

void MainWindow::remap_shortcuts() {
    QSettings settings;

    for (auto [setting_key, pair]: _shortcuts) {
        auto [shortcut, action] = pair;
        auto sequence_str = settings.value(setting_key).toString();
        if (!sequence_str.isEmpty()) {
            auto sequence = QKeySequence(sequence_str);
            shortcut->setKey(sequence);
            action->setShortcut(sequence);
        }
    }
}
