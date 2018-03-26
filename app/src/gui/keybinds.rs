use gui::qt_core::string::String as QString;
use gui::qt_gui::key_sequence::KeySequence;

use options::KeyConfig;

mod defaults {
    pub const FULLSCREEN:        &'static str = "F11";
    pub const CHAT_TOGGLE_LEFT:  &'static str = "Ctrl+Left";
    pub const CHAT_TOGGLE_RIGHT: &'static str = "Ctrl+Right";
    pub const CHANGE_CHANNEL:    &'static str = "Ctrl+O";
}

pub struct KeyBinds {
    pub fullscreen: KeySequence,
    pub chat_toggle_left: KeySequence,
    pub chat_toggle_right: KeySequence,
    pub change_channel: KeySequence
}

impl From<KeyConfig> for KeyBinds {
    fn from(config: KeyConfig) -> Self {
        use self::defaults::*;

        let make_seq = |opt_seq: Option<_>, def_seq| {
            let raw_seq = opt_seq.as_ref().map_or(def_seq, String::as_str);
            KeySequence::new(&QString::from(raw_seq))
        };

        Self {
            fullscreen:        make_seq(config.fullscreen,        FULLSCREEN),
            chat_toggle_left:  make_seq(config.chat_toggle_left,  CHAT_TOGGLE_LEFT),
            chat_toggle_right: make_seq(config.chat_toggle_right, CHAT_TOGGLE_RIGHT),
            change_channel:    make_seq(config.change_channel,    CHANGE_CHANNEL),
        }
    }
}
