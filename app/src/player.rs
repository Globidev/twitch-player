extern crate futures;
extern crate hyper;
extern crate hyper_tls;
extern crate serde_json;
extern crate tokio_core;
extern crate tokio_timer;
extern crate tokio_tls;
extern crate native_tls;

use std::io::Write;
use std::time::Duration;
use std::thread;
use std::sync::{Arc, atomic::{AtomicBool, Ordering}};

use self::futures::{Future, Stream};
use self::futures::future::{ok, err, Either};
use self::hyper::{Client, StatusCode, Chunk};
use self::hyper::client::HttpConnector;
use self::tokio_core::reactor::Core;
use self::tokio_timer::Timer;
use self::serde_json::from_slice as decode;
use self::hyper_tls::HttpsConnector;

use errors::PlayerError;
use options::Options;
use process::{run_video_player, run_chat_renderer};
use types::*;
use native::{find_window_by_name, find_window_by_pid};

type HttpClient = Client<HttpsConnector<HttpConnector>>;
type BoxFuture<T> = Box<Future<Item = T, Error = PlayerError>>;
type BadStatusError = fn(StatusCode) -> PlayerError;

#[derive(Debug)]
enum PollResult {
    Continue,
    Quit,
    SwitchChannel(String),
}

fn ticks(duration: Duration)
    -> impl Stream<Item = (), Error = PlayerError>
{
    Timer::default()
        .interval(duration)
        .map_err(|_| PlayerError::PollFail)
}

fn poll_messages<'a>(messages_in: &'a PlayerReceiver)
    -> impl Future<Item = PollResult, Error = PlayerError> + 'a
{
    use self::PlayerMessage::*;
    use self::PollResult::*;

    let to_poll_result = move |_| {
        match messages_in.try_recv() {
            Ok(ChangeChannel(channel)) => SwitchChannel(channel),
            Ok(AppQuit)                => Quit,
            _                          => Continue,
        }
    };

    ticks(Duration::from_millis(250))
        .map(to_poll_result)
        .skip_while(|poll_result| {
            match poll_result {
                &Continue => ok(true),
                _        => ok(false)
            }
        })
        .into_future()
        .map(|(r, _)| r.unwrap_or(Quit))
        .map_err(|(e, _)| e)
}

fn fetch(client: &HttpClient, url: &str, bad_status_error: BadStatusError)
    -> impl Future<Item = Chunk, Error = PlayerError>
{
    match url.parse() {
        Err(error) => Box::new(err(PlayerError::MalformedUrl(error))),
        Ok(uri) => {
            let future = client.get(uri)
                .map_err(From::from)
                .and_then(move |response| -> BoxFuture<_> {
                    match response.status() {
                        StatusCode::Ok => {
                            let future = response
                                .body()
                                .concat2()
                                .map_err(From::from);
                            Box::new(future)
                        },
                        status => Box::new(err(bad_status_error(status)))
                    }
                });
            Box::new(future) as BoxFuture<_>
        }
    }
}

fn fetch_segment(client: &HttpClient, segment_url: &str)
    -> impl Future<Item = hyper::Chunk, Error = PlayerError>
{
    fetch(client, segment_url, PlayerError::FetchSegmentFail)
}

fn fetch_playlist(client: &HttpClient, playlist_url: &str)
    -> impl Future<Item = Playlist, Error = PlayerError>
{
    fetch(client, playlist_url, PlayerError::FetchPlaylistFail)
        .map(parse_playlist)
}

fn parse_playlist(data: Chunk) -> Playlist {
    use std::str::from_utf8;
    use std::string::ToString;

    from_utf8(&data).unwrap_or("")
        .split('\n')
        .filter(|line| line.starts_with("http://"))
        .map(ToString::to_string)
        .collect()
}

fn fetch_playlists_info(client: &HttpClient, channel: &str)
    -> impl Future<Item = PlaylistsInfo, Error = PlayerError>
{
    let url = format!("https://streamer.datcoloc.com/{}", channel);

    fetch(client, &url, PlayerError::FetchPlaylistsInfoFail)
        .and_then(|chunk| {
            decode(&chunk).map_err(PlayerError::BadPlaylistsInfoFormat)
        })
}

