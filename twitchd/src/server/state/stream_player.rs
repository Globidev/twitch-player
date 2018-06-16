extern crate hyper;
extern crate tokio;
extern crate bytes;
extern crate serde_json;

use self::hyper::Chunk;
use self::tokio::timer::Interval;

use prelude::futures::*;
use prelude::http::*;

use twitch::types::{PlaylistInfo, Playlist, Segment};
use twitch::api::ApiError;

use options::Options;

use std::sync::{Arc, Mutex, RwLock};
use std::time::{Duration, Instant};
use std::collections::HashMap;

const MPEG_TS_SECTION_LENGTH: usize = 188;
// The metadata packet seems to always be the 3rd one
const PES_METADATA_OFFSET: usize = MPEG_TS_SECTION_LENGTH * 3;
// Number of raw video chunks to buffer before yielding them back to the clients
const VIDEO_DATA_BUFFER_SIZE: usize = 1024 * 128;

type RawVideoData = bytes::Bytes;

pub type PlayerSink = ResponseSink;
pub type MetaKey = String;

pub struct StreamPlayer {
    opts: Options,
    client: HttpsClient,
    sink_queue: Arc<Mutex<Vec<(MetaKey, PlayerSink)>>>,
    indexed_metadata: Arc<RwLock<HashMap<MetaKey, SegmentMetadata>>>,
}

impl StreamPlayer {
    pub fn new(opts: Options, client: HttpsClient) -> Self {
        Self {
            opts: opts,
            client: client,
            sink_queue: Default::default(),
            indexed_metadata: Default::default(),
        }
    }

