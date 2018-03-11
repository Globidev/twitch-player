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
use self::futures::future::{ok, err};
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

fn ticks(duration: Duration) 
    -> impl futures::Stream<Item = (), Error = PlayerError> 
{
    Timer::default()
        .interval(duration)
        .map_err(|_| PlayerError::PollFail)
}

fn poll_messages(messages_in: PlayerReceiver) 
    -> impl Future<Item = (), Error = PlayerError> 
{
    let should_poll_next = move || {
        match messages_in.try_recv() {
            Ok(PlayerMessage::AppQuit) => false,
            _                          => true,
        }
    };

    ticks(Duration::from_millis(250))
        .take_while(move |_| ok(should_poll_next()))
        .collect()
        .map(|_| ())
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

fn fetch_and_pipe_data<W>(client: HttpClient, playlist_url: String, writer: W)
    -> impl Future<Item = (), Error = PlayerError>
where
    W: Write + 'static
{
    struct State<W> {
        last_segment: Option<SegmentUrl>,
        writer: W
    };

    let state = State { last_segment: None, writer: writer };

    let playlist_client = client.clone();
    let stream = ticks(Duration::from_millis(1000))
        .and_then(move |_| fetch_playlist(&playlist_client, &playlist_url));

    stream.fold(state, move |mut state, mut playlist| -> BoxFuture<_> {
        let to_download = match state.last_segment {
            None => playlist.pop(),
            Some(ref segment) => {
                match playlist.iter().position(|s| s == segment) {
                    None => playlist.pop(),
                    Some(idx) => playlist.into_iter().nth(idx + 1)
                }
            }
        };

        if let Some(segment) = to_download {
            let future = fetch_segment(&client, &segment)
                .and_then(move |chunk| {
                    match state.writer.write(&chunk) {
                        Ok(size) => {
                            println!("Piped {} bytes", size);
                            state.last_segment = Some(segment);
                            ok(state)
                        },
                        Err(error) => err(From::from(error))
                    }
                });
            Box::new(future)
        } else {
            Box::new(ok(state))
        }
    }).map(|_| ())
}

fn run_impl(opts: Options, messages_in: PlayerReceiver) -> Result<(), PlayerError> {
    let mut core = Core::new()?;

    let connector = HttpsConnector::new(2, &core.handle())?;
    let client = Client::configure()
        .connector(connector)
        .build(&core.handle());

    let mut vlc = run_vlc(&opts.channel)?;
    let mut chat = run_chat(&opts.channel)?;

    let vlc_writer = vlc.stdin.take().ok_or(PlayerError::NoStdinAccess)?;

    let logic = fetch_playlists_info(&client, &opts.channel)
        .and_then(|pl| {
            let playlist_url = pl.playlists[0].url.clone();
            fetch_and_pipe_data(client, playlist_url, vlc_writer)
        })
        .select2(poll_messages(messages_in))
        .map(|_| ())
        .map_err(|e| e.split().0);

    core.run(logic)?;

    vlc.kill()?;
    chat.kill()?;
    
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
