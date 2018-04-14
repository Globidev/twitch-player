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
