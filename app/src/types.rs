use std::sync::mpsc;

pub enum PlayerMessage {
    AppQuit
}

pub enum GuiMessage {
    PlayerError(String),
    CanExit
}

pub type PlayerSender = mpsc::Sender<PlayerMessage>;
pub type PlayerReceiver = mpsc::Receiver<PlayerMessage>;

pub type GuiSender = mpsc::Sender<GuiMessage>;
pub type GuiReceiver = mpsc::Receiver<GuiMessage>;

pub fn player_channel() -> (PlayerSender, PlayerReceiver) {
    mpsc::channel()
}

pub fn gui_channel() -> (GuiSender, GuiReceiver) {
    mpsc::channel()
}
