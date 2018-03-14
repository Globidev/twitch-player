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
use process::{run_chat, run_vlc};
use types::*;

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

fn run_impl(opts: Options, messages_in: PlayerReceiver) -> Result<(), PlayerError> {
    use self::PollResult::SwitchChannel;

    let mut core = Core::new()?;

    let connector = HttpsConnector::new(2, &core.handle())?;
    let client = Client::configure()
        .connector(connector)
        .build(&core.handle());

    let mut vlc = run_vlc(&opts.channel)?;
    let mut chat = run_chat(&opts.channel)?;

    let mut vlc_writer = vlc.stdin.take().ok_or(PlayerError::NoStdinAccess)?;

    let mut channel = opts.channel;

    loop {
        let logic = play_channel(&client, &mut vlc_writer, channel, &messages_in);
        match core.run(logic)? {
            SwitchChannel(new_channel) => { channel = new_channel; },
            _                          => break
        }
    }

    vlc.kill()?;
    // Waiting for a way to prevent chat daemonizing
    // chat.kill()?;
    
    Ok(())
}

pub fn run(opts: Options, messages_in: PlayerReceiver, messages_out: GuiSender) {
    use types::GuiMessage::PlayerError;

    match run_impl(opts, messages_in) {
        Err(error) => {
            println!("Player error: {}", error);
            messages_out.send(PlayerError(error.to_string())).unwrap_or_default();
        }
        _ => (),
    }
}
