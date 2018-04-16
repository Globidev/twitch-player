extern crate hyper;
extern crate tokio_timer;

use self::hyper::Chunk;
use self::tokio_timer::Timer;

use prelude::futures::*;
use twitch::types::{PlaylistInfo, Playlist};
use twitch::api::ApiError;
use prelude::http::{HttpsClient, HttpError, fetch, ResponseSink};
use options::Options;

use std::rc::Rc;
use std::cell::RefCell;
use std::time::{Duration, Instant};

pub type PlayerSink = ResponseSink;

pub struct StreamPlayer {
    client: HttpsClient,
    sinks: Rc<RefCell<Vec<PlayerSink>>>,
    opts: Options,
}

impl StreamPlayer {
    pub fn new(opts: Options, client: HttpsClient) -> Self {
        Self {
            client: client,
            sinks: Rc::new(RefCell::new(Vec::new())),
            opts: opts,
        }
    }

    pub fn play(&self, playlist_info: PlaylistInfo)
        -> impl Future<Item = (), Error = StreamPlayerError>
    {
        let write_to_sinks = {
            let sinks = Rc::clone(&self.sinks);
            let inactive_timeout = self.opts.player_inactive_timeout;
            let mut last_active = Instant::now();

            move |data: hyper::Chunk| {
                let raw_data = data.to_vec();
                let mut sinks = sinks.borrow_mut();

                sinks.drain_filter(|sink| {
                    sink.try_send(Ok(Chunk::from(raw_data.clone())))
                        .is_err()
                });

                if !sinks.is_empty() { last_active = Instant::now(); }

                let timed_out = last_active.elapsed() > inactive_timeout;
                match timed_out {
                    true  => future::err(StreamPlayerError::InactiveTooLong),
                    false => future::ok(())
                }
            }
        };

        segment_stream(self.client.clone(), playlist_info, self.opts.player_playlist_fetch_interval)
            .for_each(write_to_sinks)
    }

    pub fn add_sink(&self, sink: PlayerSink) {
        self.sinks.borrow_mut().push(sink)
    }
}

type PlayerFuture<T> = Box<Future<Item = T, Error = StreamPlayerError>>;

fn segment_stream(client: HttpsClient, playlist_info: PlaylistInfo, fetch_interval: Duration)
    -> impl Stream<Item = hyper::Chunk, Error = StreamPlayerError>
{
    use twitch::api::playlist;
    use self::hyper::{Request, Get};

    let fetch_playlist = {
        let client = client.clone();
        move |_| {
            playlist(&client, &playlist_info.url)
                .map_err(StreamPlayerError::FetchPlaylistFail)
        }
    };

    let fetch_latest_segment = {
        let mut last_segment = None;

        move |mut playlist: Playlist| -> PlayerFuture<_> {
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
                let request = Request::new(Get, segment.parse().unwrap());
                let future = fetch(&client, request)
                    .map(Option::from)
                    .map_err(StreamPlayerError::FetchSegmentFail);
                last_segment = Some(segment);
                Box::new(future)
            } else {
                Box::new(future::ok(None))
            }
        }
    };

    let playlist_stream = Timer::default()
        .interval(fetch_interval)
        .map_err(StreamPlayerError::TimerError)
        .and_then(fetch_playlist);

    playlist_stream
        .and_then(fetch_latest_segment)
        .filter_map(|opt_segment| opt_segment)
}

pub enum StreamPlayerError {
    TimerError(tokio_timer::TimerError),
    FetchPlaylistFail(ApiError),
    FetchSegmentFail(HttpError),
    InactiveTooLong
}
