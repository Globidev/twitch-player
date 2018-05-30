extern crate nom;

use std::str::{from_utf8, Utf8Error};
use std::iter::FromIterator;

use std::collections::HashMap;
type RawInfoMap = HashMap<String, String>;

use twitch::types::*;

#[derive(Debug)]
pub struct ParseError(pub String);

pub fn parse_playlist(data: &[u8]) -> Result<Playlist, ParseError> {
    use nom::IResult::*;

    match playlist_parser(data) {
        Done(_, result) => Ok(result),
        Error(e) => {
            let detail = format!("Error: {}", e);
            Err(ParseError(detail))
        },
        Incomplete(n) => {
            let detail = format!("Incomplete: {:?}", n);
            Err(ParseError(detail))
        }
    }
}

pub fn parse_stream_index(data: &[u8]) -> Result<StreamIndex, ParseError> {
    use nom::IResult::*;

    match stream_index_parser(data) {
        Done(_, result) => Ok(result),
        Error(e) => {
            let detail = format!("Error: {}", e);
            Err(ParseError(detail))
        },
        Incomplete(n) => {
            let detail = format!("Incomplete: {:?}", n);
            Err(ParseError(detail))
        }
    }
}

fn to_string(data: &[u8]) -> Result<String, Utf8Error> {
    from_utf8(data).map(String::from)
}

fn from_raw<T: FromStr>(data: &[u8]) -> Option<T> {
    from_utf8(data).ok()
        .and_then(|s| FromStr::from_str(s).ok())
}

named!(parse_quoted_string, delimited!(
    tag!("\""), take_until!("\""), tag!("\"")
));

named!(parse_key_value<(String, String)>, separated_pair!(
    map_res!(take_until!("="), to_string),
    tag!("="),
    map_res!(
        alt!(parse_quoted_string | take_while!(|c| c != b'\n' && c != b',')),
        to_string
    )
));

named!(raw_info_parser<RawInfoMap>, map!(
    separated_list!(tag!(","), parse_key_value),
    FromIterator::from_iter
));

named!(twitch_info_parser<TwitchInfo>, preceded!(
    tag!("#EXT-X-TWITCH-INFO:"),
    map_res!(raw_info_parser, extract_twitch_info)
));

named!(media_info_parser<MediaInfo>, preceded!(
    tag!("#EXT-X-MEDIA:"),
    map_res!(raw_info_parser, extract_media_info)
));

named!(stream_info_parser<StreamInfo>, preceded!(
    tag!("#EXT-X-STREAM-INF:"),
    map_res!(raw_info_parser, extract_stream_info)
));

named!(playlist_info_parser<PlaylistInfo>, do_parse!(
    media_info:  media_info_parser                      >> tag!("\n") >>
    stream_info: stream_info_parser                     >> tag!("\n") >>
    url:         map_res!(take_until!("\n"), to_string) >> tag!("\n") >>
    (PlaylistInfo {
        media_info: media_info,
        stream_info: stream_info,
        url: url
    })
));

named!(restricted_playlist_info_parser<RestrictedPlaylistInfo>, delimited!(
    tag!("#EXT-X-TWITCH-RESTRICTED:"),
    map_res!(raw_info_parser, extract_restricted_playlist_info),
    tag!("\n")
));

named!(stream_index_parser<StreamIndex>, do_parse!(
    tag!("#EXTM3U") >> tag!("\n") >>
    twitch_info: twitch_info_parser >> tag!("\n") >>
    restricted_playlists_infos: many0!(restricted_playlist_info_parser) >>
    playlist_infos: many1!(playlist_info_parser) >>
    (StreamIndex {
        twitch_info: twitch_info,
        playlist_infos: playlist_infos,
        restricted_playlists_infos: restricted_playlists_infos
    })
));

named!(playlist_version_parser<u32>, preceded!(
    tag!("#EXT-X-VERSION:"),
    map_opt!(take_until!("\n"), from_raw::<u32>)
));

