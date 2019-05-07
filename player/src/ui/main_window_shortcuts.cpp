#include "ui/main_window.hpp"
#include "ui_main_window.h"

#include "ui/native/capabilities.hpp"

#include "ui/widgets/chat_pane.hpp"
#include "ui/widgets/stream_pane.hpp"
#include "ui/widgets/stream_widget.hpp"
#include "ui/widgets/video_widget.hpp"

#include "ui/tools/about_dialog.hpp"
#include "ui/tools/options_dialog.hpp"
#include "ui/tools/video_filters.hpp"
#include "ui/tools/vlc_log_viewer.hpp"

#include "constants.hpp"

#include "api/oauth.hpp"

#include "prelude/variant.hpp"

#include <QSettings>
#include <QShortcut>
#include <QDesktopServices>

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
        auto pane = focused_pane();
        if (pane) {
            return match(*pane, [this](auto pane) { return _grid->widget_position(pane); });
        } else {
            return Position { 0, 0 };
        }
    };

    auto window_handle = [this] {
        return to_native_handle(window()->winId());
    };

    auto still = [](auto pos) { return pos; };
    auto left  = [=](auto pos) { return pos.left(_grid->orientation()); };
    auto right = [=](auto pos) { return pos.right(_grid->orientation()); };
    auto up    = [=](auto pos) { return pos.up(_grid->orientation()); };
    auto down  = [=](auto pos) { return pos.down(_grid->orientation()); };

    auto with_active_stream = [=](auto action) {
        if (auto active_pane = focused_pane(); active_pane) {
            match(*active_pane,
                [=](StreamPane *pane) { action(pane->stream()); },
                [](auto) { }
            );
        }
    };

    // View
    auto toggle_fullscreen = [=](bool on) {
        set_fullscreen(on);
    };
    auto toggle_window_borders = [=](bool on) {
        ::toggle_window_borders(window_handle(), on);
    };
    auto toggle_menu_bar = [=](bool on) {
        _ui->menuBar->setVisible(on);
        for (auto [_key, pair]: _shortcuts) {
            auto [shortcut, _action] = pair;
            shortcut->setEnabled(!on);
        }
    };
    auto toggle_always_on_top = [=](bool on) {
        ::toggle_always_on_top(window_handle(), on);
    };
    auto toggle_stream_zoon = [=](bool on) {
        if (on) {
            if (auto active_pane = focused_pane(); active_pane) {
                match(*active_pane,
                    [=](StreamPane *pane) { zoom(pane); },
                    [](auto) { }
                );
            }
            else
                _action_stream_zoom->setChecked(false);
        }
        else {
            unzoom();
        }
    };

    _action_fullscreen = add_toggle(_ui->menuView, TOGGLE_FULL_SCREEN,    toggle_fullscreen);
    add_toggle(_ui->menuView, TOGGLE_WINDOW_BORDERS, toggle_window_borders)
        ->setChecked(true);
    add_toggle(_ui->menuView, TOGGLE_MENU_BAR, toggle_menu_bar)
        ->setChecked(true);
    add_toggle(_ui->menuView, TOGGLE_ALWAYS_ON_TOP, toggle_always_on_top);
    _action_stream_zoom = add_toggle(_ui->menuView, TOGGLE_STREAM_ZOOM,   toggle_stream_zoon);
        //
    _ui->menuView->addSeparator();
    auto add_pane_at = [=](auto dpos) {
        return [=] { unzoom(); add_stream_pane(dpos(active_pos())); };
    };
    add_shortcut(_ui->menuView, ADD_PANE_LEFT,  add_pane_at(still));
    add_shortcut(_ui->menuView, ADD_PANE_RIGHT, add_pane_at(right));
    add_shortcut(_ui->menuView, ADD_PANE_UP,    add_pane_at(up));
    add_shortcut(_ui->menuView, ADD_PANE_DOWN,  add_pane_at(down));
        //
    _ui->menuView->addSeparator();
    auto add_chat_pane_at = [=](auto dpos) {
        return [=] { unzoom(); add_chat_pane(dpos(active_pos())); };
    };
    add_shortcut(_ui->menuView, ADD_CHAT_PANE_LEFT,  add_chat_pane_at(still));
    add_shortcut(_ui->menuView, ADD_CHAT_PANE_RIGHT, add_chat_pane_at(right));
    add_shortcut(_ui->menuView, ADD_CHAT_PANE_UP,    add_chat_pane_at(up));
    add_shortcut(_ui->menuView, ADD_CHAT_PANE_DOWN,  add_chat_pane_at(down));
        //
    _ui->menuView->addSeparator();
    auto remove_active_pane = [=] {
        unzoom();
        if (auto active_pane = focused_pane(); active_pane) {
            match(*active_pane, [this](auto pane) { remove_pane(pane); });
        }
    };
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
            match(*active_pane,
                [this](StreamPane *pane) {
                    auto & mp = pane->stream()->video()->media_player();
                    auto tool = new VideoFilters(mp, this);
                    tool->show();
                },
                [](auto) { }
            );
        }
    });
    // Tools
    add_action(_ui->actionLogs, [this] {
        _vlc_log_viewer->show();
        _vlc_log_viewer->raise();
    });
    add_action(_ui->actionLoginWithTwitch, [] {
        {
            QSettings settings;

            settings.remove(constants::settings::oauth::ACCESS_TOKEN_KEY);
            settings.remove(constants::settings::oauth::REFRESH_TOKEN_KEY);
        }

        auto oauth = std::make_shared<OAuth>();
        oauth->query_token().then([_retain = std::move(oauth)](QString) { });
    });
    // Preferences
    add_action(_ui->actionOptions, [this] {
        auto options_dialog = new OptionsDialog(_pubsub, this);
        QObject::connect(options_dialog, &OptionsDialog::settings_changed, [this] {
            remap_shortcuts();
        });
        options_dialog->show();
    });
    // Help
    add_action(_ui->actionAbout, [this] {
        auto about_dialog = new AboutDialog(this);
        about_dialog->show();
    });

    add_action(_ui->actionChromeExtension, [] {
        constexpr auto CHROME_EXTENSION_URL = "https://chrome.google.com/webstore/detail/open-in-twitch-player/cafdkpbjddldmaichnoonnhamnlapdhg";
        QDesktopServices::openUrl(QUrl { CHROME_EXTENSION_URL });
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
