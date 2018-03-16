# twitch-player
performance focused twitch chat and video playback

## Motivation
Video playback of [Twitch](https://twitch.tv) streams in browsers seems to use an unusally large mount of resources, especially cpu-wise.  
When you compare it to native video players like [vlc](https://www.videolan.org/vlc/index.html) or [mpc](https://mpc-hc.org/), you can observe up to ~4 times less cpu usage.

Projects like [livestreamer](https://github.com/chrippa/livestreamer/releases) or [streamlink](https://github.com/streamlink/streamlink-twitch-gui) already exist but they lack an essential feature: a playback layout that includes chat.
This project aims at solving this specific issue by "simply" embedding a native player and a chat window inside a shared layout so that it looks like this:
![image](https://user-images.githubusercontent.com/2079561/37526808-a39ee09e-2930-11e8-8d0f-433ecc3936d7.png)

You get basic resizing functionalities, with an adjustable splitter between the video player and the chat, and some controls to enable fullscreen/toggle chat/...

