extern crate nom;

use std::str::{from_utf8, Utf8Error};
use std::iter::FromIterator;

use std::collections::HashMap;
type RawInfoMap = HashMap<String, String>;

use twitch::types::{StreamIndex, PlaylistInfo, TwitchInfo, MediaInfo, MediaType, StreamInfo, Resolution};

#[derive(Debug)]
pub enum ParseError {
    FormatError(String),
    ExtractError(ExtractInfoError),
}

pub fn parse_stream_index(data: &[u8]) -> Result<StreamIndex, ParseError> {
    use nom::IResult::*;

    match stream_index_parser(data) {
        Done(_, result) => Ok(result),
        Error(e) => Err(ParseError::FormatError(e.to_string())),
        Incomplete(n) => Err(ParseError::FormatError(String::from("incomplete"))),
    }
}

fn to_string(data: &[u8]) -> Result<String, Utf8Error> {
    from_utf8(data).map(String::from)
}

named!(parse_quoted_string, do_parse!(
    tag!("\"") >>
    content: take_until!("\"") >>
    tag!("\"") >>
    (content)
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

named!(twitch_info_parser<TwitchInfo>, do_parse!(
    tag!("#EXT-X-TWITCH-INFO:") >>
    twitch_info: map_res!(raw_info_parser, extract_twitch_info) >>
    (twitch_info)
));

named!(media_info_parser<MediaInfo>, do_parse!(
    tag!("#EXT-X-MEDIA:") >>
    media_info: map_res!(raw_info_parser, extract_media_info) >>
    (media_info)
));

named!(stream_info_parser<StreamInfo>, do_parse!(
    tag!("#EXT-X-STREAM-INF:") >>
    stream_info: map_res!(raw_info_parser, extract_stream_info) >>
    (stream_info)
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

named!(stream_index_parser<StreamIndex>, do_parse!(
    tag!("#EXTM3U") >> tag!("\n") >>
    twitch_info: twitch_info_parser >> tag!("\n") >>
    playlist_infos: many1!(playlist_info_parser) >>
    (StreamIndex {
        twitch_info: twitch_info,
        playlist_infos: playlist_infos
    })
));

#[derive(Debug)]
pub enum ExtractInfoError {
    MissingKey(String),
    FromError(String)
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
        autoselect: extract_raw(&raw_info, "AUTOSELECT")?,
        default:    extract_raw(&raw_info, "DEFAULT")?,
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

fn extract_raw<T : Extractable>(raw_info: &RawInfoMap, key: &str) -> Result<T, ExtractInfoError> {
    if let Some(value) = raw_info.get(key) {
        Extractable::extract(value)
    } else {
        Err(ExtractInfoError::MissingKey(key.to_string()))
    }
}

trait Extractable: Sized {
    fn extract(string: &String) -> Result<Self, ExtractInfoError>;
}

impl Extractable for String {
    fn extract(string: &String) -> Result<String, ExtractInfoError> {
        Ok(string.clone())
    }
}

impl Extractable for bool {
    fn extract(string: &String) -> Result<bool, ExtractInfoError> {
        let lowercased = string.to_lowercase();
        if lowercased == "true" || lowercased == "yes" {
            Ok(true)
        } else if lowercased == "false" || lowercased == "no" {
            Ok(false)
        } else {
            Err(ExtractInfoError::FromError(String::from("Bad boolean value")))
        }
    }
}

use std::net::{IpAddr, AddrParseError};

impl Extractable for IpAddr {
    fn extract(string: &String) -> Result<IpAddr, ExtractInfoError> {
        string.parse().map_err(|e: AddrParseError| ExtractInfoError::FromError(e.to_string()))
    }
}

impl Extractable for f64 {
    fn extract(string: &String) -> Result<f64, ExtractInfoError> {
        use std::num::ParseFloatError;

        string.parse().map_err(|e: ParseFloatError| ExtractInfoError::FromError(e.to_string()))
    }
}

impl Extractable for u32 {
    fn extract(string: &String) -> Result<u32, ExtractInfoError> {
        use std::num::ParseIntError;

        string.parse().map_err(|e: ParseIntError| ExtractInfoError::FromError(e.to_string()))
    }
}

impl Extractable for u64 {
    fn extract(string: &String) -> Result<u64, ExtractInfoError> {
        use std::num::ParseIntError;

        string.parse().map_err(|e: ParseIntError| ExtractInfoError::FromError(e.to_string()))
    }
}

impl Extractable for Resolution {
    fn extract(string: &String) -> Result<Resolution, ExtractInfoError> {
        use std::num::ParseIntError;

        let components: Vec<_> = string.split('x').collect();
        if components.len() < 2 {
            Err(ExtractInfoError::FromError(String::from("Bad resolution value")))
        } else {
            let width = components[0].parse()
                .map_err(|e: ParseIntError| ExtractInfoError::FromError(e.to_string()))?;
            let height = components[1].parse()
                .map_err(|e: ParseIntError| ExtractInfoError::FromError(e.to_string()))?;

            let resolution = Resolution { width: width, height: height };
            Ok(resolution)
        }
    }
}

impl Extractable for MediaType {
    fn extract(string: &String) -> Result<MediaType, ExtractInfoError> {
        if string.to_lowercase() == "video" {
            Ok(MediaType::Video)
        } else {
            Err(ExtractInfoError::FromError(String::from("Bad media type value")))
        }
    }
}
