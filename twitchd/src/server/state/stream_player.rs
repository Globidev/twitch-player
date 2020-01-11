use crate::prelude::futures::*;
use crate::prelude::http::*;
use crate::prelude::runtime;
use crate::twitch::types::{PlaylistInfo, Playlist, Segment};
use crate::twitch::api::ApiError;
use crate::options::Options;

use std::sync::{Arc, RwLock};
use std::time::{Duration, Instant};
use std::collections::HashMap;

// use hyper::Chunk;
use tokio::time::{interval};
use serde::{Serialize, Deserialize};

type RawVideoData = hyper::body::Bytes;

pub type PlayerSink = ResponseSink;
pub type MetaKey = String;

pub struct StreamPlayer {
    opts: Options,
    client: HttpsClient,
    sink_queue: Arc<RwLock<Vec<(MetaKey, PlayerSink)>>>,
    indexed_metadata: Arc<RwLock<HashMap<MetaKey, SegmentMetadata>>>,
}

impl StreamPlayer {
    pub fn new(opts: Options, client: HttpsClient) -> Self {
        Self {
            opts,
            client,
            sink_queue: Default::default(),
            indexed_metadata: Default::default(),
        }
    }

    pub fn play(&self, playlist_info: PlaylistInfo)
        -> impl Future<Output = Result<(), StreamPlayerError>>
    {
        let process_meta_data = {
            let sink_size = self.opts.player_max_sink_buffer_size;
            let sink_queue = Arc::clone(&self.sink_queue);
            let indexed_metadata = Arc::clone(&self.indexed_metadata);

            move |data: &RawVideoData, senders: &mut Vec<_>| {
                if sink_queue.read().unwrap().is_empty() { return }

                let opt_metadata = extract_metadata(data);

                let mut sink_queue = sink_queue.write().unwrap();
                let mut indexed_metadata = indexed_metadata.write().unwrap();

                sink_queue.drain(..).for_each(|(key, sink)| {
                    if let Some(metadata) = opt_metadata.as_ref() {
                        indexed_metadata.insert(key, metadata.clone());
                    }

                    let (sender, stream) = futures::channel::mpsc::channel(16);
                    senders.push(sender);

                    runtime::spawn(drain_stream(stream, sink, sink_size));
                });
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
            self.opts.player_playlist_fetch_interval,
            self.opts.player_video_chunks_size,
        );

        video_data_stream
            // .timeout(self.opts.player_fetch_timeout)
            .try_for_each(process_segment_chunk)

        // Ok(())
    }

    pub fn queue_sink(&self, sink: ResponseSink, meta_key: MetaKey) {
        self.sink_queue.write().unwrap().push((meta_key, sink))
    }

    pub fn get_metadata(&self, meta_key: &MetaKey) -> Option<SegmentMetadata> {
        self.indexed_metadata.read().unwrap().get(meta_key).cloned()
    }
}

fn segment_stream(client: HttpsClient, playlist_info: PlaylistInfo, fetch_interval: Duration, video_chunks_size: usize)
    -> impl Stream<Item = Result<SegmentChunk, StreamPlayerError>>
{
    use crate::twitch::api::playlist;

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

    let dowload_segment = { let client = client.clone(); move |segment: Segment| {
        let request = hyper::Request::get(segment_url(&segment.location))
            .body(hyper::Body::default())
            .expect("Request building error: Video Segment");

        let segment_stream = fetch_streamed(&client, request)
            .map_err(StreamPlayerError::FetchSegmentFail)
            // -> Buffer incoming data into larger chunks
            // .buffer_concat(video_chunks_size)
            // Now we need to split the head and the tail of the data to be able
            // to process the metadata that's in the head
            // -> convert the stream into a future
            .into_future()
            // A stream S = Stream<T, E>                   is converted to a
            //   future F = Future<(Option<T>, S), (E, S)>
            // We need to extract the error to get a Future<_, E>
            // -> map F's error to E, the first element of the tuple (E, S)
            // .map_err(|(error, _tail_stream)| error)
            // Now we need to reify the head chunk and the tail chunks as
            // `SegmentChunk`s and concatenate them into a stream
            // -> map the (Option<T>, S) to a Stream<T, S::E> chained to S
            .map(|(opt_head, tail_stream)| {
                let head_chunk = match opt_head {
                    Some(Ok(h)) => h,
                    Some(Err(e)) => return stream::once(future::err(e)).boxed(),
                    None => return stream::once(future::err(StreamPlayerError::EmptySegment)).boxed(),
                };

                // let head_chunk = opt_head
                //     .map(SegmentChunk::Head)
                //     .ok_or(StreamPlayerError::EmptySegment);

                let tail_chunks = tail_stream.map_ok(SegmentChunk::Tail);

                stream::once(future::ok(SegmentChunk::Head(head_chunk)))
                    .chain(tail_chunks)
                    .boxed()
            })
            // Flattens our future of stream F<S<...>> to the stream S<...>
            .flatten_stream();

        segment_stream //as BoxStream<_, _>
    } };

    let fetch_latest_segment = move |playlist: Playlist| {
        segment_to_download(playlist)
            .map(|segment| dowload_segment(segment).boxed())
            .unwrap_or_else(|| stream::empty().boxed())
    };

    let playlist_stream = interval(fetch_interval)
        .then({
            // let client = client.clone();
            move |_timer_tick| { let client = client.clone(); let url = playlist_info.url.clone(); async move {
                playlist(&client, &url)
                    .map_err(StreamPlayerError::FetchPlaylistFail)
                    .await
            } }
        });

    playlist_stream
        .map_ok(fetch_latest_segment)
        .try_flatten()
}

// Helper function that pretty much combines a `drain_filter` algorithm
// with a fan out style data dispatching
fn fanout_and_filter(senders: &mut Vec<futures::channel::mpsc::Sender<RawVideoData>>, data: RawVideoData) {
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

async fn drain_stream<S>(video_stream: S, mut sink: PlayerSink, max_buffer_size: usize)
where
    S: Stream<Item = RawVideoData>
{
    use std::mem::replace;

    let mut sink_buffer = Vec::<u8>::new();

    let data_to_send = |data: Vec<u8>, backlog: &mut Vec<u8>| {
        match backlog.is_empty() {
            true => data,
            false => {
                backlog.extend_from_slice(&data);
                replace(backlog, Vec::new())
            }
        }
    };

    let send_or_buffer = move |data: RawVideoData| async {
        // if sink_buffer.len() > max_buffer_size {
        //     return //Err(())
        // }

        // match sink.poll_ready() {
        //     Ok(Async::Ready(_)) => {
        //         let data_out = data_to_send(data, &mut sink_buffer);
        //         sink.send_data(Chunk::from(data_out))
        //             .map_err(|_chunk| {
        //                 eprintln!("Send data failed after successful polling");
        //             })
        //     },
        //     Ok(Async::NotReady) => {
        //         sink_buffer.extend_from_slice(&data);
        //         Ok(())
        //     },
        //     Err(_) => Err(())
        // }
        // Err(())
    };

    video_stream.for_each(send_or_buffer).await
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

const MPEG_TS_SECTION_LENGTH: usize = 188;
// The metadata packet seems to always be the 3rd one
const FIRST_PES_METADATA_OFFSET: usize = MPEG_TS_SECTION_LENGTH * 3;
const SECOND_PES_METADATA_OFFSET: usize = FIRST_PES_METADATA_OFFSET + MPEG_TS_SECTION_LENGTH;

fn extract_metadata(data: &RawVideoData) -> Option<SegmentMetadata> {
    let first_pes_slice = &data[FIRST_PES_METADATA_OFFSET..SECOND_PES_METADATA_OFFSET];

    let json_start_offset = first_pes_slice.iter()
        .position(|&c| c == b'{')?;
    let unbounded_json_slice = &first_pes_slice[json_start_offset..];

    if let Some(json_end_offset) = unbounded_json_slice.iter()
        .position(|&c| c == b'}')
    {
        let json_slice = &unbounded_json_slice[..=json_end_offset];
        serde_json::from_slice(json_slice).ok()
    } else {
        // The JSON data does not fit in a single PES slice
        // Twitch uses a second one and seems to align it to the end
        let json_initial_slice = unbounded_json_slice;
        let second_pes_slice = &data[
            SECOND_PES_METADATA_OFFSET
            ..
            SECOND_PES_METADATA_OFFSET + MPEG_TS_SECTION_LENGTH
        ];
        let json_continuity_end_offset = second_pes_slice.iter()
            .position(|&c| c == b'}')?;
        let json_continuity_start_offset = (0..json_continuity_end_offset)
            .rev()
            .find(|&idx| second_pes_slice[idx] == 0xFF)? + 1;

        let json_continuity_slice = &second_pes_slice[
            json_continuity_start_offset
            ..=
            json_continuity_end_offset
        ];

        let json_slice = [json_initial_slice, json_continuity_slice].concat();
        serde_json::from_slice(&json_slice).ok()
    }
}

#[derive(Debug)]
pub enum StreamPlayerError {
    TimerError(tokio::time::Error),
    FetchPlaylistFail(ApiError),
    FetchSegmentFail(HttpError),
    InactiveTooLong,
    TooLongToFetch,
    EmptySegment
}

// impl From<TimeoutError> for StreamPlayerError {
//     fn from(err: TimeoutError) -> Self {
//         use self::TimeoutError::*;
//         use self::StreamPlayerError::*;

//         match err {
//             Timer(timer_error) => TimerError(timer_error),
//             TimedOut           => TooLongToFetch
//         }
//     }
// }

#[cfg(test)]
mod tests {

    fn extract_metadata_test(data: &[u8]) {
        use super::{extract_metadata, RawVideoData};
        assert!(extract_metadata(&RawVideoData::copy_from_slice(data)).is_some());
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

    #[test]
    fn extract_source_faulty_22_may_2019() {
        extract_metadata_test(
            include_bytes!("../../../test_samples/mpeg_ts/source_faulty_22_may_2019.ts")
        );
    }

}
