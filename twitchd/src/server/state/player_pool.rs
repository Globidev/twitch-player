extern crate hyper;

use prelude::http::{HttpsClient, http_client};
use prelude::futures::*;
use options::Options;

use super::stream_player::{StreamPlayer, PlayerSink, MetaKey, SegmentMetadata};
use twitch::types::{PlaylistInfo, Stream};

use std::sync::Arc;
use std::sync::Mutex;
use std::collections::HashMap;

pub struct PlayerPool {
    client: HttpsClient,
    players: Arc<Mutex<HashMap<Stream, StreamPlayer>>>,
    opts: Options,
}

impl PlayerPool {
    pub fn new(opts: Options) -> Self {
        Self {
            players: Default::default(),
            opts: opts,
            client: http_client().expect("Failed to initialize HTTP client"),
        }
    }

    pub fn is_playing(&self, stream: &Stream) -> bool {
        self.players.lock().unwrap().contains_key(stream)
    }

    pub fn add_sink(&self, stream: &Stream, sink: PlayerSink, opt_meta_key: Option<MetaKey>) {
        if let Some(player) = self.players.lock().unwrap().get(stream) {
            player.queue_sink(sink, opt_meta_key.unwrap_or_default());
        }
    }

    pub fn get_metadata(&self, stream: &Stream, meta_key: &MetaKey) -> Option<SegmentMetadata> {
        self.players.lock().unwrap().get(stream)
            .and_then(|player| player.get_metadata(meta_key))
    }

    pub fn add_player(&self, stream: Stream, playlist: PlaylistInfo, sink: PlayerSink, opt_meta_key: Option<MetaKey>) {
        let remove_player = {
            let stream = stream.clone();
            let players = Arc::clone(&self.players);
            move |result| {
                println!("{:?}", result);
                players.lock().unwrap().remove(&stream);
                Ok(())
            }
        };

        let new_player = {
            let client = self.client.clone();
            move || {
                let player = StreamPlayer::new(self.opts.clone(), client);
                let future = player.play(playlist)
                    .then(remove_player);
                hyper::rt::spawn(future);
                player
            }
        };

        self.players.lock()
            .unwrap()
            .entry(stream)
            .or_insert_with(new_player)
            .queue_sink(sink, opt_meta_key.unwrap_or_default());
    }
}
