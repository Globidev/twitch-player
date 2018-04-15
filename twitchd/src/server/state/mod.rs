use types::Handle;

pub mod index_cache;
pub mod player_pool;
pub mod stream_player;

pub struct State {
    pub index_cache: index_cache::IndexCache,
    pub player_pool: player_pool::PlayerPool,
    pub handle: Handle,
}

impl State {
    pub fn new(handle: &Handle) -> Self {
        Self {
            index_cache: index_cache::IndexCache::new(handle),
            player_pool: player_pool::PlayerPool::new(handle),
            handle: handle.clone()
        }
    }
}
