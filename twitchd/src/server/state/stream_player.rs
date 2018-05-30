extern crate hyper;
extern crate tokio_timer;
extern crate serde_json;

use self::hyper::Chunk;
use self::tokio_timer::Timer;

use prelude::futures::*;
use twitch::types::{PlaylistInfo, Playlist};
use twitch::api::ApiError;
use prelude::http::{HttpsClient, HttpError, fetch_streamed, ResponseSink};
use options::Options;

use std::rc::Rc;
use std::cell::RefCell;
use std::time::{Duration, Instant};
use std::collections::HashMap;

pub type PlayerSink = ResponseSink;
pub type MetaKey = String;

const MPEG_TS_SECTION_LENGTH: usize = 188;
// The metadata packet seems to always be the 3rd one
const PES_METADATA_OFFSET: usize = MPEG_TS_SECTION_LENGTH * 3;

pub struct StreamPlayer {
    client: HttpsClient,
    sinks: Rc<RefCell<Vec<PlayerSink>>>,
    sink_queue: Rc<RefCell<Vec<(MetaKey, PlayerSink)>>>,
    indexed_metadata: Rc<RefCell<HashMap<MetaKey, SegmentMetadata>>>,
    opts: Options,
}

enum SegmentChunk {
    Head { data: Chunk, total_stream_time: f64 },
    Tail(Chunk)
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SegmentMetadata {
    broadc_s: u32,
    cmd: String,
    ingest_r: u32,
    ingest_s: u32,
    stream_offset: f64,
    transc_r: u64,
    transc_s: u64,
}

fn extract_metadata(data: &Chunk) -> Option<SegmentMetadata> {
    let pes_slice = &data[PES_METADATA_OFFSET..PES_METADATA_OFFSET + MPEG_TS_SECTION_LENGTH];
    let json_start_offset = pes_slice.iter()
        .position(|&c| c == b'{')?;
    let unbounded_json_slice = &pes_slice[json_start_offset..];
    let json_end_offset = unbounded_json_slice.iter()
        .position(|&c| c == b'}')?;
    let json_slice = &unbounded_json_slice[..json_end_offset + 1];
    serde_json::from_slice(json_slice).ok()
}

impl StreamPlayer {
    pub fn new(opts: Options, client: HttpsClient) -> Self {
        Self {
            client: client,
            opts: opts,
            sinks: Rc::new(RefCell::new(Vec::new())),
            sink_queue: Rc::new(RefCell::new(Vec::new())),
            indexed_metadata: Rc::new(RefCell::new(HashMap::new())),
        }
    }

