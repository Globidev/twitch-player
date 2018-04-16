use prelude::asio::Handle;
use prelude::http::{HttpsClient, http_client};
use prelude::futures::*;

use super::stream_player::{StreamPlayer, PlayerSink};
use twitch::types::PlaylistInfo;

use std::rc::Rc;
use std::cell::RefCell;
use std::collections::HashMap;

type Channel = String;
type Quality = String;
type Stream = (Channel, Quality);

pub struct PlayerPool {
    handle: Handle,
    client: HttpsClient,
    players: Rc<RefCell<HashMap<Stream, StreamPlayer>>>,
}

impl PlayerPool {
    pub fn new(handle: &Handle) -> Self {
        Self {
            client: http_client(handle).unwrap(),
            handle: handle.clone(),
            players: Rc::new(RefCell::new(HashMap::new())),
        }
    }

    pub fn is_playing(&self, stream: &Stream) -> bool {
        self.players.borrow().contains_key(&stream)
    }

    pub fn add_client(&self, stream: Stream, sink: PlayerSink) {
        if let Some(player) = self.players.borrow().get(&stream) {
            player.add_sink(sink)
        }
    }

    pub fn add_player(&self, stream: Stream, playlist_info: PlaylistInfo, sink: PlayerSink) {
        let new_player = {
            let stream = stream.clone();
            let client = self.client.clone();
            move || {
                let player = StreamPlayer::new(client, playlist_info);
                self.play(&player, stream);
                player
            }
        };

        self.players.borrow_mut()
            .entry(stream)
            .or_insert_with(new_player)
            .add_sink(sink);
    }

    fn play(&self, player: &StreamPlayer, stream: Stream) {
        let remove_player = {
            let players = Rc::clone(&self.players);
            move |result| {
                players.borrow_mut().remove(&stream);
                Ok(())
            }
        };

        let future = player.play()
            .then(remove_player);

        self.handle.spawn(future);
    }
}
