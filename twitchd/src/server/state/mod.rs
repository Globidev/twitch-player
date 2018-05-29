use prelude::asio::Handle;
use prelude::futures::*;

use options::Options;

use std::cell::Cell;

pub mod index_cache;
pub mod player_pool;
pub mod stream_player;

type ShutdownSignal = sync::oneshot::Sender<()>;

pub struct State {
    pub index_cache: index_cache::IndexCache,
    pub player_pool: player_pool::PlayerPool,
    pub shutdown_signal: Cell<Option<ShutdownSignal>>,
}

impl State {
    pub fn new(opts: Options, shutdown: ShutdownSignal, handle: &Handle) -> Self {
        Self {
            index_cache: index_cache::IndexCache::new(opts.clone(), handle),
            player_pool: player_pool::PlayerPool::new(opts, handle),
            shutdown_signal: Cell::new(Some(shutdown))
        }
    }
}
