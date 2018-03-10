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

use types::{GuiReceiver, PlayerSender};
use windows::find_window_by_name;

use std::ptr;
use self::winapi::shared::windef::HWND;

fn find_windows() -> Option<(HWND, HWND)> {
    let vlc = find_window_by_name(|title| title == "VLC media player")?;
    // let chat = find_window_by_name(|title| title.starts_with("Twitch"))?;

    // Some((vlc, chat))
    Some((vlc, vlc))
}

pub fn run(messages_in: GuiReceiver, messages_out: PlayerSender) -> ! {
    let on_exit = SlotNoArgs::new(|| {
        use types::PlayerMessage::AppQuit;

        messages_out.send(AppQuit).unwrap();
        messages_in.recv().unwrap();
    });

    let poll_messages = SlotNoArgs::new(|| {
        use types::GuiMessage::*;

        if let Ok(message) = messages_in.try_recv() {
            match message {
                PlayerError(reason) => {
                    let args = (
                        ptr::null_mut(),
                        &QString::from_std_str(&"Player error".to_string()),
                        &QString::from_std_str(&reason)
                    );
                    unsafe { MessageBox::critical(args) };
                    CoreApplication::quit();
                },
                CanExit => unreachable!()
            }
        }
    });

    Application::create_and_exit(|app| {
        // Setup window layout
        let mut main_window = MainWindow::new();

        let mut splitter = Splitter::new(());

        unsafe { 
            main_window.set_central_widget((splitter.static_cast_mut())); 
        }

        // Events
        let core_app: &CoreApplication = app.static_cast();
        core_app.signals().about_to_quit().connect(&on_exit);

        let mut poll_timer = Timer::new();
        poll_timer.signals().timeout().connect(&poll_messages);
        poll_timer.start(500);

        let mut grab_timer = Timer::new();
        let grab_windows = SlotNoArgs::new(|| {
            if let Some((vlc_handle, chat_handle)) = find_windows() {
                unsafe {
                    let window = Window::from_win_id(vlc_handle as u64);
                    let widget = Widget::create_window_container(window);
                    splitter.add_widget(widget);
                    grab_timer.stop();
                }
            }
        });
        poll_timer.signals().timeout().connect(&grab_windows);
        poll_timer.start(250);

        // use std::cell::RefCell;

        // let win_ref: RefCell<Option<*mut Window>> = RefCell::new(None);
        // let detach_vlc = SlotNoArgs::new(|| {
        //     if let Some(window) = *win_ref.borrow() {
        //         println!("DETACHING...");
        //         unsafe { (*window).set_parent(ptr::null_mut()); }
        //     }
        // });

        main_window.show();
        Application::exec()
    })
}