fn segment_stream<'a>(client: &'a HttpClient, playlist_url: String)
    -> impl Stream<Item = Chunk, Error = PlayerError> + 'a
{
    let playlists = ticks(Duration::from_millis(1_000))
        .and_then(move |_| fetch_playlist(client, &playlist_url));

    let mut last_segment = None;

    let segments = playlists.and_then(move |mut playlist| -> BoxFuture<_> {
        let to_download = match last_segment {
            None => playlist.pop(),
            Some(ref segment) => {
                match playlist.iter().position(|s| s == segment) {
                    None => playlist.pop(),
                    Some(idx) => playlist.into_iter().nth(idx + 1)
                }
            }
        };

        if let Some(segment) = to_download {
            let future = fetch_segment(client, &segment).map(Option::from);
            last_segment = Some(segment);
            Box::new(future)
        } else {
            Box::new(ok(None))
        }
    });

    segments.filter_map(|opt_segment| opt_segment)
}

fn pipe_data<W: Write>(writer: &mut W, chunk: Chunk)
    -> impl Future<Item = (), Error = PlayerError>
{
    match writer.write(&chunk) {
        Ok(size) => {
            println!("Piped {} bytes", size);
            ok(())
        },
        Err(error) => err(From::from(error))
    }
}

fn play_channel<'a, W: Write>(
    client: &'a HttpClient,
    writer: &'a mut W,
    channel: String,
    messages_in: &'a PlayerReceiver
) -> impl Future<Item = PollResult, Error = PlayerError> + 'a
{
    let to_poll_result = |select_result| {
        match select_result {
            Either::B((result, _)) => result,
            _                      => PollResult::Quit, // unreachable ?
        }
    };

    fetch_playlists_info(client, &channel)
        .and_then(move |info| {
            let playlist_url = info.playlists[0].url.clone();
            segment_stream(client, playlist_url)
                .and_then(move |chunk| pipe_data(writer, chunk))
                .collect()
        })
        .select2(poll_messages(messages_in))
        .map_err(|e| e.split().0)
        .map(to_poll_result)
}

fn run_impl<W: Write>(opts: Options, messages_in: PlayerReceiver, writer: &mut W)
    -> Result<(), PlayerError>
{
    use self::PollResult::SwitchChannel;

    let mut writer = writer;
    let mut core = Core::new()?;

    let connector = HttpsConnector::new(2, &core.handle())?;
    let client = Client::configure()
        .connector(connector)
        .build(&core.handle());

    let mut channel = opts.channel;

    loop {
        let logic = play_channel(&client, &mut writer, channel, &messages_in);
        match core.run(logic)? {
            SwitchChannel(new_channel) => { channel = new_channel; },
            _                          => break
        }
    }

    Ok(())
}

fn grab_windows(
    video_player_pid: u32,
    _chat_renderer_pid: u32,
    messages_out: GuiSender,
    should_stop: Arc<AtomicBool>
) {
    use types::GuiMessage::Handles;

    let mut player_handle = None;
    let mut chat_handle = None;

    while !should_stop.load(Ordering::Relaxed) {
        let find_player = || find_window_by_pid(|pid| pid == video_player_pid);
        let find_chat = || find_window_by_name(|name| name == "Twitch");

        player_handle = player_handle.or_else(find_player);
        chat_handle = chat_handle.or_else(find_chat);

        if let (Some(player), Some(chat)) = (player_handle, chat_handle) {
            let message = Handles(player as u64, chat as u64);
            messages_out.send(message).unwrap_or_default();
            break
        }

        thread::sleep(Duration::from_millis(250));
    }
}

pub fn run(opts: Options, messages_in: PlayerReceiver, messages_out: GuiSender)
    -> Result<(), PlayerError>
{
    use types::GuiMessage;

    let mut video_player = run_video_player(&opts)?;
    let mut chat_renderer = run_chat_renderer(&opts)?;

    let mut data_writer = video_player.stdin
        .take()
        .ok_or(PlayerError::NoStdinAccess)?;

    let messages = messages_out.clone();
    let player_id = video_player.id();
    let chat_id = chat_renderer.id();
    let should_stop = Arc::new(AtomicBool::new(false));
    let should_stop_clone = should_stop.clone();
    let handle = thread::spawn(move || grab_windows(player_id, chat_id, messages, should_stop_clone));

    match run_impl(opts, messages_in, &mut data_writer) {
        Err(error) => {
            eprintln!("Player error: {}", error);
            let message = GuiMessage::PlayerError(error.to_string());
            messages_out.send(message).unwrap_or_default();
        }
        _ => (),
    }

    should_stop.store(true, Ordering::Relaxed);
    handle.join().map_err(|_| PlayerError::PollFail)?;

    video_player.kill()?;
    chat_renderer.kill()?;

    Ok(())
}
