# twitch-player
performance focused twitch chat and video playback

## Motivation
Video playback of [Twitch](https://twitch.tv) streams in browsers seems to use an unusally large mount of resources, especially cpu-wise.
When you compare it to native video players like [vlc](https://www.videolan.org/vlc/index.html) or [mpc](https://mpc-hc.org/), you can observe up to ~4 times less cpu usage.

Projects like [livestreamer](https://github.com/chrippa/livestreamer/releases) or [streamlink](https://github.com/streamlink/streamlink-twitch-gui) already exist but they lack an essential feature: a playback layout that includes chat.
This project aims at solving this specific issue by "simply" embedding a native player and a chat window inside a shared layout so that it looks like this:
![image](https://user-images.githubusercontent.com/2079561/37526808-a39ee09e-2930-11e8-8d0f-433ecc3936d7.png)

You get basic resizing functionalities, with an adjustable splitter between the video player and the chat, and some controls to enable fullscreen/toggle chat/...

To circumvent some playback seeking limitations of vlc for live hls playlists like Twitch's, this project also includes its own hls fetcher. It can actually yield better "latency to broadcaster" than the twitch.tv player.

## Building
The project currently relies on [Qt](https://www.qt.io/) to abstract the platform's windowing system.
Before doing anything else, make sure you have it downloaded and that `qmake` is in your `$PATH`.

Once Qt is installed, with a nightly [rust toolchain](https://www.rustup.rs/), you can just run:

```sh
cargo build --release
```

## FAQ
`Q:` Why not use Electron to ease the gui development ?  
`A:` It turns out that [Electron does not support embedding of foreign windows](https://github.com/electron/electron/issues/10547) in its current state which makes it impossible to include a native video player.

`Q:` How do I use the chrome extension ?  
`A:` you can load the `chrome-ext` folder as an unpacked extension in `chrome://extensions`

`Q:` How can I specify another video player than VLC or another chat renderer than Chrome  
`A:` It is not supported for now but should be very soon