    pub fn play(&self, playlist_info: PlaylistInfo)
        -> impl Future<Item = (), Error = StreamPlayerError>
    {
        let process_meta_data = {
            let sink_queue = Arc::clone(&self.sink_queue);
            let indexed_metadata = Arc::clone(&self.indexed_metadata);

            move |data: &RawVideoData, senders: &mut Vec<_>| {
                let mut sink_queue = sink_queue.lock().unwrap();

                if sink_queue.is_empty() { return }

                if let Some(metadata) = extract_metadata(data) {
                    let mut indexed_metadata = indexed_metadata.write().unwrap();

                    sink_queue.drain(..).for_each(|(key, sink)| {
                        indexed_metadata.insert(key, metadata.clone());

                        let (sender, stream) = sync::mpsc::channel(16);
                        senders.push(sender);

                        hyper::rt::spawn(drain_stream(stream, sink));
                    });
                }
            }
        };

        let process_segment_chunk = {
            let inactive_timeout = self.opts.player_inactive_timeout;

            let mut last_active = Instant::now();
            let mut senders = Vec::new();

            move |chunk: SegmentChunk| {
                let raw_data = match chunk {
                    SegmentChunk::Tail(data) => data,
                    SegmentChunk::Head(data) => {
                        process_meta_data(&data, &mut senders);
                        data
                    }
                };

                fanout_and_filter(&mut senders, raw_data);

                if !senders.is_empty() { last_active = Instant::now(); }

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

        video_data_stream
            .timeout(self.opts.player_fetch_timeout)
            .for_each(process_segment_chunk)
    }

    pub fn queue_sink(&self, sink: ResponseSink, meta_key: MetaKey) {
        self.sink_queue.lock().unwrap().push((meta_key, sink))
    }

    pub fn get_metadata(&self, meta_key: &MetaKey) -> Option<SegmentMetadata> {
        self.indexed_metadata.read().unwrap().get(meta_key).cloned()
    }
}

fn segment_stream(client: HttpsClient, playlist_info: PlaylistInfo, fetch_interval: Duration)
    -> impl Stream<Item = SegmentChunk, Error = StreamPlayerError>
{
    use twitch::api::playlist;

    let origin = playlist_info.url
        .rsplitn(2, '/')
        .nth(1)
        .map(String::from)
        .expect("Malformed playlist info URL");

    let segment_url = move |segment_location: &str| {
        match segment_location.starts_with("http://") {
            true  => String::from(segment_location),
            false => format!("{}/{}", origin, segment_location)
        }
    };

    let fetch_playlist = {
        let client = client.clone();
        move |_timer_tick| {
            playlist(&client, &playlist_info.url)
                .map_err(StreamPlayerError::FetchPlaylistFail)
        }
    };

    let mut segment_to_download = {
        let mut previous_segment = None;

        move |mut playlist: Playlist| {
            let next_segment = match previous_segment {
                // We want to have the least delay possible so if we don't have
                // a previous segment, we just use the most recent one
                // (the last one referenced in the playlist)
                None => playlist.segments.pop(),
                // If we previously yielded a segment, we want to yield the one
                // that comes right afterwards. We have to search for it in
                // the new playlist
                Some(ref segment) => {
                    match playlist.segments.iter().position(|s| s == segment) {
                        // In case we don't find the previous segment in the
                        // new playlist, we still want to have the least delay
                        // possible, so we return the most recent segment
                        None      => playlist.segments.pop(),
                        // Otherwise return the next logical segment
                        Some(idx) => playlist.segments.into_iter().nth(idx + 1)
                    }
                }
            };
            // Update the state for the next iteration before yielding the
            // segment that we should download next
            if let Some(ref segment) = next_segment {
                previous_segment = Some(segment.clone());
            }
            next_segment
        }
    };

    let dowload_segment = move |segment: Segment| {
        let request = hyper::Request::get(segment_url(&segment.location))
            .body(hyper::Body::default())
            .expect("Request building error: Video Segment");

        let segment_stream = fetch_streamed(&client, request)
            .map_err(StreamPlayerError::FetchSegmentFail)
            // -> Buffer incoming data into larger chunks
            .buffer_concat(VIDEO_DATA_BUFFER_SIZE)
            // Now we need to split the head and the tail of the data to be able
            // to process the metadata that's in the head
            // -> convert the stream into a future
            .into_future()
            // A stream S = Stream<T, E>                   is converted to a
            //   future F = Future<(Option<T>, S), (E, S)>
            // We need to extract the error to get a Future<_, E>
            // -> map F's error to E, the first element of the tuple (E, S)
            .map_err(|(error, _tail_stream)| error)
            // Now we need to reify the head chunk and the tail chunks as
            // `SegmentChunk`s and concatenate them into a stream
            // -> map the (Option<T>, S) to a Stream<T, S::E> chained to S
            .map(|(opt_head, tail_stream)| {
                let head_chunk = opt_head
                    .map(SegmentChunk::Head)
                    .ok_or(StreamPlayerError::EmptySegment);

                let tail_chunks = tail_stream.map(SegmentChunk::Tail);

                stream::once(head_chunk).chain(tail_chunks)
            })
            // Flattens our future of stream F<S<...>> to the stream S<...>
            .flatten_stream();

        Box::new(segment_stream) as BoxStream<_, _>
    };

    let fetch_latest_segment = move |playlist: Playlist| {
        segment_to_download(playlist)
            .map(|segment| dowload_segment(segment))
            .unwrap_or_else(|| Box::new(stream::empty()))
    };

    let playlist_stream = Interval::new(Instant::now(), fetch_interval)
        .map_err(StreamPlayerError::TimerError)
        .and_then(fetch_playlist);

    playlist_stream
        .map(fetch_latest_segment)
        .flatten()
}

// Helper function that pretty much combines a `drain_filter` algorithm
// with a fan out style data dispatching
fn fanout_and_filter(senders: &mut Vec<sync::mpsc::Sender<RawVideoData>>, data: RawVideoData) {
    if senders.is_empty() { return }

    let mut i = 0;
    // Fan the input data out to every client except the last one
    while i < senders.len() - 1 {
        match senders[i].try_send(data.clone()) {
            Ok(_)  => { i += 1; },
            Err(_) => { let _ = senders.remove(i); }
        }
    }
    // The last sink can save us a clone on the input data
    if let Err(_) = senders[i].try_send(data) {
        let _ = senders.remove(i);
    }
}

fn drain_stream<S>(video_stream: S, mut sink: PlayerSink)
    -> impl Future<Item = (), Error = ()>
where
    S: Stream<Item = RawVideoData, Error = ()>
{
    use std::mem::replace;

    let mut data_backlog = RawVideoData::new();

    let data_to_send = |data: RawVideoData, backlog: &mut RawVideoData| {
        match backlog.is_empty() {
            true => data,
            false => {
                backlog.extend_from_slice(&data);
                replace(backlog, RawVideoData::new())
            }
        }
    };

    let send_or_buffer = move |data: RawVideoData| {
        // TODO: Add backlog size limit
        match sink.poll_ready() {
            Ok(Async::Ready(_)) => {
                let data_out = data_to_send(data, &mut data_backlog);
                sink.send_data(Chunk::from(data_out))
                    .map_err(|_chunk| {
                        eprintln!("Send data failed after successful polling");
                    })
            },
            Ok(Async::NotReady) => {
                data_backlog.extend_from_slice(&data);
                Ok(())
            },
            Err(_) => Err(())
        }
    };

    video_stream.for_each(send_or_buffer)
}

enum SegmentChunk {
    Head(RawVideoData),
    Tail(RawVideoData),
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

fn extract_metadata(data: &RawVideoData) -> Option<SegmentMetadata> {
    let pes_slice = &data[
        PES_METADATA_OFFSET
        ..
        PES_METADATA_OFFSET + MPEG_TS_SECTION_LENGTH
    ];

    let json_start_offset = pes_slice.iter()
        .position(|&c| c == b'{')?;
    let unbounded_json_slice = &pes_slice[json_start_offset..];

    let json_end_offset = unbounded_json_slice.iter()
        .position(|&c| c == b'}')?;
    let json_slice = &unbounded_json_slice[..=json_end_offset];

    serde_json::from_slice(json_slice).ok()
}

#[derive(Debug)]
pub enum StreamPlayerError {
    TimerError(tokio::timer::Error),
    FetchPlaylistFail(ApiError),
    FetchSegmentFail(HttpError),
    InactiveTooLong,
    TooLongToFetch,
    EmptySegment
}

impl From<TimeoutError> for StreamPlayerError {
    fn from(err: TimeoutError) -> Self {
        use self::TimeoutError::*;
        use self::StreamPlayerError::*;

        match err {
            Timer(timer_error) => TimerError(timer_error),
            TimedOut           => TooLongToFetch
        }
    }
}

#[cfg(test)]
mod tests {

    fn extract_metadata_test(data: &[u8]) {
        use super::{extract_metadata, RawVideoData};
        assert!(extract_metadata(&RawVideoData::from(data)).is_some());
    }

    #[test]
    fn extract_source_segment_metadata() {
        extract_metadata_test(
            include_bytes!("../../../test_samples/mpeg_ts/source_1080p60.ts")
        );
    }

    #[test]
    fn extract_encoded_high_quality_segment_metadata() {
        extract_metadata_test(
            include_bytes!("../../../test_samples/mpeg_ts/encoded_720p60.ts")
        );
    }

    #[test]
    fn extract_encoded_medium_quality_segment_metadata() {
        extract_metadata_test(
            include_bytes!("../../../test_samples/mpeg_ts/encoded_480p.ts")
        );
    }

    #[test]
    fn extract_encoded_low_quality_segment_metadata() {
        extract_metadata_test(
            include_bytes!("../../../test_samples/mpeg_ts/encoded_160p.ts")
        );
    }

}
