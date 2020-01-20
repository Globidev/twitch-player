use super::stream_player::{MetaKey, PlayerSink, StreamPlayer};
use crate::{
    options::Options,
    prelude::{
        http::{http_client, HttpsClient},
        runtime,
    },
    twitch::{
        mpeg_ts::SegmentMetadata,
        types::{PlaylistInfo, Stream},
    },
};

use std::{
    collections::{hash_map::Entry as HashMapEntry, HashMap},
    sync::Arc,
};

use futures::{lock::Mutex, prelude::*};

pub struct PlayerPool {
    opts: Options,
    client: HttpsClient,
    players: Arc<Mutex<HashMap<Stream, StreamPlayer>>>,
}

impl PlayerPool {
    pub fn new(opts: Options) -> Self {
        Self {
            opts,
            client: http_client(),
            players: Default::default(),
        }
    }

    pub fn entry(&self, stream: Stream) -> Entry<'_> {
        Entry { stream, pool: self }
    }

    pub async fn get_metadata(
        &self,
        stream: &Stream,
        meta_key: &MetaKey,
    ) -> Option<SegmentMetadata> {
        self.players
            .lock()
            .await
            .get(stream)?
            .get_metadata(meta_key)
            .await
    }
}

pub struct Entry<'pool> {
    stream: Stream,
    pool: &'pool PlayerPool,
}

impl<'pool> Entry<'pool> {
    pub fn or_try_insert_with<F, Fut, E>(self, default: F) -> FilledEntry<'pool, F>
    where
        F: FnOnce() -> Fut,
        Fut: Future<Output = Result<PlaylistInfo, E>>,
    {
        FilledEntry {
            entry: self,
            default,
        }
    }
}

pub struct FilledEntry<'pool, F> {
    entry: Entry<'pool>,
    default: F,
}

impl<F, Fut, E> FilledEntry<'_, F>
where
    F: FnOnce() -> Fut,
    Fut: Future<Output = Result<PlaylistInfo, E>>,
{
    pub async fn play(self, sink: PlayerSink, meta_key: MetaKey) -> Result<(), E> {
        let Self {
            entry: Entry { stream, pool },
            default,
        } = self;

        let mut guard = pool.players.lock().await;

        let player = match guard.entry(stream.clone()) {
            HashMapEntry::Occupied(entry) => entry.into_mut(),
            HashMapEntry::Vacant(entry) => {
                let playlist = default().await?;
                let player = StreamPlayer::new(pool.opts.clone(), pool.client.clone());

                let play_playlist = player.play(playlist);

                runtime::spawn({
                    let players = pool.players.clone();
                    async move {
                        let _ = play_playlist.await;
                        players.lock().await.remove(&stream)
                    }
                });

                entry.insert(player)
            }
        };

        player.queue_sink(sink, meta_key).await;

        Ok(())
    }
}
