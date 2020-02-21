use crate::prelude::http::*;
use crate::prelude::runtime;
use crate::prelude::stream_ext::StreamExtChunkConcat;
use crate::twitch::types::{PlaylistInfo, Playlist, Segment};
use crate::twitch::mpeg_ts::{SegmentMetadata, extract_metadata};
use crate::twitch::api::{self, ApiError};
use crate::options::Options;

use std::sync::Arc;
use std::time::{Duration, Instant};
use std::collections::HashMap;

use tokio::time::{interval, timeout};

use futures::{prelude::*, channel::mpsc, lock::Mutex};

type RawVideoData = Vec<u8>;

pub type PlayerSink = ResponseSink;
pub type MetaKey = String;

pub struct StreamPlayer {
    opts: Options,
    client: HttpsClient,
    sink_queue: Arc<Mutex<Vec<(MetaKey, PlayerSink)>>>,
    indexed_metadata: Arc<Mutex<HashMap<MetaKey, SegmentMetadata>>>,
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
        let opts = self.opts.clone();
        let client = self.client.clone();
        let sink_queue = Arc::clone(&self.sink_queue);
        let indexed_metadata = Arc::clone(&self.indexed_metadata);

        async move {
            let mut last_active = Instant::now();
            let mut senders = Vec::new();

            let video_data_stream = segment_stream(
                client,
                playlist_info,
                opts.player_playlist_fetch_interval,
                opts.player_video_chunks_size,
            );

            futures::pin_mut!(video_data_stream);

            loop {
                let timed_fetch_segment = timeout(opts.player_fetch_timeout, video_data_stream.next());

                let segment_chunk = match timed_fetch_segment.await {
                    Err(_elapsed) => break Err(StreamPlayerError::TooLongToFetch),
                    Ok(None) => break Ok(()),
                    Ok(Some(segment_chunk)) => segment_chunk?
                };

                if let SegmentChunk::Head(data) = &segment_chunk {
                    if !sink_queue.lock().await.is_empty() {
                        let opt_metadata = extract_metadata(&data);

                        let mut sink_queue = sink_queue.lock().await;
                        let mut indexed_metadata = indexed_metadata.lock().await;

                        for (key, sink) in sink_queue.drain(..) {
                            if let Some(metadata) = &opt_metadata {
                                indexed_metadata.insert(key, metadata.clone());
                            }

                            let (sender, stream) = mpsc::channel(16);
                            senders.push(sender);

                            runtime::spawn(drain_stream(stream, sink, opts.player_max_sink_buffer_size));
                        }
                    }
                }

                fanout_and_filter(&mut senders, segment_chunk.into_data());

                if !senders.is_empty() {
                    last_active = Instant::now();
                }

                let timed_out = last_active.elapsed() > opts.player_inactive_timeout;
                if timed_out {
                    break Err(StreamPlayerError::InactiveTooLong)
                }
            }
        }
    }

    pub async fn queue_sink(&self, sink: ResponseSink, meta_key: MetaKey) {
        self.sink_queue.lock().await.push((meta_key, sink))
    }

    pub async fn get_metadata(&self, meta_key: &MetaKey) -> Option<SegmentMetadata> {
        self.indexed_metadata.lock().await.get(meta_key).cloned()
    }
}

fn segment_stream(client: HttpsClient, playlist_info: PlaylistInfo, fetch_interval: Duration, video_chunks_size: usize)
    -> impl Stream<Item = Result<SegmentChunk, StreamPlayerError>>
{
    let mut pick_segment = segment_picker();

    let origin = playlist_info.url
        .rsplitn(2, '/')
        .nth(1)
        .map(String::from)
        .expect("Malformed playlist info URL");

    let segment_url = move |segment_location: &str| {
        if segment_location.starts_with("http://") {
            String::from(segment_location)
        } else {
            format!("{}/{}", origin, segment_location)
        }
    };

    interval(fetch_interval)
        .then({
            let client = client.clone();
            move |_tick| {
                api::playlist(&client, &playlist_info.url)
                    .map_err(StreamPlayerError::FetchPlaylistFail)
            }
        })
        .map_ok(move |playlist| {
            let segment = match pick_segment(playlist) {
                None => return stream::empty().left_stream(),
                Some(segment) => segment,
            };

            let request = hyper::Request::get(segment_url(&segment.location))
                .body(hyper::Body::default())
                .expect("Request building error: Video Segment");

            let mut segment_stream = fetch_streamed(&client, request)
                .map_err(StreamPlayerError::FetchSegmentFail)
                .chunk_concat(video_chunks_size);

            async move {
                let head_segment = segment_stream.next().await
                    .ok_or(StreamPlayerError::EmptySegment)??;

                let head_chunk = SegmentChunk::Head(head_segment);
                let tail_chunks = segment_stream.map_ok(SegmentChunk::Tail);

                let wrapped_stream = stream::once(future::ok(head_chunk))
                    .chain(tail_chunks);

                Ok(wrapped_stream)
            }
            .try_flatten_stream()
            .right_stream()
        })
        .try_flatten()
}

fn segment_picker() -> impl FnMut(Playlist) -> Option<Segment> {
    let mut previous_segment = None;

    move |mut playlist| {
        let next_segment = match &previous_segment {
            // We want to have the least delay possible so if we don't have
            // a previous segment, we just use the most recent one
            // (the last one referenced in the playlist)
            None => playlist.segments.pop(),
            // If we previously yielded a segment, we want to yield the one
            // that comes right afterwards. We have to search for it in
            // the new playlist
            Some(segment) => {
                match playlist.segments.iter().position(|s| s == segment) {
                    // In case we don't find the previous segment in the
                    // new playlist, we still want to have the least delay
                    // possible, so we return the most recent segment
                    None => playlist.segments.pop(),
                    // Otherwise return the next logical segment
                    Some(idx) => playlist.segments.into_iter().nth(idx + 1)
                }
            }
        };
        // Update the state for the next iteration before yielding the
        // segment that we should download next
        if let Some(segment) = &next_segment {
            previous_segment = Some(segment.clone());
        }
        next_segment
    }
}

// Helper function that pretty much combines a `drain_filter` algorithm
// with a fan out style data dispatching
fn fanout_and_filter(senders: &mut Vec<mpsc::Sender<RawVideoData>>, data: RawVideoData) {
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

async fn drain_stream<S>(mut video_stream: S, mut sink: PlayerSink, max_buffer_size: usize)
where
    S: Stream<Item = RawVideoData> + Unpin
{
    let mut sink_buffer = Vec::new();

    while let Some(data) = video_stream.next().await {
        let data_out = if sink_buffer.is_empty() {
            data
        } else {
            sink_buffer.extend_from_slice(&data);
            std::mem::take(&mut sink_buffer)
        };

        if let Err(data_back) = sink.try_send_data(data_out.into()) {
            sink_buffer.extend_from_slice(&data_back)
        }

        if sink_buffer.len() > max_buffer_size {
            return // TODO: Bubbling up/logging some error could be useful
        }
    }
}

enum SegmentChunk {
    Head(RawVideoData),
    Tail(RawVideoData),
}

impl SegmentChunk {
    fn into_data(self) -> RawVideoData {
        match self {
            SegmentChunk::Head(data) => data,
            SegmentChunk::Tail(data) => data,
        }
    }
}

#[derive(Debug)]
pub enum StreamPlayerError {
    FetchPlaylistFail(ApiError),
    FetchSegmentFail(HttpError),
    InactiveTooLong,
    TooLongToFetch,
    EmptySegment
}
