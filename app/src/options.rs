extern crate toml;

const PROTOCOL_PREFIX: &'static str = "twitch-player://";

use std::io::{self, Read};
use std::fs::File;
use std::fmt;
use std::error;

use self::toml::from_str as decode;

fn parse_channel(channel: &str) -> String {
    if channel.starts_with(PROTOCOL_PREFIX) {
        channel.trim_left_matches(PROTOCOL_PREFIX)
            .trim_right_matches('/')
            .to_string()
    } else {
        channel.to_string()
    }
}

fn parse_config(file_path: &str) -> Result<Config, ConfigError> {
    let mut raw_config = String::new();
    let mut file = File::open(file_path)?;

    file.read_to_string(&mut raw_config)?;
    let config = decode(&raw_config)?;

    Ok(config)
}

#[derive(StructOpt, Debug, Clone)]
#[structopt(name = "twitch-player",
            about = "A native player for Twitch on Windows")]
pub struct Options {
    #[structopt(help = "Channel name or Protocol URI",
                parse(from_str = "parse_channel"))]
    pub channel: String,
    #[structopt(help = "Configuration file",
                long = "config",
                default_value="config.toml",
                parse(try_from_str = "parse_config"))]
    pub config: Config,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "kebab-case")]
pub struct Config {
    pub video_player: ProcessConfig,
    pub chat_renderer: ProcessConfig,
    pub keybinds: Option<KeyBinds>
}

#[derive(Deserialize, Debug, Clone)]
pub struct ProcessConfig {
    pub command: String,
    pub args: Vec<String>,
    pub window_title_hint: String,
}

type KeyBind = String;

#[derive(Deserialize, Debug, Clone, Default)]
pub struct KeyBinds {
    pub fullscreen: Option<KeyBind>,
    pub chat_toggle_left: Option<KeyBind>,
    pub chat_toggle_right: Option<KeyBind>,
    pub change_channel: Option<KeyBind>,
}

#[derive(Debug)]
enum ConfigError {
    IoError(io::Error),
    TomlError(toml::de::Error)
}

impl error::Error for ConfigError {
    fn description(&self) -> &str { "Configuration Error" }
}

impl fmt::Display for ConfigError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl From<io::Error> for ConfigError {
    fn from(error: io::Error) -> Self { ConfigError::IoError(error) }
}

impl From<toml::de::Error> for ConfigError {
    fn from(error: toml::de::Error) -> Self { ConfigError::TomlError(error) }
}
