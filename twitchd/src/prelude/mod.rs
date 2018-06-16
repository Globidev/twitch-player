mod stream_ext;

pub mod futures {
    extern crate futures;

    pub use self::futures::prelude::*;
    pub use self::futures::future;
    pub use self::futures::stream;
    pub use self::futures::sync;

    pub type BoxFuture<T, E> = Box<Future<Item = T, Error = E> + Send + 'static>;
    pub type BoxStream<T, E> = Box<Stream<Item = T, Error = E> + Send + 'static>;

    pub use super::stream_ext::*;
}

pub mod http;
