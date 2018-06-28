# twitch-player
performance focused twitch chat and video playback

## Motivation
Video playback of [Twitch](https://twitch.tv) streams inside web browsers seems to use an unusally large mount of resources, especially cpu-wise.
When you use native video players like [vlc](https://www.videolan.org/vlc/index.html) or [mpc](https://mpc-hc.org/), you can observe up to ~4 times less cpu usage.

Projects like [livestreamer](https://github.com/chrippa/livestreamer/releases) or [streamlink](https://github.com/streamlink/streamlink-twitch-gui) already exist but they lack an essential feature: a playback layout that includes chat.
This project aims at solving this specific issue by "simply" embedding a native player and a chat window inside a shared layout so that it looks like this:
![image](https://user-images.githubusercontent.com/2079561/41498552-b31a451c-7170-11e8-8b72-fb8f36bac433.png)

You get basic resizing functionalities, with an adjustable splitter between the video player and the chat, some controls to enable fullscreen/toggle chat/...
You also get multistream support and a bunch of cool playback features.

## Architecture
To circumvent some playback seeking limitations of vlc for live hls playlists like Twitch's and to anticipate streaming format changes, this project also includes its own hls fetcher named `twitchd`.
This component can live on its own and can be tweaked to fit your specs and internet connection perfectly. It can actually yield better "latency to broadcaster" than the twitch.tv player. It can also be used as a proxy and broadcast entrypoint.

The graphical interface is written to target platforms natively. It relies on `libvlc` to display the video and on some instance of `twitchd` to get the video data.
This project currently does not feature a custom chat implementation and therefore relies on the native platform to embed an external window - typically a web browser - that will render chat.

## Building

### twitchd
this daemon aims at being lightweight and performant, and is therefore written in rust.

With a stable [rust toolchain](https://www.rustup.rs/), you can just run:
```sh
cargo build --release
```
inside the `./twitchd` directory and that should be enough to build a native executable for this component.

### UI

The UI aims at targetting all platforms. [Electron does not support embedding of foreign windows](https://github.com/electron/electron/issues/10547) in its current state and I couldn't find any other web based toolkit that fit my needs. I ended up picking Qt and the language of choice for that framework is C++

With a recent version of [Qt](https://www.qt.io/download) (>= 5.10) and a C++17 compliant compiler you should be able to build the UI by running:
```sh
mkdir target && cd target
qmake ..
make release # or nmake on Windows
```
inside the `./player` directory.  
This has been tested on Windows with Visual Studio 2017 and also on linux with clang 6.0
