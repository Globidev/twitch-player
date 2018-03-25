extern crate qt_core;
extern crate qt_gui;
extern crate qt_widgets;
extern crate cpp_utils;

use self::qt_core::connection::Signal;
use self::qt_core::core_application::CoreApplication;
use self::qt_core::flags::{Flags, FlaggableEnum};
use self::qt_core::list::ListCInt;
use self::qt_core::qt::GlobalColor;
use self::qt_core::qt::WindowState;
use self::qt_core::slots::SlotNoArgs;
use self::qt_core::string::String as QString;
use self::qt_core::timer::Timer;
use self::qt_gui::color::Color;
use self::qt_gui::key_sequence::KeySequence;
use self::qt_gui::palette::{Palette, ColorRole};
use self::qt_gui::window::Window;
use self::qt_widgets::application::Application;
use self::qt_widgets::main_window::MainWindow;
use self::qt_widgets::message_box::MessageBox;
use self::qt_widgets::shortcut::Shortcut;
use self::qt_widgets::splitter::Splitter;
use self::qt_widgets::widget::Widget;
use self::qt_widgets::input_dialog::InputDialog;
use self::cpp_utils::StaticCast;

use types::{GuiReceiver, PlayerSender, PlayerMessage};
use options::{Options, KeyBinds as KeyConfig};

use std::ptr;

struct WidgetRatios {
    chat: i32,
    player: i32
}

const INITIAL_RATIOS: WidgetRatios     = WidgetRatios { chat: 20, player: 80 };
const FULL_SCREEN_RATIOS: WidgetRatios = WidgetRatios { chat: 0,  player: 100 };

#[derive(PartialEq, Eq)]
enum ChatPosition { Left, Right }

enum ChatState {
    Visible(ChatPosition),
    Hidden,
}

fn toggle_chat(state: &ChatState, new_position: ChatPosition) -> ChatState {
    use self::ChatState::{Hidden, Visible};

    match state {
        &Hidden => Visible(new_position),
        &Visible(ref position) => {
            match position == &new_position {
                true => Hidden,
                false => Visible(new_position)
            }
        }
    }
}

const DEFAULT_KEYBIND_FULLSCREEN:        &'static str = "F11";
const DEFAULT_KEYBIND_CHAT_TOGGLE_LEFT:  &'static str = "Ctrl+Left";
const DEFAULT_KEYBIND_CHAT_TOGGLE_RIGHT: &'static str = "Ctrl+Right";
const DEFAULT_KEYBIND_CHANGE_CHANNEL:    &'static str = "Ctrl+O";

struct KeyBinds {
    fullscreen: KeySequence,
    chat_toggle_left: KeySequence,
    chat_toggle_right: KeySequence,
    change_channel: KeySequence
}

impl From<KeyConfig> for KeyBinds {
    fn from(config: KeyConfig) -> Self {
        let make_seq = |opt_seq: Option<_>, def_seq| {
            let raw_seq = opt_seq.as_ref().map_or(def_seq, String::as_str);
            KeySequence::new(&QString::from(raw_seq))
        };

        Self {
            fullscreen:        make_seq(config.fullscreen,        DEFAULT_KEYBIND_FULLSCREEN),
            chat_toggle_left:  make_seq(config.chat_toggle_left,  DEFAULT_KEYBIND_CHAT_TOGGLE_LEFT),
            chat_toggle_right: make_seq(config.chat_toggle_right, DEFAULT_KEYBIND_CHAT_TOGGLE_RIGHT),
            change_channel:    make_seq(config.change_channel,    DEFAULT_KEYBIND_CHANGE_CHANNEL),
        }
    }
}

fn critical(title: &str, text: &str) {
    let args = (
        ptr::null_mut(),
        &QString::from_std_str(title.to_string()),
        &QString::from_std_str(text.to_string())
    );
    unsafe { MessageBox::critical(args) };
}

fn get_text(title: &str, text: &str) -> String {
    let args = (
        ptr::null_mut(),
        &QString::from_std_str(title.to_string()),
        &QString::from_std_str(text.to_string())
    );
    unsafe { InputDialog::get_text(args).to_std_string() }
}

unsafe fn widget_from_handle(handle: u64, parent: *mut Widget) -> *mut Widget {
    let window = Window::from_win_id(handle);
    Widget::create_window_container((window, parent))
}

fn toggle_full_screen(widget: &mut Widget) {
    use self::WindowState::FullScreen;

    let state = widget.window_state();
    let new_state = match widget.is_full_screen() {
        true =>  state & Flags::from_int(!(FullScreen.to_flag_value())),
        false => state | Flags::from_enum(FullScreen)
    };
    widget.set_window_state(new_state);
}

fn resize_splitter(splitter: &mut Splitter, ratios: (i32, i32)) {
    let mut sizes = ListCInt::new(());
    sizes.append(&ratios.0);
    sizes.append(&ratios.1);
    splitter.set_sizes(&sizes)
}

