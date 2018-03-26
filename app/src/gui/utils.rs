use gui::qt_core::{
    flags::{Flags, FlaggableEnum},
    list::ListCInt,
    qt::WindowState,
    string::String as QString,
};
use gui::qt_gui::window::Window;
use gui::qt_widgets::{
    input_dialog::InputDialog,
    message_box::MessageBox,
    splitter::Splitter,
    widget::Widget,
};

pub fn critical(title: &str, text: &str) {
    let args = (
        ::std::ptr::null_mut(),
        &QString::from_std_str(title.to_string()),
        &QString::from_std_str(text.to_string())
    );
    unsafe { MessageBox::critical(args) };
}

pub fn get_text(title: &str, text: &str) -> String {
    let args = (
        ::std::ptr::null_mut(),
        &QString::from_std_str(title.to_string()),
        &QString::from_std_str(text.to_string())
    );
    unsafe { InputDialog::get_text(args).to_std_string() }
}

pub fn widget_from_handle(handle: u64, parent: *mut Widget) -> *mut Widget {
    let window = Window::from_win_id(handle);
    unsafe { Widget::create_window_container((window, parent)) }
}

pub fn toggle_full_screen(widget: &mut Widget) {
    use self::WindowState::FullScreen;

    let state = widget.window_state();
    let new_state = match widget.is_full_screen() {
        true =>  state & Flags::from_int(!(FullScreen.to_flag_value())),
        false => state | Flags::from_enum(FullScreen)
    };
    widget.set_window_state(new_state);
}

pub fn resize_splitter(splitter: &mut Splitter, sizes: Vec<i32>) {
    let mut qt_sizes = ListCInt::new(());
    sizes.iter().for_each(|r| qt_sizes.append(r));
    splitter.set_sizes(&qt_sizes)
}
