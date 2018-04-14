use types::Handle;

pub mod index_cache;

pub struct State {
    pub index_cache: index_cache::IndexCache
}

impl State {
    pub fn new(handle: &Handle) -> Self {
        Self {
            index_cache: index_cache::IndexCache::new(handle),
        }
    }
}
