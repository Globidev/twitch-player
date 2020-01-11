pub mod index_cache;
pub mod player_pool;
pub mod stream_player;

use crate::prelude::futures::*;
use crate::options::Options;

use std::sync::Mutex;

type ShutdownSignal = futures::channel::oneshot::Sender<()>;

pub struct State {
    pub index_cache: index_cache::IndexCache,
    pub player_pool: player_pool::PlayerPool,
    pub shutdown_signal: Mutex<Option<ShutdownSignal>>,
}

impl State {
    pub fn new(opts: Options, shutdown: ShutdownSignal) -> Self {
        Self {
            index_cache: index_cache::IndexCache::new(opts.clone()),
            player_pool: player_pool::PlayerPool::new(opts),
            shutdown_signal: Mutex::new(Some(shutdown))
        }
    }
}
