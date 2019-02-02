mod stream_ext;

pub mod futures {
    pub use super::stream_ext::*;

    pub use futures::prelude::*;
    pub use futures::future;
    pub use futures::stream;
    pub use futures::sync;

    pub type BoxFuture<T, E> = Box<Future<Item = T, Error = E> + Send + 'static>;
    pub type BoxStream<T, E> = Box<Stream<Item = T, Error = E> + Send + 'static>;
}

pub mod runtime;

pub mod http;