named!(playlist_target_duration_parser<u32>, preceded!(
    tag!("#EXT-X-TARGETDURATION:"),
    map_opt!(take_until!("\n"), from_raw::<u32>)
));

named!(playlist_media_sequence_parser<u32>, preceded!(
    tag!("#EXT-X-MEDIA-SEQUENCE:"),
    map_opt!(take_until!("\n"), from_raw::<u32>)
));

named!(playlist_twitch_elapsed_secs_parser<f64>, preceded!(
    tag!("#EXT-X-TWITCH-ELAPSED-SECS:"),
    map_opt!(take_until!("\n"), from_raw::<f64>)
));

named!(playlist_twitch_total_secs_parser<f64>, preceded!(
    tag!("#EXT-X-TWITCH-TOTAL-SECS:"),
    map_opt!(take_until!("\n"), from_raw::<f64>)
));

named!(extinf_segment_parser<Segment>, do_parse!(
    tag!("#EXTINF:") >> take_until!("\n") >> tag!("\n") >>
    url:  map_res!(take_until!("\n"), to_string) >> tag!("\n") >>
    (Segment { url })
));

named!(prefetch_segment_parser<Segment>, do_parse!(
    tag!("#EXT-X-TWITCH-PREFETCH:") >>
    url:  map_res!(take_until!("\n"), to_string) >> tag!("\n") >>
    (Segment { url })
));

named!(playlist_segment_parser<Segment>, alt!(
    extinf_segment_parser |
    prefetch_segment_parser
));

named!(playlist_parser<Playlist>, do_parse!(
    tag!("#EXTM3U") >> tag!("\n") >>
    version: playlist_version_parser >> tag!("\n") >>
    target_duration: playlist_target_duration_parser >> tag!("\n") >>
    media_sequence: playlist_media_sequence_parser >> tag!("\n") >>
    twitch_elapsed_secs: playlist_twitch_elapsed_secs_parser >> tag!("\n") >>
    twitch_total_secs: playlist_twitch_total_secs_parser >> tag!("\n") >>
    segments: many0!(playlist_segment_parser) >>
    (Playlist {
        version,
        target_duration,
        media_sequence,
        twitch_elapsed_secs,
        twitch_total_secs,
        segments,
    })
));

#[derive(Debug)]
enum ExtractInfoError {
    MissingKey(String),
    BadValue(BadValueError),
}

#[derive(Debug)]
struct BadValueError {
    raw_value: String,
    detail: String,
}

impl BadValueError {
    fn new(raw_value: &str, detail: &str) -> Self {
        Self {
            raw_value: String::from(raw_value),
            detail: String::from(detail)
        }
    }
}

fn extract_twitch_info(raw_info: RawInfoMap) -> Result<TwitchInfo, ExtractInfoError> {
    let twitch_info = TwitchInfo {
        node:               extract_raw(&raw_info, "NODE")?,
        manifest_node:      extract_raw(&raw_info, "MANIFEST-NODE")?,
        manifest_node_type: extract_raw(&raw_info, "MANIFEST-NODE-TYPE")?,
        cluster:            extract_raw(&raw_info, "CLUSTER")?,
        manifest_cluster:   extract_raw(&raw_info, "MANIFEST-CLUSTER")?,
        suppress:           extract_raw(&raw_info, "SUPPRESS")?,
        server_time:        extract_raw(&raw_info, "SERVER-TIME")?,
        transcode_stack:    extract_raw(&raw_info, "TRANSCODESTACK")?,
        user_ip:            extract_raw(&raw_info, "USER-IP")?,
        serving_id:         extract_raw(&raw_info, "SERVING-ID")?,
        abs:                extract_raw(&raw_info, "ABS")?,
        broadcast_id:       extract_raw(&raw_info, "BROADCAST-ID")?,
        stream_time:        extract_raw(&raw_info, "STREAM-TIME")?,
    };

    Ok(twitch_info)
}

