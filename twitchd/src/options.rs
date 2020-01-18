use crate::prelude::runtime::RuntimeStrategy;

use std::time::Duration;
use std::net::IpAddr;

use nom::*;
use structopt::StructOpt;

#[derive(StructOpt, Debug, Clone)]
#[structopt(name = "twitchd", about = "Twitch API daemon")]
pub struct Options {
    #[structopt(short = "h", long = "host", default_value = "0.0.0.0",
                help = "Host address to bind to")]
    pub host: IpAddr,
    #[structopt(short = "p", long = "port", default_value = "7777",
                help = "Port to listen on")]
    pub port: u16,

    #[structopt(long = "client-id", help = "Twitch API client ID")]
    pub client_id: String,

    #[structopt(long = "index-cache-timeout", default_value = "60s",
                parse(try_from_str = "parse_duration"),
                value_name = "duration",
                help = "Duration after which stream indexes expire from cache")]
    pub index_cache_timeout: Duration,

    #[structopt(long = "playlist-fetch-interval", default_value = "1000ms",
                parse(try_from_str = "parse_duration"),
                value_name = "duration",
                help = "Interval at which the stream playlist will be queried")]
    pub player_playlist_fetch_interval: Duration,
    #[structopt(long = "player-inactive-timeout", default_value = "60s",
                parse(try_from_str = "parse_duration"),
                value_name = "duration",
                help = "Duration after which an inactive stream will be closed")]
    pub player_inactive_timeout: Duration,
    #[structopt(long = "player-fetch-timeout", default_value = "5s",
                parse(try_from_str = "parse_duration"),
                value_name = "duration",
                help = "Maximum time allowed to fetch video data from Twitch")]
    pub player_fetch_timeout: Duration,
    #[structopt(long = "player-video-chunks-size", default_value = "128K",
                parse(try_from_str = "parse_byte_size"),
                value_name = "bytes",
                help = "Size of the video chunks in bytes sent to each client")]
    pub player_video_chunks_size: usize,
    #[structopt(long = "player-max-sink-buffer-size", default_value = "10M", // 10 MB ≅ 5 segments of 1080p60 source video
                parse(try_from_str = "parse_byte_size"),
                value_name = "bytes",
                help = "Maximum size in bytes of buffered data per client waiting to be sent")]
    pub player_max_sink_buffer_size: usize,

    #[structopt(long = "runtime", default_value = "single",
                raw(possible_values = r#"&["single", "multi"]"#),
                raw(case_insensitive = "true"),
                parse(from_str = "parse_runtime_strategy"),
                value_name = "strategy",
                help = "Runtime threading strategy")]
    pub runtime_strategy: RuntimeStrategy
}

use nom::types::CompleteByteSlice as Input;
use std::str::{FromStr, from_utf8};

fn from_raw<T: FromStr>(data: Input) -> Option<T> {
    from_utf8(&data).ok()
        .and_then(|s| FromStr::from_str(s).ok())
}

fn parse_duration(src: &str) -> Result<Duration, InvalidDuration> {
    type DurationBuilder = fn(u64) -> Duration;
    named!(parse_duration_builder<Input, DurationBuilder>, alt!(
        map!(tag_no_case!("s"),  |_| (Duration::from_secs) as DurationBuilder) |
        map!(tag_no_case!("ms"), |_| (Duration::from_millis) as DurationBuilder)
    ));

    named!(parse_final<Input, Duration>, do_parse!(
        number: map_opt!(nom::digit, from_raw) >>
        build_duration: parse_duration_builder >>
        (build_duration(number))
    ));

    parse_final(Input(src.as_bytes()))
        .map(|(_remain, result)| result)
        .map_err(|_error| InvalidDuration(String::from(src)))
}

fn parse_byte_size(src: &str) -> Result<usize, InvalidByteSize> {
    type SizeModifier = fn(usize) -> usize;
    named!(parse_modifier<Input, SizeModifier>, alt!(
        map!(tag_no_case!("b"), |_| (|x| x)              as SizeModifier) |
        map!(tag_no_case!("k"), |_| (|x| x * 1_000)      as SizeModifier) |
        map!(tag_no_case!("m"), |_| (|x| x * 1_000_000)  as SizeModifier)
    ));

    named!(parse_final<Input, usize>, exact!(do_parse!(
        n: map_opt!(nom::digit, from_raw) >>
        opt_modifier: opt!(complete!(parse_modifier)) >>
        (opt_modifier.unwrap_or(|x| x)(n))
    )));

    parse_final(Input(src.as_bytes()))
        .map(|(_remain, result)| result)
        .map_err(|_error| InvalidByteSize(String::from(src)))
}

fn parse_runtime_strategy(src: &str) -> RuntimeStrategy {
    match src.to_lowercase().as_str() {
        "single" => RuntimeStrategy::Single,
        "multi"  => RuntimeStrategy::Multi,
        _        => unreachable!()
    }
}

#[derive(Debug)]
pub struct InvalidDuration(String);

#[derive(Debug)]
pub struct InvalidByteSize(String);

use std::fmt;

impl fmt::Display for InvalidDuration {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            InvalidDuration(ref duration) => write!(f, r#"
                Invalid duration: "{}"
                Should be in the form 1234[s|ms]"#,
                duration
            )
        }
    }
}

impl fmt::Display for InvalidByteSize {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            InvalidByteSize(ref byte_size) => write!(f, r#"
                Invalid byte size: "{}"
                Should be in the form 1234[B|K|M]"#,
                byte_size
            )
        }
    }
}
