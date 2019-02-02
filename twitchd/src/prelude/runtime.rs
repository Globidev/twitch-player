use super::futures::Future;

use tokio::runtime::{
    current_thread::Runtime as SingleRuntime,
    Runtime as MultiRuntime,
};

pub use tokio::executor::spawn;

#[derive(Debug, Clone)]
pub enum RuntimeStrategy { Single, Multi }

pub enum Runtime {
    Single(SingleRuntime),
    Multi(MultiRuntime)
}

impl RuntimeStrategy {
    pub fn init_runtime(&self) -> Runtime {
        match self {
            RuntimeStrategy::Single => Runtime::Single(
                SingleRuntime::new()
                    .expect("Failed to initialize single-threaded runtime")
            ),
            RuntimeStrategy::Multi => Runtime::Multi(
                MultiRuntime::new()
                    .expect("Failed to initialize multi-threaded runtime")
            )
        }
    }
}

impl Runtime {
    pub fn block_on<T, E, F>(&mut self, future: F) -> Result<T, E>
    where
        F: Future<Item = T, Error = E> + Send + 'static,
        T: Send + 'static,
        E: Send + 'static
    {
        match self {
            Runtime::Single(rt) => rt.block_on(future),
            Runtime::Multi(rt) => rt.block_on(future),
        }
    }
}
