use tokio::runtime::{self, Runtime};
pub use tokio::spawn;

#[derive(Debug, Clone)]
pub enum RuntimeStrategy { Single, Multi }

impl RuntimeStrategy {
    pub fn init_runtime(&self) -> Runtime {
        let mut rt_builder = runtime::Builder::new();

        match self {
            RuntimeStrategy::Single => rt_builder.basic_scheduler(),
            RuntimeStrategy::Multi => rt_builder.threaded_scheduler(),
        };

        rt_builder
            .enable_all()
            .build()
            .expect("Failed to initialize runtime")
    }
}