pub fn run(opts: Options, messages_in: GuiReceiver, messages_out: PlayerSender) -> ! {
    let on_exit = SlotNoArgs::new(|| {
        use types::PlayerMessage::AppQuit;

        messages_out.send(AppQuit).unwrap_or_default();
        if let Err(error) = messages_in.recv() {
            eprintln!("Error: {:?}", error)
        }
    });

    let keybinds = KeyBinds::from(opts.config.keybinds.unwrap_or_default());

    Application::create_and_exit(|app| unsafe {
        let core_app: &CoreApplication = app.static_cast();
        core_app.signals()
            .about_to_quit()
            .connect(&on_exit);

        // Setup window layout
        let mut main_window = MainWindow::new();
        let main_window_ptr = main_window.static_cast_mut() as *mut Widget;

        let mut splitter = Splitter::new(());
        let splitter_ptr = splitter.as_mut_ptr();
        let splitter_as_widget = splitter.static_cast_mut() as *mut Widget;

        let mut player_widget: *mut Widget = ptr::null_mut();
        let player_widget_ptr = &mut player_widget as *mut _;
        let mut chat_widget: *mut Widget = ptr::null_mut();
        let chat_widget_ptr = &mut chat_widget as *mut _;

        let mut chat_state = ChatState::Visible(ChatPosition::Right);
        let chat_state_ptr = &mut chat_state as *mut _;

        let mut widget_ratios = INITIAL_RATIOS;
        let widget_ratios_ptr = &mut widget_ratios as *mut _;

        let mut palette = Palette::new(());
        palette.set_color((ColorRole::Window, &Color::new(GlobalColor::Black)));
        main_window.set_palette(&palette);

        main_window.set_central_widget(splitter.static_cast_mut());
        main_window.resize((1280, 720));
        main_window.show_maximized();

        // Logic bits
        let resize = |splitter: &mut Splitter, ratios: &WidgetRatios| {
            let player_first = splitter.index_of(player_widget) == 0;
            let ratio_list = match player_first {
                true => (ratios.player, ratios.chat),
                false => (ratios.chat, ratios.player),
            };

            resize_splitter(&mut *splitter_ptr, ratio_list);
        };

        let query_ratios = |splitter: &mut Splitter| {
            let sizes = splitter.sizes();
            let player_index = splitter.index_of(player_widget);
            let chat_index = splitter.index_of(chat_widget);

            WidgetRatios {
                player: *sizes.at(player_index),
                chat: *sizes.at(chat_index),
            }
        };

        let arrange_splitter = |splitter: &mut Splitter| {
            if let ChatState::Visible(ref pos) = *chat_state_ptr {
                let (player_index, chat_index) = match *pos {
                    ChatPosition::Left => (1, 0),
                    ChatPosition::Right => (0, 1),
                };
                splitter.insert_widget(player_index, *player_widget_ptr);
                splitter.insert_widget(chat_index, *chat_widget_ptr);
                resize(splitter, &widget_ratios);
            } else {
                *widget_ratios_ptr = query_ratios(splitter);
                resize(splitter, &FULL_SCREEN_RATIOS);
            }
        };

        let handle_message = |message| {
            use types::GuiMessage::*;

            match message {
                PlayerError(reason) => {
                    critical("Fatal player error", &reason);
                    CoreApplication::quit();
                },
                Handles(player, chat) => {
                    *player_widget_ptr = widget_from_handle(player, splitter_as_widget);
                    *chat_widget_ptr = widget_from_handle(chat, splitter_as_widget);
                    arrange_splitter(&mut *splitter_ptr);
                },
                _ => { }
            }
        };

        // Player messages polling
        let mut poll_timer = Timer::new();
        let mut poll_messages = SlotNoArgs::default();
        poll_timer.signals()
            .timeout()
            .connect(&poll_messages);
        poll_timer.start(250);

        // Shortcuts
        let new_shortcut = |seq| Shortcut::new((seq, main_window_ptr));

        let fullscreen_shortcut = new_shortcut(&keybinds.fullscreen);
        let mut on_fullscreen = SlotNoArgs::default();
        fullscreen_shortcut.signals()
            .activated()
            .connect(&on_fullscreen);

        let chat_toggle_left_shortcut = new_shortcut(&keybinds.chat_toggle_left);
        let mut on_chat_toggle_left = SlotNoArgs::default();
        chat_toggle_left_shortcut.signals()
            .activated()
            .connect(&on_chat_toggle_left);

        let chat_toggle_right_shortcut = new_shortcut(&keybinds.chat_toggle_right);
        let mut on_chat_toggle_right = SlotNoArgs::default();
        chat_toggle_right_shortcut.signals()
            .activated()
            .connect(&on_chat_toggle_right);

        let change_channel_shortcut = new_shortcut(&keybinds.change_channel);
        let mut on_change_channel = SlotNoArgs::default();
        change_channel_shortcut.signals()
            .activated()
            .connect(&on_change_channel);

        // Slots
        poll_messages.set(|| {
            if let Ok(message) = messages_in.try_recv() {
                handle_message(message)
            }
        });

        on_fullscreen.set(|| toggle_full_screen(&mut *main_window_ptr));

        on_chat_toggle_left.set(|| {
            *chat_state_ptr = toggle_chat(&*chat_state_ptr, ChatPosition::Left);
            arrange_splitter(&mut *splitter_ptr);
        });

        on_chat_toggle_right.set(|| {
            *chat_state_ptr = toggle_chat(&*chat_state_ptr, ChatPosition::Right);
            arrange_splitter(&mut *splitter_ptr);
        });

        on_change_channel.set(|| {
            let channel = get_text("Change channel", "New channel");
            if channel.len() > 0 {
                let order = PlayerMessage::ChangeChannel(channel);
                messages_out.send(order).unwrap_or_default();
            }
        });

        Application::exec()
    })
}
