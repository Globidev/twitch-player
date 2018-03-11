const PROTOCOL_PREFIX: &'static str = "twitch-player://";

fn parse_channel(channel: &str) -> String {
    if channel.starts_with(PROTOCOL_PREFIX) {
        channel.trim_left_matches(PROTOCOL_PREFIX)
            .trim_right_matches('/')
            .to_string()
    } else {
        channel.to_string()
    }
}

#[derive(StructOpt, Debug, Clone)]
#[structopt(name = "twitch-player", about = "A native player for Twitch on Windows")]
pub struct Options {
    #[structopt(help = "Channel name or Protocol URI", parse(from_str = "parse_channel"))]
    pub channel: String,
}
