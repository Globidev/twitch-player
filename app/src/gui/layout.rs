use gui::cpp_utils::{CppBox, StaticCast};
use gui::qt_widgets::{splitter::Splitter, widget::Widget};

use gui::utils::{widget_from_handle, resize_splitter};

struct WidgetSizes {
    chat: i32,
    player: i32
}

const INITIAL_SIZES: WidgetSizes     = WidgetSizes { chat: 20, player: 80 };
const FULL_SCREEN_SIZES: WidgetSizes = WidgetSizes { chat: 0,  player: 100 };

#[derive(PartialEq, Eq)]
pub enum ChatPosition { Left, Right }

enum ChatState {
    Visible(ChatPosition),
    Hidden,
}

pub struct Layout {
    splitter: CppBox<Splitter>,
    player_widget: *mut Widget,
    chat_widget: *mut Widget,
    chat_state: ChatState,
    widget_sizes: WidgetSizes
}

impl Layout {
    pub unsafe fn new(player_handle: u64, chat_handle: u64) -> Self {
        let mut splitter = Splitter::new(());

        let (player_widget, chat_widget) = {
            let splitter_as_widget = splitter.static_cast_mut();
            (
                widget_from_handle(player_handle, splitter_as_widget),
                widget_from_handle(chat_handle, splitter_as_widget),
            )
        };

        let mut layout = Self {
            splitter: splitter,
            player_widget: player_widget,
            chat_widget: chat_widget,
            chat_state: ChatState::Visible(ChatPosition::Right),
            widget_sizes: INITIAL_SIZES,
        };

        layout.rearrange();

        layout
    }

    pub fn splitter_as_widget(&mut self) -> *mut Widget {
        self.splitter.static_cast_mut()
    }

    pub unsafe fn toggle_chat(&mut self, position: ChatPosition) {
        self.chat_state = toggle_chat_state(&self.chat_state, position);
        self.rearrange();
    }

    unsafe fn rearrange(&mut self) {
        if let ChatState::Visible(ref position) = self.chat_state {
            let (player_index, chat_index) = match *position {
                ChatPosition::Left => (1, 0),
                ChatPosition::Right => (0, 1),
            };
            self.splitter.insert_widget(player_index, self.player_widget);
            self.splitter.insert_widget(chat_index, self.chat_widget);
        } else {
            self.widget_sizes = self.query_sizes();
        }

        self.resize();
    }

    unsafe fn query_sizes(&self) -> WidgetSizes {
        let sizes = self.splitter.sizes();
        let player_index = self.splitter.index_of(self.player_widget);
        let chat_index = self.splitter.index_of(self.chat_widget);

        WidgetSizes {
            player: *sizes.at(player_index),
            chat: *sizes.at(chat_index),
        }
    }

    unsafe fn resize(&mut self) {
        let sizes = match self.chat_state {
            ChatState::Visible(_) => &self.widget_sizes,
            ChatState::Hidden => &FULL_SCREEN_SIZES,
        };
        let player_first = self.splitter.index_of(self.player_widget) == 0;
        let new_sizes = match player_first {
            true => vec![sizes.player, sizes.chat],
            false => vec![sizes.chat, sizes.player],
        };

        resize_splitter(&mut self.splitter, new_sizes);
    }
}

fn toggle_chat_state(state: &ChatState, new_position: ChatPosition) -> ChatState {
    use self::ChatState::{Hidden, Visible};

    match *state {
        Hidden => Visible(new_position),
        Visible(ref position) => {
            match *position == new_position {
                true => Hidden,
                false => Visible(new_position)
            }
        }
    }
}
