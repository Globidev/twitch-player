extern crate qt_core;
extern crate qt_gui;
extern crate qt_widgets;
extern crate cpp_utils;
extern crate winapi;

use self::qt_widgets::application::Application;
use self::qt_widgets::widget::Widget;
use self::qt_widgets::main_window::MainWindow;
use self::qt_widgets::message_box::MessageBox;
use self::qt_widgets::splitter::Splitter;
use self::qt_core::slots::SlotNoArgs;
use self::qt_widgets::shortcut::Shortcut;
use self::qt_core::core_application::CoreApplication;
use self::qt_core::connection::Signal;
use self::qt_core::timer::Timer;
use self::qt_core::string::String as QString;
use self::qt_core::list::ListCInt;
use self::cpp_utils::StaticCast;
use self::qt_gui::window::Window;
use self::qt_gui::palette::{Palette, ColorRole};
use self::qt_gui::color::Color;
use self::qt_gui::key_sequence::{KeySequence, StandardKey};
use self::qt_core::qt::GlobalColor;
use self::qt_core::qt::WindowState;
use self::qt_core::flags::{Flags, FlaggableEnum};

use types::{GuiReceiver, PlayerSender};
use windows::find_window_by_name;
use options::Options;

use std::ptr;
use self::winapi::shared::windef::HWND;

fn find_windows(channel: &str) -> Option<(HWND, HWND)> {
    use process::vlc_title;

    let vlc = find_window_by_name(|title| title.contains(&vlc_title(channel)))?;
    let chat = find_window_by_name(|title| title == "Twitch")?;

    Some((vlc, chat))
}

fn critical(title: &str, text: &str) {
    let args = (
        ptr::null_mut(),
        &QString::from_std_str(title.to_string()),
        &QString::from_std_str(text.to_string())
    );
    unsafe { MessageBox::critical(args) };
}

fn widget_from_handle(handle: HWND) -> *mut Widget {
    unsafe {
        let window = Window::from_win_id(handle as u64);
        Widget::create_window_container(window)
    }
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

fn resize_splitter(splitter: &mut Splitter, first: i32, second: i32) {
    let mut sizes = ListCInt::new(());
    sizes.append(&first); 
    sizes.append(&second);
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

    let poll_messages = SlotNoArgs::new(|| {
        use types::GuiMessage::*;

        if let Ok(PlayerError(reason)) = messages_in.try_recv() {
            critical("Player error", &reason);
            CoreApplication::quit();
        }
    });

    Application::create_and_exit(|app| {
        // Setup window layout
        let mut main_window = MainWindow::new();
        main_window.resize((1280, 720));
        main_window.show_maximized();

        let mut splitter = Splitter::new(());
        let splitter_ptr = splitter.as_mut_ptr();

        let mut palette = Palette::new(());
        palette.set_color((ColorRole::Window, &Color::new(GlobalColor::Black)));
        main_window.set_palette(&palette);

        unsafe { 
            main_window.set_central_widget(splitter.static_cast_mut());
        }

        // Events
        let core_app: &CoreApplication = app.static_cast();
        core_app.signals().about_to_quit().connect(&on_exit);

        let mut poll_timer = Timer::new();
        poll_timer.signals().timeout().connect(&poll_messages);
        poll_timer.start(250);

        // VLC + Chat embedding
        let mut grab_timer = Timer::new();
        let mut grab_windows = SlotNoArgs::default();
        grab_timer.signals().timeout().connect(&grab_windows);
        grab_timer.start(250);
        grab_windows.set(|| {
            if let Some((vlc, chat)) = find_windows(&opts.channel) {
                unsafe {
                    let splitter = splitter_ptr.as_mut().unwrap();
                    splitter.add_widget(widget_from_handle(vlc));
                    splitter.add_widget(widget_from_handle(chat));
                    resize_splitter(splitter, 80, 20);
                }
                grab_timer.stop();
            }
        });

        let main_window_ptr = main_window.static_cast_mut() as *mut Widget;

        // Fullscreen handling
        let fullscreen_seq = KeySequence::new(StandardKey::FullScreen);
        let fullscreen_shortcut = unsafe { 
            Shortcut::new((&fullscreen_seq, main_window_ptr)) 
        };
        let mut on_fullscreen_request = SlotNoArgs::default();
        fullscreen_shortcut.signals()
            .activated()
            .connect(&on_fullscreen_request);
        on_fullscreen_request.set(|| 
            toggle_full_screen(unsafe { main_window_ptr.as_mut().unwrap() })
        );

        // Chat Toggle
        let mut toggle_state = true;
        let toggle_chat_seq = KeySequence::new(StandardKey::MoveToNextWord);
        let toggle_chat_shortcut = unsafe {
            Shortcut::new((&toggle_chat_seq, main_window_ptr))
        };
        let mut on_toggle_chat_request = SlotNoArgs::default();
        toggle_chat_shortcut.signals()
            .activated()
            .connect(&on_toggle_chat_request);
        on_toggle_chat_request.set(|| {
            toggle_state = !toggle_state;
            let (s1, s2) = match toggle_state {
                true  => (100, 0),
                false => (80, 20)
            };
            unsafe { resize_splitter(splitter_ptr.as_mut().unwrap(), s1, s2) }
        });
        
        Application::exec()
    })
}