fn extract_media_info(raw_info: RawInfoMap) -> Result<MediaInfo, ExtractInfoError> {
    let media_info = MediaInfo {
        type_:      extract_raw(&raw_info, "TYPE")?,
        group_id:   extract_raw(&raw_info, "GROUP-ID")?,
        name:       extract_raw(&raw_info, "NAME")?,
        autoselect: extract_raw::<BooleanLike>(&raw_info, "AUTOSELECT")?.into(),
        default:    extract_raw::<BooleanLike>(&raw_info, "DEFAULT")?.into(),
    };

    Ok(media_info)
}

fn extract_stream_info(raw_info: RawInfoMap) -> Result<StreamInfo, ExtractInfoError> {
    let stream_info = StreamInfo {
        program_id: extract_raw(&raw_info, "PROGRAM-ID")?,
        bandwidth:  extract_raw(&raw_info, "BANDWIDTH")?,
        resolution: extract_raw(&raw_info, "RESOLUTION")?,
        codecs:     extract_raw(&raw_info, "CODECS")?,
        video:      extract_raw(&raw_info, "VIDEO")?,
    };

    Ok(stream_info)
}

fn extract_restricted_playlist_info(raw_info: RawInfoMap)
    -> Result<RestrictedPlaylistInfo, ExtractInfoError>
{
    let restricted_playlist_info = RestrictedPlaylistInfo {
        group_id:    extract_raw(&raw_info, "GROUP-ID")?,
        name:        extract_raw(&raw_info, "NAME")?,
        restriction: extract_raw(&raw_info, "RESTRICTION")?,
    };

    Ok(restricted_playlist_info)
}

fn extract_raw<T : Extractable>(raw_info: &RawInfoMap, key: &str) -> Result<T, ExtractInfoError> {
    if let Some(raw_value) = raw_info.get(key) {
        Extractable::extract(raw_value).map_err(ExtractInfoError::BadValue)
    } else {
        Err(ExtractInfoError::MissingKey(key.to_string()))
    }
}

struct BooleanLike(pub bool);

impl Into<bool> for BooleanLike {
    fn into(self) -> bool {
        let BooleanLike(wrapped) = self;
        wrapped
    }
}

trait Extractable: Sized {
    fn extract(s: &String) -> Result<Self, BadValueError>;
}

use std::str::FromStr;
use std::fmt::Display;

impl<T> Extractable for T
where
    T:                   FromStr,
    <T as FromStr>::Err: Display
{
    fn extract(s: &String) -> Result<T, BadValueError> {
        s.parse().map_err(|e| {
            let detail = format!("FromStr error: {}", e);
            BadValueError::new(s, &detail)
        })
    }
}

impl Extractable for BooleanLike {
    fn extract(s: &String) -> Result<BooleanLike, BadValueError> {
        match &s.to_lowercase() as &str {
            "true"  | "yes" => Ok(BooleanLike(true)),
            "false" | "no"  => Ok(BooleanLike(false)),
            _ => Err(BadValueError::new(s, "Invalid boolean-like value"))
        }
    }
}

impl Extractable for Resolution {
    fn extract(s: &String) -> Result<Resolution, BadValueError> {
        let components: Vec<_> = s.split('x').collect();

        if components.len() < 2 {
            Err(BadValueError::new(s, "Missing resolution component"))
        } else {
            let parse_error = |e| {
                let detail = format!("Invalid resolution component: {}", e);
                BadValueError::new(s, &detail)
            };

            let resolution = Resolution {
                width: components[0].parse().map_err(parse_error)?,
                height: components[1].parse().map_err(parse_error)?,
            };

            Ok(resolution)
        }
    }
}

impl Extractable for MediaType {
    fn extract(s: &String) -> Result<MediaType, BadValueError> {
        match &s.to_lowercase() as &str {
            "video" => Ok(MediaType::Video),
            _       => Err(BadValueError::new(s, "Invalid media type value"))
        }
    }
}
