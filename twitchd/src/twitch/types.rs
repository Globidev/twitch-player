use std::net::IpAddr;

#[derive(Debug, Deserialize)]
pub struct AccessToken {
    pub token: String,
    #[serde(rename = "sig")]
    pub signature: String
}

#[derive(Debug)]
pub struct StreamIndex {
    pub twitch_info: TwitchInfo,
    pub playlist_infos: Vec<PlaylistInfo>,
}

#[derive(Debug)]
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

#[derive(Debug)]
pub struct PlaylistInfo {
    pub media_info: MediaInfo,
    pub stream_info: StreamInfo,
    pub url: String,
}

#[derive(Debug)]
pub struct MediaInfo {
    pub type_: MediaType,
    pub group_id: String,
    pub name: String,
    pub autoselect: bool,
    pub default: bool,
}

#[derive(Debug)]
pub enum MediaType {
    Video,
}

#[derive(Debug)]
pub struct StreamInfo {
    pub program_id: u32,
    pub bandwidth: u64,
    pub resolution: Resolution,
    pub codecs: String,
    pub video: String,
}

#[derive(Debug)]
pub struct Resolution {
    pub width: u32,
    pub height: u32,
}

// #EXT-X-MEDIA:
// TYPE=VIDEO
// GROUP-ID="chunked"
// NAME="720p (source)"
// AUTOSELECT=YES
// DEFAULT=YES
// #EXT-X-STREAM-INF:
// PROGRAM-ID=1
// BANDWIDTH=2661158
// RESOLUTION=1280x720
// CODECS="avc1.4D0020,mp4a.40.2"
// VIDEO="chunked"
// http://video-weaver.cdg02.hls.ttvnw.net/v1/playlist/CtcCB4WZfa6afo2HEajAgO5Td3aBMVSRK9yhGgwmMVWCHw0pOEmld_3Ge7agu7laWyyjO1VP-9qVhV0s8qIMrYuE3shEB7OfkXyqIWteEFoR77GQLLp_COEYeHatEHZjyjFE1OUtuU5VqpoASbfcw8MSmTqcs2SkK-VcuK2iKNvfj3Q8Rua4kRFp0puyBespJGwMA_TA8JOtFiV3xTMiE8pz-bICpy3TBC8ITWFSlnufP8dXLKWuDB-v6umBuUI6hocGBC_--SKOcF5cc6or5fGxcUY58Fp_YxNv8nUhT1OVTE5WE8MDEnSyVPSl0zKizsxlLrutWfBTcxzxeoktGGDDKWfK4osFR0s6ZhRRxjCOJMoOBzUU4vLf9ZOVasAYXUFZPUqoPxEw-rdaOpHq8yAy7L5NOrBA5ddmNE8qQiFUUYXDYkCydxJotopcjzQ30WEnS1znlmXZRRIQBG45WtTkAZku02cWiNtUDxoM6Vvf2iCwB64GgNGQ.m3u8
// #EXT-X-TWITCH-INFO:
// NODE="video-edge-c553a4.cdg02"
// MANIFEST-NODE-TYPE="weaver_cluster"
// CLUSTER="cdg02"
// SUPPRESS="false"
// SERVER-TIME="1523458708.55"
// TRANSCODESTACK="2017TranscodeX264_V2"
// USER-IP="92.169.154.151"
// SERVING-ID="700788e57a7e4802a2950a610f0c19fe"
// MANIFEST-NODE="video-weaver.cdg02"
// ABS="false"
// BROADCAST-ID="28290650736"
// STREAM-TIME="5242.54913306"
// MANIFEST-CLUSTER="cdg02"
