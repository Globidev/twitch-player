extern crate tokio_core;
extern crate futures;
extern crate hyper;

mod state;
mod service;

use self::state::State;
use self::service::TwitchdApi;

use std::net::SocketAddr;

use self::futures::{Future, Stream, future};

pub fn run(addr: &SocketAddr) {
    use std::rc::Rc;
    use self::tokio_core::reactor::Core;

    let mut core = Core::new().unwrap();
    let handle = &core.handle();

    let state = Rc::new(State::new(handle));
    let make_service = || Ok(TwitchdApi::new(&state));

    let incoming_stream = hyper::server::Http::new()
        .serve_addr_handle(&addr, handle, make_service)
        .unwrap();

    let server = incoming_stream.for_each(|incoming| {
        let future = incoming.map_err(|e| eprintln!("Unexpected error: {}", e));
        handle.spawn(future);
        future::ok(())
    });

    core.run(server).unwrap();
}