    pub fn play(&self, playlist_info: PlaylistInfo)
        -> impl Future<Item = (), Error = StreamPlayerError>
    {
        let write_to_sinks = {
            let sinks = Rc::clone(&self.sinks);
            let sink_queue = Rc::clone(&self.sink_queue);
            let indexed_metadata = Rc::clone(&self.indexed_metadata);
            let inactive_timeout = self.opts.player_inactive_timeout;
            let mut last_active = Instant::now();

            move |chunk: SegmentChunk| {
                let mut sinks = sinks.borrow_mut();

                let raw_chunk = match chunk {
                    SegmentChunk::Tail(data) => data.to_vec(),
                    SegmentChunk::Head { data, total_stream_time } => {
                        let mut sink_queue = sink_queue.borrow_mut();

                        if !sink_queue.is_empty() {
                            if let Some(mut metadata) = extract_metadata(&data) {
                                println!("{:#?} {}", metadata, total_stream_time);
                                metadata.stream_offset = total_stream_time - metadata.stream_offset;
                                let mut indexed_metadata = indexed_metadata.borrow_mut();
                                for (key, sink) in sink_queue.split_off(0).into_iter() {
                                    sinks.push(sink);
                                    indexed_metadata.insert(key, metadata.clone());
                                }
                            } else {
                                println!("Failed to parse metadata...");
                            }
                        }

                        data.to_vec()
                    }
                };

                sinks.drain_filter(|sink| {
                    sink.try_send(Ok(Chunk::from(raw_chunk.clone())))
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

        let video_data_stream = segment_stream(
            self.client.clone(),
            playlist_info,
            self.opts.player_playlist_fetch_interval
        );

        let video_data_stream_with_timeout = Timer::default()
            .timeout_stream(video_data_stream, self.opts.player_fetch_timeout);

        video_data_stream_with_timeout
            .for_each(write_to_sinks)
    }

    pub fn queue_sink(&self, sink: PlayerSink, meta_key: MetaKey) {
        self.sink_queue.borrow_mut().push((meta_key, sink));
    }

    pub fn get_metadata(&self, meta_key: &MetaKey) -> Option<SegmentMetadata> {
        self.indexed_metadata.borrow().get(meta_key).cloned()
    }
}

fn segment_stream(client: HttpsClient, playlist_info: PlaylistInfo, fetch_interval: Duration)
    -> impl Stream<Item = SegmentChunk, Error = StreamPlayerError>
{
    use twitch::api::playlist;
    use self::hyper::{Request, Get};

    let origin = {
        let parts = playlist_info.url.rsplitn(2, '/').collect::<Vec<_>>();
        String::from(parts[1])
    };

    let fetch_playlist = {
        let client = client.clone();
        move |_| {
            playlist(&client, &playlist_info.url)
                .map_err(StreamPlayerError::FetchPlaylistFail)
        }
    };

    let fetch_latest_segment = {
        let mut last_segment = None;

        move |mut playlist: Playlist| -> Box<Stream<Item = _, Error = _>> {
            let to_download = match last_segment {
                None => playlist.segments.pop(),
                Some(ref segment) => {
                    match playlist.segments.iter().position(|s| s == segment) {
                        None => playlist.segments.pop(),
                        Some(idx) => playlist.segments.into_iter().nth(idx + 1)
                    }
                }
            };

            let total_stream_time = playlist.twitch_total_secs;

            if let Some(segment) = to_download {
                let segment_url = if segment.url.starts_with("http://") {
                    segment.url.clone()
                } else {
                    format!("{}/{}", origin, segment.url)
                };

                let request = Request::new(Get, segment_url.parse().unwrap());

                let segment_stream = fetch_streamed(&client, request)
                    .map_err(StreamPlayerError::FetchSegmentFail)
                    .into_future()
                    .map_err(|(head_error, _tail)| head_error)
                    .map(move |(opt_head_chunk, tail)| {
                        let head_chunk = opt_head_chunk
                            .map(|data| SegmentChunk::Head { data, total_stream_time })
                            .ok_or(StreamPlayerError::EmptySegment);

                        let tail_chunks = tail.map(SegmentChunk::Tail);

                        stream::once(head_chunk)
                            .chain(tail_chunks)
                    })
                    .flatten_stream();
                last_segment = Some(segment);
                Box::new(segment_stream)
            } else {
                Box::new(stream::empty())
            }
        }
    };

    let playlist_stream = Timer::default()
        .interval(fetch_interval)
        .map_err(StreamPlayerError::TimerError)
        .and_then(fetch_playlist);

    playlist_stream
        .map(fetch_latest_segment)
        .flatten()
}

#[derive(Debug)]
pub enum StreamPlayerError {
    TimerError(tokio_timer::TimerError),
    FetchPlaylistFail(ApiError),
    FetchSegmentFail(HttpError),
    InactiveTooLong,
    TooLongToFetch,
    EmptySegment
}

impl<T> From<tokio_timer::TimeoutError<T>> for StreamPlayerError
where
    T: Stream<Error = StreamPlayerError>
{
    fn from(_err: tokio_timer::TimeoutError<T>) -> Self {
        StreamPlayerError::TooLongToFetch
    }
}
