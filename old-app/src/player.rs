extern crate futures;
extern crate hyper;
extern crate hyper_tls;
extern crate native_tls;
extern crate serde_json;
extern crate tokio_core;
extern crate tokio_timer;
extern crate tokio_tls;

use std::io::Write;
use std::time::Duration;
use std::thread;
use std::sync::{Arc, atomic::{AtomicBool, Ordering}};

use self::futures::{Future, Stream, future::{err, ok, Either}};
use self::hyper::{Chunk, Client, StatusCode, client::HttpConnector};
use self::tokio_core::reactor::Core;
use self::tokio_timer::Timer;
use self::serde_json::from_slice as decode;
use self::hyper_tls::HttpsConnector;

use errors::{self, PlayerError};
use options::Options;
use process::{run_chat_renderer, run_video_player};
use types::{GuiSender, PlayerReceiver, Playlist, PlaylistsInfo};
use native::{find_window_by_name, /*find_window_by_pid*/};

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
    use types::PlayerMessage::*;
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
            let keep_polling = match *poll_result {
                Continue => true,
                _        => false
            };
            ok(keep_polling)
        })
        .into_future()
        .map(|(result, _remain)| result.unwrap_or(Quit))
        .map_err(|(error, _remain)| error)
}

fn fetch(client: &HttpClient, url: &str, bad_status_error: BadStatusError)
    -> impl Future<Item = Chunk, Error = PlayerError>
{
    match url.parse() {
        Err(error) => Box::new(err(PlayerError::MalformedUrl(error))),
        Ok(uri) => {
            let future = client.get(uri)
                .map_err(PlayerError::from)
                .and_then(move |response| -> BoxFuture<_> {
                    match response.status() {
                        StatusCode::Ok => {
                            let future = response
                                .body()
                                .concat2()
                                .map_err(PlayerError::from);
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
    -> impl Future<Item = Chunk, Error = PlayerError>
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
        .filter(|line| line.starts_with("index-") || line.starts_with("http://"))
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
    let origin = {
        let origin_parts = playlist_url.rsplitn(2, '/').collect::<Vec<_>>();
        String::from(origin_parts[1])
    };

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
            let segment_url = if segment.starts_with("http://") {
                segment.clone()
            } else {
                format!("{}/{}", origin, segment)
            };
            let future = fetch_segment(client, &segment_url).map(Option::from);
            last_segment = Some(segment);
            Box::new(future)
        } else {
            Box::new(ok(None))
        }
    });

    segments.filter_map(|opt_segment| opt_segment)
}

fn pipe_data(writer: &mut impl Write, chunk: Chunk)
    -> impl Future<Item = (), Error = PlayerError>
{
    match writer.write(&chunk) {
        Ok(size) => {
            println!("Piped {} bytes", size);
            ok(())
        },
        Err(error) => err(PlayerError::from(error))
    }
}

fn play_channel<'a>(
    client: &'a HttpClient,
    writer: &'a mut impl Write,
    messages_in: &'a PlayerReceiver,
    channel: &str,
) -> impl Future<Item = PollResult, Error = PlayerError> + 'a
{
    let to_poll_result = |select_result| {
        match select_result {
            Either::B((result, _)) => result,
            _                      => PollResult::Quit, // unreachable ?
        }
    };

    fetch_playlists_info(client, channel)
        .and_then(move |info| {
            let playlist_url = &info.playlists[0].url;
            segment_stream(client, playlist_url.to_string())
                .and_then(move |chunk| pipe_data(writer, chunk))
                .collect()
        })
        .select2(poll_messages(messages_in))
        .map_err(|e| e.split().0)
        .map(to_poll_result)
}

fn run_impl(opts: Options, messages_in: PlayerReceiver, writer: &mut impl Write)
    -> Result<(), PlayerError>
{
    use self::PollResult::SwitchChannel;

    let mut core = Core::new()?;

    let connector = HttpsConnector::new(2, &core.handle())?;
    let client = Client::configure()
        .connector(connector)
        .build(&core.handle());

    let mut channel = opts.channel;

    loop {
        let logic = play_channel(&client, writer, &messages_in, &channel);
        match core.run(logic) {
            Ok(result) => {
                match result {
                    SwitchChannel(new_channel) => { channel = new_channel; },
                    _                          => break
                }
            },
            Err(error) => {
                eprintln!("error: {}", error);
                if errors::is_fatal(&error) {
                    return Err(error);
                }
            }
        }
    }

    Ok(())
}

fn grab_windows(opts: Options, messages_out: GuiSender, should_stop: Arc<AtomicBool>) {
    use types::GuiMessage::Handles;

    let player_window_hint = &opts.config.video_player.window_title_hint;
    let chat_window_hint = &opts.config.chat_renderer.window_title_hint;

    let mut player_handle = None;
    let mut chat_handle = None;

    while !should_stop.load(Ordering::Relaxed) {
        let find_player = || find_window_by_name(|name| name.starts_with(player_window_hint));
        let find_chat = || find_window_by_name(|name| name.starts_with(chat_window_hint));

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

    let should_stop = Arc::new(AtomicBool::new(false));
    let handle = {
        let opts = opts.clone();
        let messages_out = messages_out.clone();
        let should_stop = Arc::clone(&should_stop);
        thread::spawn(move || grab_windows(opts, messages_out, should_stop))
    };

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
