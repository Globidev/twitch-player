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

#[derive(Deserialize)]
pub struct Resolution {
    pub height: u32,
    pub width: u32,
}

#[derive(Deserialize)]
pub struct PlaylistInfo {
    pub url: String,
    pub name: String,
    pub bandwidth: u64,
    pub resolution: Resolution,
}

#[derive(Deserialize)]
pub struct PlaylistsInfo {
    pub channel: String,
    pub playlists: Vec<PlaylistInfo>,
}

pub type SegmentUrl = String;
pub type Playlist = Vec<SegmentUrl>;
