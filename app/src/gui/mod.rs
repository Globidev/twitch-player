extern crate qt_core;
extern crate qt_gui;
extern crate qt_widgets;
extern crate cpp_utils;

use self::cpp_utils::StaticCast;
use self::qt_core::{
    connection::Signal,
    core_application::CoreApplication,
    qt::GlobalColor,
    slots::SlotNoArgs,
    timer::Timer,
};
use self::qt_gui::{
    color::Color,
    palette::{Palette, ColorRole},
};
use self::qt_widgets::{
    application::Application,
    main_window::MainWindow,
    shortcut::Shortcut,
    widget::Widget,
};

use types::{GuiReceiver, PlayerSender, PlayerMessage};
use options::Options;

mod keybinds;
use self::keybinds::KeyBinds;

mod layout;
use self::layout::{Layout, ChatPosition};

mod utils;
use self::utils::{critical, toggle_full_screen, get_text};

use std::cell::RefCell;

pub fn run(opts: Options, messages_in: GuiReceiver, messages_out: PlayerSender) -> ! {
    use types::PlayerMessage::AppQuit;
    use types::GuiMessage::{PlayerError, Handles};

    let on_exit = SlotNoArgs::new(|| {
        messages_out.send(AppQuit).unwrap_or_default();
        if let Err(error) = messages_in.recv() {
            eprintln!("Error: {:?}", error)
        }
    });

    let keybinds = KeyBinds::from(opts.config.keybinds.unwrap_or_default());

    let layout: RefCell<Option<Layout>> = RefCell::new(None);

    Application::create_and_exit(|app| unsafe {
        let core_app: &CoreApplication = app.static_cast();
        core_app.signals()
            .about_to_quit()
            .connect(&on_exit);

        // Setup top level window
        let mut main_window = MainWindow::new();
        let main_window_ptr = main_window.static_cast_mut() as *mut Widget;

        let mut palette = Palette::new(());
        palette.set_color((ColorRole::Window, &Color::new(GlobalColor::Black)));
        main_window.set_palette(&palette);

        main_window.resize((1280, 720));
        main_window.show_maximized();

        // Player messages dispatch
        let mut handle_message = |message| {
            match message {
                PlayerError(reason) => {
                    critical("Fatal player error", &reason);
                    CoreApplication::quit();
                },
                Handles(player, chat) => {
                    let mut new_layout = Layout::new(player, chat);
                    main_window.set_central_widget(new_layout.splitter_as_widget());
                    layout.replace(Some(new_layout));
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
        let make_shortcut = |seq| {
            let shortcut = Shortcut::new((seq, main_window_ptr));
            let slot = SlotNoArgs::default();
            shortcut.signals().activated().connect(&slot);
            (shortcut, slot)
        };
        // /!\ Keep the shortcut named, don't use only `_`
        // Otherwise CppBox drops from the helper above
        let (_s, mut on_fullscreen)        = make_shortcut(&keybinds.fullscreen);
        let (_s, mut on_chat_toggle_left)  = make_shortcut(&keybinds.chat_toggle_left);
        let (_s, mut on_chat_toggle_right) = make_shortcut(&keybinds.chat_toggle_right);
        let (_s, mut on_change_channel)    = make_shortcut(&keybinds.change_channel);

        // Slots
        poll_messages.set(|| {
            if let Ok(message) = messages_in.try_recv() {
                handle_message(message)
            }
        });

        on_fullscreen.set(|| toggle_full_screen(&mut *main_window_ptr));

        on_chat_toggle_left.set(|| {
            if let Some(ref mut l) = layout.borrow_mut().as_mut() {
                l.toggle_chat(ChatPosition::Left)
            }
        });

        on_chat_toggle_right.set(|| {
            if let Some(ref mut l) = layout.borrow_mut().as_mut() {
                l.toggle_chat(ChatPosition::Right)
            }
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
