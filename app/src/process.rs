use std::process::{Command, Child, Stdio};
use std::io::Result;

const VLC_PATH: &'static str = r"C:\Program Files (x86)\VideoLAN\VLC\vlc.exe";
const CHROME_PATH: &'static str = r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe";

pub fn vlc_title(channel: &str) -> String {
    format!("Twitch-Player_{}", channel)
}

pub fn run_vlc(channel: &str) -> Result<Child> {
    let args = [
        "-",
        "--qt-minimal-view", "--no-qt-video-autoresize",
        "--key-faster=ctrl+f", "--key-rate-normal=ctrl+n",
        "--file-caching=1000", "--live-caching=1000",
        "--disc-caching=1000", "--network-caching=1000",
        &format!("--meta-title={}", vlc_title(channel))
    ];

    Command::new(VLC_PATH)
        .args(&args)
        .stdin(Stdio::piped())
        .spawn()
}

pub fn run_chat(channel: &str) -> Result<Child> {
    let url = format!("https://www.twitch.tv/popout/{}/chat?popout=", channel);
    let args = [
        &format!("--app={}", url)
    ];

    Command::new(CHROME_PATH)
        .args(&args)
        .spawn()
}
