use std::net::IpAddr;

#[derive(Debug, Deserialize)]
pub struct AccessToken {
    pub token: String,
    #[serde(rename = "sig")]
    pub signature: String
}

#[derive(Debug, Clone, Serialize)]
pub struct StreamIndex {
    pub twitch_info: TwitchInfo,
    pub playlist_infos: Vec<PlaylistInfo>,
    pub restricted_playlists_infos: Vec<RestrictedPlaylistInfo>,
}

#[derive(Debug, Clone, Serialize)]
pub struct TwitchInfo {
    pub node: String,
    pub manifest_node: String,
    pub manifest_node_type: String,
    pub cluster: String,
    pub manifest_cluster: String,
    pub suppress: bool,
    pub server_time: f64,
    pub transcode_stack: String,
    pub user_ip: IpAddr,
    pub serving_id: String,
    pub abs: bool,
    pub broadcast_id: String,
    pub stream_time: f64,
}

#[derive(Debug, Clone, Serialize)]
pub struct PlaylistInfo {
    pub media_info: MediaInfo,
    pub stream_info: StreamInfo,
    pub url: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct MediaInfo {
    pub type_: MediaType,
    pub group_id: String,
    pub name: String,
    pub autoselect: bool,
    pub default: bool,
}

#[derive(Debug, Clone, Serialize)]
pub enum MediaType {
    Video,
}

#[derive(Debug, Clone, Serialize)]
pub struct StreamInfo {
    pub program_id: u32,
    pub bandwidth: u64,
    pub resolution: Resolution,
    pub codecs: String,
    pub video: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct Resolution {
    pub width: u32,
    pub height: u32,
}

#[derive(Debug, Clone, Serialize)]
pub struct RestrictedPlaylistInfo {
    pub group_id: String,
    pub name: String,
    pub restriction: String,
}

#[derive(Debug, Clone, Serialize, PartialEq, Eq)]
pub struct Segment {
    pub location: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct Playlist {
    pub version: u32,
    pub target_duration: u32,
    pub media_sequence: u32,
    pub twitch_elapsed_secs: f64,
    pub twitch_total_secs: f64,
    pub segments: Vec<Segment>,
}

pub type Channel = String;
pub type Stream = (Channel, Quality);

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Quality {
    Exact(String),
    Approx(ApproxQuality)
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum ApproxQuality {
    Best,
    Worst
}
