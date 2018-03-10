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
use self::qt_widgets::push_button::PushButton;
use self::qt_core::slots::SlotNoArgs;
use self::qt_core::core_application::CoreApplication;
use self::qt_core::connection::Signal;
use self::qt_core::timer::Timer;
use self::qt_core::string::String as QString;
use self::cpp_utils::{StaticCast, CppBox};
use self::qt_gui::window::Window;
use self::qt_gui::palette::{Palette, ColorRole};
use self::qt_gui::color::Color;
use self::qt_core::qt::GlobalColor;

use types::{GuiReceiver, PlayerSender};
use windows::find_window_by_name;

use std::ptr;
use self::winapi::shared::windef::HWND;

fn find_windows() -> Option<(HWND, HWND)> {
    let vlc = find_window_by_name(|title| title == "VLC media player")?;
    let chat = find_window_by_name(|title| title.starts_with("Twitch"))?;

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

pub fn run(messages_in: GuiReceiver, messages_out: PlayerSender) -> ! {
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
        let mut splitter = Splitter::new(());

        let mut palette = Palette::new(());
        palette.set_color((ColorRole::Window, &Color::new(GlobalColor::Black)));
        main_window.set_palette(&palette);

        unsafe { 
            main_window.set_central_widget((splitter.static_cast_mut()));
        }

        let mut add_widget_from_handle = |handle: HWND| {
            unsafe {
                let window = Window::from_win_id(handle as u64);
                let widget = Widget::create_window_container(window);
                splitter.add_widget(widget);
            }
        };

        // Events
        let core_app: &CoreApplication = app.static_cast();
        core_app.signals().about_to_quit().connect(&on_exit);

        let mut poll_timer = Timer::new();
        poll_timer.signals().timeout().connect(&poll_messages);
        poll_timer.start(250);

        let mut grab_timer = Timer::new();
        let grab_windows = SlotNoArgs::new(|| {
            if let Some((vlc_handle, chat_handle)) = find_windows() {
                add_widget_from_handle(vlc_handle);
                add_widget_from_handle(chat_handle);
                grab_timer.stop();
            }
        });
        poll_timer.signals().timeout().connect(&grab_windows);
        poll_timer.start(250);

        // Startup
        main_window.resize((1280, 720));
        main_window.show_maximized();
        Application::exec()
    })
}
