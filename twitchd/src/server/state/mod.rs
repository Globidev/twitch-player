use prelude::asio::Handle;

use options::Options;

pub mod index_cache;
pub mod player_pool;
pub mod stream_player;

pub struct State {
    pub index_cache: index_cache::IndexCache,
    pub player_pool: player_pool::PlayerPool,
}

impl State {
    pub fn new(opts: Options, handle: &Handle) -> Self {
        Self {
            index_cache: index_cache::IndexCache::new(opts.clone(), handle),
            player_pool: player_pool::PlayerPool::new(opts, handle),
        }
    }
}
