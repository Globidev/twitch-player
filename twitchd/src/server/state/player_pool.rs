use prelude::asio::Handle;
use prelude::http::{HttpsClient, http_client};
use prelude::futures::*;
use options::Options;

use super::stream_player::{StreamPlayer, PlayerSink, MetaKey, SegmentMetadata};
use twitch::types::{PlaylistInfo, Stream};

use std::rc::Rc;
use std::cell::RefCell;
use std::collections::HashMap;

pub struct PlayerPool {
    handle: Handle,
    client: HttpsClient,
    players: Rc<RefCell<HashMap<Stream, StreamPlayer>>>,
    opts: Options,
}

impl PlayerPool {
    pub fn new(opts: Options, handle: &Handle) -> Self {
        Self {
            client: http_client(handle).unwrap(),
            handle: handle.clone(),
            players: Rc::new(RefCell::new(HashMap::new())),
            opts: opts,
        }
    }

    pub fn is_playing(&self, stream: &Stream) -> bool {
        self.players.borrow().contains_key(stream)
    }

    pub fn add_sink(&self, stream: &Stream, sink: PlayerSink, opt_meta_key: Option<MetaKey>) {
        if let Some(player) = self.players.borrow().get(stream) {
            player.queue_sink(sink, opt_meta_key.unwrap_or_default());
        }
    }

    pub fn get_metadata(&self, stream: &Stream, meta_key: &MetaKey) -> Option<SegmentMetadata> {
        self.players.borrow().get(stream)
            .and_then(|player| player.get_metadata(meta_key))
    }

    pub fn add_player(&self, stream: Stream, playlist: PlaylistInfo, sink: PlayerSink, opt_meta_key: Option<MetaKey>) {
        let remove_player = {
            let stream = stream.clone();
            let players = Rc::clone(&self.players);
            move |result| {
                println!("{:?}", result);
                players.borrow_mut().remove(&stream);
                Ok(())
            }
        };

        let new_player = {
            let client = self.client.clone();
            move || {
                let player = StreamPlayer::new(self.opts.clone(), client);
                let future = player.play(playlist)
                    .then(remove_player);
                self.handle.spawn(future);
                player
            }
        };

        self.players.borrow_mut()
            .entry(stream)
            .or_insert_with(new_player)
            .queue_sink(sink, opt_meta_key.unwrap_or_default());
    }
}
