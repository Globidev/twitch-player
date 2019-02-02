use crate::prelude::http::{HttpsClient, http_client};
use crate::prelude::futures::*;
use crate::prelude::runtime;
use crate::options::Options;
use crate::twitch::types::{PlaylistInfo, Stream};
use super::stream_player::{StreamPlayer, PlayerSink, MetaKey, SegmentMetadata};

use std::sync::{Arc, RwLock};
use std::collections::HashMap;

pub struct PlayerPool {
    opts: Options,
    client: HttpsClient,
    players: Arc<RwLock<HashMap<Stream, StreamPlayer>>>,
}

impl PlayerPool {
    pub fn new(opts: Options) -> Self {
        Self {
            opts: opts,
            client: http_client().expect("Failed to initialize HTTP client"),
            players: Default::default(),
        }
    }

    pub fn is_playing(&self, stream: &Stream) -> bool {
        self.players.read().unwrap().contains_key(stream)
    }

    pub fn add_sink(&self, stream: &Stream, sink: PlayerSink, opt_meta_key: Option<MetaKey>) {
        if let Some(player) = self.players.read().unwrap().get(stream) {
            player.queue_sink(sink, opt_meta_key.unwrap_or_default());
        }
    }

    pub fn get_metadata(&self, stream: &Stream, meta_key: &MetaKey) -> Option<SegmentMetadata> {
        self.players.read().unwrap().get(stream)
            .and_then(|player| player.get_metadata(meta_key))
    }

    pub fn add_player(&self, stream: Stream, playlist: PlaylistInfo, sink: PlayerSink, opt_meta_key: Option<MetaKey>) {
        let remove_player = {
            let stream = stream.clone();
            let players = Arc::clone(&self.players);
            move |result| {
                println!("{:?}", result);
                players.write().unwrap().remove(&stream);
                Ok(())
            }
        };

        let new_player = {
            let client = self.client.clone();
            move || {
                let player = StreamPlayer::new(self.opts.clone(), client);
                let future = player.play(playlist)
                    .then(remove_player);
                runtime::spawn(future);
                player
            }
        };

        self.players.write().unwrap()
            .entry(stream)
            .or_insert_with(new_player)
            .queue_sink(sink, opt_meta_key.unwrap_or_default());
    }
}
