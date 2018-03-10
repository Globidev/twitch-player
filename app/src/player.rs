extern crate futures;
extern crate hyper;
extern crate hyper_tls;
extern crate serde_json;
extern crate tokio_core;
extern crate tokio_timer;
extern crate tokio_tls;

use std::io;
use self::futures::{Future, Stream};
use self::futures::future::Either;
use self::futures::future::{loop_fn, Loop, ok, err};
use self::hyper::{Client, StatusCode};
use self::hyper::client::HttpConnector;
use self::tokio_core::reactor::Core;
use self::tokio_timer::{Interval, Timer};
use self::serde_json::from_slice as decode;
use self::hyper_tls::HttpsConnector;

use types::{GuiSender, PlayerReceiver};
use std::time::Duration;
use std::process;

#[derive(Deserialize)]
struct Resolution {
    height: u32,
    width: u32,
}

#[derive(Deserialize)]
struct PlaylistInfo {
    url: String,
    name: String,
    bandwidth: u64,
    resolution: Resolution,
}

#[derive(Deserialize)]
struct PlaylistsInfo {
    channel: String,
    playlists: Vec<PlaylistInfo>,
}

#[derive(Debug)]
enum PlayerError {
    PollFail,
    HyperError(hyper::Error),
    IoError(io::Error),
    FetchPlaylistsInfoFail(StatusCode),
    BadPlaylistsInfoFormat
}

use std::error;
impl error::Error for PlayerError {
    fn description(&self) -> &str { "Player Error" }
}

use std::fmt;
impl fmt::Display for PlayerError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:?}", self)
    }
}

impl From<hyper::Error> for PlayerError {
    fn from(error: hyper::Error) -> Self {
        PlayerError::HyperError(error)
    }
}

impl From<io::Error> for PlayerError {
    fn from(error: io::Error) -> Self {
        PlayerError::IoError(error)
    }
}

type Segment = String;
type Playlist = Vec<Segment>;

type HttpClient = Client<HttpsConnector<HttpConnector>>;

fn poll_messages(messages_in: PlayerReceiver) -> impl Future<Item = (), Error = PlayerError> {
    use types::PlayerMessage::AppQuit;

    let ticks = Timer::default().interval(Duration::from_millis(250));

    ticks
        .take_while(move |_| {
            let cont = match messages_in.try_recv() {
                Ok(AppQuit) => false,
                _ => true,
            };
            ok(cont)
        })
        .collect()
        .map(|_| ())
        .map_err(|_| PlayerError::PollFail)
}

type BoxFuture<T, E> = Box<Future<Item = T, Error = E> + Send>;

fn fetch_playlists_info(
    client: &HttpClient,
    channel: &str,
) -> impl Future<Item = PlaylistsInfo, Error = PlayerError> {
    use self::PlayerError::*;

    let url = format_args!("https://streamer.datcoloc.com/{}", channel).to_string();
    client.get(url.parse().unwrap())
        .map_err(From::from)
        .and_then(|res| {
            if res.status() == StatusCode::Ok {
                let playlists = res.body()
                    .concat2()
                    .map_err(From::from)
                    .and_then(move |body| {
                        match decode::<PlaylistsInfo>(&body) {
                            Ok(playlists) => ok(playlists),
                            Err(error) => err(BadPlaylistsInfoFormat)
                        }
                    });
                Box::new(playlists) as BoxFuture<_,_>
            } else {
                let error = err(FetchPlaylistsInfoFail(res.status()));
                Box::new(error) as BoxFuture<_,_>
            }
        })
}

fn fetch_and_pipe_data(
    client: &HttpClient,
    playlist_url: &str,
) -> impl Future<Item = (), Error = PlayerError> {
    use self::PlayerError::*;

    println!("{}", playlist_url);
    client.get(playlist_url.parse().unwrap())
        .map_err(From::from)
        .and_then(|res| {
            println!("status: {}", res.status());
            ok(())
        })
}

fn run_impl(messages_in: PlayerReceiver, messages_out: &GuiSender) -> Result<(), PlayerError> {
    let mut core = Core::new()?;

    let client = Client::configure()
        .connector(HttpsConnector::new(4, &core.handle()).unwrap())
        .build(&core.handle());

    let logic = fetch_playlists_info(&client, "lirik")
        .and_then(|pl| fetch_and_pipe_data(&client, &pl.playlists[0].url))
        .select2(poll_messages(messages_in))
        .map(|_| ())
        .map_err(|e| {
            match e {
                Either::A((e, _)) => e,
                Either::B((e, _)) => e,
            }
        });

    core.run(logic)
}

pub fn run(messages_in: PlayerReceiver, messages_out: GuiSender) {
    use types::GuiMessage::PlayerError;

    match run_impl(messages_in, &messages_out) {
        Err(error) => {
            println!("Player error: {:?}", error);
            messages_out.send(PlayerError(error.to_string()));
        }
        _ => (),
    }
}
