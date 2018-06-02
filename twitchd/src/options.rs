extern crate nom;

use std::time::Duration;
use std::net::IpAddr;

fn parse_duration(src: &str) -> Result<Duration, ParseDurationError> {
    use std::str::FromStr;
    use std::str::from_utf8;

    named!(parse_u64<u64>, map_res!(
        map_res!(nom::digit,from_utf8),
        FromStr::from_str
    ));

    type DurationBuilder = fn(u64) -> Duration;
    named!(parse_duration_builder<DurationBuilder>, alt!(
        map!(tag!("s"),  |_| (Duration::from_secs) as DurationBuilder) |
        map!(tag!("ms"), |_| (Duration::from_millis) as DurationBuilder)
    ));

    named!(parse_final<Duration>, do_parse!(
        number: parse_u64 >>
        build_duration: parse_duration_builder >>
        (build_duration(number))
    ));

    parse_final(src.as_bytes())
        .map(|(_remain, result)| result)
        .map_err(|_error| ParseDurationError)
}

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
                help = "Duration after which stream indexes expire from cache")]
    pub index_cache_timeout: Duration,

    #[structopt(long = "playlist-fetch-interval", default_value = "1000ms",
                parse(try_from_str = "parse_duration"),
                help = "Interval at which the stream playlist will be queried")]
    pub player_playlist_fetch_interval: Duration,
    #[structopt(long = "player-inactive-timeout", default_value = "60s",
                parse(try_from_str = "parse_duration"),
                help = "Duration after which an inactive stream will be closed")]
    pub player_inactive_timeout: Duration,
    #[structopt(long = "player-fetch-timeout", default_value = "5s",
                parse(try_from_str = "parse_duration"),
                help = "Maximum time allowed to fetch video data from Twitch")]
    pub player_fetch_timeout: Duration,
}

#[derive(Debug)]
pub struct ParseDurationError;

use std::{error, fmt};

impl error::Error for ParseDurationError {
    fn description(&self) -> &str { "Failed to parse a duration" }
}

impl fmt::Display for ParseDurationError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}
