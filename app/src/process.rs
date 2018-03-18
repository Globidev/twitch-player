use std::process::{Command, Child, Stdio};
use std::io::Result;

use options::Options;

pub fn run_video_player(opts: &Options) -> Result<Child> {
    let config = &opts.config.video_player;
    let args: Vec<_> = config.args.iter()
        .map(|arg| arg.replace("{channel_name}", &opts.channel))
        .collect();

    Command::new(&config.command)
        .args(&args)
        .stdin(Stdio::piped())
        .spawn()
}

pub fn run_chat_renderer(opts: &Options) -> Result<Child> {
    let config = &opts.config.chat_renderer;
    let args: Vec<_> = config.args.iter()
        .map(|arg| arg.replace("{channel_name}", &opts.channel))
        .collect();

    Command::new(&config.command)
        .args(&args)
        .spawn()
}
